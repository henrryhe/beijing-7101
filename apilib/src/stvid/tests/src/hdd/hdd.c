/*****************************************************************************

File name   :  hdd.c

Description :  Contains the high level commands for HDD driver

COPYRIGHT (C) ST-Microelectronics 2005

Date               Modification                 Name
----               ------------                 ----
 02/11/00          Created                      FR and SB
 08/01/00          Adapted for STVID (STAPI)    AN
 11/09/01          Adapted for driver STATAPI   FQ
 10/01/02          Put HDD data in N cache area FQ
 06/02/02          Read HDD 64K by 64K bytes    FQ
*****************************************************************************/

/* Flag to use when debug with STTBX... functions. */
 /*#define HDD_TRACE_DEBUG*/

/* Flag to use when debug with uart spy. */
/*#define HDD_TRACE_UART*/
/*#define HDD_INJECTION_TRACE_UART*/

/* Flag to save the content of the PES injected to a file */
/*#define SAVE_TO_DISK*/
/* Includes ----------------------------------------------------------------- */
#if defined (HDD_TRACE_UART) || defined(HDD_INJECTION_TRACE_UART)
#include "trace.h"
#ifdef STVID_TRACE_BUFFER_UART
#if (STVID_TRACE_BUFFER_UART==2)
#define TraceDebug(x) TraceMessage x
#define TraceGraph(x) TraceVal x
#endif  /* STVID_TRACE_BUFFER_UART==2 */
#endif /*STVID_TRACE_BUFFER_UART*/
#else
#define TraceBuffer(x)
#endif

#ifdef ST_OS21
#include <stdio.h>
#include <unistd.h>     /* read() functions */
#include "os21debug.h"
#include "stlite.h"
#include <fcntl.h>
#endif
#ifdef ST_OS20
#include <stdio.h>
#include <task.h>
#include <ostime.h>
#include <debug.h>
#endif
#include <string.h>
#include <stdlib.h>

#include "testcfg.h"

#include "stdevice.h"
#include "stevt.h"
#include "stvid.h"
#include "stavmem.h"
#include "stddefs.h"
#include "sttbx.h"
#include "stcommon.h"
#include "testtool.h"
#include "swconfig.h"
#include "startup.h"

#include "statapi.h"

#if (defined (DVD_TRANSPORT_STPTI) || defined (DVD_TRANSPORT_STPTI4)) && !defined(STPTI_UNAVAILABLE)
#include "stpti.h"
#endif

#if (defined (DVD_TRANSPORT_PTI) || defined (DVD_TRANSPORT_LINK)) && !defined(STPTI_UNAVAILABLE)
#include "pti.h"
#endif

#ifdef DVD_TRANSPORT_LINK /* To avoid a compilation warning with link 2.0.0 */
#include "ptilink.h"
#endif
#include "hdd.h"
#include "trictest.h"
#include "stsys.h"
#include "clevt.h"
#if defined(mb282b)
#include "stfpga.h"
#endif
#if defined(ST_5516) || defined(ST_5517)
#include "stcfg.h"
#endif
#include "vid_inj.h"
#include "vid_util.h"

#if defined(ST_5162)
#include "cli2c.h"
#endif


#if defined(mb411)
#ifndef DONT_USE_FDMA
#define DONT_USE_FDMA
#endif
#ifndef DU8
#pragma ST_device(DU8)
typedef volatile unsigned char DU8;
#endif
#ifndef DU16
#pragma ST_device(DU16)
typedef volatile unsigned short DU16;
#endif
#ifndef DU32
#pragma ST_device(DU32)
typedef volatile unsigned int DU32;
#endif
#endif /* defined(mb411) */

/* Compilation options...---------------------------------------------------- */
#define WA_USEDRIFTTIMENOTTIMEJUMP
/* STATAPI_OLD_FPGA for 5512 */
/* Private Constants -------------------------------------------------------- */

/* For debugging(saving the injected PES to a file) */
#ifdef SAVE_TO_DISK
#define PES_FILE_NAME       "hdd.dat"
    volatile long int HDDFileDescriptor;
#endif /* SAVE_TO_DISK */

/* Devices */

#define STATAPI_DEVICE_NAME "STATAPI1"

/* Hardware data */

#if (defined(ST_5508) | defined(ST_5518))
    #define     ATA_INTERRUPT_NUMBER    EXTERNAL_1_INTERRUPT
    #define     ATA_BASE_ADDRESS        (volatile U32 *)    0x50000000
    #define     ATA_HRD_RST             (volatile U16 *)    0x50000000

#elif defined(ST_5512)
    /* if it does not work, use old or current fpga.tff and memory settings */
  #if defined(STATAPI_OLD_FPGA)
    #define     ATA_INTERRUPT_NUMBER    EXTERNAL_0_INTERRUPT
    #define     ATA_BASE_ADDRESS        (volatile U32 *)    0x60000000
    #define     ATA_HRD_RST             (volatile U16 *)    0x60100000
  #else
    #define     ATA_INTERRUPT_NUMBER    EXTERNAL_0_INTERRUPT
    #define     ATA_BASE_ADDRESS        (volatile U32 *)    0x70B00000
    #define     ATA_HRD_RST             (volatile U16 *)    0x70A00000
  #endif

#elif defined(ST_5514)
    #define     ATA_BASE_ADDRESS        (volatile U32 *)    ST5514_HDDI_BASE_ADDRESS
    #define     ATA_INTERRUPT_NUMBER    ST5514_HDDI_INTERRUPT
    /*#define     ATA_HRD_RST            (volatile U16 *)    ST5514_HDDI_BASE_ADDRESS + 0x84*/
    #define     ATA_HRD_RST             (volatile U16 *)    ST5514_HDDI_BASE_ADDRESS
    #undef      ATA_INTERRUPT_LEVEL
    #define     ATA_INTERRUPT_LEVEL     5
    #define     HDD_USE_DMA             1

#elif defined(ST_5516) || defined(ST_5517)
    #undef      ATA_BASE_ADDRESS
    #define     ATA_BASE_ADDRESS        (volatile U32 *)    0x70000000
    #define     ATA_INTERRUPT_NUMBER    EXTERNAL_1_INTERRUPT
    #define     ATA_HRD_RST             (volatile U16 *)    0x70000000

#elif defined(ST_5528)
    #define     ATA_INTERRUPT_NUMBER    ST5528_HDDI_INTERRUPT
    #define     ATA_BASE_ADDRESS        (volatile U32 *)    ST5528_HDDI_BASE_ADDRESS
    #define     ATA_HRD_RST             (volatile U16 *)    (ST5528_HDDI_BASE_ADDRESS + 0x84)
    #define     HDD_USE_DMA             1

#elif defined(ST_5100)
    #define     ATA_INTERRUPT_NUMBER    EXTERNAL_1_INTERRUPT
    #define     ATA_BASE_ADDRESS        (volatile U32 *)0x40000000
    #define     ATA_HRD_RST             (volatile U16 *)ATA_BASE_ADDRESS

#elif defined(ST_5162)
	#define     ATA_INTERRUPT_NUMBER    EXTERNAL_1_INTERRUPT
	#define     ATA_BASE_ADDRESS        (volatile U32 *)0x7F400000  /* Bank 2 */
    #define     ATA_HRD_RST             (volatile U16 *)ATA_BASE_ADDRESS
	#define     I2C_SLAVE_ADDRESS       0x4E

#elif defined(ST_51xx_Device)
#error Platform not up-to-date !

#elif defined(ST_7710)
    #define     ATA_INTERRUPT_NUMBER    EXTERNAL_3_INTERRUPT
    #define     ATA_BASE_ADDRESS        (volatile U32 *)0x43000000
    #define     ATA_HRD_RST             (volatile U16 *)ATA_BASE_ADDRESS

#elif defined(ST_7100) || defined(ST_7109)
    #define     ATA_HRD_RST             (volatile U16 *)ATA_BASE_ADDRESS
#if defined (SATA_SUPPORTED)
    #define     ATA_INTERRUPT_NUMBER    SATA_INTERRUPT
    #define     ATA_BASE_ADDRESS        SATA_BASE_ADDRESS
#else
    #define     ATA_INTERRUPT_NUMBER    OS21_INTERRUPT_IRL_ENC_7
    #define     ATA_BASE_ADDRESS        (volatile U32 *)0xA2800000
#endif

#elif defined(ST_7200)
    /* SATA only supported */
    #define     ATA_INTERRUPT_NUMBER    SATA_INTERRUPT
    #define     ATA_BASE_ADDRESS        SATA_BASE_ADDRESS
    #define     ATA_HRD_RST             (volatile U16 *)ATA_BASE_ADDRESS

#else
    #error chip settings not yet defined
#endif

/* Units used for the user point of vue: 1 LBA = 16 sectors=16*512 bytes */
/* Units used by STATAPI                 1 LBA = 1 sectors = 512 bytes   */
/* Units used in macros                  1 Block = 16 LBA = 16*512 bytes   */

#if defined(ST_7020)
#define NBSECTORSTOINJECTATONCE         16 /* 16 *SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK = 128 KBytes at once  */
#else
#define NBSECTORSTOINJECTATONCE         8 /* 8 *SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK = 64 KBytes at once  */
#endif
#define SECTOR_SIZE                     512
#define HDD_NB_OF_SECTORS_PER_BLOCK     16
#define HDD_MIN_NUM_OF_BLOCK            1
#define HDD_MAX_BLOCK_TO_TRANSFERT      HDD_NB_OF_SECTORS_PER_BLOCK
#define HDD_SIZE_OF_PICTURE_START_CODE  8 /* Nb of byte for picture start code + temporal ref and Picture type + 2 bytes to keep aligned on 4 bytes for 5514 */
#define MS_TO_NEXT_TRANSFER             (3*2)
#define HDD_MAX_FILENAME_LENGTH			128

/* Private Types ------------------------------------------------------------ */

/* Structure of the information concerning a I Picture */
typedef struct
{
    U32 IPicturePosition;   /* Position of the I Picture. */
    U32 NbPictureToLastI;   /* Number of picture to reach the next I Picture. */
} HDD_IPictureCharacteristic_t;

/* Structure of the first block of the zone containing a file. */
typedef struct
{
    U32 SizeOfFile;
    U32 NumberOfIPicture;
    HDD_IPictureCharacteristic_t TabIPicturePosition[((SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK)/sizeof(U32))-2];
} HDD_FirstBlock_t;

/* Private Variables ------------------------------------------------------------ */

static STHDD_Handle_t       HDD_Handle;         /* Internal parameters of the HDD process. */
static STATAPI_Handle_t     STATAPI_Handle;     /* Internal parameters of the HDD process. */

/******************************************************************************************/
/************************************ SPEED MANAGEMENT ************************************/
/******************************************************************************************/

static S32  GlobalCurrentSpeed;       /* Current speed wanted by the application. */
static S32  PreviousCurrentBlock;
static S32  PreviousBlocksToRead;

/******************************************************************************************/
/************************************* EVENT  MANAGER *************************************/
/******************************************************************************************/

static STEVT_Handle_t  EvtHandle;  /* handle for the subscriber */

/* Pointer to the copy in memory of the first block stored in HDD for a file. */
static HDD_FirstBlock_t *HDDFirstBlockBuffer_Alloc_p, *HDDFirstBlockBuffer_p;

static U32  HDD_PositionOfIPictureDisplayed;            /* Position of the I Picture displayed during backward */
static BOOL HDD_PositionOfIPictureDisplayedAvailable;   /* Flag to indicate the I Position is available or not. */
static S32  HDD_PreviousDriftTime;

static STATAPI_EvtParams_t STATAPI_EvtCmdData;     /* event's associated data */

semaphore_t     *CmdCompleteEvtSemaphore_p;         /* wait for cmd compl. event */

static U32     ClocksPerSecond ;                    /* Holds the number of clocks per second */

/* Variables copied from PLAYREC (STFAE) to make trictest.c module work in both
 * environments */
static STEVT_EventID_t          PLAYRECi_EVT_END_OF_PLAYBACK;
static STEVT_EventID_t          PLAYRECi_EVT_END_OF_FILE;
static STEVT_EventID_t          PLAYRECi_EVT_ERROR;
static STEVT_EventID_t          PLAYRECi_EVT_PLAY_STOPPED;
static STEVT_EventID_t          PLAYRECi_EVT_PLAY_STARTED;
static STEVT_EventID_t          PLAYRECi_EVT_PLAY_SPEED_CHANGED;
static STEVT_EventID_t          PLAYRECi_EVT_PLAY_JUMP;

/* Global Variables --------------------------------------------------------- */
extern VID_Injection_t VID_Injection[VIDEO_MAX_DEVICE];

static U32 HDDNbOfVideoUnderflowEvent;          /* statistics : nb of events */
static U32 HDDNbOfTreatedVideoUnderflowEvent;   /* statistics : nb of treated events */
static U32 HDDNbOfVideoDriftThresholdEvent;     /* statistics : nb of events */
static U32 HDDNbOfLBARead;                      /* statistics : LBA count */
static U32 HDDNbOfTemporalDiscontinuity;        /* statistics : nb of discont. */
static U32 HDDNbOfRelatedToPrevious;            /* statistics : nb of Rel. to Prev. */
static U32 HDDBitratesSum;                      /* statistics : sum of all the bitrates */
static U32 HDDNbBitrateZero;                    /* statistics : nb of bitrates equal to zero */
static U32 HDDMaxBitrate;                       /* statistics : max bitrate */
static U32 HDDMinBitrate;                       /* statistics : min bitrate */
static U32 HDDAverageBitrate;                   /* statistics : average bitrate */
static U32 HDDNbOfForwardUnderflow;             /* statistics : nb of forward underflow requests */
static U32 HDDNbOfBackwardUnderflow;            /* statistics : nb of backward underflow requests */

static U32 ShiftInjectionCoef = 0;              /* If different from 0, it means that in backward play
                                                 * we have to shift each injection LBA start position
                                                 * with a size equal to ShiftInjectionCoef%, in order to have
                                                 * a bigger overlap than the one requested by the driver */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static BOOL     HDD_TrickmodeEventManagerInit (void);
static void     HDD_TrickmodeEventManagerTask (void);

static BOOL     HDD_Info     (STTST_Parse_t * Parse_p, char *result);
static BOOL     HDD_PlayEvent(STTST_Parse_t * Parse_p, char *result);
static BOOL     HDD_Copy     (STTST_Parse_t * Parse_p, char *result);
static BOOL     HDD_StopEvent(STTST_Parse_t * Parse_p, char *result);
static BOOL     HDD_SetSpeed (STTST_Parse_t * Parse_p, char *result);

static BOOL     HDD_ReadAndInjectBlocks(char *ReadBuffer_p, U32 LBAPositionToRead, U32 PartNumOfBlockToRead);

#if defined (mb282b) || defined (mb275) || defined (mb275_64) || defined (mb314) || defined (mb290) || defined (mb361) || defined (mb382) || defined (mb376) || defined (espresso) || defined(mb390) || defined(mb391) || defined (mb411) || defined(mb634)
/*static void     HDD_TrickModeCallBack ( STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData, const void *SubscriberData_p);*/
static void     HDD_TrickModeCallBack ( STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData);
static BOOL     HDD_CheckRWCompletion( STATAPI_Cmd_t *RWCmd,
                                   ST_ErrorCode_t RWErrorCode,
                                   STATAPI_CmdStatus_t *RWStatus );
#endif

static void     HDD_BlockJumpManagement (S32 *CurrentBlock, S32 StartBlock, S32 EndBlock,
                                         S32 CurrentSpeed, S32 *BlocksToRead,
                                         S32 JumpToPerform, S32 *NumberOfPlaying,
                                         BOOL *Discontinuity, BOOL *RelatedToPrevious);
static U32      HDD_SeekLastIPictureInFile (U32 CurrentBlockRead, HDD_FirstBlock_t*);

#if defined(ST_5516) || defined(ST_5517) || defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
static ST_ErrorCode_t DoCfg(void);
#endif
    /* Extern Functions --------------------------------------------------------- */
#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109)
extern ST_ErrorCode_t GetWriteAddress(void * const Handle, void ** const Address_p);
extern void InformReadAddress(void * const Handle, void * const Address);
#endif
extern void VID_ReportInjectionAlreadyOccurs(U32 CDFifoNb);


/*******************************************************************************
Name        : HDD_Init
Description : Initialization of HDD device
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ else
*******************************************************************************/
ST_ErrorCode_t  HDD_Init (void)
{
    ST_ErrorCode_t ErrCode;
    STATAPI_InitParams_t InitParams;
    STATAPI_OpenParams_t OpenParams;
    STATAPI_DriveType_t DriveType;
    ST_ClockInfo_t ClockInfo;

#if defined(mb282b)
    U16 Name, Version;
#elif defined(mb275) || defined(mb275_64) || defined(mb314) || defined (mb290) || defined (mb361) \
  || defined (mb382) || defined (mb376) || defined (espresso) || defined (mb390) || defined (mb391) || defined (mb411) || defined(mb634)
#else
    /* HDD may not be supported */
    return(FALSE);
#endif

    memset(&InitParams, 0, sizeof(STATAPI_InitParams_t));

    /* --- Specific settings --- */
#ifdef mb282b
    /* For board mb282 rev B & C */
    STFPGA_GetRevision ( &Name, &Version );
    if ( ( Name != 0xBEEF ) || ( Version != 7 ) )
    {
        ErrCode = STFPGA_Init();
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("Unable to init FPGA device\n"));
        }
    }

    /* Change the EMI configuration (bank 3) to set PIO mode 4 */
#if defined(STATAPI_OLD_FPGA)
    *(volatile U32*) 0x2020 = 0x16D1;
    *(volatile U32*) 0x2024 = 0x50F0;
    *(volatile U32*) 0x2028 = 0x400F;
    *(volatile U32*) 0x202C = 0x0002;
#else /* STATAPI_OLD_FPGA */
    /* "New" (supported) FPGA */
    *(volatile U32*) 0x00002030 = 0x1791;
    *(volatile U32*) 0x00002034 = 0x50f0;
    *(volatile U32*) 0x00002038 = 0x50f0;
    *(volatile U32*) 0x0000203C = 0x0002;
#endif /* STATAPI_OLD_FPGA */
#endif /* mb282b */

#if defined(mb275_64) || defined(mb275)
    *(volatile unsigned long*) 0x2010 = 0xe791;

    /* For PIO mode 4 */
    *(volatile unsigned long*) 0x2014 = 0x70b0;
    *(volatile unsigned long*) 0x2018 = 0x70b0;

    /* For PIO mode 2 */
/*    *(volatile unsigned long*) 0x2014 = 0xd0b0;*/
/*    *(volatile unsigned long*) 0x2018 = 0xd0b0;*/

    *(volatile unsigned long*) 0x201C = 0x0002;

    STSYS_WriteRegDev8((void*)0x200387A0, 0x01);    /* Configure FEI_ATAPI_CFG */
    STSYS_WriteRegDev8((void*)0x2000C048, 0x6);     /* Configure PIO0-1 and PIO0-2 */
    STSYS_WriteRegDev8((void*)0x2000C034, 0x6);     /* Glue is internal in 5508 */
    STSYS_WriteRegDev8((void*)0x2000C028, 0x6);
#endif

        ST_GetClockInfo(&ClockInfo);
        InitParams.ClockFrequency = ClockInfo.HDDI;
        if (InitParams.ClockFrequency == 0)
            InitParams.ClockFrequency = ClockInfo.CommsBlock;

#if (defined(ST_5516) || defined(ST_5517) || defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined(ST_7109)) && (!defined(ST_5162))
			/* Need this to Reset HDDI */
		if (DoCfg() != ST_NO_ERROR)
        {
            return 1;
        }
#endif

    /* --- Starts STATAPI --- */

#ifdef HDD_USE_DMA
    InitParams.DeviceType           = STATAPI_SATA;
#else
#if !defined(SATA_SUPPORTED)
    InitParams.DeviceType       = STATAPI_EMI_PIO4;
#else
    InitParams.DeviceType           = STATAPI_SATA;
#endif
#endif

    InitParams.DriverPartition = DriverPartition_p;
#if defined(ATAPI_FDMA)
    InitParams.NCachePartition = NCachePartition_p;
#endif
    InitParams.BaseAddress     = (volatile U32 *)ATA_BASE_ADDRESS;
    InitParams.HW_ResetAddress = ATA_HRD_RST;
    InitParams.InterruptNumber = ATA_INTERRUPT_NUMBER;
    InitParams.InterruptLevel  = ATA_INTERRUPT_LEVEL;

    strcpy(InitParams.EVTDeviceName, STEVT_DEVICE_NAME);

#if defined(ST_5162)
	strcpy(InitParams.I2CDeviceName, STI2C_BACK_DEVICE_NAME);
	InitParams.DeviceCode = I2C_SLAVE_ADDRESS;
#endif

    ErrCode = STATAPI_Init(STATAPI_DEVICE_NAME, &InitParams);

    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("Unable to init STATAPI interface : error=0x%x !\n", ErrCode));
    }
    else
    {
        OpenParams.DeviceAddress = STATAPI_DEVICE_0;
        ErrCode = STATAPI_Open(STATAPI_DEVICE_NAME, &OpenParams, &STATAPI_Handle);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("Unable to open STATAPI device : error=0x%x !\n", ErrCode));
        }
        else
        {
            STTBX_Print(("STATAPI  : %-21.21s Initalized and opened\n", STATAPI_GetRevision()));

            ErrCode= STATAPI_GetDriveType(STATAPI_Handle, &DriveType);
            if (DriveType==STATAPI_ATAPI_DRIVE)
            {
#if defined (SATA_SUPPORTED)
                STTBX_Print(("STATAPI  : Device found at address %d is SATAPI\n",OpenParams.DeviceAddress));
#else
                STTBX_Print(("STATAPI  : Device found at address %d is ATAPI\n",OpenParams.DeviceAddress));
#endif /*SATA_SUPPORTED*/
            }
            else
            {
#if defined (SATA_SUPPORTED)
                STTBX_Print(("STATAPI  : Device found at address %d0 is SATA\n",OpenParams.DeviceAddress));
#else
                STTBX_Print(("STATAPI  : Device found at address %d0 is ATA\n",OpenParams.DeviceAddress));
#endif /*SATA_SUPPORTED*/
            }
#ifdef HDD_USE_DMA
            ErrCode = STATAPI_SetDmaMode(STATAPI_Handle, STATAPI_DMA_MWDMA_MODE_2);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STATAPI  : Unable to set DmaMmode STATAPI_DMA_MWDMA_MODE_2 : error=0x%x !\n", ErrCode));
            }
            else
            {
                STTBX_Print(("STATAPI  : DmaMode set to STATAPI_DMA_MWDMA_MODE_2 \n", ErrCode));
            }
#else
            ErrCode = STATAPI_SetPioMode(STATAPI_Handle, STATAPI_PIO_MODE_4);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STATAPI  : Unable to set PioMode STATAPI_PIO_MODE_4 : error=0x%x !\n", ErrCode));
            }
            else
            {
                STTBX_Print(("STATAPI  : PioMode set to STATAPI_PIO_MODE_4 \n", ErrCode));
            }
#endif /* HDD_USE_DMA */
        }
    }

    /* --- Starts Trickmode Management --- */

    if ( ErrCode == ST_NO_ERROR )
    {
        ErrCode = HDD_TrickmodeEventManagerInit ( );
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("Trickmode Process Manager init. failed !\n"));
        }
        /**else
        {
            ErrCode = HDD_ProcessManagerInit ( );
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("HDD Process Manager init. failed !\n"));
            }
        }**/
    }

    /* --- Start the trickmode module for automatic tests --- */

    if ( ErrCode == ST_NO_ERROR )
    {
        ErrCode|=STEVT_Register(EvtHandle, PLAYREC_EVT_END_OF_PLAYBACK,&PLAYRECi_EVT_END_OF_PLAYBACK);
        ErrCode|=STEVT_Register(EvtHandle, PLAYREC_EVT_END_OF_FILE    ,&PLAYRECi_EVT_END_OF_FILE);
        ErrCode|=STEVT_Register(EvtHandle, PLAYREC_EVT_ERROR          ,&PLAYRECi_EVT_ERROR);
        ErrCode|=STEVT_Register(EvtHandle, PLAYREC_EVT_PLAY_STOPPED   ,&PLAYRECi_EVT_PLAY_STOPPED);
        ErrCode|=STEVT_Register(EvtHandle, PLAYREC_EVT_PLAY_STARTED   ,&PLAYRECi_EVT_PLAY_STARTED);
        ErrCode|=STEVT_Register(EvtHandle, PLAYREC_EVT_PLAY_SPEED_CHANGED,&PLAYRECi_EVT_PLAY_SPEED_CHANGED);
        ErrCode|=STEVT_Register(EvtHandle, PLAYREC_EVT_PLAY_JUMP,&PLAYRECi_EVT_PLAY_JUMP);
        if ( ErrCode == ST_NO_ERROR )
        {
            ErrCode = TRICKMOD_Debug();
        }
    }

    /* --- Set the default current speed --- */

    GlobalCurrentSpeed = 100;

    return (!ErrCode);
} /* end of HDD_Init() */

/*******************************************************************************
Name        : HDD_Term
Description : Termination of HDD device
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t  HDD_Term (void)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR ;
    STATAPI_TermParams_t TermParams;

    ErrCode = STATAPI_Close(STATAPI_Handle);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("Unable to close STATAPI device ! error=0x%x\n", ErrCode));
    }

    TermParams.ForceTerminate = TRUE;
    ErrCode = STATAPI_Term (STATAPI_DEVICE_NAME, &TermParams );
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("Unable to term STATAPI interface ! error=0x%x\n", ErrCode));
    }
    /**else
    {
        ErrCode = HDD_ProcessManagerTerm ( );
    }**/
    return (!ErrCode);
} /* end of HDD_Term() */


/*******************************************************************************
Name        : HDD_Info
Description : Returns HDD driver information
Parameters  : Parse_p, result
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
BOOL HDD_Info (STTST_Parse_t * Parse_p, char *result)
{
    STATAPI_Capability_t STATAPI_Capa;
    ST_ErrorCode_t ErrorCode;

    UNUSED_PARAMETER(Parse_p);
    STTBX_Print( ("HDD informations :\n"));
    STTBX_Print( ("STATAPI revision = %s \n", STATAPI_GetRevision() ));
    ErrorCode = STATAPI_GetCapability(STATAPI_DEVICE_NAME, &STATAPI_Capa);
    if ( !(ErrorCode) )
    {
        STTBX_Print( ("STATAPI supported device type = %d \n", STATAPI_Capa.DeviceType ));
        STTBX_Print( ("STATAPI supported IO modes    = %d \n", STATAPI_Capa.SupportedPioModes ));
        STTBX_Print( ("STATAPI supported DMA modes   = %d \n", STATAPI_Capa.SupportedDmaModes ));
    }
    else
    {
        STTBX_Print( ("STATAPI_GetCapability(\"%s\") failure %d !\n", STATAPI_DEVICE_NAME ));
    }
    STTST_AssignInteger(result, ErrorCode, FALSE);
    return ( FALSE );
} /* end of HDD_Info() */


/*-------------------------------------------------------------------------
 * Function : HDD_CheckRWCompletion
 *            Wait & check I/O completion
 * Input    : RWCmd (command given for I/O action)
 *            RWErrorCode (value immediatly returned by API)
 *            RWStatus (data returned by API)
 * Output   :
 * Returns  : FALSE if no error, else TRUE
 * ----------------------------------------------------------------------*/
static BOOL HDD_CheckRWCompletion( STATAPI_Cmd_t *RWCmd,
                                   ST_ErrorCode_t RWErrorCode,
                                   STATAPI_CmdStatus_t *RWStatus )
{
    BOOL RetBad;
    STATAPI_CmdStatus_t * CmdStat_p;
#ifdef ST_OS21
    osclock_t TimeOut;
#endif
#ifdef ST_OS20
    clock_t TimeOut;
#endif
    int CodeRet;

    /* --- Check command transfer --- */

    RetBad = FALSE;
    if ( RWErrorCode == ST_NO_ERROR || RWErrorCode == ST_ERROR_DEVICE_BUSY )
    {
        TimeOut = time_plus( time_now(), (ClocksPerSecond * 15) ); /* wait 15 seconds */
        CodeRet = semaphore_wait_timeout(CmdCompleteEvtSemaphore_p, &TimeOut);

        if ( CodeRet == -1 ) /* if event not occured */
        {
            RetBad = TRUE;
            if (( RWCmd->CmdCode == STATAPI_CMD_READ_MULTIPLE ) ||
                ( RWCmd->CmdCode == STATAPI_CMD_READ_DMA ))
            {
                STTBX_Print(("HDD read cmd uncomplete on LBA %d !\n", RWCmd->UseLBA ));
            }
            else if(RWCmd->CmdCode == STATAPI_CMD_SET_MULTIPLE_MODE)
            {
                STTBX_Print(("HDD command STATAPI_CMD_SET_MULTIPLE_MODE uncomplete !\n"));
            }
            else
            {
                STTBX_Print(("HDD write cmd uncomplete on LBA %d !\n", RWCmd->UseLBA ));
            }
        }
        else /* if event detected */
        {
            CmdStat_p = STATAPI_EvtCmdData.EvtParams.CmdCompleteParams.CmdStatus ;
            if ((CmdStat_p != NULL) && (CmdStat_p->Error!=0))
            {
                /* meaningfull info if error */
                STTBX_Print(("STATAPI_CMD_COMPLETE_EVT : error=0x%x status=0x%x \n   lba=%d sector=%d cylinder=%d/%d head=%d \n",
                    CmdStat_p->Error, CmdStat_p->Status,
                    CmdStat_p->LBA, CmdStat_p->Sector,
                    CmdStat_p->CylinderLow, CmdStat_p->CylinderHigh,
                    CmdStat_p->Head ));
            }
        }
    }
    else
    {
        RetBad = TRUE;
        /***
        STTBX_Print(("HDD_CheckRWCompletion() : RW cmd RWErrorCode=%d CmdCode=%d Features=%d UseLBA=0x%x LBA=%d LBAExtended=%d Sector=%d CylinderLow=%d CylinderHigh=%d Head=%d SectorCount=%d!\n",
                            RWErrorCode, RWCmd->CmdCode,RWCmd->Features,RWCmd->UseLBA,
                            RWCmd->LBA, RWCmd->LBAExtended, RWCmd->Sector, RWCmd->CylinderLow,
                            RWCmd->CylinderHigh, RWCmd->Head, RWCmd->SectorCount));
        ***/
        STTBX_Print(("HDD_CheckRWCompletion() : RW status Status=0x%x LBA=%d LBAExtended=%d Sector=%d CylinderLow=%d CylinderHigh=%d Head=%d SectorCount=%d!\n",
                            RWStatus->Error, RWStatus->Status, RWStatus->LBA, RWStatus->LBAExtended, RWStatus->Sector, RWStatus->CylinderLow,
                            RWStatus->CylinderHigh, RWStatus->Head, RWStatus->SectorCount ));
    }

    return ( RetBad );

} /* end of HDD_CheckRWCompletion */

/*-------------------------------------------------------------------------
 * Function : HDD_LogIOEvent
 *            Log the occured event (callback function)
 * Input    : Event, EventData
 * Output   : STATAPI_EvtCmdData
 * Return   :
 * ----------------------------------------------------------------------*/
static void HDD_LogIOEvent ( STEVT_CallReason_t Reason,
                             const ST_DeviceName_t RegistrantName,
                              STEVT_EventConstant_t Event,
                             const void *EventData,
                             const void *SubscriberData_p )
{
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(SubscriberData_p);
    switch(Event)
    {
        case STATAPI_CMD_COMPLETE_EVT:
            semaphore_signal(CmdCompleteEvtSemaphore_p);
            memcpy( &STATAPI_EvtCmdData, EventData, sizeof(STATAPI_EvtCmdData) );
            break;
        default:
            break;
    }
} /* end of HDD_LogIOEvent */

/*******************************************************************************
Name        : HDD_PlayEvent
Description : Starts waiting for video's event to read data from HDD.
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
static BOOL     HDD_PlayEvent(STTST_Parse_t * Parse_p, char *result)
{
#if defined (mb282b) || defined (mb275) || defined (mb275_64) || defined (mb314) || defined (mb290) || defined (mb361) || defined (mb382) || defined (mb376) || defined (espresso) || defined(mb390) || defined(mb391) || defined (mb411) || defined(mb634)
    BOOL RetErr;
    ST_ErrorCode_t                          ErrorCode;
    STEVT_SubscribeParams_t                 HddSubscribeParams;
    STVID_DataInjectionCompletedParams_t    DataInjectionCompletedParams;
    STATAPI_Cmd_t       Cmd;
    U32                 BytesRead;
    STATAPI_CmdStatus_t Status;

    /* STEVT_DeviceSubscribeParams_t   HddSubscribeParams; */

    VID_Inj_WaitAccess(0);
    if(VID_Inj_GetInjectionType(0) != NO_INJECTION)
    {
        VID_ReportInjectionAlreadyOccurs(1);
        VID_Inj_SignalAccess(0);
        STTBX_Print(("HDD_PlayEvent() : command cancelled !\n"));
        return(TRUE);
    }
#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109)
    else
    {
        U32 InputBuffersize;
        /* Get Input buffer characteristics     */
        ErrorCode = STVID_GetDataInputBufferParams( VID_Inj_GetDriverHandle(0),
            &VID_Injection[0].Base_p, &InputBuffersize);
        if ( ErrorCode!= ST_NO_ERROR )
        {
            STTBX_Print(("HDD_PlayEvent() : STVID_GetDataInputBufferParams() error %d !\n", ErrorCode));
            VID_Inj_SignalAccess(0);
            return(TRUE);
        }
        /* Update injection pointer.            */
        VID_Injection[0].Top_p  = (void *)((U32)VID_Injection[0].Base_p + InputBuffersize - 1);
        /* Align the write and read pointer to the beginning of the input buffer */
        VID_Injection[0].Write_p = VID_Injection[0].Base_p;
        VID_Injection[0].Read_p  = VID_Injection[0].Base_p;

        /* Configure the interface-object       */
        ErrorCode = STVID_SetDataInputInterface(
                                VID_Inj_GetDriverHandle(0),
                                GetWriteAddress,InformReadAddress,(void*)(0));
        if ( ErrorCode!= ST_NO_ERROR )
        {
            STTBX_Print(("HDD_PlayEvent() : STVID_SetDataInputInterface() error %d !\n", ErrorCode));
            VID_Inj_SignalAccess(0);
            return(TRUE);
        }
        VID_Inj_SetInjectionType(0, VID_HDD_INJECTION);

        /* Video Load Buffer in use */
        /* (TODO) VID_BufferLoad[BufferNb-1].UsedCounter++;*/
        /* Free injection access */
    }
#endif /* defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109) */
    VID_Inj_SignalAccess(0);


    if ( HDD_Handle.ProcessRunning )
    {
        /* The HDD read-out process is already running */
        STTBX_Print(("HDD_PlayEvent() : process already running !\n"));
        return(FALSE);
    }

    /* --- Get input arguments --- */

    RetErr = STTST_GetInteger ( Parse_p , 0 , (S32 *)&HDD_Handle.LBANumber );
    if (RetErr )
    {
        STTST_TagCurrentLine ( Parse_p , "Invalid LBA number selected" );
        return ( TRUE );
    }

    RetErr = STTST_GetInteger ( Parse_p , 0 , &HDD_Handle.NumberOfPlay );
    if (RetErr )
    {
        STTST_TagCurrentLine ( Parse_p , "Invalid number of play (1, 2, 3... or 0=infinite)" );
        return ( TRUE );
    }
    if (HDD_Handle.NumberOfPlay > 0)
    {
        HDD_Handle.InfinitePlaying = FALSE;
    }
    else
    {
        HDD_Handle.InfinitePlaying = TRUE;
    }
    STTBX_Print(("HDD_PlayEvent() : LBA=%d NumberOfPlay=%d InfinitePlaying=%d\n",
        HDD_Handle.LBANumber, HDD_Handle.NumberOfPlay, HDD_Handle.InfinitePlaying));

    /* --- Read the file header --- */

    HDDFirstBlockBuffer_Alloc_p = (HDD_FirstBlock_t *) STOS_MemoryAllocate ( NCachePartition_p, (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) + 255 );
    HDDFirstBlockBuffer_p = (HDD_FirstBlock_t *)(((U32)HDDFirstBlockBuffer_Alloc_p + 255) & 0xffffff00);
    if ( HDDFirstBlockBuffer_Alloc_p == NULL )
    {
        STTBX_Print(("HDD_PlayEvent() : Unable to allocate in memory %d bytes(2)\n", (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) ));
        HDDFirstBlockBuffer_p = NULL;
        STTST_AssignInteger(result, TRUE, FALSE);
        return(TRUE);
    }
    /* Read-out of the first block of the stream */
#ifdef HDD_USE_DMA
    Cmd.CmdCode = STATAPI_CMD_READ_DMA;
#else
    Cmd.CmdCode = STATAPI_CMD_READ_MULTIPLE ;
#endif /* HDD_USE_DMA */
    Cmd.Features = 0x0;
    Cmd.UseLBA = TRUE;
    Cmd.LBA = HDD_Handle.LBANumber*HDD_NB_OF_SECTORS_PER_BLOCK;
    Cmd.LBAExtended = 0;
    Cmd.SectorCount = HDD_NB_OF_SECTORS_PER_BLOCK;

    ErrorCode = STATAPI_CmdIn( STATAPI_Handle, &Cmd,
                                (U8 *)HDDFirstBlockBuffer_p, (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK),
                                &BytesRead, &Status );
    if ( ErrorCode==ST_ERROR_DEVICE_BUSY)
    {
        STTBX_Print(("HDD_PlayEvent() info : ST_ERROR_DEVICE_BUSY while reading 1st block\n"));
        task_delay( ClocksPerSecond / 250);  /* wait 4ms */
        ErrorCode = STATAPI_CmdIn( STATAPI_Handle, &Cmd,
                                (U8 *)HDDFirstBlockBuffer_p, (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK),
                                &BytesRead, &Status );
    }
    RetErr = HDD_CheckRWCompletion( &Cmd, ErrorCode, &Status );
    if (RetErr)
    {
        STTBX_Print(("HDD read first block failure : ErrorCode=%d LBA=%d BytesRead=%d !\n",
                    ErrorCode, Cmd.LBA, BytesRead ));
        memory_deallocate( NCachePartition_p, HDDFirstBlockBuffer_Alloc_p);
        STTST_AssignInteger(result, TRUE, FALSE);
        return(TRUE);
    }
    /**ErrorCode  = STATA_ReadMultiple
            ( (U32)((char *) (HDDFirstBlockBuffer_p)), HDD_Handle.LBANumber, 1, HDD_NB_OF_SECTORS_PER_BLOCK);**/

    /* --- Prepare HDD Handle --- */

    HDD_Handle.LBANumber ++;
    HDD_Handle.LBAStart = HDD_Handle.LBANumber;
    /* Retreive of the size of the file : */
#ifdef ST_5512
    if (HDDFirstBlockBuffer_p->NumberOfIPicture != 0)
    {
        HDD_Handle.LBAEnd =
                HDDFirstBlockBuffer_p->TabIPicturePosition[HDDFirstBlockBuffer_p->NumberOfIPicture].IPicturePosition;
    }
    else
#endif
    {
        HDD_Handle.LBAEnd =
                HDD_Handle.LBAStart + ((HDDFirstBlockBuffer_p->SizeOfFile-1)/(SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK))+1;
    }

#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\n------ HDD_PlayEvent() : LBAStart=%d LBAEnd=%d, NumberOfPlay=%d\r\n",
                HDD_Handle.LBAStart, HDD_Handle.LBAEnd, HDD_Handle.NumberOfPlay));
#endif

    HDD_PositionOfIPictureDisplayed = 0;
    HDD_PositionOfIPictureDisplayedAvailable = FALSE;
    HDD_Handle.UnderFlowEventOccured            = FALSE;
    HDD_Handle.SpeedDriftThresholdEventOccured  = FALSE;
    HDD_PreviousDriftTime = 0;
    HDD_Handle.ProcessRunning = TRUE;

    /* --- Subscribe to trickmode events --- */

    /* Specific call-back function */
    HddSubscribeParams.NotifyCallback       = HDD_TrickModeCallBack;

    ErrorCode =  STEVT_Subscribe (EvtHandle, STVID_DATA_UNDERFLOW_EVT,       &HddSubscribeParams);
    ErrorCode |=  STEVT_Subscribe (EvtHandle, STVID_DATA_OVERFLOW_EVT,        &HddSubscribeParams);
    ErrorCode |=  STEVT_Subscribe (EvtHandle, STVID_SPEED_DRIFT_THRESHOLD_EVT,&HddSubscribeParams);
    if ( ErrorCode!= ST_NO_ERROR )
    {
        STTBX_Print(("HDD_PlayEvent() : STEVT_Subscribe() error %d !\n", ErrorCode));
        RetErr = TRUE;
    }

    /* Do DataInjectionCompleted() to be sure to received a first UNderflow event, because API says
       "The event Underflow is notified once when the bit buffer level is reaching the lower threshold,
       then it is not sent again unless the bit buffer level goes over the upper threshold,
       or unless the function STVID_DataInjectionCompleted() is called." */

    PreviousCurrentBlock = 0x7FFFFFFF;
    PreviousBlocksToRead = 0;
    DataInjectionCompletedParams.TransferRelatedToPrevious = FALSE;
    ErrorCode = STVID_DataInjectionCompleted( VID_Inj_GetDriverHandle(0), &DataInjectionCompletedParams);
    if ( ErrorCode!= ST_NO_ERROR )
    {
        STTBX_Print(("HDD_PlayEvent() : STVID_DataInjectionCompleted() error %d !\n", ErrorCode));
        RetErr = TRUE;
    }
#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\nHDD_PlayEvent() : DataInjectionCompleted(%d) done",DataInjectionCompletedParams.TransferRelatedToPrevious));
#endif
    STTST_AssignInteger(result, ErrorCode, FALSE);

    /* Notify events */
    {
        char Filename[256];

        sprintf(Filename, "LBA%d", HDD_Handle.LBANumber);
        STEVT_Notify(EvtHandle, PLAYRECi_EVT_PLAY_STARTED, Filename);
    }

    return ( RetErr );

#else
    return(TRUE);
#endif
} /* end of HDD_PlayEvent() */
/* ========================================================================
Name        : HDD_Copy
Description : Copy a file from (or to) board HDD to (or from) host HDD.
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
======================================================================== */
static BOOL     HDD_Copy     (STTST_Parse_t * Parse_p, char *result)
{
    S32 RefLBA, RefSec; /* Parameters given by the host (begin)*/
    char FileName[HDD_MAX_FILENAME_LENGTH];  /* Parameters given by the host  (end) */

    S32 NumberAccess = 0;   /* Number of accesses to copy the entire file */

    S32 Loop;           /* Loop counter */

    char                *TestBuffer_Alloc_p, *TestBuffer_p;        /* Temporary buffer of transfering file */
    HDD_FirstBlock_t    *FirstBlockBuffer_Alloc_p, *FirstBlockBuffer_p;/* Pointer to the first block structure */
    long int            FileHandle;         /* File Handle of the file to be copied */

    BOOL Error;                 /* step by step error flag */
    ST_ErrorCode_t ApiError;    /* API error flag */

    U32     LoopInBlock;                /* Loop counter to analyze data in the current block. */
    U32     IPictureIndexPosition =0;   /* Index to store in the array, the position of a I Picture. */
    BOOL    IPictureSeen=FALSE;         /* Flag to know if a I Picture has been seen. */
    U32     NbPictureBetweenI = 0;      /* Number of picture seen between two I Pictures. */
    BOOL    EndOfLoop = FALSE;          /* Flag to indicate the end of the loop. */

    U8      EndOfLastFile[HDD_SIZE_OF_PICTURE_START_CODE];  /* Storage of the end of the previous block. */
    char    FillPattern = 0x00;                               /* Pattern to fill a buffer with */
#ifdef ST_OS21
    struct stat FileStatus;
#endif
    STATAPI_Cmd_t       Cmd;
    STATAPI_CmdStatus_t Status;
    U32                 BytesRead;
    U32                 BytesWritten;

    STOS_Clock_t        StartingTime;

    /* --- Get input arguments --- */

    /* Get the name of host file to be read (or written) */
    Error = STTST_GetString (Parse_p, "",(char *) FileName, HDD_MAX_FILENAME_LENGTH-1 );
    if ( ( Error ) || ( strlen (FileName) == 0 ) )
    {
        STTST_TagCurrentLine ( Parse_p , "Invalid host filename (HDD_MAX_FILENAME_LENGTH-1 characters max.)" );
        Error = TRUE;
    }

    if ( !(Error) )
    {
        /* Get the LBA reference parameter */
        Error = STTST_GetInteger ( Parse_p , 0 , &RefLBA );
        if ( Error )
        {
            STTST_TagCurrentLine ( Parse_p , "Invalid LBA reference" );
            Error = TRUE;
        }
    }

    if ( !(Error) )
    {
        /* Get the the reference of sector parameter */
        Error = STTST_GetInteger ( Parse_p , 0 , &RefSec );
        if ( Error )
        {
            STTST_TagCurrentLine ( Parse_p , "Invalid sector reference (0=host to HDD N=HDD sectors to host)" );
            Error = TRUE;
        }
    }

    /* Checking of all parameters */
    if ( Error )
    {
        STTBX_Print(("Syntax is HDD_COPY <Filename><LBA><NumLBA>\n"));
        STTBX_Print(("  -Filename   : source or destination host file\n"));
        STTBX_Print(("  -LBA        : logic first block array\n"));
        STTBX_Print(("  -NumLBA  : if 0, copy the host file to the HDD\n"));
        STTBX_Print(("                if >0 copy NumLBA from HDD to host file\n"));

        /* End of function */
        return ( TRUE );
    }

    /* --- Memory allocation --- */

    FirstBlockBuffer_Alloc_p = (HDD_FirstBlock_t *) STOS_MemoryAllocate ( NCachePartition_p, sizeof (HDD_FirstBlock_t) + 255 );
    FirstBlockBuffer_p = (HDD_FirstBlock_t *)(((U32)FirstBlockBuffer_Alloc_p + 255) & 0xffffff00);
    if ( FirstBlockBuffer_Alloc_p == NULL )
    {
        STTBX_Print(("HDD_Copy() unable to allocate in memory %d bytes(3)\n", sizeof (HDD_FirstBlock_t) ));
        FirstBlockBuffer_p = NULL;
        return(TRUE);
    }
    else
    {
        FillPattern = 0x00;
        STAVMEM_FillBlock1D((char*) &FillPattern, sizeof(FillPattern), (char*) (FirstBlockBuffer_p), sizeof (HDD_FirstBlock_t));
    }

    /* Constraints : TestBuffer aligned on 4 bytes for HDD read, */
    /* and TestBuffer+HDD_SIZE_OF_PICTURE_START_CODE aligned on 4 bytes for HDD write */
    TestBuffer_Alloc_p = STOS_MemoryAllocate ( NCachePartition_p, (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK)+HDD_SIZE_OF_PICTURE_START_CODE + 0xFF);
    TestBuffer_p = (char *)(((U32)TestBuffer_Alloc_p + 0xFF) & 0xffffff00);
    if ( TestBuffer_Alloc_p == NULL )
    {
        STTBX_Print(("HDD_Copy() unable to allocate in memory %d bytes(4)\n", sizeof (HDD_FirstBlock_t) ));
        /* deallocate the memory used */
        memory_deallocate( NCachePartition_p, FirstBlockBuffer_Alloc_p );
        FirstBlockBuffer_Alloc_p = NULL;
        FirstBlockBuffer_p = NULL;
        TestBuffer_p = NULL;
        return(TRUE);
    }

    /* --- Open the requested file --- */

    if ( RefSec == 0 )
    {
        /* A file should be copied from the host HDD to the Board HDD */
#ifdef ST_OS20
        FileHandle  = debugopen( FileName,"rb" );
#endif /* ST_OS20 */
#ifdef ST_OS21
        FileHandle  = open(FileName, O_RDONLY | O_BINARY  );
#endif /* ST_OS21 */

        if ( FileHandle < 0 )
        {
            STTBX_Print(("HDD_Copy() unable to open Source file %s\n", FileName));
            Error = TRUE;
        }
        else
        {
            #ifdef ST_OS21
            fstat(FileHandle, &FileStatus);
            FirstBlockBuffer_p->SizeOfFile = FileStatus.st_size;
            NumberAccess = ((FirstBlockBuffer_p->SizeOfFile - 1)/(SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK))+1;
            #endif
            #ifdef ST_OS20
            FirstBlockBuffer_p->SizeOfFile = (U32)debugfilesize ( FileHandle );
            NumberAccess = (((U32)debugfilesize ( FileHandle )-1)/(SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK))+1;
            #endif
        }
    }
    else
    {
        /* A file should be copied from the board HDD to the host HDD */
#ifdef ST_OS20
        FileHandle  = debugopen( FileName,"wb" );
#endif /* ST_OS20 */
#ifdef ST_OS21
        FileHandle  = open(FileName, O_WRONLY | O_BINARY  );
#endif /* ST_OS21 */
        if ( FileHandle < 0 )
        {
            STTBX_Print(("HDD_Copy() unable to open Source file %s\n", FileName));
            Error = TRUE;
        }
        else
        {
            NumberAccess = RefSec;
        }
    }
    if ( Error )
    {
        /* deallocate the memory used */
        memory_deallocate( NCachePartition_p, FirstBlockBuffer_Alloc_p );
        memory_deallocate( NCachePartition_p, TestBuffer_Alloc_p );
        return ( TRUE );
    }

    /* --- Copy the data --- */

    RefLBA ++; /* skip the 1st LBA, reserved for header */
    FillPattern = (char) 0xFF;
    STAVMEM_FillBlock1D((char*) &FillPattern, sizeof(FillPattern), (char*) (EndOfLastFile), HDD_SIZE_OF_PICTURE_START_CODE);
    /* to allow start code research in TestBuffer, a fill pattern is added at the begining */

    StartingTime = STOS_time_now();
    for ( Loop = 0 ; Loop < NumberAccess ; Loop ++ )
    {
        if ( RefSec == 0 )
        {
            /* Read of the current block, put it at address TestBuffer+4 */
            BytesRead = (U32)debugread ( FileHandle, (void *) &TestBuffer_p[HDD_SIZE_OF_PICTURE_START_CODE] , (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) );
            if ( BytesRead < (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) )
            {
                /* end of source stream is reached : complete with xFF for stuffing */
                STAVMEM_FillBlock1D((char*) &FillPattern, sizeof(FillPattern),
                        (char*)(&TestBuffer_p[BytesRead+HDD_SIZE_OF_PICTURE_START_CODE]), (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK)-BytesRead );
                /* later, during HDD injection, no unexpected bytes will be encoutered at end of LBA */
            }

            STAVMEM_CopyBlock1D( (void * const) EndOfLastFile, (void * const) TestBuffer_p, sizeof (EndOfLastFile) );

            /* Loop, to test if a picture_start_code is present in this block*/
            for ( LoopInBlock = 0; (LoopInBlock < (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK-1)) && (!(EndOfLoop)) ; LoopInBlock ++)
            {
                if (TestBuffer_p[LoopInBlock]==0x00)
                {
                    if (TestBuffer_p[LoopInBlock+1]==0x00)
                    {
                        if (TestBuffer_p[LoopInBlock+2]==0x01)
                        {
                            if (TestBuffer_p[LoopInBlock+3]==0x00)
                            {
                                if (LoopInBlock < (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK-1))
                                {
                                    /* A picture_start_code is found. Now, we test the kind of Picture.*/
                                    switch ( (TestBuffer_p[LoopInBlock+5]>>3) & 0x07 )
                                    {
                                        case 0x01 : /* I Picture found */

                                            /* Now, we store its position in the array */
                                            if (LoopInBlock < HDD_SIZE_OF_PICTURE_START_CODE)
                                            {
                                                FirstBlockBuffer_p->TabIPicturePosition[IPictureIndexPosition].IPicturePosition = RefLBA+Loop-1;
                                            }
                                            else
                                            {
                                                FirstBlockBuffer_p->TabIPicturePosition[IPictureIndexPosition].IPicturePosition = RefLBA+Loop;
                                            }

                                            if (!(IPictureSeen))
                                            {
                                                FirstBlockBuffer_p->TabIPicturePosition[IPictureIndexPosition].NbPictureToLastI = 0;
                                            }
                                            else
                                            {
                                                FirstBlockBuffer_p->TabIPicturePosition[IPictureIndexPosition].NbPictureToLastI =
                                                        NbPictureBetweenI+1;
                                            }

                                            /* At least one I Picture has been seen. */
                                            IPictureSeen = TRUE;
                                            /* Raz of the number of picture seen between two I Pictures */
                                            NbPictureBetweenI = 0;
                                            /* Increment the number of I Picture */
                                            FirstBlockBuffer_p->NumberOfIPicture ++;

                                            STTBX_Print(("HDD_Copy() (analyze): %03d I Pict. (block %05d, last I %02d pictures before)\n"
                                                    , FirstBlockBuffer_p->NumberOfIPicture
                                                    , FirstBlockBuffer_p->TabIPicturePosition[IPictureIndexPosition].IPicturePosition
                                                    , FirstBlockBuffer_p->TabIPicturePosition[IPictureIndexPosition].NbPictureToLastI));

                                            /* Check if end of array... */
                                            if ( (++IPictureIndexPosition) > (((SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK)/sizeof(U32))-2) )
                                            {

                                                STTBX_Print(("HDD_Copy() (analyze): Too many I Pictures to store !!!\n"));
                                                EndOfLoop = TRUE;
                                            }
                                            break;

                                        case 0x02 : /* P Picture found */
                                        case 0x03 : /* B Picture found */

                                            /* Just increment the number of picture */
                                            NbPictureBetweenI ++;

                                            break;
                                        default:
                                            STTBX_Print(("HDD_Copy() (analyze): None available picture %d at address %d, block %d\n"
                                                    ,(TestBuffer_p[LoopInBlock+5]>>3) & 0x07, LoopInBlock ,Loop));

                                            break;
                                    }
                                }
                            }
                        }
                    }
                }
            } /* end for LoopInBlock */

            STAVMEM_CopyBlock1D(    (void * const) &(TestBuffer_p[SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK]),
                                    (void * const) EndOfLastFile,
                                    sizeof (EndOfLastFile) );

            Cmd.CmdCode = STATAPI_CMD_WRITE_MULTIPLE ;
            Cmd.Features = 0x0;
            Cmd.UseLBA = TRUE;
            Cmd.LBA = HDD_NB_OF_SECTORS_PER_BLOCK*(RefLBA+Loop);
            Cmd.LBAExtended = 0;
            Cmd.SectorCount = (U8)HDD_NB_OF_SECTORS_PER_BLOCK;

            ApiError = STATAPI_CmdOut( STATAPI_Handle, &Cmd,
                                        (U8 *)&TestBuffer_p[HDD_SIZE_OF_PICTURE_START_CODE],
                                        HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE,
                                        &BytesWritten, &Status );
            if ( ApiError==ST_ERROR_DEVICE_BUSY)
            {
                STTBX_Print(("HDD_Copy() info : ST_ERROR_DEVICE_BUSY while writing blocks\n"));
            }
            /**ApiError = STATA_WriteMultiple ( (U32) &TestBuffer_p[HDD_SIZE_OF_PICTURE_START_CODE] , RefLBA+Loop , 1 );**/
            Error = HDD_CheckRWCompletion( &Cmd, ApiError, &Status );
            if (! Error)
            {
                if (Loop % 400 == 50)
                {
                    U32     SpentSeconds = ((U32)STOS_time_minus(STOS_time_now(), StartingTime)/((U32)ST_GetClocksPerSecond()));
                    U32     RemainingSeconds;

                    RemainingSeconds = (SpentSeconds*(NumberAccess - Loop))/Loop;
                    STTBX_Print(("Written LBA:%d Remains:%d Time: ~%d:%.2d:%.2d\n",
                                    RefLBA + Loop,
                                    NumberAccess - Loop,
                                    RemainingSeconds/3600, (RemainingSeconds%3600)/60, RemainingSeconds%60));
                }
            }
            else
            {
                STTBX_Print(("HDD write ApiError=%d LBA=%d BytesWritten=%d 1\n",
                            ApiError, Cmd.LBA, BytesWritten));
                STTST_AssignInteger(result, TRUE, FALSE);
            }
        }
        else
        {
            memset( &Status, 0, sizeof(Status) );
            Cmd.Features = (U8)0x0;
            Cmd.UseLBA = TRUE;
            Cmd.LBA = (RefLBA+Loop)*HDD_NB_OF_SECTORS_PER_BLOCK;
            Cmd.LBAExtended = 0;
            Cmd.SectorCount = (U8)HDD_NB_OF_SECTORS_PER_BLOCK;
#ifdef HDD_USE_DMA
            Cmd.CmdCode = STATAPI_CMD_READ_DMA;
#else
            Cmd.CmdCode = STATAPI_CMD_READ_MULTIPLE ;
#endif /* HDD_USE_DMA */

            ApiError = STATAPI_CmdIn( STATAPI_Handle, &Cmd,
                                        (U8 *)TestBuffer_p,  (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK),
                                        &BytesRead, &Status );
            if ( ApiError==ST_ERROR_DEVICE_BUSY)
            {
                STTBX_Print(("HDD_Copy() info : ST_ERROR_DEVICE_BUSY while reading blocks\n"));
            }
            /**ApiError = STATA_ReadMultiple ( (U32) TestBuffer_p  , RefLBA+Loop , 1 , HDD_NB_OF_SECTORS_PER_BLOCK);**/
            Error = HDD_CheckRWCompletion( &Cmd, ApiError, &Status );
            if (Error)
            {
                STTBX_Print(("HDD read ApiError=%d LBA=%d BytesRead=%d\n",
                            ApiError, Cmd.LBA, BytesRead ));
            }
            else
            {
                debugwrite ( FileHandle, TestBuffer_p , (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) );
            }
        }
    } /* end for Loop */

    /* --- Update the file header on HDD -- */

    if ( RefSec == 0 )
    {
        /* Write the 1st LBA, containing the result of the analysis (header required for HDD injection) */
        RefLBA--;

        Cmd.Features = 0x0;
        Cmd.UseLBA = TRUE;
        Cmd.LBA = RefLBA*HDD_NB_OF_SECTORS_PER_BLOCK;
        Cmd.LBAExtended = 0;
        Cmd.SectorCount = (U8)HDD_NB_OF_SECTORS_PER_BLOCK;
        Cmd.CmdCode = STATAPI_CMD_WRITE_MULTIPLE ;

        ApiError = TRUE;
        Loop = 0;
        while ( (Loop<3) && (ApiError) )
        {
            ApiError = STATAPI_CmdOut( STATAPI_Handle, &Cmd,
                                        (U8 *)FirstBlockBuffer_p,
                                        HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE,
                                        &BytesWritten, &Status );
           if ( ApiError==ST_ERROR_DEVICE_BUSY)
           {
                STTBX_Print(("HDD_Copy() info : ST_ERROR_DEVICE_BUSY while writing header\n"));
           }
            /**ApiError  = STATA_WriteMultiple ( (U32) ((char *) (FirstBlockBuffer_p))  , RefLBA , 1 );**/
            Error = HDD_CheckRWCompletion( &Cmd, ApiError, &Status );
            if (Error)
            {
                STTBX_Print(("Unable to store the first block array\n"));
            }
            Loop++;
        }

    } /* end if RefSec */
    else
    {
            /* Get header info */
            RefLBA--;
            memset( &Status, 0, sizeof(Status) );
            Cmd.Features = (U8)0x0;
            Cmd.UseLBA = TRUE;
            Cmd.LBA = RefLBA*HDD_NB_OF_SECTORS_PER_BLOCK;
            Cmd.LBAExtended = 0;
            Cmd.SectorCount = (U8)HDD_NB_OF_SECTORS_PER_BLOCK;
#ifdef HDD_USE_DMA
            Cmd.CmdCode = STATAPI_CMD_READ_DMA;
#else
            Cmd.CmdCode = STATAPI_CMD_READ_MULTIPLE ;
#endif /* HDD_USE_DMA */

            ApiError = STATAPI_CmdIn( STATAPI_Handle, &Cmd,
                                        (U8 *)FirstBlockBuffer_p,  (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK),
                                        &BytesRead, &Status );
            if ( ApiError==ST_ERROR_DEVICE_BUSY)
            {
                STTBX_Print(("HDD_Copy() info : ST_ERROR_DEVICE_BUSY while reading header\n"));
            }
            Error = HDD_CheckRWCompletion( &Cmd, ApiError, &Status );
            if (Error)
            {
                STTBX_Print(("HDD header read ApiError=%d LBA=%d BytesRead=%d\n",
                            ApiError, Cmd.LBA, BytesRead ));
            }
            else
            {
                STTBX_Print(("Header : SizeOfFile=%d NumberOfIPicture=%d\n",
                    FirstBlockBuffer_p->SizeOfFile, FirstBlockBuffer_p->NumberOfIPicture ));
            }
    }

    /* --- Statistics --- */

    if ( RefSec==0 )
    {
        STTBX_Print(("Status : data written from LBA %d to LBA %d \n         %d bytes in %d sectors (=%d LBA) with %d I pictures\n",
            RefLBA, RefLBA+NumberAccess,
            FirstBlockBuffer_p->SizeOfFile,
            (NumberAccess*HDD_NB_OF_SECTORS_PER_BLOCK),
            NumberAccess,
            (FirstBlockBuffer_p->NumberOfIPicture) ));
    }
    else
    {
        STTBX_Print(("Status : data read from LBA %d to LBA %d \n         %d bytes in %d sectors (=%d LBA) with %d I pictures\n",
            RefLBA, RefLBA+NumberAccess,
            NumberAccess*(SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK),
            (NumberAccess*HDD_NB_OF_SECTORS_PER_BLOCK),
            NumberAccess,
            (FirstBlockBuffer_p->NumberOfIPicture) ));
    }

    if ( Error )
    {
        STTBX_Print(("File may be corrupted or may not have the correct size !\n"));
    }

    /* deallocate the memory used */
    memory_deallocate( NCachePartition_p, FirstBlockBuffer_Alloc_p );
    memory_deallocate( NCachePartition_p, TestBuffer_Alloc_p );
    debugclose ( FileHandle );

    STTST_AssignInteger(result, FALSE, FALSE);
    return ( FALSE );

} /* end of HDD_Copy() */




/*******************************************************************************
Name        : HDD_StopEvent
Description : Stops the event management while trick mode process.
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
static BOOL     HDD_StopEvent (STTST_Parse_t * Parse_p, char *result)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL RetErr;
    U32 TimeOut = 10000;

    RetErr = FALSE;
    UNUSED_PARAMETER(Parse_p);

    VID_Inj_WaitAccess(0);
#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109)
    ErrCode = STVID_DeleteDataInputInterface( VID_Inj_GetDriverHandle(0) );
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("HDD_StopEvent() : STVID_DeleteDataInputInterface() error %d !\n", ErrCode));
        RetErr = TRUE;
    }
#endif /* defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109) */
    VID_Inj_SetInjectionType(0, NO_INJECTION);
    VID_Inj_SignalAccess(0);
    /* Test if the HDD read-out process is running */
    if ( HDD_Handle.ProcessRunning )
    {
        ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_DATA_UNDERFLOW_EVT);
        ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_DATA_OVERFLOW_EVT);
        ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_SPEED_DRIFT_THRESHOLD_EVT);
        if ( ErrCode!= ST_NO_ERROR )
        {
            STTBX_Print(("HDD_StopEvent() : STEVT_Unsubscribe(), error %d !\n", ErrCode));
            RetErr = TRUE;
        }
        else
        {
            STTBX_Print(("HDD_StopEvent() : STEVT_Unsubscribe() well done\n"));
        }

        while ( (!(HDD_Handle.EndOfLoopReached)) && ( TimeOut >0 ) )
        {
            task_delay ( 1 );
            TimeOut --;
        }
        STTBX_Print(("HDD_StopEvent() : playback stopped around LBANumber=%d NumberOfPlay=%d\n",
                HDD_Handle.LBANumber, HDD_Handle.NumberOfPlay ));

        /* De-allocation of HDDFirstBlockBuffer_p */
        if(HDDFirstBlockBuffer_Alloc_p != NULL)
        {
            memory_deallocate( NCachePartition_p, HDDFirstBlockBuffer_Alloc_p );
            HDDFirstBlockBuffer_Alloc_p = NULL;
            HDDFirstBlockBuffer_p = NULL;
        }
        HDD_Handle.ProcessRunning = FALSE;
    }
#if !(defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109))
    else
    {
            STTBX_Print(("HDD_StopEvent() : do nothing because process not running\n", ErrCode));
    }
#endif /* !(defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109)) */
#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\n------ HDD_StopEvent() : ProcessRunning=%d EndOfLoopReached=%d NumberOfPlay=%d\r\n",
                HDD_Handle.ProcessRunning, HDD_Handle.EndOfLoopReached, HDD_Handle.NumberOfPlay));
#endif
    STTST_AssignInteger(result, FALSE, FALSE);

    /* Notify events */
    STEVT_Notify(EvtHandle, PLAYRECi_EVT_PLAY_STOPPED,NULL);

    return (RetErr);

} /* end of HDD_StopEvent() */

/*******************************************************************************
Name        : HDD_SetSpeed
Description : Initialization of the current speed in the driver.
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
static BOOL     HDD_SetSpeed (STTST_Parse_t * Parse_p, char *result)
{
    S32     Speed;         /* Speed wanted by the application */
    BOOL    Error;         /* step by step error flag */

    /* Get the the reference of sector parameter */
    Error = STTST_GetInteger ( Parse_p , 0 , &Speed );
    if ( Error )
    {
        STTST_TagCurrentLine ( Parse_p , "Invalid speed parameter" );
    }
    else
    {
        if (Speed >= 0)
        {
            HDD_PositionOfIPictureDisplayedAvailable = FALSE;

            if (GlobalCurrentSpeed < 0)
            {
                if (HDD_PositionOfIPictureDisplayed >= HDDFirstBlockBuffer_p->NumberOfIPicture-1)
                {
                    HDD_Handle.LBANumber = HDD_Handle.LBAStart;
                }
                else
                {
                    HDD_Handle.LBANumber =
                            HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed+1].IPicturePosition;
                }
            }
        }

        /* Save locally the speed of the system */
        GlobalCurrentSpeed = Speed;
    }
    STTST_AssignInteger(result, Error, FALSE);

    /* Notify events */
    STEVT_Notify(EvtHandle, PLAYRECi_EVT_PLAY_SPEED_CHANGED, &Speed);

    return(Error);

} /* end of HDD_SetSpeed */


/*******************************************************************************
Name        : HDD_TrickmodeEventManagerInit
Description : Initializes the Trickmode event manager.
              This process manages the events that occur from the video driver
              (trickmode feature), and perform the feeding of the CD FIFO via
              a pti_video_dma_inject(..) function call from the hard disk.
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error occured, and TRUE otherwise.
*******************************************************************************/
static BOOL     HDD_TrickmodeEventManagerInit ( void )
{

    ST_ErrorCode_t          ErrCode = ST_NO_ERROR;
    STEVT_OpenParams_t      OpenParams;

#ifdef SAVE_TO_DISK
    HDDFileDescriptor = debugopen(PES_FILE_NAME, "wb" );
#endif /* SAVE_TO_DISK */

    /* Initialize the counting semaphore with the value zero */
    HDD_Handle.TrickModeDriverRequest_p = semaphore_create_fifo(0);

    /* Create a task which will be in charge of trichmode driver events */
    HDD_Handle.TrickModeEventTask_p     = task_create ((void (*)(void *)) HDD_TrickmodeEventManagerTask,
                                        NULL,
                                        HDD_TASK_SIZE,
                                        HDD_TASK_PRIORITY,
                                        "hdd_TrickMode_evt_manager",
                                        task_flags_no_min_stack_size );

    if ( HDD_Handle.TrickModeEventTask_p == NULL )
    {
        STTBX_Print(("HDD_TrickmodeEventManagerInit () : Unable to create the HDD Trickmode event manager\n"));
        return ( TRUE );
    }

    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams, &EvtHandle);
    if ( ErrCode!= ST_NO_ERROR )
    {
        STTBX_Print(("HDD_TrickmodeEventManagerInit() : STEVT_Open() error %d for subscriber !\n", ErrCode));
        semaphore_delete(HDD_Handle.TrickModeDriverRequest_p);
        task_delete(HDD_Handle.TrickModeEventTask_p);
        return ( TRUE );
    }
    else
    {
        STTBX_Print(("HDD_TrickmodeEventManagerInit() : STEVT_Open() for subscriber done\n"));
    }

    return ( FALSE );

} /* end of HDD_TrickmodeEventManagerInit() */


/*******************************************************************************
Name        : HDD_TrickmodeEventManagerTask
Description : Process that keeps running, and perform playback task
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
static void    HDD_TrickmodeEventManagerTask ( void )
{
    ST_ErrorCode_t                          ErrCode = ST_NO_ERROR;
    S32                                     NumOfBlockToRead;       /* Total Number of block to read */
    S32                                     NumOfBlockToSkip;       /* Total number of block to skip */
    S32                                     RealSpeed;              /* Current Speed of the video driver. */
    STEVT_EventConstant_t                   TrickModeEvent = 0;
    STVID_DataInjectionCompletedParams_t    DataInjectionCompletedParams;
    STVID_DataUnderflow_t                   UnderFlowEventData;
    STVID_SpeedDriftThreshold_t             SpeedDriftThresholdData;
    char                *HDDReadBuffer_Alloc_p, *HDDReadBuffer_p;
    BOOL                IsTemporalDiscontinuity ;  /* true when temporal discontinuity with previous injection */
    BOOL                Error;
    BOOL                DoSkip = FALSE;
    U32                 LastLBANumber = 0xFFFFFFFF;
    U32                 PatchedOverlap;
    U32                 BackwardOverlapSize;

#ifdef HDD_TRACE_UART
    /* Variable to measure time. */
    clock_t InjectStartTime;
    U32 TickPerSecond;
    /* Get the time base */
#if !defined(ST_7100)
    TickPerSecond = ST_GetClocksPerSecond();
#endif /*!ST_7100*/
#endif

    Error = FALSE;
    STTBX_Print(("HDD_TrickmodeEventManagerTask() : task started, ready to read HDD\n"));

    HDDReadBuffer_Alloc_p = STOS_MemoryAllocate ( NCachePartition_p, (NBSECTORSTOINJECTATONCE*SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) + 255 );
    /* Put the pointer at an address multipe of 255 (memory alignment) */
    HDDReadBuffer_p = (char *)(((U32)HDDReadBuffer_Alloc_p + 255) & 0xffffff00);
    if ( HDDReadBuffer_p == NULL )
    {
        STTBX_Print(("Unable to allocate %d bytes of memory(6)\n", (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) ));
        HDDReadBuffer_p = NULL;
        return;
    }

    while (1)
    {

        /* --- Stand-by : wait for a video event signaled by HDD_TrickModeCallBack() --- */

        semaphore_wait ( HDD_Handle.TrickModeDriverRequest_p );

        /* --- Event received --- */

        HDD_Handle.EndOfLoopReached = FALSE;

        interrupt_lock();
        if (HDD_Handle.SpeedDriftThresholdEventOccured )
        {
            TrickModeEvent = STVID_SPEED_DRIFT_THRESHOLD_EVT;
        }
        else if (HDD_Handle.UnderFlowEventOccured)
        {
            TrickModeEvent = STVID_DATA_UNDERFLOW_EVT;
        }
        interrupt_unlock();

        if ( ( (HDD_Handle.NumberOfPlay != 0) || (HDD_Handle.InfinitePlaying) )
             && (HDD_Handle.ProcessRunning) )
        {

            /* --- Reading&injection required & events management enabled --- */

            switch (TrickModeEvent)
            {
                case STVID_SPEED_DRIFT_THRESHOLD_EVT :

                    /* Get the data of this event */
                    memcpy(&SpeedDriftThresholdData, &HDD_Handle.SpeedDriftThresholdData, sizeof (STVID_SpeedDriftThreshold_t));

                    /* indicate the event has been used by this process. */
                    interrupt_lock();
                    HDD_Handle.SpeedDriftThresholdEventOccured = FALSE;
                    interrupt_unlock();

                    /*  Available fields for this parameter  :              */
                    /*    - S32 DriftTime;                                  */
                    /*    - U32 BitRateValue; in units of 400 bits/second   */
                    /*    - U32 SpeedRatio;                                 */

#ifdef HDD_TRACE_UART
                    TraceBuffer(("\r\nDRIFT_THRESHOLD request : DriftTime=%d BitRate=%d BlockToSkip=%d ",
                                 SpeedDriftThresholdData.DriftTime, SpeedDriftThresholdData.BitRateValue,
                                 HDD_Handle.NumberOfBlockToSkip));
#endif

                    /* Check for "nothing to do" cases */
                    if ( (!SpeedDriftThresholdData.DriftTime) || (!SpeedDriftThresholdData.BitRateValue) )
                    {
#ifdef HDD_TRACE_UART
                        TraceBuffer(("\r\n!!!!! invalid DRIFT_THRESHOLD data\r\n"));
#endif
                        STTBX_Print(("HDD_TrickmodeEventManagerTask() invalid STVID_SPEED_DRIFT_THRESHOLD_EVT data\n"));
                        break;
                    }

                    SpeedDriftThresholdData.DriftTime += HDD_PreviousDriftTime;

                    /* Calculate the asked jump that will be performed on next STVID_DATA_UNDERFLOW_EVT event
                     * (based on BitRateValue field) */
					HDD_Handle.NumberOfBlockToSkip =
                        (S32) ( ((SpeedDriftThresholdData.DriftTime/100) * (S32)(SpeedDriftThresholdData.BitRateValue/HDD_NB_OF_SECTORS_PER_BLOCK)) )/(18*(SECTOR_SIZE));

                    if (HDD_Handle.NumberOfBlockToSkip == 0)
                    {
                        DoSkip = FALSE;
                        /* The DriftTime is not big enough to perform a jump action. */
                        /* I store it for the next STVID_SPEED_DRIFT_THRESHOLD_EVT event occurrence */
                        HDD_PreviousDriftTime = SpeedDriftThresholdData.DriftTime;
                    }
                    else
                    {
                        DoSkip = TRUE;
                        HDD_PreviousDriftTime = SpeedDriftThresholdData.DriftTime -
                            (S32)((1800*SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK/(S32)(SpeedDriftThresholdData.BitRateValue))
                                  *(S32)HDD_Handle.NumberOfBlockToSkip) ;
                    }
                    break;

                case STVID_DATA_UNDERFLOW_EVT :

                    /* Get the data of this event */
                    memcpy( &UnderFlowEventData, &HDD_Handle.UnderflowEventData, sizeof (STVID_DataUnderflow_t));

                    /* indicate the event has been used by this process. */
                    interrupt_lock();
                    HDD_Handle.UnderFlowEventOccured = FALSE;
                    interrupt_unlock();

                    /* Statistics */
                    HDDNbOfTreatedVideoUnderflowEvent++;
                    if(UnderFlowEventData.BitRateValue == 0)
                    {
                        HDDNbBitrateZero++;
                    }
                    else
                    {
                        if(UnderFlowEventData.BitRateValue > HDDMaxBitrate)
                        {
                            HDDMaxBitrate = UnderFlowEventData.BitRateValue;
                        }
                        if(UnderFlowEventData.BitRateValue < HDDMinBitrate)
                        {
                            HDDMinBitrate = UnderFlowEventData.BitRateValue;
                        }
                        HDDBitratesSum += UnderFlowEventData.BitRateValue;
                        HDDAverageBitrate = HDDBitratesSum / (HDDNbOfTreatedVideoUnderflowEvent - HDDNbBitrateZero);
                    }

                    /*  Available fields for this parameter :               */
                    /*    U32 BitBufferFreeSize;                            */
                    /*    U32 BitRateValue; in units of 400 bits/second     */
                    /*    S32 RequiredTimeJump;                             */
                    /*    U32 RequiredDuration;                             */

#ifdef HDD_TRACE_UART
                    TraceBuffer(("\r\nUNDERFLOW request : BBFree=%d BRate=%d TimeJ=%d Duration=%d",
                                 UnderFlowEventData.BitBufferFreeSize, UnderFlowEventData.BitRateValue,
                                 UnderFlowEventData.RequiredTimeJump, UnderFlowEventData.RequiredDuration));


                    if ( (UnderFlowEventData.RequiredTimeJump<0) &&
                         (-UnderFlowEventData.RequiredTimeJump==UnderFlowEventData.RequiredDuration))
                    {
                        TraceBuffer(("\r\n********** Negative jump equals required duration ! ***************"));
                        STTBX_Print(("\r\n********** Negative jump equals required duration ! ***************\n"));
                    }
#endif
                    /* Statistics */
                    if(UnderFlowEventData.RequiredTimeJump < 0)
                    {
                        HDDNbOfBackwardUnderflow++;
                    }
                    else
                    {
                        HDDNbOfForwardUnderflow++;
                    }

                    IsTemporalDiscontinuity = FALSE;

                    /* Check for "nothing to do" cases */
                    if (UnderFlowEventData.BitBufferFreeSize == 0)
                    {
                        STTBX_Print(("HDD_TrickmodeEventManagerTask(): Invalid STVID_DATA_UNDERFLOW_EVT data\n"));
                        break;
                    }

                    /* Get the speed value */
                    if (STVID_GetSpeed( VID_Inj_GetDriverHandle(0), &RealSpeed) != ST_NO_ERROR)
                    {
                        STTBX_Print(("HDD_TrickmodeEventManagerTask(): Can't get the current speed value\n"));
                        break;
                    }

#ifdef WA_USEDRIFTTIMENOTTIMEJUMP
                    /* --- Calculate the jump required by previous STVID_SPEED_DRIFT_THRESHOLD_EVT --- */

                    if (UnderFlowEventData.RequiredTimeJump == 0)
                    {
                        if (DoSkip)
                        {
                            NumOfBlockToSkip = HDD_Handle.NumberOfBlockToSkip;
                            DoSkip = FALSE;
                        }
                        else
                        {
                            NumOfBlockToSkip = 0;
                        }
                    }
                    else
                    {
                    /* ignore HDD_Handle.NumberOfBlockToSkip set on drift_threshold, because this */
                    /* jump should not be cumulated with the jump required by underflow even      */

                    /* --- Calculate the jump required by current STVID_DATA_UNDERFLOW_EVT --- */

                        NumOfBlockToSkip =
                            (S32) ((((UnderFlowEventData.RequiredTimeJump / 1800) *(S32)(UnderFlowEventData.BitRateValue))
                                    /(HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE)));


                    }
#else
                    /* ignore HDD_Handle.NumberOfBlockToSkip set on drift_threshold, because this */
                    /* jump should not be cumulated with the jump required by underflow even      */

                    /* --- Calculate the jump required by current STVID_DATA_UNDERFLOW_EVT --- */

                    NumOfBlockToSkip =
                        (S32) ((((UnderFlowEventData.RequiredTimeJump / HDD_NB_OF_SECTORS_PER_BLOCK) *(S32)(UnderFlowEventData.BitRateValue))
                                /(1800 * SECTOR_SIZE)));
#endif /* WA_USEDRIFTTIMENOTTIMEJUMP */

                    /* --- Calculate the data size required by current STVID_DATA_UNDERFLOW_EVT --- */
                    if ( (UnderFlowEventData.RequiredDuration == 0) || (UnderFlowEventData.BitRateValue == 0) )
                    {
                        NumOfBlockToRead = UnderFlowEventData.BitBufferFreeSize / (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK);
                    }
                    else
                    {
                        NumOfBlockToRead =
                            (S32) ((((UnderFlowEventData.RequiredDuration / HDD_NB_OF_SECTORS_PER_BLOCK) * (S32)(UnderFlowEventData.BitRateValue))
                                    /(1800 * SECTOR_SIZE)));
                    }
#ifdef HDD_TRACE_UART
                    TraceBuffer(("\r\nUnderflow : jump requested by Underflow is NumOfBlockToSkip=%d NumOfBlockToRead=%d RealSpeed=%d",
                                 NumOfBlockToSkip, NumOfBlockToRead, RealSpeed));
#endif
                    /* --- Check the jump required by current STVID_DATA_UNDERFLOW_EVT --- */

#ifdef ST_5512
                    if (GlobalCurrentSpeed < 0)
                    {
                        /* Backward I only */

                        if (!(HDD_PositionOfIPictureDisplayedAvailable))
                        {
                            HDD_PositionOfIPictureDisplayed = HDD_SeekLastIPictureInFile (HDD_Handle.LBANumber, HDDFirstBlockBuffer_p);
                            HDD_PositionOfIPictureDisplayedAvailable = TRUE;

                            HDD_Handle.LBANumber =
                                HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed].IPicturePosition ;
                        }
                        else
                        {
                            HDD_Handle.LBANumber =
                                HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed].IPicturePosition ;
                        }
                        NumOfBlockToRead = HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed+1].IPicturePosition
                            - HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed].IPicturePosition ;

                        IsTemporalDiscontinuity = TRUE;
                    }
                    else
#endif
                    {
                        /* Forward or smooth Backward */
                        HDD_BlockJumpManagement (&HDD_Handle.LBANumber, HDD_Handle.LBAStart,
                                                 HDD_Handle.LBAEnd, RealSpeed,
                                                 &NumOfBlockToRead, NumOfBlockToSkip,
                                                 &HDD_Handle.NumberOfPlay, &IsTemporalDiscontinuity,
                                                 &DataInjectionCompletedParams.TransferRelatedToPrevious);
                    }

                    /* Check if the size of data to transmit is compatible with the BitBufferFreeSize. */
                    if ((NumOfBlockToRead * SECTOR_SIZE * HDD_NB_OF_SECTORS_PER_BLOCK) > (S32) UnderFlowEventData.BitBufferFreeSize)
                    {
                        NumOfBlockToRead = (UnderFlowEventData.BitBufferFreeSize/(SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK));
                        IsTemporalDiscontinuity = TRUE;
                    }

                    /* Now patch the overlap size (WARNING : THIS CODE IS ONLY USED FOR TESTING THE PICTURE MATCHING
                     * ALGORITHME IN BACKWARD PLAYBACK). THIS SHOULD NOT BE CONSIDERED AS THE NORMAL BEHAVIOUR. */
                    if(ShiftInjectionCoef != 0)
                    {
                        /* Overlap patching is enabled */
                        if(  (UnderFlowEventData.RequiredTimeJump < 0)
                           &&((HDD_Handle.LBANumber + NumOfBlockToRead) > (S32)LastLBANumber))
                        {
                            /* We are in smooth backward and we are sure there is an overlap */
                            BackwardOverlapSize = (HDD_Handle.LBANumber + NumOfBlockToRead) - LastLBANumber ;
                            PatchedOverlap      = (NumOfBlockToRead * ShiftInjectionCoef) / 100 ;
                            /* We assume that when we force a different overlap, the new overlap is always bigger bigger
                             * than the one required by the driver. */
                            if(PatchedOverlap > BackwardOverlapSize)
                            {
                                HDD_Handle.LBANumber = (LastLBANumber - NumOfBlockToRead) + PatchedOverlap;
                            }
                        }
                    }
                    /* -------------------------- End of patching --------------------------------------------------*/

                    /* --- Inform STVID of temporal discontinuity before sending data --- */

                    if (IsTemporalDiscontinuity)
                    {
                        STVID_InjectDiscontinuity( VID_Inj_GetDriverHandle(0));
#ifdef HDD_TRACE_UART
                        TraceBuffer((" (discontinuity injected)"));
#endif
                        HDDNbOfTemporalDiscontinuity++;
                    }
#ifdef HDD_TRACE_UART
                    TraceBuffer(("\r\nUnderflow : do read at LBANumber=%d to %d (%d blocks)", HDD_Handle.LBANumber,
                                 (HDD_Handle.LBANumber+NumOfBlockToRead-1), NumOfBlockToRead));
                    InjectStartTime = time_now();
#endif

                    /* --- Read HDD & send NumOfBlockToRead of data to video driver --- */
                    LastLBANumber = HDD_Handle.LBANumber;
                    Error = HDD_ReadAndInjectBlocks( HDDReadBuffer_p,
                                                    HDD_Handle.LBANumber,
                                                    NumOfBlockToRead);

                    HDD_Handle.LBANumber += NumOfBlockToRead;
                    HDDNbOfLBARead += NumOfBlockToRead;
                    if (RealSpeed > 0)
                    {
                        if (HDD_Handle.LBANumber > HDD_Handle.LBAEnd)
                        {
                            HDD_Handle.LBANumber = HDD_Handle.LBAStart;
                            /* when exactly the last LBA has been read at EOF, HDD_BlockJumpManagement() had not decrease the loop count */
                            if (HDD_Handle.NumberOfPlay>0)
                            {
                                HDD_Handle.NumberOfPlay-- ;
                            }
                        }
                    }

                    /* --- Inform STVID that injection is done --- */

                    ErrCode = STVID_DataInjectionCompleted( VID_Inj_GetDriverHandle(0), &DataInjectionCompletedParams);
                    if ( ErrCode != ST_NO_ERROR )
                    {
                        STTBX_Print(("HDD_TrickmodeEventManagerTask() : error %d when calling STVID_DataInjectionCompleted() !\n", ErrCode));
#ifdef HDD_TRACE_UART
                    TraceBuffer((" (injection completed(%d) :%dms)\r\n",
                            DataInjectionCompletedParams.TransferRelatedToPrevious ,
                            (time_minus(time_now(), InjectStartTime)*1000)/TickPerSecond  ));
#endif
                    }
                    else
                    {
                        TraceBuffer(("\r\nInjection done (%d)", NumOfBlockToRead));
                    }
                    if (DataInjectionCompletedParams.TransferRelatedToPrevious)
                    {
                        HDDNbOfRelatedToPrevious++;
                    }
                    if ((HDD_Handle.NumberOfPlay == 0) && (!HDD_Handle.InfinitePlaying))
                    {
#ifdef HDD_TRACE_UART
                        TraceBuffer(("\r\nUnderflow : end of playing, NumOfPlay=%d",
                            HDD_Handle.NumberOfPlay));
#endif
                        STEVT_Notify(EvtHandle, PLAYRECi_EVT_END_OF_FILE, NULL);
                        break;
                    }
                    break;

                case STVID_DATA_OVERFLOW_EVT :
                    break;

                default :
                    /* Nothing special to do !!! */
                    break;

            } /* End of switch (HDD_Handle.TrickModeEvent) */
            TrickModeEvent = 0; /* set a value different from STVID_DATA_OVERFLOW_EVT or STVID_DATA_UNDERFLOW_EVT */
            /* to avoid executing the case several times for the same event. */
        }
        else
        {
            /* --- Reading&injection is ended or events management is disabled --- */

            STTBX_Print(("HDD_TrickmodeEventManagerTask() : end of playing\n    NumberOfPlay=%d InfinitePlaying=%d ProcessRunning=%d\n",
                    HDD_Handle.NumberOfPlay, HDD_Handle.InfinitePlaying , HDD_Handle.ProcessRunning ));

            /* De-allocation of HDDFirstBlockBuffer_p */
            if (HDDFirstBlockBuffer_Alloc_p != NULL)
            {
                memory_deallocate( NCachePartition_p, HDDFirstBlockBuffer_Alloc_p );
                HDDFirstBlockBuffer_Alloc_p = NULL;
                HDDFirstBlockBuffer_p = NULL;
            }

            ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_DATA_UNDERFLOW_EVT);
            ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_DATA_OVERFLOW_EVT);
            ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_SPEED_DRIFT_THRESHOLD_EVT);

            HDD_Handle.ProcessRunning = FALSE;

            if ( ErrCode!= ST_NO_ERROR )
            {
                STTBX_Print(("HDD_TrickmodeEventManagerTask() : STEVT_Unsubscribe(), error %d !\n", ErrCode));
            }
            else
            {
                STTBX_Print(("HDD_TrickmodeEventManagerTask() : STEVT_Unsubscribe() well done\n"));
            }
        }
        HDD_Handle.EndOfLoopReached = TRUE;

    } /* end of while (1) */

    memory_deallocate( NCachePartition_p, HDDReadBuffer_Alloc_p );

} /* end of HDD_TrickmodeEventManagerTask */

/*******************************************************************************
Name        : HDD_ReadBlocks
Description : Read data from HDD
Parameters  : Statapi cmd., buffer address, buffer size
Assumptions : buffer allocated for read data
Limitations :
Returns     : TRUE if error
*******************************************************************************/
static BOOL HDD_ReadBlocks(STATAPI_Cmd_t *Cmd, char *ReadBuffer_p, U32 BufferSize)
{
    ST_ErrorCode_t      ErrCode;
    BOOL                Error;
    U32                 BytesRead;
    STATAPI_CmdStatus_t Status;

    BytesRead = 0;
    ErrCode = STATAPI_CmdIn( STATAPI_Handle, Cmd,
                                    (U8 *)ReadBuffer_p,
                                    BufferSize,
                                    &BytesRead, &Status );
    if (ErrCode==ST_ERROR_DEVICE_BUSY)
    {
           STTBX_Print(("HDD_ReadAndInjectBlocks() info : ST_ERROR_DEVICE_BUSY while reading HDD\n"));
    }
    Error = HDD_CheckRWCompletion( Cmd, ErrCode, &Status);
    if (Error)
    {
            STTBX_Print(("HDD read LBA failure : ErrCode=%d Status=%0x0 LBA=%d BytesRead=%d Cmd.SectorCount=%d BufferSize=%d !\n",
                            ErrCode, Status.Status, Cmd->LBA, BytesRead, Cmd->SectorCount, BufferSize ));
    }
    return(Error);

} /* HDD_ReadBlocks() */

/*******************************************************************************
Name        : HDD_InjectBlocks
Description : Inject HDD data to video bit buffer
Parameters  : buffer address, buffer size
Assumptions : buffer allocated for read data
Limitations :
Returns     : none
*******************************************************************************/
static void HDD_InjectBlocks(char *ReadBuffer_p, U32 BufferSize)
{
#ifndef DVD_TRANSPORT_STPTI
    BOOL                Error;
#endif
#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109)
    U32 InputBufferFreeSize,CopiedNB;
    void*               Read_p; /* Shortcuts */
    void*               Write_p; /* Shortcuts */
#else /* not ST_5100 */
#ifdef DVD_TRANSPORT_STPTI
    STPTI_DMAParams_t   STDMA_Params;
#ifdef DVD_TRANSPORT_STPTI4
    U32 lDMAUSed;
#endif /* DVD_TRANSPORT_STPTI4 */
#endif /* DVD_TRANSPORT_STPTI */
#endif /* not ST_5100 */


#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109)
    clock_t t_next;
    clock_t period;
#endif /* ST_51xx_Device || ST_7710 || ST_7100 || ST_7109 */

            /* --- Data injection in bit buffer --- */

#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109)
            /* copy 'buffer-size' in input buffer */
            CopiedNB = 0;
#ifdef HDD_INJECTION_TRACE_UART
            TraceBuffer(("Begin copy to input buffer(1)\r\n"));
#endif /* HDD_INJECTION_TRACE_UART */

            while(CopiedNB != BufferSize)
            {
                /* As the Read pointer can be changed by the driver, take a copy at the beginning and work with it */
                Read_p = VID_Injection[0].Read_p;
                Write_p= VID_Injection[0].Write_p;
                if(Read_p > Write_p)
                {
                    InputBufferFreeSize = (U32)Read_p
                                        - (U32)Write_p;
                }
                else
                {
                    InputBufferFreeSize = (U32)VID_Injection[0].Top_p
                                        - (U32)Write_p + 1
                                        + (U32)Read_p
                                        - (U32)VID_Injection[0].Base_p;
                }
                /* Leave always 1 empty byte to avoid Read=Write */
                if(InputBufferFreeSize != 0)
                {
                    InputBufferFreeSize-=1;
                }

                if(CopiedNB + InputBufferFreeSize >= BufferSize)
                {
                    InputBufferFreeSize = BufferSize-CopiedNB;
                }
                /* Input buffer is full ? So, wait for one transfer period */
                if(InputBufferFreeSize == 0)
                {
                    /* calculate the periode for transfers (constant) */
#ifdef HDD_INJECTION_TRACE_UART
                    TraceBuffer(("Waiting !\r\n"));
#endif /* HDD_INJECTION_TRACE_UART */
                    period = ClocksPerSecond * MS_TO_NEXT_TRANSFER / 1000;
                    t_next = time_plus(time_now(), period);
                    task_delay_until(t_next);
                    continue;
                }
                /* Write to the input buffer */
                if((U32)Write_p + InputBufferFreeSize > (U32)VID_Injection[0].Top_p + 1)
                {
                    /* First write (in case of loop) */
#ifndef	DONT_USE_FDMA
                    STAVMEM_CopyBlock1D(/* src  */ (void*)((U32)ReadBuffer_p + CopiedNB),
                                        /* dest */ Write_p,
                                        /* size */(U32)VID_Injection[0].Top_p - (U32)Write_p + 1);
#else
                    memcpy              (/* dest */ Write_p,
                                        /* src  */ (void*)((U32)ReadBuffer_p + CopiedNB),
                                        /* size */(U32)VID_Injection[0].Top_p - (U32)Write_p + 1);
#endif  /* DONT_USE_FDMA */
#ifdef HDD_INJECTION_TRACE_UART
                    TraceBuffer(("src=%08x dest=%08x bytes=%08x Top=%08x Base=%08x\r\n",
                        ((U32)ReadBuffer_p + CopiedNB),
                        Write_p,
                        (U32)VID_Injection[0].Top_p - (U32)Write_p + 1,
                        (U32)VID_Injection[0].Top_p,
                        (U32)VID_Injection[0].Base_p));
#endif /* HDD_INJECTION_TRACE_UART */
                    InputBufferFreeSize -= (U32)VID_Injection[0].Top_p - (U32)Write_p + 1;
                    CopiedNB += (U32)VID_Injection[0].Top_p - (U32)Write_p + 1;
                    Write_p = VID_Injection[0].Base_p;
                    TraceBuffer(("Looping !\r\n"));
                }
#ifndef	DONT_USE_FDMA
                STAVMEM_CopyBlock1D(/* src  */ (void*)((U32)ReadBuffer_p + CopiedNB),
                                    /* dest */ Write_p,
                                    /* size */InputBufferFreeSize);
#else
                memcpy				(/* dest */ Write_p,
					 /* src  */ (void*)((U32)ReadBuffer_p + CopiedNB),
                                    /* size */InputBufferFreeSize);

#endif  /* DONT_USE_FDMA */
#ifdef SAVE_TO_DISK
                debugwrite(HDDFileDescriptor,
                        (void*)((U32)ReadBuffer_p + CopiedNB),
                        InputBufferFreeSize);
#endif /* SAVE_TO_DISK */
#ifdef HDD_INJECTION_TRACE_UART
                TraceBuffer(("src=%08x dest=%08x bytes=%08x Top=%08x Base=%08x\r\n",
                    ((U32)ReadBuffer_p + CopiedNB),
                    Write_p,
                    InputBufferFreeSize,
                    (U32)VID_Injection[0].Top_p,
                    (U32)VID_Injection[0].Base_p));
#endif /* HDD_INJECTION_TRACE_UART */
                Write_p = (void*)((U32)Write_p + InputBufferFreeSize);
                if(Write_p == (void*)((U32)VID_Injection[0].Top_p + 1))
                {
                    Write_p = VID_Injection[0].Base_p;
                }
                VID_Injection[0].Write_p = Write_p;
                CopiedNB += InputBufferFreeSize;
            }
#else /* not ST_51xx_Device and not ST_7710 and not ST_7100 and not ST_7109 */
#ifndef DVD_TRANSPORT_STPTI
#ifdef DVD_TRANSPORT_LINK
            /* PLE : new since link 2.0.0 : pti_video_dma_open has to be called before any pti_dma_inject */
            pti_video_dma_open();
#endif /* DVD_TRANSPORT_LINK */
            /* The stream of data is directly injected to decoder */
            pti_video_dma_inject ( (unsigned char*)ReadBuffer_p, (int)BufferSize );
            Error = pti_video_dma_synchronize ( );
            if (Error)
            {
                STTBX_Print(("pti_video_dma_synchronize() : failed !\n"));
            }
#ifdef DVD_TRANSPORT_LINK
            /* PLE : new since link 2.0.0 : pti_video_dma_open has to be called before any pti_dma_inject */
            pti_video_dma_close();
#endif /* DVD_TRANSPORT_LINK */

#else /* else #ifndef DVD_TRANSPORT_STPTI */
#if defined(ST_7020)
            STDMA_Params.Destination = VIDEO_CD_FIFO1;
            STDMA_Params.WriteLength = 4;
#else   /* of #if defined(ST_7020)*/
            STDMA_Params.Destination = VIDEO_CD_FIFO;
            STDMA_Params.WriteLength = 1;
#endif  /* of #if defined(ST_7020)*/
            STDMA_Params.Holdoff = 1;
            STDMA_Params.CDReqLine = STPTI_CDREQ_VIDEO;
#ifdef DVD_TRANSPORT_STPTI4
            STDMA_Params.BurstSize = STPTI_DMA_BURST_MODE_ONE_BYTE;
            STDMA_Params.DMAUsed = &lDMAUSed;
#endif /* DVD_TRANSPORT_STPTI4 */

            ErrCode = STPTI_UserDataWrite((U8 *)ReadBuffer_p, (U32)BufferSize, &STDMA_Params);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STPTI_UserDataWrite(): error=%d !\n", ErrCode));
                Error = TRUE;
            }
            else
            {
                ErrCode = STPTI_UserDataSynchronize(&STDMA_Params);
                if ( ErrCode != ST_NO_ERROR )
                {
                    STTBX_Print(("STPTI_UserDataSynchronize() : error=%d !\n", ErrCode));
                    Error = TRUE;
                }
            }
#endif  /* of #ifndef DVD_TRANSPORT_STPTI */
#endif /* not ST_51xx_Device and not ST_7710 and not ST_7100 and not ST_7109 */


} /* HDD_InjectBlocks() */

/*******************************************************************************
Name        : HDD_ReadAndInjectBlocks
Description : Read data from HDD and inject them to the bit buffer
              (do it by step of HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE bytes)
Parameters  : address of the buffer, LBA, nb of blocks
Assumptions : buffer allocated with size=2*(HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE)
Limitations :
Returns     : TRUE if error
*******************************************************************************/
static BOOL HDD_ReadAndInjectBlocks(char *ReadBuffer_p,
                                    U32 LBAPositionToRead, U32 PartNumOfBlockToRead)
{
    BOOL                Error;
    STATAPI_Cmd_t       Cmd;
    U32                 Count;
    U32                 MaxCount;
    U32                 BytesRead;
    U32                 BufferSize;
    U32                 Coeff;

    /**ErrCode = STATA_ReadMultiple ( 0, LBAPositionToRead, PartNumOfBlockToRead, HDD_NB_OF_SECTORS_PER_BLOCK);**/

    /* Units conversion for data addressing on HDD :                                  */
    /* When STVID requests 1 block, STATAPI should read (16 sectors x 512 bytes)=8Kb  */
    /* When STVID requests the LBA number N, STATAPI should select address (N x 16)   */
    /* because for STATAPI 1 LBA is equal to 1 sectors = 512 bytes                    */

    Error = FALSE;

    #ifdef HDD_USE_DMA
    Cmd.CmdCode = STATAPI_CMD_READ_DMA;
    #else
    Cmd.CmdCode = STATAPI_CMD_READ_MULTIPLE ;
    #endif /* HDD_USE_DMA */
    Cmd.Features = (U8)0x0;
    Cmd.UseLBA = TRUE;
    Cmd.LBA = (LBAPositionToRead*HDD_NB_OF_SECTORS_PER_BLOCK);
    Cmd.LBAExtended = 0;

/*#define TRACE_HDD_READ 1*/

    Coeff = NBSECTORSTOINJECTATONCE; /* set it to 1, 2, 3... to read 8KB, 16KB, 24KB at each HDD read */
                /* (buffer pointed by ReadBuffer_p should be sized accordingly)  */
    BufferSize = Coeff*(HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE);
    Cmd.SectorCount = (U8)Coeff*HDD_NB_OF_SECTORS_PER_BLOCK;

    Count=0;
    /* at each loop, read Coeff blocks (or Coeff*HDD_NB_OF_SECTORS_PER_BLOCK sectors) */
    MaxCount = PartNumOfBlockToRead / Coeff ;

#ifdef TRACE_HDD_READ
    STTBX_Print(("Request : LBAPositionToRead=%d PartNumOfBlockToRead=%d (or %d sectors)\n",
                LBAPositionToRead, PartNumOfBlockToRead, PartNumOfBlockToRead*HDD_NB_OF_SECTORS_PER_BLOCK));
    STTBX_Print(("Repeat  : do MaxCount=%d times read&inject Cmd.SectorCount=%d in BufferSize=%d\n",
                MaxCount, Cmd.SectorCount, BufferSize));
#endif
#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\nRead&Inject : LBAPositionToRead=%d PartNumOfBlockToRead=%d (or %d sectors)",
                LBAPositionToRead, PartNumOfBlockToRead, PartNumOfBlockToRead*HDD_NB_OF_SECTORS_PER_BLOCK));
    TraceBuffer(("\r\n              %d times Cmd.SectorCount=%d in BufferSize=%d ",
                MaxCount, Cmd.SectorCount, BufferSize));
#endif

    /* --- Read HDD and inject data --- */

    while ( (Count < MaxCount) && (!(Error)) )
    {
        Error = HDD_ReadBlocks( &Cmd, ReadBuffer_p, BufferSize);
        if (!Error)
        {
#ifdef TRACE_HDD_READ
            STTBX_Print(("Inj. Count=%d Cmd.LBA=%d BufferSize=%d\n", Count, Cmd.LBA, BufferSize));
#endif
            HDD_InjectBlocks( ReadBuffer_p, BufferSize);

            Cmd.LBA = Cmd.LBA + (Coeff*HDD_NB_OF_SECTORS_PER_BLOCK);
            Count++;
        } /* end read completed */
    } /* end while Count */

    /* --- Read and inject remaining data --- */

    MaxCount = PartNumOfBlockToRead - (Coeff*MaxCount) ; /* nb of remaining blocks */
    if ( MaxCount > 0 )
    {
        BytesRead = 0;
        BufferSize = MaxCount*(HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE);
        Cmd.SectorCount = (U8)MaxCount*HDD_NB_OF_SECTORS_PER_BLOCK;
#ifdef TRACE_HDD_READ
    STTBX_Print(("Remaining : do 1 read&inject Cmd.SectorCount=%d in BufferSize=%d\n",
                Cmd.SectorCount, BufferSize));
#endif
#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\n              1 time LBA=%d Cmd.SectorCount=%d in BufferSize=%d",
                (Cmd.LBA/HDD_NB_OF_SECTORS_PER_BLOCK), Cmd.SectorCount, BufferSize));
#endif
        Error = HDD_ReadBlocks( &Cmd, ReadBuffer_p, BufferSize);
        if (!Error)
        {
            HDD_InjectBlocks( ReadBuffer_p, BufferSize);
        } /* end read completed */
    } /* end if remaining blocks */

    return(Error);

} /* end of HDD_ReadAndInjectBlocks() */

#if defined (mb282b) || defined (mb275) || defined (mb275_64) || defined (mb314) || defined (mb290) || defined (mb361) || defined (mb382) || defined (mb376) || defined (espresso) || defined (mb390) || defined(mb391) || defined (mb411) || defined(mb634)
/*******************************************************************************
Name        : HDD_TrickModeCallBack
Description : Process running on STVID_DATA_UNDERFLOW_EVT, STVID_DATA_OVERFLOW_EVT
              or STVID_SPEED_DRIFT_THRESHOLD_EVT event occurrence.
Parameters  : Reason, type of the event.
              Event, nature of the event.
              EventData, data associted to the event.
Assumptions : At least one event have been subscribed.
Limitations :
Returns     : Nothing.
*******************************************************************************/
static void     HDD_TrickModeCallBack
/*        ( STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData, const void *SubscriberData_p)*/
        ( STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData)
{
    UNUSED_PARAMETER(Reason);
    switch (Event)
    {
        case STVID_DATA_OVERFLOW_EVT :
            break;

        case STVID_DATA_UNDERFLOW_EVT :
            /* Check if the event is idle, or not yet used */
            if (!(HDD_Handle.UnderFlowEventOccured))
            {
                /* Save the event nature and the event data */
                HDD_Handle.UnderFlowEventOccured = TRUE;

                memcpy(&HDD_Handle.UnderflowEventData, EventData, sizeof (STVID_DataUnderflow_t));

                /* Signal the occurence of this event. */
                semaphore_signal ( HDD_Handle.TrickModeDriverRequest_p );
            }
            HDDNbOfVideoUnderflowEvent++;
            break;

        case STVID_SPEED_DRIFT_THRESHOLD_EVT :
            /* Check if the event is idle, or not yet used */
            if (!(HDD_Handle.SpeedDriftThresholdEventOccured))
            {
                /* Save the event nature and the event data */
                HDD_Handle.SpeedDriftThresholdEventOccured = TRUE;

                memcpy(&HDD_Handle.SpeedDriftThresholdData, EventData, sizeof (STVID_SpeedDriftThreshold_t));

                /* Signal the occurence of this event. */
                semaphore_signal ( HDD_Handle.TrickModeDriverRequest_p );
            }
            HDDNbOfVideoDriftThresholdEvent++;
            break;

        default :
            /* Nothing special to do !!! */
            break;
    }

} /* end of HDD_TrickModeCallBack */
#endif

/*******************************************************************************
Name        : HDD_BlockJumpManagement
Description : Calculation of new blocks to be read according to Underflow event
Input       : CurrentBlock  (current position of the playing)
              StartBlock    (begin of the stream)
              EndBlock      (end of the stream)
              Speed         (current speed)
              BlocksToRead  (number of blocks to be read)
              JumptoPerform (number of blocks to jump)
              NumberOfPlaying (number of playing the sequence should make)
              Discontinuity  (true if no temporal continuity)
              RelatedToPrevious (true if related to previous)
Output      : CurrentBlock  (new position to be read)
              BlocksToRead  (new number of blocks to be read)
              NumberOfPlaying (number of playing the sequence should make)
              Discontinuity  (true if no temporal continuity)
              RelatedToPrevious (true if related to previous)
Returns     : None
*******************************************************************************/
static void HDD_BlockJumpManagement (S32 *CurrentBlock, S32 StartBlock, S32 EndBlock,
                                     S32 CurrentSpeed, S32 *BlocksToRead,
                                     S32 JumpToPerform, S32 *NumberOfPlaying,
                                     BOOL *Discontinuity, BOOL *RelatedToPrevious)
{
    S32 RelativePosition;
    S32 NewBlock;
    S32 NextEndOfRead, NextStartOfRead;

    RelativePosition = *CurrentBlock + JumpToPerform ;
    *RelatedToPrevious = FALSE;

    NextEndOfRead = RelativePosition + *BlocksToRead - 1;
    NextStartOfRead = RelativePosition;

    if ((JumpToPerform<0) && (*BlocksToRead == -JumpToPerform))
    {
#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\nNegative Jump equals # of block to read %d %d %d\r\n",*CurrentBlock, JumpToPerform, *BlocksToRead));
#endif
    }
    if ((NextStartOfRead >= StartBlock) && (NextEndOfRead <= EndBlock))
    {
        /* requirement is [......###.] on HDD, do read [......###.] */
        NewBlock = RelativePosition;
    }
    else
    {
        if ((*NumberOfPlaying > 1) || (HDD_Handle.InfinitePlaying))
        {
            /* if loop has to be done : sent a all requested amount from new suitable location according to speed sign */
            if (CurrentSpeed < 0)
            {
                /* requirement is ##[#.........] or [........#]## on HDD, do read [.......###] */
                /* Check the request don't lead to unreachable jump if the requested size if greater than the stream size */
                if (*BlocksToRead > (EndBlock - StartBlock+1))
                {
                    /* Requested amount of data exceeds the stream length, just inject the available data */
                    NewBlock = StartBlock;
                    *BlocksToRead = EndBlock - StartBlock + 1;
                }
                else
                {
                    NewBlock = EndBlock - *BlocksToRead + 1 ;
                }
            }
            else if(RelativePosition > EndBlock)
            {
                /* requirement is [...] ### on HDD, do read [###.......] */
                NewBlock = StartBlock;
                /* Check the request don't lead to unreachable jump if the requested size if greater than the stream size */
                if (*BlocksToRead > (EndBlock - StartBlock+1))
                {
                    *BlocksToRead = EndBlock - StartBlock + 1;
                }
            }
            else
            {
                /* requirement is [.........#]## on HDD, do read [.........#] */
                /* truncation : don't read what is after the end of the stream */
                *BlocksToRead = EndBlock - RelativePosition + 1 ;
                NewBlock = RelativePosition;
            }
            (*NumberOfPlaying)-- ;
        }
        else
        {
            /* Single or last loop : send only available taking into account both requested data amount & stream limits */
            if (NextStartOfRead < StartBlock)
            {
                /* requirement is ##[#.........] on HDD, do read [#.........] */
                NewBlock = StartBlock;
                *BlocksToRead = *BlocksToRead - (StartBlock - RelativePosition + 1);
                /* Check the request don't lead to unreachable jump if the requested size if greater than the stream size */
                if (*BlocksToRead > (EndBlock - StartBlock+1))
                {
                    *BlocksToRead = EndBlock - StartBlock + 1;
                }
            }
            else
            {
                if (RelativePosition > EndBlock)
                {
                    /* requirement is [.........] ### on HDD, do injection from beginning of the stream */
                    /* This will lead to unexpected display in case of HDD jump but it is better than injecting nothing
                     * at the driver's point of view */
                    NewBlock = StartBlock;
                    /* Check the request don't lead to unreachable jump if the requested size if greater than the stream size */
                    if (*BlocksToRead > (EndBlock - StartBlock+1))
                    {
                        /* Requested amount of data exceeds the stream length, just inject the available data */
                        *BlocksToRead = EndBlock - StartBlock + 1;
                    }
                }
                else
                {
                    /* requirement is [.........#]## on HDD, do read [.........#] */
                    /* truncation : don't read what is after the end of the stream */
                    *BlocksToRead = EndBlock - RelativePosition + 1 ;
                    NewBlock = RelativePosition;
                }
            }
            (*NumberOfPlaying)-- ;
        }
    }

    /* Set Discountinuity & related to previous flags */
    if ( (PreviousCurrentBlock + PreviousBlocksToRead) == NewBlock)
    {
        /* only RelativePosition value can reveal a forward wraparound, because NewBlock is already adjusted ! */
        if ( RelativePosition > EndBlock)
        {
            /* forward wraparound */
            *RelatedToPrevious = FALSE;
            *Discontinuity = TRUE;
        }
        else
        {
            /* normal case */
            *RelatedToPrevious = TRUE;
            if (CurrentSpeed < 0)
            {
                *Discontinuity = TRUE;
            }
            else
            {
                *Discontinuity = FALSE;
            }
        }
    }
    else
    {
        *Discontinuity = TRUE;
        *RelatedToPrevious = FALSE;
        if ( ((NewBlock > PreviousCurrentBlock) && (NewBlock <= (PreviousCurrentBlock + PreviousBlocksToRead))) ||
             ((NewBlock <= PreviousCurrentBlock) && ((NewBlock + *BlocksToRead) >= PreviousCurrentBlock)) )
        {
            *RelatedToPrevious = TRUE;
        }
    }


    if (CurrentSpeed < 0)
    {
        HDD_PositionOfIPictureDisplayed = HDD_SeekLastIPictureInFile (NewBlock, HDDFirstBlockBuffer_p);
    }

#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\nJumpMgt : need jump=%d from %d to %d, do jump to %d, read %d, RelatedToP.=%d NumOfPlay=%d",
            JumpToPerform, *CurrentBlock, RelativePosition, NewBlock, *BlocksToRead,
            *RelatedToPrevious, *NumberOfPlaying));
#endif
    *CurrentBlock = NewBlock; /* new LBA to read */
    PreviousCurrentBlock = *CurrentBlock ;  /* save this value for the next jump */
    PreviousBlocksToRead = *BlocksToRead ;

} /* end of HDD_BlockJumpManagement */


/*******************************************************************************
Name        : HDD_SeekLastIPictureInFile
Description : Function that seek the position of the I picture just after the
              current position.
Parameters  : CurrentBlockRead, pointer to the current position of the playing.
              FirstBlockOfTheFile, pointer to the first block of the played file.
Limitations :
Returns     : The position of the I Picture to display (in FirstBlock array number).
              If the CurrentBlockRead value is low enough so that the last I
              Picture is at the end of the file, le returned position will be
              according this fact.
*******************************************************************************/
static U32      HDD_SeekLastIPictureInFile (U32 CurrentBlockRead, HDD_FirstBlock_t *FirstBlockOfTheFile)
{

    U32     SeekIndex;                  /* Index value for the seek of the position. */
    U32     NumberOfIPicture;           /* Safe of the number of I Picture to play. */

    NumberOfIPicture = FirstBlockOfTheFile->NumberOfIPicture;

    /* Test of the error case : No I Picture counted in the file ... */
    if (!NumberOfIPicture)
    {
        /* Just return the current position. */
        return(0);
    }

    /* Test of the special first */
    if (CurrentBlockRead <= FirstBlockOfTheFile->TabIPicturePosition[0].IPicturePosition)
    {
        return(0);
    }

    /* Test of the special last position. */
    if ( (CurrentBlockRead >= FirstBlockOfTheFile->TabIPicturePosition[NumberOfIPicture-1].IPicturePosition) )
    {
        /* The last I Picture is located at the end of the file. */
        return(FirstBlockOfTheFile->NumberOfIPicture-1);
    }

    SeekIndex = 0;

    while ( ((SeekIndex+50)<FirstBlockOfTheFile->NumberOfIPicture-1)
            && (CurrentBlockRead>FirstBlockOfTheFile->TabIPicturePosition[SeekIndex+50].IPicturePosition) )
    {
        SeekIndex+=50;
    }

    for (; SeekIndex<FirstBlockOfTheFile->NumberOfIPicture; SeekIndex++)
    {
        if (CurrentBlockRead < FirstBlockOfTheFile->TabIPicturePosition[SeekIndex].IPicturePosition)
        {
            return(SeekIndex);
        }
    }

    return(CurrentBlockRead);

} /* End of HDD_SeekLastIPictureInFile */


/*-------------------------------------------------------------------------
 * Function : HDD_ChangeSpeed
 *            Interactive test of STVID_SetSpeed() for trickmode tests
 *            STVID must be running, and HDD injection in progress
 *            hdd_setspeed() must be then launched with specified speed
 *            ( define a macro to repeat this command + hdd_setspeed )
 * Input    :
 * Output   :
 * Return   : FALSE if no error, else TRUE
 * ----------------------------------------------------------------------*/
static BOOL HDD_ChangeSpeed(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    S32 Speed, LVar;
    STVID_Handle_t VideoHndl;
    char Hit;

    RetErr = STTST_GetInteger( Parse_p, (int)VID_Inj_GetDriverHandle(0), &LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( Parse_p, "expected video handle" );
        return ( TRUE );
    }
    VideoHndl = (STVID_Handle_t)LVar;


    RetErr = STTST_GetInteger( Parse_p, 100, &Speed );
    if ( RetErr )
    {
        STTST_TagCurrentLine( Parse_p, "expected Speed (100=normal)" );
        return ( TRUE );
    }

    Hit=0;
    while (Hit!='q' && Hit!='Q' && Hit!='\r')
    {
        STTBX_Print(("Change HDD & STVID speed interactively\nPress S=stop N=normal H=high V=very high F=forward B=backward Q=quit\n     + or - to increase/decrease, * or / to multiply/divide by 2\n"));
        STTBX_InputChar(&Hit);
        switch(Hit)
        {
            /**case 'q':
            case 'Q':
                Speed = 99999;
                break;***/
            case 's':
            case 'S':
                Speed = 0;
                break;
            case 'n':
            case 'N':
                Speed = 100;
                break;
            case 'h':
            case 'H':
                Speed = 300;
                break;
            case 'v':
            case 'V':
                Speed = 800;
                break;
            case 'f':
            case 'F':
                if (Speed<0)
                {
                    Speed = Speed * (-1);
                }
                break;
            case 'b':
            case 'B':
                if (Speed>0)
                {
                    Speed = Speed * (-1);
                }
                break;
            case '-':
                if (Speed>100 || Speed<-100)
                {
                    Speed-=10;
                }
                else
                {
                    Speed-=1;
                }
                break;
            case '+':
                if (Speed>100 || Speed<-100)
                {
                    Speed+=10;
                }
                else
                {
                    Speed+=1;
                }
                break;
            case '/':
                Speed = Speed / 2;
                break;
            case '*':
                Speed = Speed * 2;
                break;
            default:
                break;
        } /* end switch */

        /* --- Change the STVID & HDD speed and return its value --- */

        if ( Hit != 'q' && Hit != 'Q' && Hit != '\r' )
        {
            ErrCode = STVID_SetSpeed(VideoHndl, Speed);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STVID_SetSpeed(%d,%d) failed !\n",VideoHndl, Speed));
                RetErr = TRUE;
            }
            else
            {
                if (Speed >= 0)
                {
                    HDD_PositionOfIPictureDisplayedAvailable = FALSE;
                    if (GlobalCurrentSpeed < 0)
                    {
                        if (HDD_PositionOfIPictureDisplayed >= HDDFirstBlockBuffer_p->NumberOfIPicture-1)
                        {
                            HDD_Handle.LBANumber = HDD_Handle.LBAStart;
                        }
                        else
                        {
                            HDD_Handle.LBANumber =
                                HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed+1].IPicturePosition;
                        }
                    }
                }
                STTBX_Print(("Speed set to  %d \n", Speed));
                /* Save locally the speed of the system */
                GlobalCurrentSpeed = (S32) (Speed);
            }
        }
    } /* end while */

    STTST_AssignInteger(result_sym_p, Speed, FALSE);

    return ( RetErr );

} /* end of HDD_ChangeSpeed() */

/*-------------------------------------------------------------------------
 * Function : HDD_GetStatistics
 * Input    :
 * Output   :
 * Return   : FALSE if no error, else TRUE
 * ----------------------------------------------------------------------*/
static BOOL HDD_GetStatistics(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL    EnablePrint = TRUE;
    BOOL    RetErr;

    /* Evaluate the EnablePrint parameter(enables printing of the statistics on the screen) */
    RetErr = STTST_GetInteger( Parse_p, (int)EnablePrint, &EnablePrint);
    if ( RetErr )
    {
        STTST_TagCurrentLine( Parse_p, "expected (TRUE/FALSE)");
        return(TRUE);
    }

    if(EnablePrint)
    {
        STTST_Print((" Nb of UNDERFLOW_EVT received ......: %d\n", HDDNbOfVideoUnderflowEvent ));
        STTST_Print(("    Nb of positive time jumps ..... : %d\n", HDDNbOfForwardUnderflow ));
        STTST_Print(("    Nb of negative time jumps ..... : %d\n", HDDNbOfBackwardUnderflow ));
        STTST_Print(("    Nb of zero bitrate ............ : %d\n", HDDNbBitrateZero ));
        STTST_Print(("    Maximum bitrate received ...... : %d\n", HDDMaxBitrate ));
        STTST_Print(("    Minimal bitrate received ...... : %d\n", HDDMinBitrate ));
        STTST_Print(("    Average bitrate received ...... : %d\n", HDDAverageBitrate ));
        STTST_Print((" Nb of DRIFT_THRESHOLD_EVT received : %d\n", HDDNbOfVideoDriftThresholdEvent ));
        STTST_Print((" Nb of LBA read from HDD .......... : %d\n", HDDNbOfLBARead ));
        STTST_Print((" Nb of TemporalDiscontinuity ...... : %d\n", HDDNbOfTemporalDiscontinuity ));
        STTST_Print((" Nb of RelatedToPrevious .......... : %d\n", HDDNbOfRelatedToPrevious ));
        STTST_Print((" LBA current read position ........ : %d\n", HDD_Handle.LBANumber ));
        STTST_Print((" LBA start position of the stream . : %d\n", HDD_Handle.LBAStart ));
        STTST_Print((" LBA end position of the stream ... : %d\n", HDD_Handle.LBAEnd ));

        if (HDDFirstBlockBuffer_p != NULL)
        {
            STTST_Print((" Nb of bytes of the stream ........ : %d\n", HDDFirstBlockBuffer_p->SizeOfFile ));
            STTST_Print((" Nb of I pictures of the stream ... : %d\n", HDDFirstBlockBuffer_p->NumberOfIPicture ));
        }
    }
    STTST_AssignInteger("RET_VAL1", (int) HDDNbOfVideoUnderflowEvent, FALSE);
    STTST_AssignInteger("RET_VAL2", (int) HDDNbOfForwardUnderflow, FALSE);
    STTST_AssignInteger("RET_VAL3", (int) HDDNbOfBackwardUnderflow, FALSE);
    STTST_AssignInteger("RET_VAL4", (int) HDDNbBitrateZero, FALSE);
    STTST_AssignInteger("RET_VAL5", (int) HDDMaxBitrate, FALSE);
    STTST_AssignInteger("RET_VAL6", (int) HDDMinBitrate, FALSE);
    STTST_AssignInteger("RET_VAL7", (int) HDDAverageBitrate, FALSE);
    STTST_AssignInteger("RET_VAL8", (int) HDDNbOfVideoDriftThresholdEvent, FALSE);
    STTST_AssignInteger("RET_VAL9", (int) HDDNbOfLBARead, FALSE);
    STTST_AssignInteger("RET_VAL10",(int) HDDNbOfTemporalDiscontinuity, FALSE);
    STTST_AssignInteger("RET_VAL11",(int) HDDNbOfRelatedToPrevious, FALSE);

    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return ( FALSE );
}

/*-------------------------------------------------------------------------
 * Function : HDD_ResetStatistics
 * Input    :
 * Output   :
 * Return   : FALSE if no error, else TRUE
 * ----------------------------------------------------------------------*/
static BOOL HDD_ResetStatistics(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(result_sym_p);

    HDDNbOfVideoUnderflowEvent = 0;
    HDDNbOfVideoDriftThresholdEvent = 0;
    HDDNbOfLBARead = 0;
    HDDNbOfTemporalDiscontinuity = 0;
    HDDNbOfRelatedToPrevious = 0;
    HDDNbOfTreatedVideoUnderflowEvent = 0;
    HDDNbBitrateZero = 0;
    HDDBitratesSum = 0;
    HDDMaxBitrate = 0;
    HDDMinBitrate = 0xFFFFFFFF;
    HDDAverageBitrate = 0;
    HDDNbOfForwardUnderflow = 0;
    HDDNbOfBackwardUnderflow = 0;

    return ( FALSE );
}

/* ========================================================================
Name        : HDD_ShiftInjectionStartPosition
Description : Shifts the LBA start of each backward injection according to
              the given coefficient in order to increase the overlap
Parameters  : A coefficient
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
======================================================================== */
static BOOL HDD_ShiftInjectionStartPosition (STTST_Parse_t * Parse_p, char *result)
{
    BOOL Error;

    /* Get the overlap ratio */
    Error = STTST_GetInteger ( Parse_p , 0 , (S32*)&ShiftInjectionCoef);
    if ((Error) || (ShiftInjectionCoef > 100))
    {
        STTST_TagCurrentLine ( Parse_p , "Expected a number between (0 and 100)");
        Error = TRUE;
    }

    STTST_AssignInteger(result, FALSE, FALSE);
    return(Error);
} /* end of HDD_ShiftInjectionStartPosition() */

/*##########################################################################*/
/*###########################  GLOBAL FUNCTIONS  ###########################*/
/*##########################################################################*/

/*******************************************************************************
Name        : HDD_InitCommand
Description : Registration of HDD commands
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
static BOOL HDD_InitCommand (void)
{
    BOOL RetErr ;

    RetErr =  STTST_RegisterCommand ("HDD_CHANGESPEED", HDD_ChangeSpeed , "Ask and change HDD & video speed");
    RetErr |= STTST_RegisterCommand ("HDD_COPY",      HDD_Copy     , "<file> <LBA> 0  copy host file on HDD at LBA position\n\t\t<file> <LBA> <NbLBA> copy NbLBA from HDD stream to host file");
    RetErr |= STTST_RegisterCommand ("HDD_GETSTAT",   HDD_GetStatistics , "Get statistics related to HDD Player");
    RetErr |= STTST_RegisterCommand ("HDD_INFO",      HDD_Info     , "Return STATAPI informations");
    RetErr |= STTST_RegisterCommand ("HDD_PLAYEVENT", HDD_PlayEvent, "<LBA> <NumOfPlay> Play HDD stream (default: LBA=0, Num=0=infinite");
    RetErr |= STTST_RegisterCommand ("HDD_RESETSTAT", HDD_ResetStatistics , "Reset statistics related to HDD Player");
    RetErr |= STTST_RegisterCommand ("HDD_SET_SPEED", HDD_SetSpeed , "<Speed> init of the speed currently played");
    RetErr |= STTST_RegisterCommand ("HDD_STOPEVENT", HDD_StopEvent, "Stop playing from HDD with event management");
    RetErr |= STTST_RegisterCommand ("HDD_SHIFT", HDD_ShiftInjectionStartPosition, "<percent> Forces the overlap size to be always equal to <percent> of the injection size");

    RetErr |= HDD_ResetStatistics(NULL, NULL);

    return (RetErr);
} /* end of HDD_InitCommand() */


/*******************************************************************************
Name        : HDD_CmdStart
Description : Subscribe to events needed by HDD appli. and create HDD commands
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if ok
*******************************************************************************/
BOOL HDD_CmdStart()
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STEVT_DeviceSubscribeParams_t DevSubscribeParams;
    STEVT_OpenParams_t EvtParams;
    STEVT_Handle_t  EvtHandleForSTATAPI;
    STATAPI_Cmd_t Cmd;
    STATAPI_CmdStatus_t CmdStatus;

    RetErr = FALSE;

    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &EvtParams, &EvtHandleForSTATAPI);
    if ( ErrCode!= ST_NO_ERROR)
    {
        STTBX_Print(("Failed to open handle on event device !!\n"));
    }
    else
    {
        /* Remember the number of clocks per second, to avoid calling this function many times */
        ClocksPerSecond = ST_GetClocksPerSecond();
        /* Subscribe to the events */
        DevSubscribeParams.SubscriberData_p = NULL;
        DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)HDD_LogIOEvent;

        ErrCode = STEVT_SubscribeDeviceEvent(EvtHandleForSTATAPI, STATAPI_DEVICE_NAME,
                (STEVT_EventConstant_t)STATAPI_CMD_COMPLETE_EVT, &DevSubscribeParams);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("HDD_CmdStart() : subscribe event error 0x%x !\n", RetErr));
        }
        else
        {
            CmdCompleteEvtSemaphore_p = semaphore_create_fifo(0);

            Cmd.SectorCount = HDD_NB_OF_SECTORS_PER_BLOCK;
            Cmd.CmdCode = STATAPI_CMD_SET_MULTIPLE_MODE;

            ErrCode = STATAPI_CmdNoData(STATAPI_Handle, &Cmd, &CmdStatus);
            RetErr = HDD_CheckRWCompletion( &Cmd, ErrCode, &CmdStatus );
            if (RetErr)
            {
                STTBX_Print(("Unable to set STATAPI_CMD_SET_MULTIPLE_MODE : error=0x%x !\n", ErrCode));
            }
            else
            {
                RetErr = HDD_InitCommand();
            }
        }
    }

    return(RetErr ? FALSE : TRUE);

} /* end HDD_CmdStart */

#if defined(ST_5516) || defined(ST_5517) || defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
/* Error reporting not quite ideal (it or's all the errors together),
 * but if one fails the rest are likely to anyway...
 */
static ST_ErrorCode_t DoCfg(void)
{
    ST_ErrorCode_t      error = ST_NO_ERROR;
#if !defined(ST_51xx_Device) && !defined(ST_7710) && !defined(ST_7100) && !defined(ST_7109)
    STCFG_Handle_t      CfgHandle;

    error |= STCFG_Open("cfg", NULL, &CfgHandle);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error opening STCFG\n"));
    }

    error |= STCFG_CallCmd(CfgHandle, STCFG_CMD_HDDI_ENABLE);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error enabling HDDI\n"));
    }
    error |= STCFG_CallCmd(CfgHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                          STCFG_EXTINT_1, STCFG_EXTINT_INPUT);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error enabling interrupt\n"));
    }

    STCFG_Close(CfgHandle);
#elif defined(ST_51xx_Device)
    /* Ugly, but easy... */

    #define FMICONFIG_BASE_ADDRESS         0x20200000
    #define FMI_GENCFG                     (FMICONFIG_BASE_ADDRESS + 0x28)
    #define FMICONFIGDATA0_BANK0           (FMICONFIG_BASE_ADDRESS + 0x100)
    #define FMICONFIGDATA1_BANK0           (FMICONFIG_BASE_ADDRESS + 0x108)
    #define FMICONFIGDATA2_BANK0           (FMICONFIG_BASE_ADDRESS + 0x110)
    #define FMICONFIGDATA3_BANK0           (FMICONFIG_BASE_ADDRESS + 0x118)

    volatile U32 *regs;
    volatile U8 *regs_8;

    /* Enable HDDI on FMI is done by an EPLD register (EPLD in Bank 1) */
    regs_8 = (volatile U8 *)0x41a00000;
    *regs_8 |= 0x01;

    /* Configuring ATAPI is done using FMI_GENCFG bits 30 (bank0) and 31
     * (bank1). Due to a bug on Cut1.0 of 5100, bit 31 actually
     * configures both. We'll write both 30 and 31 anyway, so we don't
     * have to care which cut we're on.
     */
    regs = (volatile U32 *)FMI_GENCFG;
    *regs = 0xc0000000;

    /* 3- You need to update the ATAPI registers and Interrupt number
     * (as in stbgr-prj-vali5100\boot\interrupt.c and
     * stbgr-prj-vali5100\include\sti5100\sti5100.h) as i did it on 5100
     * validation VOB. */

    /* Configure bank0 for ATAPI */
    regs = (volatile U32 *)FMICONFIGDATA0_BANK0;
    *regs = 0x00540791;
    regs = (volatile U32 *)FMICONFIGDATA1_BANK0;
    *regs = 0x8E003321;
    regs = (volatile U32 *)FMICONFIGDATA2_BANK0;
    *regs = 0x8E003321;
    regs = (volatile U32 *)FMICONFIGDATA3_BANK0;
    *regs = 0x0000000a;
    /* defined(ST_51xx_Device) */
#else
#if defined (mb411) && !defined (SATA_SUPPORTED)

#define FMICONFIG_BASE_ADDRESS     0xBA100000
#define FMI_GENCFG                 (FMICONFIG_BASE_ADDRESS + 0x28)
#define FMICONFIGDATA0_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1C0)
#define FMICONFIGDATA1_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1C8)
#define FMICONFIGDATA2_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1D0)
#define FMICONFIGDATA3_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1D8)

    volatile U8 *regs_8;
    U32 config = 0x00;

    /* Enable ATAPI on EPLD */
    *(DU8 *)0xA3900000 = 0x01;
    while (*(DU8 *)0xA3900000!=0x01) *(DU8 *)0xA3900000=0x01;
    /* Enable ATAPI Interrupt - Set bit 7 of ITM 0 */
    *(DU8 *)0xA3100000 = 0x80;
    /* Setup PIO4 interface */
    /* Please note that this configuration is only working with MAXTOR Hdd */
    *((DU32 *)0xBA1001C0) =  0x00021791; /*0x00021791;*/
    *((DU32 *)0xBA1001C8) =  0x0A004241; /*0x08004141;*/
    *((DU32 *)0xBA1001D0) =  0x0A004241; /*0x08004141;*/
    *((DU32 *)0xBA1001D8) =  0x00000000;

    /* Write EMI_GEN_CFG */
    STSYS_WriteRegDev32LE(((U32 *)FMI_GENCFG), 0x10);
    config = STSYS_ReadRegDev32LE(((U32 *)FMI_GENCFG));
    printf("EMI_GEN_CFG Programmed to %x\n", config);

   /* Optimised PIO Mode 4 timing on Bank 3 */
   *(volatile U32 *)FMICONFIGDATA0_BANK0 = 0x00021791;
   *(volatile U32 *)FMICONFIGDATA1_BANK0 = 0x08004141;
   *(volatile U32 *)FMICONFIGDATA2_BANK0 = 0x08004141;
   *(volatile U32 *)FMICONFIGDATA3_BANK0 = 0x00000000;

    /* Enable HDD on FMI is done by an EPLD register */
    regs_8 = (volatile U8 *)0xA3900000;
    *regs_8 |= 0x01;

    while (*regs_8 != 0x01)
        *regs_8=0x01;
#if defined (ATAPI_USING_INTERRUPTS)
    /* Enable ATAPI Interrupt - Set bit 7 of ITM 0 */
    *(DU8 *)0xA3100000 = 0x80;
#endif

#else   /* 411 & !SATA */

    #define EMI_BANK3_DATA0                 (EMI_BASE_ADDRESS + 0x1c0)
    #define EMI_BANK3_DATA1                 (EMI_BASE_ADDRESS + 0x1c8)
    #define EMI_BANK3_DATA2                 (EMI_BASE_ADDRESS + 0x1d0)
    #define EMI_BANK3_DATA3                 (EMI_BASE_ADDRESS + 0x1d8)

    /* Enable HDDI on FMI is done by an EPLD register */

#if !defined (ST_7100) && !defined (ST_7109)
    volatile U8 *regs_8;
    regs_8 = (volatile U8 *)EPLD_BASE_ADDRESS + EPLD_ATAPI_OFFSET;
    *regs_8 |= 0x01;
#endif /* !defined ST_7100 */

   /* Optimised PIO Mode 4 timing */
   *(volatile U32 *)EMI_BANK3_DATA0 = 0x00118791;
   *(volatile U32 *)EMI_BANK3_DATA1 = 0x8a212100;
   *(volatile U32 *)EMI_BANK3_DATA2 = 0x8a212100;
   *(volatile U32 *)EMI_BANK3_DATA3 = 0x0000000a;

 /* defined(ST_7710) */
#endif

#endif
    return error;
}
#endif

/* ------------------------------- End of file ---------------------------- */

