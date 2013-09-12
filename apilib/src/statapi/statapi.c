/************************************************************************
COPYRIGHT (C) STMicroelectronics  2000

Source file name : statapi.c

ATA/ATAPI driver.


************************************************************************/

/* Includes --------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "stlite.h"

#include "stos.h"
#include "sttbx.h"
#include "stevt.h"
#include "statapi.h"
#include "hal_atapi.h"
#include "ata.h"

/*Private Types--------------------------------------------------------*/

/*Private Constants----------------------------------------------------*/

/* Defines */
#define    STATAPI_HARD_RESET_ID            0   /* Offsets for events array  */
#define    STATAPI_SOFT_RESET_ID            1
#define    STATAPI_CMD_COMPLETE_ID          2
#define    STATAPI_PKT_COMPLETE_ID          3

/* Bus Access Task defines */
#if defined(ST_OS20)
#define    BAT_STACK_SIZE                   4096
#else
/* OS21 and unknown. We only actually need the 16KB stack if we're going
 * to do printing, but since STTBX_Print is always defined, we can't
 * tell one way or the other. So we'll just have to play it safe.
 */
#define    BAT_STACK_SIZE                   4096+16384/*tbc*/
#endif

#ifndef    BAT_PRIORITY
#define    BAT_PRIORITY                     MIN_USER_PRIORITY
#endif

#define    CLOSING_TIMEOUT                  20000
#define    SET_MULTIPLE_MODE                0xC6

#define    NUM_CMD_FIXED_LENGTH             8

/* 5514/5528/8010, may vary for other HDDI implementations */
#define     DMA_ALIGNMENT                   64


/*Private Variables----------------------------------------------------*/

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
static BOOL     TimingRescaled;
#endif

                      /* 0     1    2    3    4    5    6    7    8   9  */
static U8 CmdCodes[52]={0xC0,0x03,0x87,0xCD,0x38,0xE5,0x08,0x90,0xE7,0xEC,
                        0xA1,0xE3,0xE1,0x91,0xED,0x00,0xA0,0xE4,0xC8,0xC7,
                        0xC4,0x20,0x40,0x70,0xEF,0xC6,0xE6,0xE2,0xE0,0xE8,
                        0xCA,0xCC,0xC5,0x30,0xF1,0xF6,0xF2,0x24,0x25,0x26,
                        0x27,0x29,0x34,0x35,0x36,0x37,0x39,0x42,0xEA,0xB0,
                        0xB0,0xB0};

                        /* 0     1    2    3    4    5    6    7    8   9  */
static U8 PktCodes[62]={ 0xA1,0x5B,0x39,0x2C,0x35,0x04,0x46,0x4A,0xAC,0x12,
                         0xA6,0x36,0x4C,0x4D,0xBD,0x55,0x5A,0x4B,0x45,0x47,
                         0xBC,0x34,0x1E,0x28,0xA8,0x3C,0x25,0xBE,0xB9,0x51,
                         0xAD,0x23,0x44,0x42,0x43,0x52,0x1C,0x17,0x57,0x58,
                         0xA4,0x03,0x16,0x56,0x53,0xBA,0x2B,0x5D,0x1D,0xBF,
                         0xA2,0xA3,0x54,0xA7,0xB6,0x1B,0x4E,0x00,0x2F,0x2A,
                         0x2E,0x3B};

/* Table storing commands we support whose length is predefined by the
 * standard, and sector-count may be legally be anything (undef). Used
 * by GetSizeRequired later on. STATAPI_CMD_SMART_IN is an exception
 * (since it can return both fixed and non-fixed lengths).
 */
static U8 CmdFixedLength[NUM_CMD_FIXED_LENGTH] =
                        { STATAPI_CMD_CFA_TRANSLATE_SECTOR,
                          STATAPI_CMD_IDENTIFY_DEVICE,
                          STATAPI_CMD_IDENTIFY_PACKET_DEVICE,
                          STATAPI_CMD_READ_BUFFER,
                          STATAPI_CMD_WRITE_BUFFER,
                          STATAPI_CMD_SECURITY_SET_PASSWORD,
                          STATAPI_CMD_SECURITY_DISABLE_PASSWORD,
                          STATAPI_CMD_SECURITY_UNLOCK };

/* Stores the defined length for the commands above; should be in the
 * same order, for lookup purposes.
 */
static U32 CmdFixedLengthValues[NUM_CMD_FIXED_LENGTH] =
                        { 512, 512, 512, 512, 512, 512, 512, 512 };

/*  DMA/PIO timing values, specified in 10ns units. These will be rescaled
 *  at init (for 5514). Taken from Table 10, HDDI functional spec,
 *  ADCS #7180863.
 */
#if defined(ST_5514) || defined(ST_5528)
/*
    U32 InitSequenceDelay;
    U32 IoRdySetupDelay;
    U32 WaitTime;
    U32 WriteDelay;
    U32 ReadDelay;
    U32 AddressHoldDelay;
    U32 WriteEnableOutDelay;
    U32 WriteRecoverDelay;
    U32 ReadRecoverDelay;
*/
static STATAPI_PioTiming_t PioTimingDefaults[5] =
        {
            /* Mode 0 */
            { 7, 4, WAIT, 24, 24, 2, 1, 22, 22 },
            /* Mode 1 */
            { 5, 4, WAIT, 24, 24, 2, 0,  3,  3 },
            /* Mode 2 */
/*            { 3, 4, WAIT, 24, 24, 1, 1,  0,  1 }, */
            { 3, 4, WAIT,  5,  5, 1, 1, 10, 11 },
            /* Mode 3 */
            { 3, 4, WAIT,  3,  3, 1, 0,  7,  7 },
            /* Mode 4 */
            { 3, 4, WAIT,  2,  2, 1, 0,  4,  4 }
        };

/*
    U32 NotDIoRwAssertedT;
    U32 WriteDataHoldT;
    U32 DIoRwToDMAckHoldT;
    U32 NotDIoRNegatedT;
    U32 NotDIoWNegatedT;
    U32 CsToNotDIoRwT;
    U32 CsHoldT;
*/
static STATAPI_MwDmaTiming_t MwDmaTimingDefaults[3] =
        {
            /* Mode 0 */
            { 25, 2, 2, 23, 23, 5, 2 },
            /* Mode 1 */
            {  9, 2, 1,  7,  7, 3, 3 },
            /* Mode 2 */
            {  7, 1, 1,  5,  5, 3, 3 }
        };

/*
    U32 AckT;                       = ACK
    U32 InitEnvelopeT;              = ENV
    U32 MinAssertStopNegateT;       = RP
    U32 MinInterlockT;              = MLI
    U32 LimitedInterlockT;          = TLI (5514)

    U32 DataOutSetupT;              = DVS
    U32 DataOutHoldT;               = DVH

    U32 HostStrobeToStopSetupT;     = SS
    U32 ReadyToFinalStrobeT;        = RFS (5514)

    U32 CRCSetupT;                  = CVS (5528)
*/
        /* Mode 4 timings adjusted again after feedback from validation,
         * March 2003, cut 3.2 */
        /* Went through and added CRCSetup (keeping TLI for each mode).
         * DVS and DVH in 7405513 seem totally different to what we had
         * here... 09/10/03 */
        /* Mode 5 timings acquired from AL in validation 11/05/04 */
static STATAPI_UltraDmaTiming_t UltraDmaTimingDefaults[6] =
        {
            /* Mode 0 */
            { 3, 3, 16, 2, 15, 11, 1, 5, 0, 70 },
            /* Mode 1 */
            { 3, 3, 13, 2, 15,  7, 1, 5, 0, 48 },
            /* Mode 2 */
            { 3, 3, 10, 2, 15,  5, 1, 5, 0, 31 },
            /* Mode 3 */
            { 3, 3, 10, 2, 10,  3, 1, 5, 0, 20 },
            /* Mode 4 */
            { 3, 3, 10, 2,  5,  2, 1, 5, 6,  7 },
            /* Mode 5 */
            { 3, 3, 10,  2,  2, 1, 1, 5, 2,  2 }
        };

/* In Packet Transfers 4 K per DRQ assertion */
#define BYTE_COUNT_LOW   0x00
#define BYTE_COUNT_HIGH  0x10

#elif defined(ST_8010) 

static STATAPI_PioTiming_t PioTimingDefaults[5] =
        {
            /* Mode 0 */
            { 7, 4, WAIT, 12, 12, 2, 1, 34, 33 },
            /* Mode 1 */
            { 5, 4, WAIT, 8, 8, 2, 1,  19,  19 },
            /* Mode 2 */
            { 3, 4, WAIT,  5,  5, 1, 1, 10, 11 },
            /* Mode 3 */
            { 3, 4, WAIT,  3,  3, 1, 1,  7,  7 },
            /* Mode 4 */
            { 3, 4, WAIT,  2,  2, 1, 1,  3,  3 }
        };

static STATAPI_MwDmaTiming_t MwDmaTimingDefaults[3] =
        {
            /* Mode 0 */
            { 31, 2, 2, 29, 29, 6, 2 },
            /* Mode 1 */
            {  10, 2, 1,  6,  6, 3, 1 },
            /* Mode 2 */
            {  9, 1, 1,  3,  3, 3, 1 }
        };

static STATAPI_UltraDmaTiming_t UltraDmaTimingDefaults[6] =
        {
            /* Mode 0 */
            { 3, 3, 16, 2, 15, 11, 1, 5, 7, 70 },
            /* Mode 1 */
            { 3, 3, 13, 2, 15,  7, 1, 5, 7, 48 },
            /* Mode 2 */
            { 3, 3, 10, 2, 15,  5, 1, 5, 6, 31 },
            /* Mode 3 */
            { 3, 3, 10, 2, 10,  3, 1, 5, 6, 20 },
            /* Mode 4 */
            { 3, 3, 10, 2,  5,  2, 1, 5, 6, 10 },
            /* Mode 5 */
            { 3, 3, 8,  2,  2, 1, 1, 5, 6,  10 }
        };

/* In Packet Transfers 4 K per DRQ assertion */
/* Added When porting for 8010 */
#define BYTE_COUNT_LOW   0xFE
#define BYTE_COUNT_HIGH  0xFF

#else
/* In Packet Transfers 4 K per DRQ assertion */
#define BYTE_COUNT_LOW   0x00
#define BYTE_COUNT_HIGH  0x10

#endif /* ST_5514 || ST_5528 || ST_8010 */

/*Private Macros-------------------------------------------------------*/

/*Private functions prototypes-----------------------------------------*/
static ST_ErrorCode_t RegisterATAEvents(const ST_DeviceName_t DeviceName,
										const ST_DeviceName_t EvtHandlerName,
                                        STEVT_Handle_t *EvtHndl);
                                        
static ST_ErrorCode_t GetDeviceCapabilities(ata_ControlBlock_t *ATAHandle,
										    STATAPI_DeviceAddr_t DevAddress);
										
static ST_ErrorCode_t UnregisterATAEvents( const ST_DeviceName_t DeviceName,
										   STEVT_Handle_t EvtHndl );
										   
static BOOL IsOutsideRange( U32 ValuePassed,
                            U32 LowerBound,
                            U32 UpperBound );
static BOOL ParseIdentifyBlock(STATAPI_DeviceAddr_t DevNo,
                               U8 *Data,
                               STATAPI_Params_t *Params,
                               ata_ControlBlock_t *ATAHandle);
static U8 ChooseProtocolCmd(U16 CommandCode, ata_DevCtrlBlock_t *Dev);
static U32 GetSizeRequired(const STATAPI_Cmd_t *Cmd);

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
static void CalculateTiming(U32 Mult, U32 Div);
#endif

/* ATAPI Queue Management */
static void ATAPI_QueueRemove(ata_ControlBlock_t *QueueItem_p);
static void ATAPI_QueueAdd(ata_ControlBlock_t    *QueueItem_p);
static BOOL ATAPI_IsInit(const ST_DeviceName_t DeviceName);

/* Bus Access Task */
void BusAccessTask(ata_ControlBlock_t *ATAHandle);   

/*****************************************************************************
ATAPIQueueHead_p points to the head of the queue of ATA devices that
have been initialized i.e., STATAPI_Init() has been called to append them
to the queue.
At system startup, ATAPIQueueHead_p will be NULL.  When the first ATA device is
initialized, ATAPIQueueHead_p will point to the ATA control block for that
ATA device once STATAPI_Init() has been called.
*****************************************************************************/

/* Queue of initialized ATAPI devices */
static ata_ControlBlock_t *ATAPIQueueHead_p = NULL;
/* How many copies have been initialised */
static U32 ATAInstanceCount = 0;   

/******************************************************************************
*        PUBLIC DRIVER FUNCTIONS
******************************************************************************/

/****************************************************************************
Name         : STATAPI Init()

Description  : Initializes the ATAPI driver and the devices connected in the bus

Parameters   : ST_DeviceName_t      DeviceName      ATAPI driver name
               STATAPI_InitParams_t InitParams      Params structure

Return Value :  ST_NO_ERROR                   No error.
                ST_ERROR_ALREADY_INITIALIZED  The device name has already been initialized.
                ST_ERROR_BAD_PARAMETER        One or more of the parameters are invalid.
                ST_ERROR_INTERRUPT_INSTALL    Unable to install interrupts.
                ST_ERROR_NO_MEMORY            Unable to allocate required memory.
 ****************************************************************************/
ST_ErrorCode_t STATAPI_Init(const ST_DeviceName_t DeviceName,
                            const STATAPI_InitParams_t *InitParams)
{
    ST_ErrorCode_t     rVal= ST_NO_ERROR;
    ata_ControlBlock_t *ATAHandle = NULL;
    
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    if ((DeviceName == NULL) || (InitParams == NULL) ||
            (strlen(DeviceName) >= ST_MAX_DEVICE_NAME))
    {
        return ST_ERROR_BAD_PARAMETER;
    }
    if ((InitParams->DriverPartition == NULL) ||
        (InitParams->BaseAddress == NULL) ||
        (InitParams->HW_ResetAddress == NULL))
    {
        return ST_ERROR_BAD_PARAMETER;
    }
    if(IsOutsideRange(InitParams->DeviceType,
                      STATAPI_EMI_PIO4,
                      STATAPI_SATA))
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif /* STATAPI_NO_PARAMETER_CHECK */

   /*--------------End of parameters check -------------------*/
    
    if(!ATAPI_IsInit(DeviceName))
    {
	    /*Allocate High level Handle*/
		ATAHandle =(ata_ControlBlock_t*)STOS_MemoryAllocate(InitParams->DriverPartition,
														    sizeof(ata_ControlBlock_t));
		if(ATAHandle == NULL)
		{
			return ST_ERROR_NO_MEMORY;
		}

		/* Ensure the control block is wiped before we use it. */
		memset(ATAHandle, 0, sizeof(ata_ControlBlock_t));
		
		/*  Setup initial values */
		strcpy( ATAHandle->DeviceName,DeviceName);
		strcpy( ATAHandle->EvtDeviceName,InitParams->EVTDeviceName);
		ATAHandle->DriverPartition = InitParams->DriverPartition;
	    ATAHandle->DeviceType      = (STATAPI_Device_t)InitParams->DeviceType;
#if defined(ATAPI_FDMA) 
		ATAHandle->NCachePartition = InitParams->NCachePartition;
#endif
		ATAHandle->UsedHandles    = 0;
		ATAHandle->DeviceSelected = DEVICE_0;
		ATAHandle->BusBusy        = FALSE;
		ATAHandle->Terminate      = FALSE;
		ATAHandle->LastExtendedErrorCode = 0x00;
		
		ATAHandle->Handles[0].Opened = FALSE;
		ATAHandle->Handles[1].Opened = FALSE;	
	    /* Create semaphores to signal the end of internal command issuing; they
		* have to be created here, because functions such as softreset use them.
		*/
		ATAHandle->Handles[STATAPI_DEVICE_0].EndCmdSem_p = \
		                    STOS_SemaphoreCreateFifoTimeOut(ATAHandle->DriverPartition,0);
		ATAHandle->Handles[STATAPI_DEVICE_1].EndCmdSem_p = \
							STOS_SemaphoreCreateFifoTimeOut(ATAHandle->DriverPartition,0);
		ATAHandle->BusMutexSemaphore_p = \
							STOS_SemaphoreCreateFifoTimeOut(ATAHandle->DriverPartition,1);
		ATAHandle->BATSemaphore_p = \
						    STOS_SemaphoreCreateFifoTimeOut(ATAHandle->DriverPartition,0);

		/*Everything seems Ok now add the control block to Queue*/
		ATAPI_QueueAdd(ATAHandle);
    	ATAInstanceCount++ ;
    	
	    /*Init hal*/
	    if( hal_Init(DeviceName,InitParams,&ATAHandle->HalHndl))
	    {
	        memory_deallocate(InitParams->DriverPartition, ATAHandle);
	        ATAHandle = NULL;
	         /* Remove device from queue */
    		ATAPI_QueueRemove(ATAHandle);
    		ATAInstanceCount--;
	        return ST_ERROR_INTERRUPT_INSTALL;
	    }
    	
   		/* Register the ATA events*/
		rVal= RegisterATAEvents(ATAHandle->DeviceName,InitParams->EVTDeviceName,&ATAHandle->EvtHndl);
		if(rVal != ST_NO_ERROR)
		{
			hal_Term(DeviceName,ATAHandle->HalHndl);
			/* Remove device from queue */
    		ATAPI_QueueRemove(ATAHandle);
    		ATAInstanceCount--;
			STOS_MemoryDeallocate(InitParams->DriverPartition, ATAHandle);
			ATAHandle = NULL;
			return ST_ERROR_BAD_PARAMETER;
		}
	
		if ((NULL == ATAHandle->Handles[STATAPI_DEVICE_0].EndCmdSem_p)  ||
			(NULL == ATAHandle->Handles[STATAPI_DEVICE_1].EndCmdSem_p)  ||
			(NULL == ATAHandle->BusMutexSemaphore_p)                    ||
			(NULL == ATAHandle->BATSemaphore_p))
		{   /* Error: clean and exit */
		
			if (ATAHandle->BusMutexSemaphore_p)
			    STOS_SemaphoreDelete(ATAHandle->DriverPartition,ATAHandle->BusMutexSemaphore_p);
			if (ATAHandle->BATSemaphore_p)
			    STOS_SemaphoreDelete(ATAHandle->DriverPartition,ATAHandle->BATSemaphore_p);
			if (ATAHandle->Handles[STATAPI_DEVICE_0].EndCmdSem_p)
			    STOS_SemaphoreDelete(ATAHandle->DriverPartition,\
			    				ATAHandle->Handles[STATAPI_DEVICE_0].EndCmdSem_p);
			if (ATAHandle->Handles[STATAPI_DEVICE_1].EndCmdSem_p)
			    STOS_SemaphoreDelete(ATAHandle->DriverPartition,\
			    				ATAHandle->Handles[STATAPI_DEVICE_1].EndCmdSem_p);
			hal_Term(DeviceName,ATAHandle->HalHndl);
			/* Remove device from queue */
   			ATAPI_QueueRemove(ATAHandle);
    		ATAInstanceCount--;
			STOS_MemoryDeallocate(InitParams->DriverPartition, ATAHandle);
		
			/* BusTaskSTack not used for this OS */
			ATAHandle = NULL;
			return STATAPI_ERROR_DEVICE_NOT_PRESENT;
		}
		
		/* Init interface */
		if(ata_ctrl_HardReset(ATAHandle))
		{
			/* Clean and exit */
			UnregisterATAEvents(ATAHandle->DeviceName,ATAHandle->EvtHndl );
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,ATAHandle->BusMutexSemaphore_p);
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,ATAHandle->BATSemaphore_p);
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,\
						        	ATAHandle->Handles[STATAPI_DEVICE_0].EndCmdSem_p);
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,\
								    ATAHandle->Handles[STATAPI_DEVICE_1].EndCmdSem_p);
			hal_Term(DeviceName,ATAHandle->HalHndl);
			  /* Remove device from queue */
    		ATAPI_QueueRemove(ATAHandle);
    		ATAInstanceCount--;
			STOS_MemoryDeallocate(InitParams->DriverPartition, ATAHandle);
			ATAHandle = NULL;
			return STATAPI_ERROR_DEVICE_NOT_PRESENT;
		}
		
		/* Detect devices  */
		if (ata_ctrl_Probe(ATAHandle) != FALSE)
		{
		 	STTBX_Print(("Error performing ata_ctrl_Probe()\n"));
		}
		
		/* Check that at least there is one */
		if (((ATAHandle->DevInBus[0] == NONE_DEVICE)  &&
		 	(ATAHandle->DevInBus[1] == NONE_DEVICE)) /*||
			(ata_ctrl_HardReset(ATAHandle) == TRUE)*/)
		{   
			/* Clean and exit */
			UnregisterATAEvents(ATAHandle->DeviceName,ATAHandle->EvtHndl );
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,ATAHandle->BusMutexSemaphore_p);
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,ATAHandle->BATSemaphore_p);
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,\
						        	ATAHandle->Handles[STATAPI_DEVICE_0].EndCmdSem_p);
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,\
								    ATAHandle->Handles[STATAPI_DEVICE_1].EndCmdSem_p);
			hal_Term(DeviceName,ATAHandle->HalHndl);
			 /* Remove device from queue */
			ATAPI_QueueRemove(ATAHandle);
			ATAInstanceCount--;
			STOS_MemoryDeallocate(InitParams->DriverPartition, ATAHandle);
			ATAHandle = NULL;
			return STATAPI_ERROR_DEVICE_NOT_PRESENT;
		}
		/*Create the Bus Access Task*/
		
		rVal = STOS_TaskCreate((void(*)(void *))BusAccessTask,
		                                (void*)ATAHandle,
		                                ATAHandle->DriverPartition,
		                                BAT_STACK_SIZE,
		                                &ATAHandle->BusTaskStack,
		                                ATAHandle->DriverPartition,
		                                &ATAHandle->BAT_p,
		                                &ATAHandle->BusTaskDesc,
		                                BAT_PRIORITY,
		                                "Bus Access Task",
		                                task_flags_no_min_stack_size);
		      
		if (rVal != ST_NO_ERROR)
		{
			/* Error: clean and exit */
			UnregisterATAEvents(ATAHandle->DeviceName,ATAHandle->EvtHndl );
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,ATAHandle->BusMutexSemaphore_p);
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,ATAHandle->BATSemaphore_p);
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,\
						        	ATAHandle->Handles[STATAPI_DEVICE_0].EndCmdSem_p);
			STOS_SemaphoreDelete(ATAHandle->DriverPartition,\
								    ATAHandle->Handles[STATAPI_DEVICE_1].EndCmdSem_p);  
			hal_Term(DeviceName,ATAHandle->HalHndl);
			 /* Remove device from queue */
    		ATAPI_QueueRemove(ATAHandle);
    		ATAInstanceCount--;
			STOS_MemoryDeallocate(InitParams->DriverPartition, ATAHandle);
			ATAHandle = NULL;
			return ST_ERROR_NO_MEMORY;
		}
		
#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
		/* Calculate the timing frequencies for 5514. */
		if (InitParams->DeviceType == STATAPI_HDD_UDMA4)
		{
			U32 Mult;
			
			Mult = InitParams->ClockFrequency;
			
			Mult /= 1000000;
			CalculateTiming(Mult, 100);
		
		}
		
		/* Start up in PIO mode 2. I have no idea why mode 2 as opposed to
		* 0, but there you go. Since we're just init'ing, timing can't have
		* been changed yet, so safe to use defaults.
		*/
		hal_SetPioMode(ATAHandle->HalHndl, STATAPI_PIO_MODE_2);
		hal_SetPioTiming(ATAHandle->DeviceName,ATAHandle->HalHndl, &PioTimingDefaults[2]);
#endif

	}
	else
	{
        /* The ATA device is already in use */
        return ST_ERROR_ALREADY_INITIALIZED;
	}

    return ST_NO_ERROR;
}


/****************************************************************************
Name         : STATAPI Term()

Description  : Terminates the ATAPI driver and deallocate all the driver control
               structures

Parameters   : ST_DeviceName_t      DeviceName      ATAPI driver name
               STATAPI_InitParams_t InitParams      Params structure

Return Value :  ST_NO_ERROR                     No error.
                ST_ERROR_OPEN_HANDLE            Could not terminate the driver; not all
                                                handles have not been closed.
                ST_ERROR_INTERRUPT_UNINSTALL    Unable to uninstall interrupts.
                ST_ERROR_UNKNOWN_DEVICE         Invalid DeviceName.
                STATAPI_ERROR_TERMINATE_BAT     Could not delete the Bus Access
                                                Task
 ****************************************************************************/
ST_ErrorCode_t STATAPI_Term(const ST_DeviceName_t DeviceName,
                            const STATAPI_TermParams_t *TermParams)
{
    ST_ErrorCode_t rVal=ST_NO_ERROR;
    clock_t TO;
    ata_ControlBlock_t *ATAHandle;

    /* Obtain the control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    if (NULL == ATAHandle)
        return ST_ERROR_UNKNOWN_DEVICE;
    if (strcmp(DeviceName, ATAHandle->DeviceName))
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    if (TermParams == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif
    if (TermParams->ForceTerminate == TRUE)
    {
        if (ATAHandle->UsedHandles !=0 )
        {
            /* Close existing handles */
            if (ATAHandle->Handles[0].Opened == TRUE)
            {
                rVal = STATAPI_Close((STATAPI_Handle_t)&ATAHandle->Handles[0]);
            }

            if (rVal != ST_NO_ERROR)
                return rVal;


            if (ATAHandle->Handles[1].Opened == TRUE)
            {
                rVal = STATAPI_Close((STATAPI_Handle_t)&ATAHandle->Handles[1]);
            }

            if (rVal != ST_NO_ERROR)
                return rVal;
        }
    }
    else if (ATAHandle->UsedHandles != 0 )
    {
        return ST_ERROR_OPEN_HANDLE;
    }

    /* Unregister events */
    rVal = UnregisterATAEvents(ATAHandle->DeviceName,ATAHandle->EvtHndl);
    if(rVal != ST_NO_ERROR)
    {
        return rVal;
    }

    /* After closing all the handles we are sure that the BAT is waiting and
     * that the bus is idle.. so we have to set terminate
     * to make the task to exit from the main loop before signal
     */
    ATAHandle->Terminate = TRUE;
    STOS_SemaphoreSignal(ATAHandle->BATSemaphore_p);
    
    
    TO = STOS_time_plus(STOS_time_now(), ATA_TIMEOUT);

    if(STOS_TaskWait(&ATAHandle->BAT_p,&TO))
    {
        return STATAPI_ERROR_TERMINATE_BAT;
    }

    /* Deallocate all resources */
    STOS_SemaphoreDelete(ATAHandle->DriverPartition,ATAHandle->BusMutexSemaphore_p);
    STOS_SemaphoreDelete(ATAHandle->DriverPartition,ATAHandle->BATSemaphore_p);
    STOS_SemaphoreDelete(ATAHandle->DriverPartition,\
    			        	ATAHandle->Handles[STATAPI_DEVICE_0].EndCmdSem_p);
    STOS_SemaphoreDelete(ATAHandle->DriverPartition,\
    					    ATAHandle->Handles[STATAPI_DEVICE_1].EndCmdSem_p); 
    hal_Term(DeviceName,ATAHandle->HalHndl);

    STOS_TaskDelete(ATAHandle->BAT_p,
    				ATAHandle->DriverPartition,
    			    ATAHandle->BusTaskStack,
    			    ATAHandle->DriverPartition);
    /* Clean and deallocate */
   
    /* Remove device from queue */
    ATAPI_QueueRemove(ATAHandle);
    ATAInstanceCount--;
    
    STOS_MemoryDeallocate(ATAHandle->DriverPartition, ATAHandle);
    ATAHandle = NULL;
    return ST_NO_ERROR;
}
    
        

/****************************************************************************
Name         : STATAPI Open()

Description  : Open a connection to the ATAPI driver and allocate a structure
                to control that connection

Parameters   : ST_DeviceName_t      DeviceName      ATAPI driver name
               STATAPI_OpenParams_t OpenParams      Params structure
               STATAPI_Handle_t     Handle          Stored handle identifier for
                                                    communicating with the
                                                    ATA/ATAPI driver

Return Value :
  ST_NO_ERROR                       No error.
  ST_ERROR_BAD_PARAMETER            The device address parameter is invalid.
  ST_ERROR_NO_FREE_HANDLES          The ATA/ATAPI device is already open.
  ST_ERROR_UNKNOWN_DEVICE           Invalid device name or unable to identify the
                                    device when issuing a IDENTIFY DEVICE command.
  STATAPI_ERROR_DEVICE_NOT_PRESENT  The ATA/ATAPI device at the nominated address is
                                        not present.
 ****************************************************************************/

ST_ErrorCode_t STATAPI_Open(const ST_DeviceName_t          DeviceName,
                             const  STATAPI_OpenParams_t   *OpenParams,
                             STATAPI_Handle_t              *Handle)
 {
    ST_ErrorCode_t rVal=ST_NO_ERROR;
    ata_ControlBlock_t *ATAHandle;
    STATAPI_DeviceAddr_t Num;
    
    /* Obtain the ATAPI control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);
    
#if !defined(STATAPI_NO_PARAMETER_CHECK)
   
    if (NULL == ATAHandle)
    {
     /* The named device does not exist */
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    if (strcmp(DeviceName, ATAHandle->DeviceName) != 0)
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    if ((OpenParams == NULL) || (Handle == NULL))
    {
        return ST_ERROR_BAD_PARAMETER;
    }
    if (IsOutsideRange(OpenParams->DeviceAddress,
                       STATAPI_DEVICE_0,
                       STATAPI_DEVICE_1))
    {
       return( ST_ERROR_BAD_PARAMETER );
    }
    if(ATAHandle->DeviceType == STATAPI_SATA)   
    {
    	if(OpenParams->DeviceAddress == STATAPI_DEVICE_1)
    	{
    		 return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
#endif

    Num = OpenParams->DeviceAddress;
    
    if (ATAHandle->Handles[Num].Opened)
    {
        return ST_ERROR_NO_FREE_HANDLES;
    }

    if ((ATAHandle->DevInBus[Num] == NONE_DEVICE) ||
        (ATAHandle->DevInBus[Num] == UNKNOWN_DEVICE))
    {
        return STATAPI_ERROR_DEVICE_NOT_PRESENT;
    }

    /* Identify device and set the common capabilities (host and device) */
    rVal = GetDeviceCapabilities(ATAHandle,Num);
    if (rVal != ST_NO_ERROR)
    {
        return rVal;
    }

    /* Set up the other fields*/
    ATAHandle->Handles[Num].Abort = FALSE;
    ATAHandle->Handles[Num].Notify = FALSE;
    ATAHandle->Handles[Num].MultiCnt = 1;
    ATAHandle->Handles[Num].IsModeSet = FALSE;
    ATAHandle->Handles[Num].DeviceParams.CurrentDmaMode =
                                            STATAPI_DMA_NOT_SUPPORTED;

    /* Ensure these are false. */
    memset(&ATAHandle->Handles[Num].PioTimingOverridden,
           FALSE, sizeof(ATAHandle->Handles[Num].PioTimingOverridden));
    memset(&ATAHandle->Handles[Num].MWDMATimingOverridden,
           FALSE, sizeof(ATAHandle->Handles[Num].MWDMATimingOverridden));
    memset(&ATAHandle->Handles[Num].UDMATimingOverridden,
           FALSE, sizeof(ATAHandle->Handles[Num].UDMATimingOverridden));

    *Handle = (STATAPI_Handle_t)&ATAHandle->Handles[Num];
    ATAHandle->Handles[Num].Opened = TRUE;
    ATAHandle->UsedHandles++;

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : STATAPI Close()

Description  : Closes the connection to the ATA/ATAPI device associated with the
               device handle.

Parameters   : STATAPI_Handle_t     Handle        Handle to ATA/ATAPI device on
                                                  host bus.

Return Value :
          ST_NO_ERROR                   No error
          ST_ERROR_INVALID_HANDLE       Handle invalid or not open.
 ****************************************************************************/
ST_ErrorCode_t STATAPI_Close(STATAPI_Handle_t Handle)
 {

    ata_DevCtrlBlock_t  *Dev;
    ata_ControlBlock_t *ATAHandle;
    U8                  DevNo;
    clock_t             TO;
    
    Dev=(ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;
    if((Dev!= &ATAHandle->Handles[0]) && (Dev!= &ATAHandle->Handles[1]))
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    if (Dev->Opened==FALSE)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
#endif

    if(Dev== &ATAHandle->Handles[0])
    {
        DevNo=DEVICE_0;
    }
    else if(Dev== &ATAHandle->Handles[1])
    {
        DevNo=DEVICE_1;
    }
    else
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Try to acquire the bus */
    if(ata_bus_Acquire(ATAHandle))
    {
        /*We fail: someone is using the bus*/
        if(ATAHandle->DeviceSelected==DevNo)
        { /* It's me ... I have to abort */
            Dev->Abort=TRUE;
            TO = time_plus(time_now(), CLOSING_TIMEOUT);
            if(STOS_SemaphoreWaitTimeOut(Dev->EndCmdSem_p,&TO))
            {
                return ST_ERROR_INVALID_HANDLE;
            }
        }
         /* It's not me... nothing to do */
    }

    Dev->Opened= FALSE;
    Dev->IsModeSet= FALSE;
    Dev->Notify= FALSE;
    Dev->Abort= FALSE;
    ATAHandle->UsedHandles--;
    ata_bus_Release(ATAHandle);

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : ATAPI_SetPioMode()

Description  : Sets the current Pio mode on the device handle. This
               function is used internally by both STATAPI_SetPioMode()
               and ParseIdentifyBlock(). It assumes all housework
               (acquiring bus, validating mode etc.) has been done
               beforehand.

Parameters   : STATAPI_PioMode_t    Mode          PIO mode to invoke.


Return Value :
    TRUE        An error occurred
    FALSE       All okay
****************************************************************************/
BOOL ATAPI_SetPioMode(ata_ControlBlock_t *ATAHandle,ata_DevCtrlBlock_t  *Dev,
                      								STATAPI_PioMode_t Mode)
{
    BOOL error = FALSE;

    error = hal_SetPioMode(ATAHandle->HalHndl, Mode);
    if (FALSE == error)
    {
        Dev->DeviceParams.CurrentPioMode = Mode;

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
        /* Set the timing, based on the stored value */
        {
            U32 index;

            index = PIOMODE_TO_INDEX(Mode);
            if (Dev->PioTimingOverridden[index])
                hal_SetPioTiming(ATAHandle->DeviceName,ATAHandle->HalHndl,
                             &Dev->PioTimingOverrides[index]);
            else
                hal_SetPioTiming(ATAHandle->DeviceName,ATAHandle->HalHndl,
                                 &PioTimingDefaults[index]);
        }
#endif
    }

    return error;
}

/****************************************************************************
Name         : STATAPI SetPioMode()

Description  : Sets the current PIO mode on the device handle.

Parameters   : STATAPI_Handle_t     Handle        Handle to ATA/ATAPI device on
                                                  host bus.
               STATAPI_PIOMode_t    Mode          PIO mode to invoke.


Return Value :
    ST_NO_ERROR                      The PIO mode was set without error.
    ST_ERROR_BAD_PARAMETER           Invalid mode parameter.
    ST_ERROR_DEVICE_BUSY             The ATA/ATAPI device or host controller
                                     is busy.
    ST_ERROR_INVALID_HANDLE          The handle is invalid.
    ST_ERROR_FEATURE_NOT_SUPPORTED   The host controller or ATA/ATAPI device does
                                     not  supported the PIO mode.
 ****************************************************************************/
ST_ErrorCode_t STATAPI_SetPioMode(STATAPI_Handle_t Handle,
                                  STATAPI_PioMode_t Mode)
{
    ST_ErrorCode_t RetCode;
    ata_ControlBlock_t *ATAHandle = NULL;    
    ata_DevCtrlBlock_t *Dev;
    U32 Dummy;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;    
        
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    RetCode = ST_NO_ERROR;
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Check Parameters  */
    if ((Dev!= &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (IsOutsideRange( Mode,STATAPI_PIO_MODE_0, STATAPI_PIO_MODE_4))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }
    else if(Dev->Opened==FALSE)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    /* Check if the mode is supported  */
    Dummy = 1 << Mode;

    /* Note! This *will* need to be changed if the driver gets
     * extended to support more PIO modes.
     */
    if (Dev->DeviceParams.SupportedPioModes & Dummy)
    {
        if (Dev->DeviceParams.CurrentPioMode != Mode)
        {
            /* The RetCode isn't a perfect match, but it does kind of
             * fit.
             */
            if (ATAPI_SetPioMode(ATAHandle,Dev, Mode) == TRUE)
                RetCode = STATAPI_ERROR_MODE_NOT_SET;
        }
    }
    else
    {
        RetCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    /* Only go to 'mode set' state if all okay */
    if (ST_NO_ERROR == RetCode)
        Dev->IsModeSet = TRUE;

    ata_bus_Release(ATAHandle);

    return RetCode;
}

/****************************************************************************
Name         : ATAPI_SetDmaMode()

Description  : Sets the current Dma mode on the device handle. This
               function is used internally by both STATAPI_SetDmaMode()
               and ParseIdentifyBlock(). It assumes all housework
               (acquiring bus, validating mode etc.) has been done
               beforehand.

Parameters   : STATAPI_DmaMode_t    Mode          DMA mode to invoke.


Return Value :
    TRUE        An error occurred
    FALSE       All okay
****************************************************************************/
BOOL ATAPI_SetDmaMode(ata_ControlBlock_t *ATAHandle,ata_DevCtrlBlock_t  *Dev,
                      								STATAPI_DmaMode_t Mode)
{
    BOOL error = FALSE;

    if(ATAHandle->DeviceType != STATAPI_EMI_PIO4)  
    {
	    error = hal_SetDmaMode(ATAHandle->HalHndl, Mode);
	
	    if (error == FALSE)
	    {
	        /* Set DMA mode okay, so restore last known timing
	         * parameters for this mode
	         */
	   
	        Dev->DeviceParams.CurrentDmaMode = Mode;

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
		{
			U32 index;
	        if (IsOutsideRange(Mode, STATAPI_DMA_MWDMA_MODE_0,
	                           STATAPI_DMA_MWDMA_MODE_2))
	        {
	            /* UDMA */
	            index = UDMAMODE_TO_INDEX(Mode);
	            if (Dev->UDMATimingOverridden[index])
	                hal_SetUDMATiming(ATAHandle->DeviceName,ATAHandle->HalHndl,
	                                  &Dev->UltraDmaTimingOverrides[index]);
	            else
	                hal_SetUDMATiming(ATAHandle->DeviceName,ATAHandle->HalHndl,
	                                  &UltraDmaTimingDefaults[index]);
	        }
	        else
	        {
	            index = MWDMAMODE_TO_INDEX(Mode);
	            if (Dev->MWDMATimingOverridden[index])
	                hal_SetMWDMATiming(ATAHandle->DeviceName,ATAHandle->HalHndl,
	                                   &Dev->MwDmaTimingOverrides[index]);
	            else
	                hal_SetMWDMATiming(ATAHandle->DeviceName,ATAHandle->HalHndl,
	                                   &MwDmaTimingDefaults[index]);
            }
        }
#endif /*Needed only in case of HDDI*/
      }
    }
    else
    {
       error = TRUE;
	}/*if*/
    return error;
}

/****************************************************************************
Name         : STATAPI SetDmaMode()

Description  : Sets the current Dma mode on the device handle.

Parameters   : STATAPI_Handle_t     Handle        Handle to ATA/ATAPI device on
                                                  host bus.
               STATAPI_DmaMode_t    Mode          DMA mode to invoke.


Return Value :
    ST_NO_ERROR                      The DMA mode was set without error.
    ST_ERROR_BAD_PARAMETER           Invalid mode parameter.
    ST_ERROR_DEVICE_BUSY             The ATA/ATAPI device or host controller
                                     is busy.
    ST_ERROR_INVALID_HANDLE          The handle is invalid.
    ST_ERROR_FEATURE_NOT_SUPPORTED   The host controller or ATA/ATAPI device does
                                     not  supported the DMA mode.
****************************************************************************/
ST_ErrorCode_t STATAPI_SetDmaMode(STATAPI_Handle_t Handle,
                                  STATAPI_DmaMode_t Mode)
{
    ST_ErrorCode_t RetCode;
    ata_ControlBlock_t *ATAHandle = NULL;    
    ata_DevCtrlBlock_t *Dev;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;    
        
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    RetCode = ST_NO_ERROR;
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Check Parameters  */
    if (NULL == Dev)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if ((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->Opened == FALSE)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (IsOutsideRange(Mode,
                       STATAPI_DMA_MWDMA_MODE_0, STATAPI_DMA_NOT_SUPPORTED))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }
    else if (Mode == STATAPI_DMA_NOT_SUPPORTED)
    {
        /* This would be a stupid thing to try and set. */
        RetCode = ST_ERROR_BAD_PARAMETER;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

   if(ATAHandle->DeviceType != STATAPI_EMI_PIO4)  
   {
	    /* Check if the mode is supported */
	    if (Dev->DeviceParams.SupportedDmaModes & (1 << Mode))
	    {
	        /* Set the mode */
	        if(Dev->DeviceParams.CurrentDmaMode != Mode)
	        {
	            /* The RetCode isn't a perfect match, but it does kind of
	             * fit.
	             */
	            if (ATAPI_SetDmaMode(ATAHandle,Dev, Mode) == TRUE)
	                RetCode = STATAPI_ERROR_MODE_NOT_SET;
	        }
	    }
    	else
    		        RetCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    	
    }
    else
    {
    	   RetCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    /* Only go to 'mode set' state if all okay */
    if (ST_NO_ERROR == RetCode)
        Dev->IsModeSet = TRUE;

    ata_bus_Release(ATAHandle);

    return RetCode;
}

/*************************************************************************
Name         : STATAPI_SetDmaTiming()

Description  : Set up the given DMA timing values.

Parameters   :
    STATAPI_Handle_t     Handle
    STATAPI_DmaTiming_t  Timing values to set

Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_iNVALID_HANDLE   Handle given is invalid
  ST_ERROR_BAD_PARAMETER    Invalid capability parameter.

 ************************************************************************/
ST_ErrorCode_t STATAPI_SetDmaTiming(STATAPI_Handle_t Handle,
                                    STATAPI_DmaTiming_t *TimingParams)
{
    ST_ErrorCode_t RetCode;
    ata_ControlBlock_t *ATAHandle = NULL;    
    ata_DevCtrlBlock_t  *Dev;
    BOOL error = FALSE;

    Dev = (ata_DevCtrlBlock_t *)Handle;

    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE; 
         
    if(ATAHandle->DeviceType == STATAPI_SATA)          
    {
    	 /*No Timing needed in case of sata*/
    	return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
          
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    RetCode = ST_NO_ERROR;
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Check Parameters  */
    if ((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->Opened == FALSE)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    /* DMA supported? */
    if (Dev->DeviceParams.CurrentDmaMode == STATAPI_DMA_NOT_SUPPORTED)
    {
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
    else
    {
        error = hal_SetDmaTiming(ATAHandle->DeviceName,ATAHandle->HalHndl, TimingParams);
        if (error == FALSE)
        {
            /* Store the timing. Yes, this is ugly... */
            if (IsOutsideRange(Dev->DeviceParams.CurrentDmaMode,
                               STATAPI_DMA_MWDMA_MODE_0,
                               STATAPI_DMA_MWDMA_MODE_2))
            {
                /* UDMA */
                Dev->UltraDmaTimingOverrides[UDMAMODE_TO_INDEX(Dev->DeviceParams.CurrentDmaMode)] =
                        TimingParams->DmaTimingParams.UltraDmaTimingParams;
            }
            else
            {
                Dev->MwDmaTimingOverrides[MWDMAMODE_TO_INDEX(Dev->DeviceParams.CurrentDmaMode)] =
                        TimingParams->DmaTimingParams.MwDmaTimingParams;
            }
        }
    }

    ata_bus_Release(ATAHandle);

    return (error == FALSE)?ST_NO_ERROR:ST_ERROR_BAD_PARAMETER;
}

/*************************************************************************
Name         : STATAPI_SetPioTiming()

Description  : Set up the given PIO timing values.

Parameters   :
    STATAPI_Handle_t     Handle
    STATAPI_DmaTiming_t  Timing values to set

Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_iNVALID_HANDLE   Handle given is invalid
  ST_ERROR_BAD_PARAMETER    Invalid capability parameter.

 ************************************************************************/
ST_ErrorCode_t STATAPI_SetPioTiming(STATAPI_Handle_t Handle,
                                    STATAPI_PioTiming_t *TimingParams)
{
    ST_ErrorCode_t 		 error,RetCode = ST_NO_ERROR;
    ata_ControlBlock_t  *ATAHandle = NULL;    
    ata_DevCtrlBlock_t  *Dev;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;   
        
    if(ATAHandle->DeviceType == STATAPI_SATA)          
    {
    	 /*No Timing needed in case of sata*/
    	return ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Check Parameters  */
    if ((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->Opened == FALSE)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    error = hal_SetPioTiming(ATAHandle->DeviceName,ATAHandle->HalHndl, TimingParams);

    if (error == FALSE)
    {
        U32 index = PIOMODE_TO_INDEX(Dev->DeviceParams.CurrentPioMode);

        /* Store the timing values */
        Dev->PioTimingOverrides[index] = *TimingParams;
        Dev->PioTimingOverridden[index] = TRUE;
    }

    ata_bus_Release(ATAHandle);

    return (error == FALSE)?ST_NO_ERROR:ST_ERROR_BAD_PARAMETER;
}

/*************************************************************************
Name         : STATAPI GetCapability()

Description  : Get information regarding the host's hardware capabilities.

Parameters   :
    ST_DeviceName_t DeviceName           Device name of an initialized driver
                                         instance.

    STATAPI_Capability_t *Capability    Pointer to capability structure for storing
                                        capability result.

Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_UNKNOWN_DEVICE   Unknown device name.
  ST_ERROR_BAD_PARAMETER    Invalid capability parameter.

 ************************************************************************/
ST_ErrorCode_t STATAPI_GetATAStatus(STATAPI_Handle_t Handle,
                                    U8 *Status,
                                    U8 *AltStatus,
                                    U8 *Error)
{
    ST_ErrorCode_t 		  RetCode = ST_NO_ERROR;
    ata_DevCtrlBlock_t   *Dev;
    ata_ControlBlock_t   *ATAHandle = NULL;
    
    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;
   
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /* Can't be valid, not initialised */

    if ((NULL == Dev) || (FALSE == Dev->Opened))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if ((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if ((NULL == Status) || (NULL == AltStatus) || (NULL == Error))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    /* Because this function may be called during another command (to
     * check completion status), and because it's non-destructive, no
     * bus acquisition is made.
     */
    *Status = hal_RegInByte(ATAHandle->HalHndl, ATA_REG_STATUS);
    *AltStatus = hal_RegInByte(ATAHandle->HalHndl, ATA_REG_ALTSTAT);
    *Error = hal_RegInByte(ATAHandle->HalHndl, ATA_REG_ERROR);

    ata_bus_Release(ATAHandle);

    return ST_NO_ERROR;
}

/*************************************************************************
Name         : STATAPI GetCapability()

Description  : Get information regarding the host's hardware capabilities.

Parameters   :
    ST_DeviceName_t DeviceName           Device name of an initialized driver
                                         instance.

    STATAPI_Capability_t *Capability    Pointer to capability structure for storing
                                        capability result.

Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_UNKNOWN_DEVICE   Unknown device name.
  ST_ERROR_BAD_PARAMETER    Invalid capability parameter.

 ************************************************************************/
ST_ErrorCode_t  STATAPI_GetCapability(const ST_DeviceName_t DeviceName,
                                       STATAPI_Capability_t *Capability)
{
    ST_ErrorCode_t 		RetCode = ST_NO_ERROR;
    ata_ControlBlock_t *ATAHandle = NULL;
    
    /* Obtain the control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);
    
    if (NULL == ATAHandle)
        return ST_ERROR_UNKNOWN_DEVICE;
        
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Parameter checking  */
       
    if (strcmp(ATAHandle->DeviceName, DeviceName) != 0)
    {
        RetCode = ST_ERROR_UNKNOWN_DEVICE;
    }   
    /* Parameter checking  */
    if (Capability == NULL)
        RetCode = ST_ERROR_BAD_PARAMETER;

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    if (hal_GetCapabilities(ATAHandle->DeviceName,ATAHandle->HalHndl, Capability))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }

    ata_bus_Release(ATAHandle);

    return RetCode;
}

/*************************************************************************
Name         : STATAPI GetParams()

Description  :  Obtains the device parameters.
Parameters   :
    STATAPI_Handle_t     Handle     Handle allowing access to host device.
    STATAPI_Params_t     *Params    Pointer to the device parameters structure.


Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_INVALID_HANDLE   Device handle is invalid.
  ST_ERROR_BAD_PARAMETER    Invalid parameter.

 ************************************************************************/
ST_ErrorCode_t  STATAPI_GetParams(STATAPI_Handle_t     Handle,
                                  STATAPI_Params_t     *Params)
{
    ST_ErrorCode_t      RetCode = ST_NO_ERROR;
    ata_ControlBlock_t *ATAHandle = NULL;
    ata_DevCtrlBlock_t *Dev;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;
       
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Parameter checking  */

    if ((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->Opened==FALSE)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    memcpy(Params,&Dev->DeviceParams,sizeof(STATAPI_Params_t));

    ata_bus_Release(ATAHandle);

    return ST_NO_ERROR;
}

/*************************************************************************
Name         : STATAPI GetDriveType()

Description  :  Obtains what kind of drive is (ATA or ATAPI).
Parameters   :
    STATAPI_Handle_t     Handle     Handle allowing access to host device.
    STATAPI_DriveType_t     *Type   Pointer to the drive type variable.


Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_INVALID_HANDLE   Device handle is invalid.
  ST_ERROR_BAD_PARAMETER    Invalid parameter.

 ************************************************************************/
ST_ErrorCode_t  STATAPI_GetDriveType(STATAPI_Handle_t     Handle,
                                     STATAPI_DriveType_t     *Type)
{
    ST_ErrorCode_t       RetCode = ST_NO_ERROR;
    ata_ControlBlock_t  *ATAHandle = NULL;
    ata_DevCtrlBlock_t  *Dev;
    U8      			 DevNo;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;    
       
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Parameter checking  */

    if((Dev!= &ATAHandle->Handles[0]) && (Dev!= &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->Opened==FALSE)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Type== NULL)
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    if (Dev == &ATAHandle->Handles[0])
    {
        DevNo = DEVICE_0;
    }
    else
    {
        DevNo = DEVICE_1;
    }

    if (ATAHandle->DevInBus[DevNo] == ATAPI_DEVICE)
    {
        *Type = STATAPI_ATAPI_DRIVE;
    }
    else if (ATAHandle->DevInBus[DevNo] == ATA_DEVICE)
    {
        *Type = STATAPI_ATA_DRIVE;
    }
    else
    {
        *Type = STATAPI_UNKNOWN_DRIVE;
    }

    ata_bus_Release(ATAHandle);

    return ST_NO_ERROR;
}

/*************************************************************************
Name         : STATAPI_GetExtendedError()

Description  : Returns the internal extended error code. Undocumented
               function. Bus acquisition has deliberately been left out,
               so that this can be called at any time.

Parameters   :

    STATAPI_Handle_t    Handle      Handle allowing access to host device.
    U8                  ErrorCode   Pointer to where to store the code

Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_INVALID_HANDLE   Device handle is invalid.
  ST_ERROR_BAD_PARAMETER    Invalid parameter.

 ************************************************************************/
ST_ErrorCode_t STATAPI_GetExtendedError(STATAPI_Handle_t     Handle,
                                        U8          *ErrorCode)
{
    ata_DevCtrlBlock_t  *Dev;
    ata_ControlBlock_t  *ATAHandle = NULL;
    
    Dev = (ata_DevCtrlBlock_t*) Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);

    /*  Parameter checking  */
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;
    if ((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    if (Dev->Opened == FALSE)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    *ErrorCode = ATAHandle->LastExtendedErrorCode;

    return ST_NO_ERROR;
}

/*************************************************************************
Name         : STATAPI GetPioMode()

Description  :  Obtains the PIO mode and timing parameters used for the device driver
Parameters   :
    STATAPI_Handle_t    Handle          Handle allowing access to host device.
    STATAPI_PioMode_t   Mode            Stores the current PIO mode.
    STATAPI_PioTiming_t TimingParams    Stores the current timing parameters
                                        associated with the PIO
mode.

Return Value :
  ST_NO_ERROR                 No error.
  ST_ERROR_INVALID_HANDLE     Device handle is invalid.
  ST_ERROR_BAD_PARAMETER      Invalid parameter.
  STATAPI_ERROR_MODE_NOT_SET   Mode not set yet
 ************************************************************************/
ST_ErrorCode_t  STATAPI_GetPioMode(STATAPI_Handle_t     Handle,
                                   STATAPI_PioMode_t *Mode,
                                   STATAPI_PioTiming_t *TimingParams)
{
    ST_ErrorCode_t 		 RetCode = ST_NO_ERROR;
    ata_ControlBlock_t  *ATAHandle = NULL;    
    ata_DevCtrlBlock_t  *Dev;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);  
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;   
      
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Parameter checking  */
 
    if((Dev!= &ATAHandle->Handles[0]) && (Dev!= &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->Opened==FALSE)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if ((Mode == NULL) || (TimingParams == NULL))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }
    else if (Dev->IsModeSet==FALSE)
    {
        RetCode = STATAPI_ERROR_MODE_NOT_SET;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    if (hal_GetPioTiming(ATAHandle->DeviceName,ATAHandle->HalHndl, TimingParams))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }

    *Mode = Dev->DeviceParams.CurrentPioMode;

    ata_bus_Release(ATAHandle);

    return RetCode;
}

/*************************************************************************
Name         : STATAPI GetDmaMode()

Description  :  Obtains the DMA mode and timing parameters used for the device driver
Parameters   :
    STATAPI_Handle_t    Handle          Handle allowing access to host device.
    STATAPI_DmaMode_t   Mode            Stores the current Dma mode.
    STATAPI_DmaTiming_t TimingParams    Stores the current timing parameters
                                        associated with the Dma
mode.

Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_INVALID_HANDLE   Device handle is invalid.
  ST_ERROR_BAD_PARAMETER    Invalid parameter.
 STATAPI_ERROR_MODE_NOT_SET   Mode not set yet
 ************************************************************************/
ST_ErrorCode_t  STATAPI_GetDmaMode(STATAPI_Handle_t     Handle,
                                   STATAPI_DmaMode_t *Mode,
                                   STATAPI_DmaTiming_t *TimingParams)
{
    ST_ErrorCode_t RetCode;
    ata_DevCtrlBlock_t  *Dev;
    ata_ControlBlock_t *ATAHandle = NULL;
    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;
        
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    RetCode = ST_NO_ERROR;
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Parameter checking  */
    if((Dev!= &ATAHandle->Handles[0]) && (Dev!= &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->Opened==FALSE)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->IsModeSet==FALSE)
    {
        RetCode = STATAPI_ERROR_MODE_NOT_SET;
    }
    else if ((Mode == NULL) || (TimingParams == NULL))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }
    else if (hal_GetDmaTiming(ATAHandle->DeviceName,ATAHandle->HalHndl, TimingParams))
    {
        RetCode =  ST_ERROR_BAD_PARAMETER;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    *Mode=Dev->DeviceParams.CurrentDmaMode;

    ata_bus_Release(ATAHandle);

    return ST_NO_ERROR;
}
/*************************************************************************
Name         : STATAPI Abort()

Description  : Aborts any pending command or packet transfer in progress with
               an ATA/ATAPI device. Can't do bus acquire/release, since
               (by the nature of the function) a transfer may be in progress.

Parameters   :
  STATAPI_Handle_t    Handle          Handle allowing access to host device.

Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_INVALID_HANDLE   Device handle is invalid.


 ************************************************************************/
ST_ErrorCode_t  STATAPI_Abort(STATAPI_Handle_t  Handle)
{
    ST_ErrorCode_t RetCode;
    ata_DevCtrlBlock_t  *Dev;
    ata_ControlBlock_t *ATAHandle = NULL;    
    U8 DevNo;

    Dev = (ata_DevCtrlBlock_t *)Handle;

    RetCode = ST_NO_ERROR;
    
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Parameter checking  */
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;
    
    if ((Dev!= &ATAHandle->Handles[0]) && (Dev!= &ATAHandle->Handles[1]))
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->Opened==FALSE)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
#endif

    if (Dev == &ATAHandle->Handles[0])
    {
        DevNo = DEVICE_0;
    }
    else
    {
        DevNo = DEVICE_1;
    }

    if (ATAHandle->DeviceSelected != DevNo)
    {
        RetCode = ST_NO_ERROR;    /* Nothing to abort */
    }
    else if (ATAHandle->BusBusy == FALSE)
    {
        RetCode = ST_NO_ERROR;    /* Nothing to abort */
    }
    else
    {
        /* Set abort flag */
        Dev->Abort = TRUE;
    }

    return RetCode;
}

/*************************************************************************
Name         : STATAPI HardReset()

Description  : Performs a hardware reset.
Parameters   :
    STATAPI_Handle_t    Handle          Handle allowing access to host device.


Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_INVALID_HANDLE   Device handle is invalid.
  STATAPI_ERROR_DEVICE_NOT_PRESENT  The ATA/ATAPI device at the nominated address is
                                        not present.

 ************************************************************************/
ST_ErrorCode_t  STATAPI_HardReset(const ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t  Error;

#if defined(ST_5512) || defined(ST_5514) || defined(ST_5528) || defined(ST_8010) || defined(ST_7100) \
|| defined(ST_7109)
    STATAPI_EvtParams_t   EvtData;
#endif
    
    ata_ControlBlock_t *ATAHandle = NULL;
   
    /* Obtain the control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);

    /*  Parameter checking  */
    if (NULL == ATAHandle)
    { 
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    if (strcmp(ATAHandle->DeviceName,DeviceName) != 0)
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

#if defined(ST_5512) || defined(ST_5514) || defined(ST_5528) || defined(ST_8010) ||\
defined(ST_7100) || defined(ST_7109)
   
    /* No hard reset functionality present in EMI ATA*/
    if(ATAHandle->DeviceType == STATAPI_EMI_PIO4)
    {
    	Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
    else
    {
	    /* Abort any possible command */
	    ATAHandle->Handles[0].Abort = TRUE;
	    ATAHandle->Handles[1].Abort = TRUE;
	
	    ata_ctrl_HardReset(ATAHandle);
	
	    /* Notify the user that a Hard reset has been performed */
	
	    EvtData.Error = ST_NO_ERROR;
	    STEVT_Notify(ATAHandle->EvtHndl,
	                 ATAHandle->EvtIds[STATAPI_HARD_RESET_ID],
	                 &EvtData);
	
	    /* Now let's see what we have on the bus */
	    ata_ctrl_Probe(ATAHandle);
	
	    ata_ctrl_HardReset(ATAHandle);
	
	    ATAHandle->Handles[0].Abort = FALSE;
	    ATAHandle->Handles[1].Abort = FALSE;
	
	    /* Refresh the device parameters */
	    if(ATAHandle->Handles[0].Opened == TRUE)
	    {
	        Error = GetDeviceCapabilities(ATAHandle,STATAPI_DEVICE_0);
	        ATAHandle->Handles[0].IsModeSet = FALSE;
	        ATAHandle->Handles[0].MultiCnt = 1;
	        /* Ensure these don't match anything, so when the user sets the
	        * next mode everything gets reprogrammed.
	        */
	        ATAHandle->Handles[0].DeviceParams.CurrentPioMode =
	                                                STATAPI_PIO_MODE_4 + 1;
	        ATAHandle->Handles[0].DeviceParams.CurrentDmaMode =
	                                                STATAPI_DMA_NOT_SUPPORTED + 1;
	    }
	
	    if(ATAHandle->Handles[1].Opened == TRUE)
	    {
	        Error = GetDeviceCapabilities(ATAHandle,STATAPI_DEVICE_1);
	        ATAHandle->Handles[1].IsModeSet = FALSE;
	        ATAHandle->Handles[1].MultiCnt = 1;
	        /* Ensure these don't match anything, so when the user sets the
	         * next mode everything gets reprogrammed.
	         */
	        ATAHandle->Handles[1].DeviceParams.CurrentPioMode =
	                                                STATAPI_PIO_MODE_4 + 1;
	        ATAHandle->Handles[1].DeviceParams.CurrentDmaMode =
	                                                STATAPI_DMA_NOT_SUPPORTED + 1;
	    }
    }
#else
    /* i.e. not 5512, 5514 or 5528 or 8010 */
    Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

    return Error;
}

/*************************************************************************
Name         : STATAPI SoftReset()


Description  : Performs a software reset.
Parameters   :
    ST_DeviceName_t  DeviceName          NAme of the device to reset.


Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_INVALID_HANDLE   Device handle is invalid.
  STATAPI_ERROR_DEVICE_NOT_PRESENT  The ATA/ATAPI device at the nominated address is
                                        not present.

 ************************************************************************/
ST_ErrorCode_t  STATAPI_SoftReset(const ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t  Error;
    ata_ControlBlock_t *ATAHandle = NULL;    
    STATAPI_EvtParams_t   EvtData;

    /* Obtain the control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);

    /*  Parameter checking  */
    if (ATAHandle == NULL)
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }    
    if (strcmp(ATAHandle->DeviceName,DeviceName) != 0)
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Abort any possible command */
    ATAHandle->Handles[0].Abort= TRUE;
    ATAHandle->Handles[1].Abort= TRUE;

    /* This can fail (returning an error). However, if it does, then
     * the GetDeviceCapabilities below should also fail to do device
     * diagnostics, and return an error to the user. So we'll leave this
     * alone.
     */
    ata_ctrl_SoftReset(ATAHandle);

    /* Notify the user that a Soft reset has been perfomed */
    EvtData.Error = ST_NO_ERROR;
    STEVT_Notify(ATAHandle->EvtHndl,
                 ATAHandle->EvtIds[STATAPI_SOFT_RESET_ID],
                 &EvtData);

    ATAHandle->Handles[0].Abort= FALSE;
    ATAHandle->Handles[1].Abort= FALSE;

    /* Don't know what the devices *now* support, so find out and
     * get back in sync
     */
    Error = GetDeviceCapabilities(ATAHandle,(STATAPI_DeviceAddr_t)ATAHandle->DeviceSelected);

    /* Refresh the device parameter */
    /* TODO: since we did the selected device above, do we need to do
     * both again below? -- PW
     */
    if (Error == FALSE)
    {
        if(ATAHandle->Handles[0].Opened== TRUE)
        {
            Error= GetDeviceCapabilities(ATAHandle,STATAPI_DEVICE_0);
            ATAHandle->Handles[0].IsModeSet= FALSE;
        }

        if(ATAHandle->Handles[1].Opened== TRUE)
        {
            Error= GetDeviceCapabilities(ATAHandle,STATAPI_DEVICE_1);
            ATAHandle->Handles[1].IsModeSet= FALSE;
        }
    }

    return Error;
}

/****************************************************************************
Name         : ChooseProtocolCmd

Description  : Selects the transfer protocol (PIO/DMA) based on the command;
               see return value below.

Parameters   :
    CommandCode         The command to be executed
    Dev                 Pointer to a device control block, for accessing
                        required capability details
Return Value :
    ATA_CMD_NODATA ...  Which transfer protocol is required by the command
                        and supported by both the host and device
    0                   The command requires a protocol that isn't supported
                        by both device and host
****************************************************************************/
static U8 ChooseProtocolCmd(U16 CommandCode, ata_DevCtrlBlock_t *Dev)
{
    U8 Type;
    BOOL Supported = TRUE;

#if defined (ATAPI_DEBUG)
	STTBX_Print((" CommandCode=%d",CommandCode));
#endif

    /* Figure out what type the command implies */
    switch (CommandCode)
    {
        case STATAPI_CMD_READ_SECTORS:
        case STATAPI_CMD_CFA_TRANSLATE_SECTOR:
        case STATAPI_CMD_READ_MULTIPLE:
        case STATAPI_CMD_IDENTIFY_DEVICE:
        case STATAPI_CMD_READ_BUFFER:
        case STATAPI_CMD_SMART_IN:
        case STATAPI_CMD_IDENTIFY_PACKET_DEVICE:
            Type = ATA_CMD_PIOIN;
            break;

        case STATAPI_CMD_READ_SECTORS_EXT:
        case STATAPI_CMD_READ_MULTIPLE_EXT:
            Type = ATA_CMD_PIOIN_EXT;
            break;

        case STATAPI_CMD_WRITE_SECTORS:
        case STATAPI_CMD_CFA_WRITE_SECTORS_WO_ERASE:
        case STATAPI_CMD_WRITE_MULTIPLE:
        case STATAPI_CMD_CFA_WRITE_MULTIPLE_WO_ERASE:
        case STATAPI_CMD_WRITE_BUFFER:
        case STATAPI_CMD_SECURITY_SET_PASSWORD:
        case STATAPI_CMD_SECURITY_UNLOCK:
        case STATAPI_CMD_SECURITY_DISABLE_PASSWORD:
        case STATAPI_CMD_SMART_OUT:
            Type = ATA_CMD_PIOOUT;
            break;

        case STATAPI_CMD_WRITE_SECTORS_EXT:
        case STATAPI_CMD_WRITE_MULTIPLE_EXT:
            Type = ATA_CMD_PIOOUT_EXT;
            break;

        case STATAPI_CMD_READ_DMA:
            Type = ATA_CMD_DMAIN;
            break;

        case STATAPI_CMD_WRITE_DMA:
            Type = ATA_CMD_DMAOUT;
            break;

        case STATAPI_CMD_READ_DMA_EXT:
            Type = ATA_CMD_DMAIN_EXT;
            break;

        case STATAPI_CMD_WRITE_DMA_EXT:
            Type = ATA_CMD_DMAOUT_EXT;
            break;

        case STATAPI_CMD_READ_VERIFY_SECTORS:
        case STATAPI_CMD_SEEK:
        case STATAPI_CMD_SET_MULTIPLE_MODE:
        case STATAPI_CMD_IDLE:
        case STATAPI_CMD_IDLE_IMMEDIATE:
        case STATAPI_CMD_STANDBY:
        case STATAPI_CMD_STANDBY_IMMEDIATE:
        case STATAPI_CMD_CHECK_POWER_MODE:
        case STATAPI_CMD_SLEEP:
        case STATAPI_CMD_FLUSH_CACHE:
        case STATAPI_CMD_MEDIA_EJECT:
        case STATAPI_CMD_SET_FEATURES:
        case STATAPI_CMD_CFA_REQUEST_EXT_ERR_CODE:
        case STATAPI_CMD_CFA_ERASE_SECTORS:
        case STATAPI_CMD_SMART_NODATA:
            Type = ATA_CMD_NODATA;
            break;

        case STATAPI_CMD_FLUSH_CACHE_EXT:
        case STATAPI_CMD_SET_MAX_ADDRESS_EXT:
        case STATAPI_CMD_READ_NATIVE_MAX_ADDRESS_EXT:
        case STATAPI_CMD_READ_VERIFY_SECTORS_EXT:
            Type = ATA_CMD_NODATA_EXT;
            break;
        case STATAPI_CMD_EXECUTE_DEVICE_DIAGNOSTIC:
             Type = ATA_EXEC_DIAG;
             break;
        case STATAPI_CMD_READ_DMA_QUEUED:
        case STATAPI_CMD_WRITE_DMA_QUEUED:
        case STATAPI_CMD_READ_DMA_QUEUED_EXT:
        case STATAPI_CMD_WRITE_DMA_QUEUED_EXT:
        default:
            Type = ATA_CMD_NOT_SUPPORTED;
            break;

    }

    /* Now see if both the host and the device can support that. These checks
     * are only for capability, *not* to see if host/device are in the correct
     * modes. That's an application problem.
     */
    if ((Type == ATA_CMD_DMAIN)     || (Type == ATA_CMD_DMAOUT) ||
        (Type == ATA_CMD_DMAIN_EXT) || (Type == ATA_CMD_DMAOUT_EXT))
    {
        /* If DMA disabled, or no DMA modes in common for host and device. */
        if ((Dev->DmaEnabled == FALSE) ||
            (Dev->DeviceParams.SupportedDmaModes == 0))
            Supported = FALSE;
    }

    /* This check is almost certainly pointless; maybe it should be removed. */
    if ((Type == ATA_CMD_PIOIN)     || (Type == ATA_CMD_PIOOUT) ||
        (Type == ATA_CMD_PIOIN_EXT) || (Type == ATA_CMD_PIOOUT_EXT))
    {
        if (Dev->DeviceParams.SupportedPioModes == 0)
            Supported = FALSE;
    }

    if (Type == ATA_CMD_NOT_SUPPORTED)
        Supported = FALSE;

    /* Exit point */
    if (Supported == TRUE)
        return Type;
    else
        return 0;
}

 /****************************************************************************
Name         : STATAPI CmdNoData()

Description  : Issues a command with no data transfer associated

Parameters   : STATAPI_Handle_t     Handle        Handle to ATA/ATAPI device on
                                                  host bus.
               STATAPI_Cmd_t       Cmd            Pointer to the command
                                                  information
               STATAPI_CmdStatus_t  CmdStat       Pointer that will be updated
                                                  when the command is completed


Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_INVALID_HANDLE   Device handle is invalid.
  ST_ERROR_DEVICE_BUSY      The command can not be executed as the BSY bit
                            is set in the status register, indicating the device is
                            busy.
  STATAPI_ERROR_CMD_ABORT   The ABRT bit is set in the error register indicating
                            that the command failed to complete e.g., a bad
                            parameter or unsupported command.
  STATAPI_ERROR_CMD_ERROR   A command-specific error has occured and the
                            user should interrogate the error.
  STATAPI_ERROR_USER_ABORT  The command or operation has been explicitly
                            aborted by the user.
STATAPI_ERROR_MODE_NOT_SET   Mode not set yet
 ****************************************************************************/
ST_ErrorCode_t STATAPI_CmdNoData(STATAPI_Handle_t Handle,
                                 const STATAPI_Cmd_t *Cmd,
                                 STATAPI_CmdStatus_t *CmdStatus)
{
    ST_ErrorCode_t RetCode;
    ata_ControlBlock_t *ATAHandle = NULL;    
    ata_DevCtrlBlock_t *Dev;
    U32 Dummy32 = 0;
    U8  Dummy8 = 0;
    U8  DevNo;

    Dev = (ata_DevCtrlBlock_t *)Handle;
 
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;
    
    /* Acquire the bus */
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    RetCode = ST_NO_ERROR;
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Check Parameters  */
    if ((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if ((Cmd == NULL) || (CmdStatus == NULL))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }
    else if (Dev->Opened == FALSE)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->IsModeSet == FALSE)
    {
        RetCode = STATAPI_ERROR_MODE_NOT_SET;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    if (Dev == &ATAHandle->Handles[0])
    {
        DevNo = DEVICE_0;
    }
    else
    {
        DevNo = DEVICE_1;
    }

    Dev->Notify = TRUE;
    Dev->Abort = FALSE;

    Dev->CmdType = ChooseProtocolCmd(Cmd->CmdCode, Dev);
    if (Dev->CmdType == 0)
    {
        /* Host/device do not support the transport implied by the command */
        ata_bus_Release(ATAHandle);
        return STATAPI_ERROR_PROTOCOL_NOT_SUPPORTED;
    }

    /* Set the Cmd structure */
    Dev->CmdToSend.Feature = Cmd->Features;
    Dev->CmdToSend.Stat.CmdStat = CmdStatus;
    Dev->CmdToSend.CommandCode = CmdCodes[Cmd->CmdCode];
    Dev->CmdType = ChooseProtocolCmd(Cmd->CmdCode, Dev);

    Dev->CmdToSend.CmdType = Dev->CmdType;

    if ((Cmd->UseLBA) && ((Dev->CmdType & ATA_EXT_BIT) == 0))
    {
        Dev->CmdToSend.SecCount= Cmd->SectorCount;

        /* Need to chop and set the bit */
        Dummy32 = Cmd->LBA;
        Dev->CmdToSend.SecNum   =  Dummy32 & 0x000000FF;
        Dummy32 = Dummy32 >> 8;
        Dev->CmdToSend.CylLow   =  Dummy32 & 0x000000FF;
        Dummy32 = Dummy32 >> 8;
        Dev->CmdToSend.CylHigh  =  Dummy32 & 0x000000FF;
        Dummy32 = Dummy32 >> 8;
        Dummy8  = Dummy32 & 0x0000000F;
        Dev->CmdToSend.DevHead = Dummy8 | LBA_SET_BIT;
    }
    else if ((Cmd->UseLBA) && ((Dev->CmdType & ATA_EXT_BIT) != 0))
    {
        /* Split the sector count */
        Dev->CmdToSend.SecCount  = (Cmd->SectorCount & 0xff00) >> 8;
        Dev->CmdToSend.SecCount2 = (Cmd->SectorCount & 0x00ff);

        /* Now split the LBA, higher bits first into the registers. */
        Dev->CmdToSend.CylHigh  = (Cmd->LBAExtended & 0xff00) >> 8;
        Dev->CmdToSend.CylLow   = (Cmd->LBAExtended & 0x00ff);
        Dev->CmdToSend.SecNum   = (Cmd->LBA  & 0xff000000) >> 24;

        Dev->CmdToSend.CylHigh2 = (Cmd->LBA  & 0x00ff0000) >> 16;
        Dev->CmdToSend.CylLow2  = (Cmd->LBA  & 0x0000ff00) >>  8;
        Dev->CmdToSend.SecNum2  = (Cmd->LBA  & 0x000000ff);

        /* Bits 5,7 obsolete; 3:0 reserved; LBA bit must be on, DEV bit must
         * be set appropriately.
         */
        Dev->CmdToSend.DevHead = LBA_SET_BIT;
        if (DevNo == DEVICE_0)
            Dev->CmdToSend.DevHead |= DEVHEAD_DEV0;
        else
            Dev->CmdToSend.DevHead |= DEVHEAD_DEV1;
    }
    else
    {
        Dev->CmdToSend.SecCount = Cmd->SectorCount;

        Dev->CmdToSend.CylLow = Cmd->CylinderLow;
        Dev->CmdToSend.CylHigh = Cmd->CylinderHigh;
        Dev->CmdToSend.DevHead = Cmd->Head & 0x0F;
        Dev->CmdToSend.SecNum = Cmd->Sector;
    }

    /* Add device select bit */
    if (DevNo == DEVICE_1)
    {
        Dev->CmdToSend.DevHead |= DEVICE_SELECT_1_MASK;
    }

    ATAHandle->DeviceSelected = DevNo;
    STOS_SemaphoreSignal(ATAHandle->BATSemaphore_p);

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : STATAPI CmdIn()

Description  : Issues a command to read data from the device

Parameters   : STATAPI_Handle_t     Handle        Handle to ATA/ATAPI device on
                                                  host bus.
               STATAPI_PIOMode_t    Mode          PIO mode to invoke.
               U8 *                 Data          Pointer to the buffer
                                                  allocated to store the data
               U32                 BufferSize     Size of the buffer
               U32 *               NumberRead     Pointer to a variable to be
                                                  updated with the number of
                                                  bytes actually read

Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_INVALID_HANDLE   Device handle is invalid.
  ST_ERROR_BAD_PARAMETER
  ST_ERROR_DEVICE_BUSY      The command can not be executed as the BSY bit
                            is set in the status register, indicating the device
                            is busy.
  STATAPI_ERROR_CMD_ABORT   The ABRT bit is set in the error register indicating
                            that the command failed to complete e.g., a bad
                            parameter or unsupported command.
  STATAPI_ERROR_CMD_ERROR   A command-specific error has occured and the
                            user should interrogate the error.
  STATAPI_ERROR_USER_ABORT  The command or operation has been explicitly
                            aborted by the user.
  STATAPI_ERROR_MODE_NOT_SET   Mode not set yet
 ****************************************************************************/
ST_ErrorCode_t STATAPI_CmdIn(STATAPI_Handle_t Handle,const STATAPI_Cmd_t *Cmd,
                              U8 *Data, U32 BufferSize, U32 *NumberRead,
                              STATAPI_CmdStatus_t *CmdStatus)
{
    ST_ErrorCode_t RetCode;
    ata_ControlBlock_t *ATAHandle = NULL;
    ata_DevCtrlBlock_t  *Dev;
    U32 Dummy32= 0;
    U32 SizeRequired = 0;
    U8  Dummy8=0;
    U8  DevNo;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* That seems okay; has this thing been initialised/opened? */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
        
    /* Acquire the bus */
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    RetCode = ST_NO_ERROR;
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Check Parameters  */
    if ((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if ((Cmd == NULL) || (CmdStatus == NULL) || (NumberRead == NULL))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }
    else if (Dev->Opened == FALSE)
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->IsModeSet==FALSE)
    {
        RetCode = STATAPI_ERROR_MODE_NOT_SET;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    /* Parameters coherence checking */
    SizeRequired = GetSizeRequired(Cmd);
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    if ((BufferSize < SizeRequired) || (Data == NULL))
    {
        ata_bus_Release(ATAHandle);
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

    /* Specific command checkings */
    if ((Cmd->CmdCode == STATAPI_CMD_READ_MULTIPLE) ||
        (Cmd->CmdCode == STATAPI_CMD_READ_MULTIPLE_EXT))
    {
        Dev->CmdToSend.MultiCnt= Dev->MultiCnt;
    }
    else
    {
        Dev->CmdToSend.MultiCnt=1;
    }

    /*------------------------All OK Let's do it-----------------------------*/

    if(Dev== &ATAHandle->Handles[0])
    {
        DevNo= DEVICE_0;
    }
    else
    {
        DevNo= DEVICE_1;
    }

    Dev->Notify= TRUE;
    Dev->Abort= FALSE;

    Dev->CmdType = ChooseProtocolCmd(Cmd->CmdCode, Dev);
    if (Dev->CmdType == 0)
    {
        /* Host/device do not support the transport implied by the command */
        ata_bus_Release(ATAHandle);
        return STATAPI_ERROR_PROTOCOL_NOT_SUPPORTED;
    }

    if ((Dev->CmdType == ATA_CMD_DMAIN)     ||
        (Dev->CmdType == ATA_CMD_DMAIN_EXT))
    {
        /* Check buffer alignment */
        if ( (((U32)Data / DMA_ALIGNMENT) * DMA_ALIGNMENT) != (U32)Data )
        {
            ata_bus_Release(ATAHandle);
            return ST_ERROR_BAD_PARAMETER;
        }

        /* Has a 256-sector limit (0 == 256); caller should know this. */
        Dev->CmdToSend.MultiCnt = Cmd->SectorCount & 0xff;
    }
#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
    else
    {
        /* currently also requires 4-byte alignment on PIO
         * transfers, due to HAL implementation and LMI requirements.
         */
        if ( (((U32)Data / 4) * 4) != (U32)Data )
        {
            ata_bus_Release(ATAHandle);
            return ST_ERROR_BAD_PARAMETER;
        }
     }
#endif

    /*------- Set the Cmd structure -----*/
    Dev->CmdToSend.Feature= Cmd->Features;
    Dev->CmdToSend.Stat.CmdStat= CmdStatus;
    Dev->CmdToSend.CommandCode= CmdCodes[Cmd->CmdCode];
    Dev->CmdToSend.DataBuffer= Data;
    Dev->CmdToSend.BytesRW= NumberRead;
    Dev->CmdToSend.CmdType = Dev->CmdType;
    Dev->CmdToSend.UseDMA = TRUE;       /* Most times we'll want this */

    if ((Cmd->UseLBA) && ((Dev->CmdType & ATA_EXT_BIT) == 0))
    {
        /* Calculate the sector count, instead of just copying. For
         * "special" cases (eg identify device) sector count passed in
         * is potentially undefined; for the other cases, this is just
         * reversing the calculation done in GetSizeRequired.
         */
        Dev->CmdToSend.SecCount = (SizeRequired / SECTOR_BSIZE);

        /* Need to chop and set the bit */
        Dummy32= Cmd->LBA;
        Dev->CmdToSend.SecNum=  Dummy32 & 0x000000FF;
        Dummy32= Dummy32 >> 8;
        Dev->CmdToSend.CylLow=  Dummy32 & 0x000000FF;
        Dummy32= Dummy32 >> 8;
        Dev->CmdToSend.CylHigh=  Dummy32 & 0x000000FF;
        Dummy32= Dummy32 >> 8;
        Dummy8=  Dummy32 & 0x0000000F;
        Dev->CmdToSend.DevHead= Dummy8 | LBA_SET_BIT;
    }
    else if ((Cmd->UseLBA) && ((Dev->CmdType & ATA_EXT_BIT) != 0))
    {
        /* Split the sector count */
        Dev->CmdToSend.SecCount  = (Cmd->SectorCount & 0xff00) >> 8;
        Dev->CmdToSend.SecCount2 = (Cmd->SectorCount & 0x00ff);

        /* Now split the LBA, higher bits first into the registers. */
        Dev->CmdToSend.CylHigh  = (Cmd->LBAExtended & 0xff00) >> 8;
        Dev->CmdToSend.CylLow   = (Cmd->LBAExtended & 0x00ff);
        Dev->CmdToSend.SecNum   = (Cmd->LBA  & 0xff000000) >> 24;

        Dev->CmdToSend.CylHigh2 = (Cmd->LBA  & 0x00ff0000) >> 16;
        Dev->CmdToSend.CylLow2  = (Cmd->LBA  & 0x0000ff00) >>  8;
        Dev->CmdToSend.SecNum2  = (Cmd->LBA  & 0x000000ff);

        /* Bits 5,7 obsolete; 3:0 reserved; LBA bit must be on, DEV bit must
         * be set appropriately.
         */
        Dev->CmdToSend.DevHead = LBA_SET_BIT;
        if (DevNo == DEVICE_0)
            Dev->CmdToSend.DevHead |= DEVHEAD_DEV0;
        else
            Dev->CmdToSend.DevHead |= DEVHEAD_DEV1;
    }
    else
    {
        /* Calculate the sector count, instead of just copying. For
         * "special" cases (eg identify device) sector count passed in
         * is potentially undefined; for the other cases, this is just
         * reversing the calculation done in GetSizeRequired.
         */
        Dev->CmdToSend.SecCount = (SizeRequired / SECTOR_BSIZE) % 256;

        Dev->CmdToSend.CylLow= Cmd->CylinderLow;
        Dev->CmdToSend.CylHigh= Cmd->CylinderHigh;
        Dev->CmdToSend.DevHead= Cmd->Head & 0x0F;
        Dev->CmdToSend.SecNum= Cmd->Sector;
    }

    /* Add device select bit */
    if(DevNo== DEVICE_1)
    {
      Dev->CmdToSend.DevHead |= DEVICE_SELECT_1_MASK ;
    }

    ATAHandle->DeviceSelected= DevNo;
    STOS_SemaphoreSignal(ATAHandle->BATSemaphore_p);

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : STATAPI CmdOut()

Description  : Performs a command that implies a data transfer  to the disk

Parameters   : STATAPI_Handle_t     Handle        Handle to ATA/ATAPI device on
                                                  host bus.
               STATAPI_PIOMode_t    Mode          PIO mode to invoke.


Return Value :
  ST_NO_ERROR               No error.
  ST_ERROR_INVALID_HANDLE   Device handle is invalid.
  ST_ERROR_BAD_PARAMETER
  ST_ERROR_DEVICE_BUSY      The command can not be executed as the BSY bit
                            is set in the status register, indicating the device is
                            busy.
  STATAPI_ERROR_CMD_ABORT   The ABRT bit is set in the error register indicating
                            that the command failed to complete e.g., a bad
                            parameter or unsupported command.
  STATAPI_ERROR_CMD_ERROR   A command-specific error has occured and the
                            user should interrogate the error.
  STATAPI_ERROR_USER_ABORT  The command or operation has been explicitly
                            aborted by the user.
  STATAPI_ERROR_MODE_NOT_SET   Mode not set yet
 ****************************************************************************/
ST_ErrorCode_t STATAPI_CmdOut(STATAPI_Handle_t Handle,const STATAPI_Cmd_t *Cmd,
                              const U8 *Data, U32 BufferSize, U32 *NumberRead,
                              STATAPI_CmdStatus_t *CmdStatus)
{
    ST_ErrorCode_t RetCode;
    ata_ControlBlock_t *ATAHandle = NULL;    
    ata_DevCtrlBlock_t  *Dev;
    U32 Dummy32= 0;
    U32 SizeRequired = 0;
    U8  Dummy8=0;
    U8  DevNo;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;    
    
    /* Acquire the bus */
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    RetCode = ST_NO_ERROR;
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Check Parameters  */
    if ((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if ((Cmd == NULL) || (CmdStatus == NULL) || (NumberRead == NULL))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }
    else if (Dev->Opened == FALSE)
    {
        RetCode =  ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->IsModeSet == FALSE)
    {
        RetCode = STATAPI_ERROR_MODE_NOT_SET;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    /* Parameter checking */
    SizeRequired = GetSizeRequired(Cmd);
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    if ((BufferSize < SizeRequired) || (Data == NULL))
    {
        ata_bus_Release(ATAHandle);
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

    /* Specific command checking */
    if ((Cmd->CmdCode == STATAPI_CMD_WRITE_MULTIPLE) ||
        (Cmd->CmdCode == STATAPI_CMD_WRITE_MULTIPLE_EXT) ||
        (Cmd->CmdCode == STATAPI_CMD_CFA_WRITE_MULTIPLE_WO_ERASE))
    {
        Dev->CmdToSend.MultiCnt = Dev->MultiCnt;
    }
    else
    {
        Dev->CmdToSend.MultiCnt = 1;
    }

    /*------------------------All OK Let's do it-----------------------------*/
    if(Dev == &ATAHandle->Handles[0])
    {
        DevNo = DEVICE_0;
    }
    else
    {
        DevNo = DEVICE_1;
    }

    Dev->Notify= TRUE;
    Dev->Abort= FALSE;

    Dev->CmdType = ChooseProtocolCmd(Cmd->CmdCode, Dev);
    if (Dev->CmdType == 0)
    {
        /* Either host or device does not support the transport implied
         * by the command (which realistically will be DMA)
         */
        ata_bus_Release(ATAHandle);
        return STATAPI_ERROR_PROTOCOL_NOT_SUPPORTED;
    }

    if ((Dev->CmdType == ATA_CMD_DMAOUT)    ||
        (Dev->CmdType == ATA_CMD_DMAOUT_EXT))
    {
        /* Check buffer alignment */
        if ( (((U32)Data / DMA_ALIGNMENT) * DMA_ALIGNMENT) != (U32)Data )
        {
            ata_bus_Release(ATAHandle);
            return ST_ERROR_BAD_PARAMETER;
        }

        /* Has a 256 (0 == 256) limit. Caller should know this. */
        Dev->CmdToSend.MultiCnt = Cmd->SectorCount & 0xff;
    }
#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
    else
    {
        /* currently also requires 4-byte alignment on PIO
         * transfers, due to HAL implementation and LMI requirements.
         */
        if ( (((U32)Data / 4) * 4) != (U32)Data )
        {
            ata_bus_Release(ATAHandle);
            return ST_ERROR_BAD_PARAMETER;
        }
     }
#endif

    /*------- Set the Cmd structure -----*/
    Dev->CmdToSend.Stat.CmdStat= CmdStatus;

    Dev->CmdToSend.Feature= Cmd->Features;
    Dev->CmdToSend.CommandCode= CmdCodes[Cmd->CmdCode];
    Dev->CmdToSend.DataBuffer= (U8*) Data;
    Dev->CmdToSend.BytesRW= NumberRead;
    Dev->CmdToSend.CmdType = Dev->CmdType;
    Dev->CmdToSend.UseDMA = TRUE;     /* DMA data to port (if appropriate) */

    if ((Cmd->UseLBA) && ((Dev->CmdType & ATA_EXT_BIT) == 0))
    {
        /* Calculate the sector count, instead of just copying. For
         * "special" cases (eg identify device) sector count passed in
         * is potentially undefined; for the other cases, this is just
         * reversing the calculation done in GetSizeRequired.
         */
        Dev->CmdToSend.SecCount = (SizeRequired / SECTOR_BSIZE);

        /* Need to chop and set the bit */
        Dummy32= Cmd->LBA;
        Dev->CmdToSend.SecNum=  Dummy32 & 0x000000FF;
        Dummy32= Dummy32 >> 8;
        Dev->CmdToSend.CylLow=  Dummy32 & 0x000000FF;
        Dummy32= Dummy32 >> 8;
        Dev->CmdToSend.CylHigh=  Dummy32 & 0x000000FF;
        Dummy32= Dummy32 >> 8;
        Dummy8=  Dummy32 & 0x0000000F;
        Dev->CmdToSend.DevHead= Dummy8 | LBA_SET_BIT;
    }
    else if ((Cmd->UseLBA) && ((Dev->CmdType & ATA_EXT_BIT) != 0))
    {
        /* Split the sector count */
        Dev->CmdToSend.SecCount  = (Cmd->SectorCount & 0xff00) >> 8;
        Dev->CmdToSend.SecCount2 = (Cmd->SectorCount & 0x00ff);

        /* Now split the LBA, higher bits first into the registers. */
        Dev->CmdToSend.CylHigh  = (Cmd->LBAExtended & 0xff00) >> 8;
        Dev->CmdToSend.CylLow   = (Cmd->LBAExtended & 0x00ff);
        Dev->CmdToSend.SecNum   = (Cmd->LBA  & 0xff000000) >> 24;

        Dev->CmdToSend.CylHigh2 = (Cmd->LBA  & 0x00ff0000) >> 16;
        Dev->CmdToSend.CylLow2  = (Cmd->LBA  & 0x0000ff00) >>  8;
        Dev->CmdToSend.SecNum2  = (Cmd->LBA  & 0x000000ff);

        /* Bits 5,7 obsolete; 3:0 reserved; LBA bit must be on, DEV bit must
         * be set appropriately.
         */
        Dev->CmdToSend.DevHead = LBA_SET_BIT;
        if (DevNo == DEVICE_0)
            Dev->CmdToSend.DevHead |= DEVHEAD_DEV0;
        else
            Dev->CmdToSend.DevHead |= DEVHEAD_DEV1;
    }
    else
    {
        /* Calculate the sector count, instead of just copying. For
         * "special" cases (eg identify device) sector count passed in
         * is potentially undefined; for the other cases, this is just
         * reversing the calculation done in GetSizeRequired.
         */
        Dev->CmdToSend.SecCount = (SizeRequired / SECTOR_BSIZE) % 256;

        Dev->CmdToSend.CylLow= Cmd->CylinderLow;
        Dev->CmdToSend.CylHigh= Cmd->CylinderHigh;
        Dev->CmdToSend.DevHead= Cmd->Head & 0x0F;
        Dev->CmdToSend.DevHead |= (DevNo == DEVICE_0)?DEVHEAD_DEV0:DEVHEAD_DEV1;
        Dev->CmdToSend.SecNum= Cmd->Sector;
    }

    /* Add device select bit */
    if(DevNo== DEVICE_1)
    {
      Dev->CmdToSend.DevHead |= DEVICE_SELECT_1_MASK ;
    }

    ATAHandle->DeviceSelected= DevNo;
    STOS_SemaphoreSignal(ATAHandle->BATSemaphore_p);

    return ST_NO_ERROR;
}

/************************************************************************
Name: BusAccessTask

Description: This is the entry point to the Task that manages the access
             to the bus.

Parameters:
Return:

************************************************************************/
void BusAccessTask(ata_ControlBlock_t *ATAHandle)
{
    /* Index of the Event id array returned from the registration.
     * Default to command complete.
     */
    U8  Evt = STATAPI_CMD_COMPLETE_ID;
    STATAPI_EvtParams_t   EvtData;
    ata_DevCtrlBlock_t *DevHndl;
    /* Default to true; if we don't match a command in the switch
     * statement this should raise an error.
     */
    BOOL    rVal = TRUE;

    while (ATAHandle->Terminate==FALSE)
    {
         STOS_SemaphoreWait(ATAHandle->BATSemaphore_p);

     if(ATAHandle->Terminate) break;

     DevHndl= &ATAHandle->Handles[ATAHandle->DeviceSelected];
     EvtData.Error = ST_NO_ERROR;

     if(DevHndl->Abort==FALSE)
     {

     /* Try to select the device */
     if(ata_ctrl_SelectDevice(ATAHandle,ATAHandle->DeviceSelected))
     {
         STTBX_Print(("BAT: DeviceSelect failed\n"));
        EvtData.Error=ST_ERROR_DEVICE_BUSY;

        if(DevHndl->CmdType > ATA_PKT_NODATA)
        {
          Evt= STATAPI_PKT_COMPLETE_ID;
        }else
        {
          Evt= STATAPI_CMD_COMPLETE_ID;
        }

     }  else /* We have selected the device */
      {
        EvtData.DeviceAddress = (STATAPI_DeviceAddr_t)ATAHandle->DeviceSelected;
        switch(DevHndl->CmdType)
        {
            case ATA_CMD_NODATA_EXT:
            case ATA_CMD_NODATA:
            {
              rVal= ata_cmd_NoData(ATAHandle,&DevHndl->CmdToSend);

              EvtData.EvtParams.CmdCompleteParams.CmdStatus =
                                            DevHndl->CmdToSend.Stat.CmdStat;
              Evt= STATAPI_CMD_COMPLETE_ID;
              if(DevHndl->CmdToSend.CommandCode==SET_MULTIPLE_MODE)
              {
                if ((rVal == FALSE) && (DevHndl->CmdToSend.SecCount > 0))
                {
                 DevHndl->MultiCnt=DevHndl->CmdToSend.SecCount;
                }
              }
              break;
            }
            case ATA_EXEC_DIAG:
            {
              rVal= ata_cmd_ExecuteDiagnostics(ATAHandle,&DevHndl->CmdToSend);
              Evt= STATAPI_CMD_COMPLETE_ID;
              EvtData.EvtParams.CmdCompleteParams.CmdStatus =
                                                DevHndl->CmdToSend.Stat.CmdStat;
              break;
            }
            case ATA_CMD_PIOIN_EXT:
            case ATA_CMD_PIOIN:
            {
              rVal= ata_cmd_PioIn(ATAHandle, &DevHndl->CmdToSend);
              EvtData.EvtParams.CmdCompleteParams.CmdStatus =
                                                DevHndl->CmdToSend.Stat.CmdStat;
              Evt= STATAPI_CMD_COMPLETE_ID;
              break;
            }
            case ATA_CMD_PIOOUT_EXT:
            case ATA_CMD_PIOOUT:
            {
              rVal= ata_cmd_PioOut(ATAHandle,&DevHndl->CmdToSend);
              EvtData.EvtParams.CmdCompleteParams.CmdStatus =
                                                DevHndl->CmdToSend.Stat.CmdStat;
              Evt= STATAPI_CMD_COMPLETE_ID;
              break;
            }
            case ATA_CMD_DMAIN_EXT:
            case ATA_CMD_DMAIN:
            {
                rVal= ata_cmd_DmaIn(ATAHandle, &DevHndl->CmdToSend);
                EvtData.EvtParams.CmdCompleteParams.CmdStatus =
                                                DevHndl->CmdToSend.Stat.CmdStat;
                Evt= STATAPI_CMD_COMPLETE_ID;
                if (ATAHandle->LastExtendedErrorCode == EXTERROR_CRCERROR)
                    EvtData.Error = STATAPI_ERROR_CRC_ERROR;
                break;
            }
            case ATA_CMD_DMAOUT_EXT:
            case ATA_CMD_DMAOUT:
            {
                rVal= ata_cmd_DmaOut(ATAHandle, &DevHndl->CmdToSend);
                EvtData.EvtParams.CmdCompleteParams.CmdStatus =
                                                DevHndl->CmdToSend.Stat.CmdStat;
                Evt= STATAPI_CMD_COMPLETE_ID;
                if (ATAHandle->LastExtendedErrorCode == EXTERROR_CRCERROR)
                    EvtData.Error = STATAPI_ERROR_CRC_ERROR;
                break;
            }
            case ATA_PKT_NODATA:
            {

              rVal= ata_packet_NoData(ATAHandle,&DevHndl->CmdToSend);
              EvtData.EvtParams.PktCompleteParams.PktStatus =
                                                DevHndl->CmdToSend.Stat.PktStat;
              Evt= STATAPI_PKT_COMPLETE_ID;
              break;
            }
            case ATA_PKT_PIOIN:
            {
              rVal= ata_packet_PioIn(ATAHandle,&DevHndl->CmdToSend);
              EvtData.EvtParams.PktCompleteParams.PktStatus =
                                                DevHndl->CmdToSend.Stat.PktStat;
              Evt= STATAPI_PKT_COMPLETE_ID;
              break;
            }
            case ATA_PKT_PIOOUT:
            {

              rVal= ata_packet_PioOut(ATAHandle,&DevHndl->CmdToSend);
              EvtData.EvtParams.PktCompleteParams.PktStatus =
                                                DevHndl->CmdToSend.Stat.PktStat;
              Evt= STATAPI_PKT_COMPLETE_ID;
              break;
            }
            case ATA_PKT_DMAIN:
            {
                rVal= ata_packet_DmaIn(ATAHandle,&DevHndl->CmdToSend);
                EvtData.EvtParams.PktCompleteParams.PktStatus =
                                                DevHndl->CmdToSend.Stat.PktStat;
                Evt= STATAPI_PKT_COMPLETE_ID;
                break;
            }
            case ATA_PKT_DMAOUT:
            {
                rVal= ata_packet_DmaOut(ATAHandle,&DevHndl->CmdToSend);
                EvtData.EvtParams.PktCompleteParams.PktStatus =
                                                DevHndl->CmdToSend.Stat.PktStat;
                Evt= STATAPI_PKT_COMPLETE_ID;
                break;
            }

            default:
            {
            }
        } /* switch end */

        /* Depending on return value chose EvtData */

       if (rVal==TRUE)
       {
       	  if (EvtData.Error == ST_NO_ERROR)
               EvtData.Error = STATAPI_ERROR_CMD_ERROR;
       }
       else
       {
          EvtData.Error= ST_NO_ERROR;
       }

       if (DevHndl->CmdToSend.Stat.CmdStat->Error & ERROR_REG_ABORT_MASK)
       {
          EvtData.Error=STATAPI_ERROR_CMD_ABORT;
       }
 	   if(ATAHandle->DeviceType != STATAPI_EMI_PIO4)  
 	   {
	       if (ATAHandle->HalHndl->DmaAborted == TRUE)
	       {
	           EvtData.Error = STATAPI_ERROR_CMD_ABORT;
	       }
       }

     } /* else end (Select device) */

     }
     else /* end of 1st Abort if*/
     {
        EvtData.Error=STATAPI_ERROR_USER_ABORT;
        if(DevHndl->CmdType >= ATA_PKT_NODATA)
        {
          Evt= STATAPI_PKT_COMPLETE_ID;
        }
        else
        {
          Evt= STATAPI_CMD_COMPLETE_ID;
        }
     }

      /* 2nd User abort check */
      if(DevHndl->Abort==TRUE)
      {
        EvtData.Error=STATAPI_ERROR_USER_ABORT;
        if(DevHndl->CmdType > ATA_PKT_NODATA)
        {
          Evt= STATAPI_PKT_COMPLETE_ID;
        }else
        {
          Evt= STATAPI_CMD_COMPLETE_ID;
        }
      }

      /* Finally release the bus */
      ata_bus_Release(ATAHandle);

      /* Depending on the user: notify or signal*/
        if(DevHndl->Notify)
        {
            if(DevHndl->Abort==TRUE)
            {
                 DevHndl->Abort=FALSE;
                 STOS_SemaphoreSignal(DevHndl->EndCmdSem_p);
            }
            STEVT_Notify(ATAHandle->EvtHndl,ATAHandle->EvtIds[Evt],&EvtData);
        }else
        {
            DevHndl->Abort=FALSE;
            STOS_SemaphoreSignal(DevHndl->EndCmdSem_p);
        }



    }  /* end of normal loop */

    /* Terminating the driver*/
    /* Tidy up stuff */
}

 /****************************************************************************
Name         : STATAPI PacketNoData()

Description  : Issues a packet command with no data transfer associated

Parameters   : STATAPI_Handle_t     Handle        Handle to ATA/ATAPI device on
                                                  host bus.
               STATAPI_Packet_t       Pkt         Pointer to the packet command
                                                  information
               STATAPI_PktStatus_t  PktStat       Pointer that will be updated
                                                  when the command is completed

Return Value :
 ST_NO_ERROR               No error.

 ST_ERROR_INVALID_HANDLE   Device handle is invalid.

 ST_ERROR_DEVICE_BUSY      The command can not be executed as the BSY bit
                           is set in the status register, indicating the device is
                           busy.

 STATAPI_ERROR_PKT_CHECK   Sense key information is available in the packet
                            status structure.

 STATAPI_ERROR_PKT_NOT_SUPPORTED The ATA device does not support the packet fea-ture
                                    set.

 STATAPI_ERROR_USER_ABORT     The command or operation has been explicitly
                              aborted by the user.

 STATAPI_ERROR_MODE_NOT_SET   Mode not set yet

 ****************************************************************************/
ST_ErrorCode_t STATAPI_PacketNoData(STATAPI_Handle_t Handle,
                                 const STATAPI_Packet_t *Pkt,
                                 STATAPI_PktStatus_t *PktStatus)
{
    ST_ErrorCode_t RetCode;
    ata_ControlBlock_t *ATAHandle = NULL;    
    ata_DevCtrlBlock_t  *Dev;
    U8  DevNo;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;    
        
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    RetCode = ST_NO_ERROR;
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Check Parameters  */
    if ((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if ((Pkt == NULL) || (PktStatus == NULL))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }
    else if(Dev->Opened==FALSE)
    {
        RetCode =  ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->IsModeSet==FALSE)
    {
        RetCode = STATAPI_ERROR_MODE_NOT_SET;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    if(Dev== &ATAHandle->Handles[0])
    {
        DevNo= DEVICE_0;
    }else
    {
        DevNo= DEVICE_1;
    }

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    if(ATAHandle->DevInBus[DevNo]!= ATAPI_DEVICE)
    {
        ata_bus_Release(ATAHandle);
        return STATAPI_ERROR_PKT_NOT_SUPPORTED;
    }
#endif

    Dev->Notify= TRUE;
    Dev->Abort= FALSE;

    /* No need to choose protocol here */
    Dev->CmdType= ATA_PKT_NODATA;

    /* Set the Cmd structure */
    Dev->CmdToSend.Stat.PktStat= PktStatus;

    Dev->CmdToSend.Feature=0x00;   /* This registers are not used */
    Dev->CmdToSend.SecCount= 0x00;
    Dev->CmdToSend.SecNum= 0x00;

    Dev->CmdToSend.CommandCode= CmdCodes[STATAPI_CMD_PACKET];
    Dev->CmdToSend.CylLow= BYTE_COUNT_LOW;
    Dev->CmdToSend.CylHigh= BYTE_COUNT_HIGH;
    Dev->CmdToSend.DevHead= 0x00;

    /* Add device select bit */
    if(DevNo== DEVICE_1)
    {
      Dev->CmdToSend.DevHead |= DEVICE_SELECT_1_MASK ;
    }

    /* Set the packet */
    /* Set the correct Op code */

    Dev->CmdToSend.Pkt[0] = PktCodes[Pkt->OpCode] ;
    memcpy(&Dev->CmdToSend.Pkt[1],Pkt->Data,15);

    ATAHandle->DeviceSelected= DevNo;
    STOS_SemaphoreSignal(ATAHandle->BATSemaphore_p);


    return ST_NO_ERROR;
}

/****************************************************************************
Name         : STATAPI PacketIn()

Description  : Issues a packet command with an input data transfer associated

Parameters   :
    STATAPI_Handle_t     Handle        Handle to ATA/ATAPI device on
                                       host bus.
    STATAPI_Packet_t       Pkt            Pointer to the command  information

    STATAPI_PktStatus_t  PktStat       Pointer that will be updated when the
                                       command is completed
    U8 *                 Data          Pointer to the buffer allocated to
                                       store the data

    U32                 BufferSize     Size of the buffer

    U32 *               NumberRead     Pointer to a variable to be updated
                                       with the number of bytes actually read

Return Value :
 ST_NO_ERROR               No error.

 ST_ERROR_INVALID_HANDLE   Device handle is invalid.

 ST_ERROR_DEVICE_BUSY      The command can not be executed as the BSY bit
                           is set in the status register, indicating the device is
                           busy.

 STATAPI_ERROR_PKT_CHECK   Sense key information is available in the packet
                            status structure.

 STATAPI_ERROR_PKT_NOT_SUPPORTED The ATA device does not support the packet fea-ture
                                    set.

 STATAPI_ERROR_USER_ABORT     The command or operation has been explicitly
                              aborted by the user.

 STATAPI_ERROR_MODE_NOT_SET   Mode not set yet

 ****************************************************************************/

ST_ErrorCode_t STATAPI_PacketIn(STATAPI_Handle_t Handle,
                                 const STATAPI_Packet_t *Pkt,U8 *Data,
                                 U32 NumberToRead,U32 *NumberRead,
                                 STATAPI_PktStatus_t *PktStatus)
{
    ST_ErrorCode_t RetCode;
    ata_ControlBlock_t *ATAHandle = NULL;    
    ata_DevCtrlBlock_t  *Dev;
    U8  DevNo;
    U32 Block_Size=0;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;    

    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    RetCode = ST_NO_ERROR;
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Check Parameters  */
    if((Dev != &ATAHandle->Handles[0]) && (Dev != &ATAHandle->Handles[1]))
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }
    else if ((Pkt == NULL) || (PktStatus == NULL) || (NumberRead == NULL))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }
    else if(Dev->Opened==FALSE)
    {
        RetCode =  ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->IsModeSet==FALSE)
    {
        RetCode = STATAPI_ERROR_MODE_NOT_SET;
    }
    else if (Data== NULL)
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    if(Dev== &ATAHandle->Handles[0])
    {
        DevNo= DEVICE_0;
    }else
    {
        DevNo= DEVICE_1;
    }

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    if(ATAHandle->DevInBus[DevNo]!= ATAPI_DEVICE)
    {
        ata_bus_Release(ATAHandle);
        return STATAPI_ERROR_PKT_NOT_SUPPORTED;
    }
#endif

    Dev->Notify= TRUE;
    Dev->Abort= FALSE;

    if(PktCodes[Pkt->OpCode]==READ_CD_OPCODE)
        Block_Size=CD_SECTOR_BSIZE;
    else
        Block_Size=DVD_SECTOR_BSIZE;


    /* If we can do DMA, then use it */

    if (Dev->DeviceParams.SupportedDmaModes != 0)
        Dev->CmdType = ATA_PKT_DMAIN;
    else
        Dev->CmdType = ATA_PKT_PIOIN;

    if ( ( (U32)Data & ( (U32)DMA_ALIGNMENT - 1 ) ) \
			|| ( NumberToRead & ( (U32)DMA_ALIGNMENT - 1 ) ) \
			|| ( NumberToRead <= 4*Block_Size )	)
	{
		Dev->CmdType = ATA_PKT_PIOIN;
	}

    /* Set the Cmd structure */
    Dev->CmdToSend.Stat.PktStat= PktStatus;
    Dev->CmdToSend.BufferSize= NumberToRead;
    Dev->CmdToSend.BytesRW= NumberRead;
    Dev->CmdToSend.DataBuffer= (U8*)Data;

    if ( Dev->CmdType == ATA_PKT_DMAIN )
	{
		Dev->CmdToSend.Feature=0x01;
	}
	else
	{
		Dev->CmdToSend.Feature=0x00;
	}

    Dev->CmdToSend.SecCount= 0x00;
    Dev->CmdToSend.SecNum= 0x00;

    Dev->CmdToSend.CommandCode= CmdCodes[STATAPI_CMD_PACKET];
    Dev->CmdToSend.CylLow= BYTE_COUNT_LOW;
    Dev->CmdToSend.CylHigh= BYTE_COUNT_HIGH;
    Dev->CmdToSend.DevHead= 0x00;

    /* Add device select bit */
    if(DevNo== DEVICE_1)
    {
      Dev->CmdToSend.DevHead |= DEVICE_SELECT_1_MASK ;
    }

    /* Set the packet */
    /* Set the correct Op code */

    Dev->CmdToSend.Pkt[0] = PktCodes[Pkt->OpCode] ;
    memcpy(&Dev->CmdToSend.Pkt[1],Pkt->Data,15);


    ATAHandle->DeviceSelected= DevNo;
    STOS_SemaphoreSignal(ATAHandle->BATSemaphore_p);


    return ST_NO_ERROR;
}

 /****************************************************************************
Name         : STATAPI PacketOut()

Description  : Issues a packet command with an output data transfer associated

Parameters   :
    STATAPI_Handle_t     Handle        Handle to ATA/ATAPI device on
                                       host bus.
    STATAPI_Packet_t       Pkt            Pointer to the command  information

    STATAPI_PktStatus_t  PktStat       Pointer that will be updated when the
                                       command is completed
    U8 *                 Data          Pointer to the buffer allocated to
                                       store the data

    U32                 NumberToWrite   Number of bytes to be writen

    U32 *               NumberRead     Pointer to a variable to be updated
                                       with the number of bytes actually read

Return Value :
 ST_NO_ERROR               No error.

 ST_ERROR_INVALID_HANDLE   Device handle is invalid.

 ST_ERROR_DEVICE_BUSY      The command can not be executed as the BSY bit
                           is set in the status register, indicating the device is
                           busy.

 STATAPI_ERROR_PKT_CHECK   Sense key information is available in the packet
                            status structure.

 STATAPI_ERROR_PKT_NOT_SUPPORTED The ATA device does not support the packet fea-ture
                                    set.

 STATAPI_ERROR_USER_ABORT     The command or operation has been explicitly
                              aborted by the user.

 STATAPI_ERROR_MODE_NOT_SET   Mode not set yet

 ****************************************************************************/

ST_ErrorCode_t STATAPI_PacketOut(STATAPI_Handle_t Handle,
                                 const STATAPI_Packet_t *Pkt,const U8 *Data,
                                 U32 NumberToWrite,U32 *NumberWrite,
                                 STATAPI_PktStatus_t *PktStatus)
{
    ST_ErrorCode_t RetCode;
    ata_ControlBlock_t *ATAHandle = NULL;    
    ata_DevCtrlBlock_t  *Dev;
    U8  DevNo;

    Dev = (ata_DevCtrlBlock_t *)Handle;
    /* Obtain the control block from the passed in handle */
    ATAHandle = ATAPI_GetControlBlockFromHandle(Handle);
    
    if (NULL == ATAHandle)
        return ST_ERROR_INVALID_HANDLE;    
        
    if (ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    RetCode = ST_NO_ERROR;
#if !defined(STATAPI_NO_PARAMETER_CHECK)
    /*  Check Parameters  */
    if((Dev!= &ATAHandle->Handles[0]) && (Dev!= &ATAHandle->Handles[1]))
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    else if ((Pkt == NULL) || (PktStatus == NULL) || (NumberWrite == NULL))
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }
    else if(Dev->Opened==FALSE)
    {
        RetCode =  ST_ERROR_INVALID_HANDLE;
    }
    else if (Dev->IsModeSet==FALSE)
    {
        RetCode = STATAPI_ERROR_MODE_NOT_SET;
    }
    else if (Data== NULL)
    {
        RetCode = ST_ERROR_BAD_PARAMETER;
    }

    if (RetCode != ST_NO_ERROR)
    {
        ata_bus_Release(ATAHandle);
        return RetCode;
    }
#endif

    if(Dev== &ATAHandle->Handles[0])
    {
        DevNo= DEVICE_0;
    }else
    {
        DevNo= DEVICE_1;
    }

#if !defined(STATAPI_NO_PARAMETER_CHECK)
    if(ATAHandle->DevInBus[DevNo]!= ATAPI_DEVICE)
    {
        ata_bus_Release(ATAHandle);
        return STATAPI_ERROR_PKT_NOT_SUPPORTED;
    }
#endif

    Dev->Notify= TRUE;
    Dev->Abort= FALSE;

    /* If we can do DMA, then use it */
    if (Dev->DeviceParams.SupportedDmaModes != 0)
        Dev->CmdType = ATA_PKT_DMAOUT;
    else
        Dev->CmdType = ATA_PKT_PIOOUT;

    if ( ( (U32)Data & ( (U32)DMA_ALIGNMENT - 1 ) ) || ( NumberToWrite & ( (U32)DMA_ALIGNMENT - 1 ) ) )
		{
			Dev->CmdType = ATA_PKT_PIOOUT;
		}

    /* Set the Cmd structure */
    Dev->CmdToSend.Stat.PktStat= PktStatus;
    Dev->CmdToSend.BufferSize= NumberToWrite;
    Dev->CmdToSend.BytesRW= NumberWrite;
    Dev->CmdToSend.DataBuffer= (U8*)Data;

    if ( Dev->CmdType == ATA_PKT_DMAOUT )
	{
		Dev->CmdToSend.Feature=0x01;
	}
	else
	{
		Dev->CmdToSend.Feature=0x00;
	}

    Dev->CmdToSend.SecCount= 0x00;
    Dev->CmdToSend.SecNum= 0x00;

    Dev->CmdToSend.CommandCode= CmdCodes[STATAPI_CMD_PACKET];
    Dev->CmdToSend.CylLow= BYTE_COUNT_LOW;
    Dev->CmdToSend.CylHigh= BYTE_COUNT_HIGH;
    Dev->CmdToSend.DevHead= 0x00;

    /* Add device select bit */
    if(DevNo== DEVICE_1)
    {
        Dev->CmdToSend.DevHead |= DEVICE_SELECT_1_MASK ;
    }

    /* Set the packet; set the correct Op code */
    Dev->CmdToSend.Pkt[0] = PktCodes[Pkt->OpCode] ;
    memcpy(&Dev->CmdToSend.Pkt[1],Pkt->Data,15);

    ATAHandle->DeviceSelected= DevNo;
    STOS_SemaphoreSignal(ATAHandle->BATSemaphore_p);

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : STATAPI_GetRevision

Description  : Gets the current revision of the driver.

Parameters   :

Return Value : ST_Revision_t  The revision of the driver
 ****************************************************************************/
ST_Revision_t STATAPI_GetRevision(void){
    return((ST_Revision_t)STATAPI_REVISION);
}

/****************************************************************************
Name         : STATAPI_ResetInterface

Description  : If available on this chip, reset the hard disk interface

Parameters   :
    DeviceName  The name of the instance to reset

Return Value :
    ST_NO_ERROR                     No problems
    ST_ERROR_FEATURE_NOT_SUPPORTED  Interface reset not available on
                                    this chip
 ****************************************************************************/
ST_ErrorCode_t STATAPI_ResetInterface(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

#if !defined(ST_5514) && !defined(ST_5528) && !defined(ST_8010)
    /* Chips without HDDI on... */
    error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    hal_ResetInterface();
#endif

    return error;
}

/****************************************************************************
Name         : STATAPI_SetRegisters

Description  : Change the register map used by the HAL

Parameters   :
    RegisterMap_p       Pointer to a register map to use

Return Value :
    ST_NO_ERROR                     No problems
    ST_ERROR_BAD_PARAMETER          Invalid pointer
 ****************************************************************************/
ST_ErrorCode_t STATAPI_SetRegisters(const STATAPI_RegisterMap_t *RegisterMap_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    /* We can't do any checking, other than basic valid pointer. */
    if (RegisterMap_p == NULL)
        return ST_ERROR_BAD_PARAMETER;

    hal_SetRegisterMap(RegisterMap_p);

    return error;
}

/****************************************************************************
Name         : STATAPI_GetRegisters

Description  : Returns the register map used by the HAL

Parameters   :
    RegisterMap_p       Pointer to where to store it

Return Value :
    ST_NO_ERROR                     No problems
    ST_ERROR_BAD_PARAMETER          Invalid pointer
 ****************************************************************************/
ST_ErrorCode_t STATAPI_GetRegisters(STATAPI_RegisterMap_t *RegisterMap_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    if (RegisterMap_p == NULL)
        return ST_ERROR_BAD_PARAMETER;

    hal_GetRegisterMap(RegisterMap_p);
    return error;
}

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
/****************************************************************************
Name         : CalculateTiming

Description  : Adjusts the default timing values for the current clock speed
               (expressed as multiplier/divider)

Parameters   :
    Mult        amount to multiple value by
    Div         amount to divide value by

Return Value :  none
 ****************************************************************************/

static U32 Adapt(U32 Value, U32 Mult, U32 Div)
{
    U32 result;

    Value *= Mult;
    result = Value / Div;

    /* Take care of partial values */
    if ((Value % Div) != 0)
        result++;

    /* And adjust for register write */
    if (result > 0)
        result--;

    return result;
}

static void CalculateTiming(U32 Mult, U32 Div)
{
    U32 Mode = 0;

    /* They're changed in-place, so make sure we only do this once */
    if (TRUE == TimingRescaled)
        return;
    else
        TimingRescaled = TRUE;

/*
    U32 InitSequenceDelay;
    U32 IoRdySetupDelay;
    U32 WaitTime;
    U32 WriteDelay;
    U32 ReadDelay;
    U32 AddressHoldDelay;
    U32 WriteEnableOutDelay;
    U32 WriteRecoverDelay;
    U32 ReadRecoverDelay;
*/
    /* PIO timing */
    for (Mode = 0; Mode < 5; Mode++)
    {
        PioTimingDefaults[Mode].InitSequenceDelay =
                Adapt(PioTimingDefaults[Mode].InitSequenceDelay, Mult, Div);
        PioTimingDefaults[Mode].IoRdySetupDelay =
                Adapt(PioTimingDefaults[Mode].IoRdySetupDelay, Mult, Div);
        PioTimingDefaults[Mode].WaitTime =
                Adapt(PioTimingDefaults[Mode].WaitTime, Mult, Div);
        PioTimingDefaults[Mode].WriteDelay =
                Adapt(PioTimingDefaults[Mode].WriteDelay, Mult, Div);
        PioTimingDefaults[Mode].ReadDelay =
                Adapt(PioTimingDefaults[Mode].ReadDelay, Mult, Div);
        PioTimingDefaults[Mode].AddressHoldDelay =
                Adapt(PioTimingDefaults[Mode].AddressHoldDelay, Mult, Div);
        PioTimingDefaults[Mode].WriteEnableOutDelay =
                Adapt(PioTimingDefaults[Mode].WriteEnableOutDelay, Mult, Div);
        PioTimingDefaults[Mode].WriteRecoverDelay =
                Adapt(PioTimingDefaults[Mode].WriteRecoverDelay, Mult, Div);
        PioTimingDefaults[Mode].ReadRecoverDelay =
                Adapt(PioTimingDefaults[Mode].ReadRecoverDelay, Mult, Div);
    }

/*
    U32 NotDIoRwAssertedT;
    U32 WriteDataHoldT;
    U32 DIoRwToDMAckHoldT;
    U32 NotDIoRNegatedT;
    U32 NotDIoWNegatedT;
    U32 CsToNotDIoRwT;
    U32 CsHoldT;
*/
    /* MWDMA timing */
    for (Mode = 0; Mode < 3; Mode++)
    {
        MwDmaTimingDefaults[Mode].NotDIoRwAssertedT =
                Adapt(MwDmaTimingDefaults[Mode].NotDIoRwAssertedT, Mult, Div);
        MwDmaTimingDefaults[Mode].WriteDataHoldT =
                Adapt(MwDmaTimingDefaults[Mode].WriteDataHoldT, Mult, Div);
        MwDmaTimingDefaults[Mode].DIoRwToDMAckHoldT =
                Adapt(MwDmaTimingDefaults[Mode].DIoRwToDMAckHoldT, Mult, Div);
        MwDmaTimingDefaults[Mode].NotDIoRNegatedT =
                Adapt(MwDmaTimingDefaults[Mode].NotDIoRNegatedT, Mult, Div);
        MwDmaTimingDefaults[Mode].NotDIoWNegatedT =
                Adapt(MwDmaTimingDefaults[Mode].NotDIoWNegatedT, Mult, Div);
        MwDmaTimingDefaults[Mode].CsToNotDIoRwT =
                Adapt(MwDmaTimingDefaults[Mode].CsToNotDIoRwT, Mult, Div);
        MwDmaTimingDefaults[Mode].CsHoldT =
                Adapt(MwDmaTimingDefaults[Mode].CsHoldT, Mult, Div);
    }

/*
    U32 AckT;
    U32 InitEnvelopeT;
    U32 MinAssertStopNegateT;
    U32 MinInterlockT;
    U32 LimitedInterlockT;

    U32 DataOutSetupT;
    U32 DataOutHoldT;

    U32 HostStrobeToStopSetupT;
    U32 ReadyToFinalStrobeT;
*/
    /* UDMA timing */
    for (Mode = 0; Mode < 6; Mode++)
    {
        UltraDmaTimingDefaults[Mode].AckT =
                Adapt(UltraDmaTimingDefaults[Mode].AckT, Mult, Div);
        UltraDmaTimingDefaults[Mode].InitEnvelopeT =
                Adapt(UltraDmaTimingDefaults[Mode].InitEnvelopeT, Mult, Div);
        UltraDmaTimingDefaults[Mode].MinAssertStopNegateT =
                Adapt(UltraDmaTimingDefaults[Mode].MinAssertStopNegateT, Mult, Div);
        UltraDmaTimingDefaults[Mode].MinInterlockT =
                Adapt(UltraDmaTimingDefaults[Mode].MinInterlockT, Mult, Div);
#if defined(ST_5514)
        UltraDmaTimingDefaults[Mode].LimitedInterlockT =
                Adapt(UltraDmaTimingDefaults[Mode].LimitedInterlockT, Mult, Div);
#elif defined(ST_5528) || defined(ST_8010)
        UltraDmaTimingDefaults[Mode].CRCSetupT =
                Adapt(UltraDmaTimingDefaults[Mode].CRCSetupT, Mult, Div);
#endif
        UltraDmaTimingDefaults[Mode].DataOutSetupT =
                Adapt(UltraDmaTimingDefaults[Mode].DataOutSetupT, Mult, Div);
        UltraDmaTimingDefaults[Mode].DataOutHoldT =
                Adapt(UltraDmaTimingDefaults[Mode].DataOutHoldT, Mult, Div);
        UltraDmaTimingDefaults[Mode].HostStrobeToStopSetupT =
                Adapt(UltraDmaTimingDefaults[Mode].HostStrobeToStopSetupT, Mult, Div);
        UltraDmaTimingDefaults[Mode].ReadyToFinalStrobeT =
                Adapt(UltraDmaTimingDefaults[Mode].ReadyToFinalStrobeT, Mult, Div);
    }
}
#endif

/****************************************************************************
Name         : GetDeviceCapabilities()

Description  : Issues a Identify command and parses the 512 bytes received to
                know the modes that the device supports and calls
                hal_GetCapabilities. Then set the field  DeviceParams of the
                ata_DevCtrlBlock_t structure with the common capabilites
                supported by the device and the host

Parameters   : STATAPI_DeviceAddr_t DevAddress   device address


Return Value : ST_NO_ERROR              Successful
               ST_ERROR_DEVICE_BUSY     Unable to acquire the bus
               ST_ERROR_UNKNOWN_DEVICE  Unable to extract info from the
                                       Identification block received when issuing
                                       a IDENTIFY DEVICE command.
 ****************************************************************************/
static ST_ErrorCode_t GetDeviceCapabilities(ata_ControlBlock_t *ATAHandle,
											STATAPI_DeviceAddr_t DevAddress)
{
    ata_DevCtrlBlock_t      *DevHndl;
    STATAPI_Capability_t    HostParams;
    STATAPI_Params_t        DeviceParams;
    STATAPI_CmdStatus_t     Stat;
    U32                     BytesRead=0;
#if defined (ATAPI_FDMA)    
    U8                     *DataBuffer2;
#endif    

    DevHndl= &ATAHandle->Handles[DevAddress];

    /* Issuing  EXECUTE DIAGNOSTICS CMD */
    if(ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }
    ATAHandle->DeviceSelected= DevAddress;
    DevHndl->Notify= FALSE;
    DevHndl->Abort= FALSE;
    DevHndl->CmdType= ATA_EXEC_DIAG;

    DevHndl->CmdToSend.Stat.CmdStat=&Stat;
    DevHndl->CmdToSend.DevHead= DevAddress?DEVHEAD_DEV1:DEVHEAD_DEV0;
    DevHndl->CmdToSend.CommandCode= CmdCodes[STATAPI_CMD_EXECUTE_DEVICE_DIAGNOSTIC];

    STOS_SemaphoreSignal(ATAHandle->BATSemaphore_p);
    STOS_SemaphoreWait(DevHndl->EndCmdSem_p);

    if(Stat.Error != 0x01)
    {
    	 return STATAPI_ERROR_DIAGNOSTICS_FAIL ;
    }

    /* Issuing IDENTIFY DEVICE CMD OR IDENTIFY PACKET DEVICE */
    if(ata_bus_Acquire(ATAHandle))
    {
        return ST_ERROR_DEVICE_BUSY;
    }

    ATAHandle->DeviceSelected= DevAddress;
    DevHndl->Notify= FALSE;
    DevHndl->Abort= FALSE;
    DevHndl->CmdType= ATA_CMD_PIOIN;


    /* Set CmdToSend ... only need the DEVHEAD register and data-in buffer */
    DevHndl->CmdToSend.BytesRW=&BytesRead;
    DevHndl->CmdToSend.Stat.CmdStat=&Stat;
    DevHndl->CmdToSend.MultiCnt= 1;
    DevHndl->CmdToSend.SecCount= 1;
    DevHndl->CmdToSend.DevHead= DevAddress?DEVHEAD_DEV1:DEVHEAD_DEV0;

#if defined (ATAPI_FDMA) || defined(STATAPI_CRYPT) 
    DevHndl->CmdToSend.UseDMA = TRUE;
#else
    DevHndl->CmdToSend.UseDMA = FALSE;
#endif
    if(ATAHandle->DevInBus[DevAddress]==ATAPI_DEVICE)
    {
      DevHndl->CmdToSend.CommandCode= CmdCodes[STATAPI_CMD_IDENTIFY_PACKET_DEVICE];
    }else
    {
      DevHndl->CmdToSend.CommandCode= CmdCodes[STATAPI_CMD_IDENTIFY_DEVICE];
    }
#if defined (ATAPI_FDMA)
     /*Bug GNBvd52838 */
     DataBuffer2=(U8*)(STOS_MemoryAllocate(
                                      ATAHandle->NCachePartition,
                                      SECTOR_BSIZE + 31));

    if(DataBuffer2 == NULL)
    {
#if defined (ATAPI_DEBUG)
        STTBX_Print(("Failed to allocate Memory in GetDeviceCapabilities"));
#endif
        ata_bus_Release(ATAHandle);
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check For alignment 31 */
        DevHndl->CmdToSend.DataBuffer = (U8*)(((U32)(DataBuffer2 + 31)) & ~31);
    }

#else
    DevHndl->CmdToSend.DataBuffer=(U8*)STOS_MemoryAllocate(
                                                ATAHandle->DriverPartition,
                                                SECTOR_BSIZE);
#endif

    STOS_SemaphoreSignal(ATAHandle->BATSemaphore_p);
    STOS_SemaphoreWait(DevHndl->EndCmdSem_p);

    /* Reset this, since not really expecting it to be used by other calls */
    DevHndl->CmdToSend.UseDMA = TRUE;

    /* Now we parse the info block */
    if(BytesRead!=SECTOR_BSIZE)
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Default to no DMA */
    DevHndl->DmaEnabled = FALSE;
    if(ParseIdentifyBlock(DevAddress, DevHndl->CmdToSend.DataBuffer,
                          &DeviceParams, ATAHandle))
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }
#if defined(ATAPI_FDMA)
    STOS_MemoryDeallocate(ATAHandle->NCachePartition,
                      DataBuffer2);
#else/*!ATAPI_FDMA*/
    STOS_MemoryDeallocate(ATAHandle->DriverPartition,
                      DevHndl->CmdToSend.DataBuffer);
#endif/*ATAPI_FDMA*/
    if (hal_GetCapabilities( ATAHandle->DeviceName,ATAHandle->HalHndl, &HostParams) == TRUE)
    {
        /* If the getcapabilities failed (unlikely!), stop the user
         * making any transfers.
         */
        HostParams.SupportedPioModes = 0;
        HostParams.SupportedDmaModes = 0;
    }
    /* We are only interested in the common capabilities */
    DevHndl->DeviceParams.SupportedPioModes= (HostParams.SupportedPioModes &
                                              DeviceParams.SupportedPioModes );
    DevHndl->DeviceParams.SupportedDmaModes= (HostParams.SupportedDmaModes &
                                              DeviceParams.SupportedDmaModes );

    DevHndl->DeviceParams.CurrentDmaMode= DeviceParams.CurrentDmaMode;
    DevHndl->DeviceParams.CurrentPioMode= DeviceParams.CurrentPioMode;

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : ParseIdentifyBlock()

Description  : Parses the 512 bytes block received after issuing an Identify
               device command, and extracts the supported and current PIO and DMA
               modes.

Parameters   : U8*                  Data         The 512 bytes data block
               STATAPI_Params_t     *Params      Place to store the parsed data

Return Value :

 ****************************************************************************/
static BOOL ParseIdentifyBlock(STATAPI_DeviceAddr_t DevNo,
                               U8 *Data,
                               STATAPI_Params_t *Params,
                               ata_ControlBlock_t *ATAHandle)
{
	ata_DevCtrlBlock_t *DevHndl = NULL;
    U16 W;
    U32 Dummy = 0;
    BOOL Error = TRUE;
    
    DevHndl= &ATAHandle->Handles[DevNo];
    
    /* If it's an ATAPI device,
       lets find out the size of the packet (12 or 16)*/
    memcpy(&W,&Data[0],sizeof(U16));
    if (W & 0x8000)
    {
        ATAHandle->Handles[DevNo].PktSize=6; /* 12 bytes */
        if (W & 0x0003)
        {
            ATAHandle->Handles[DevNo].PktSize=8; /* 16 bytes */
        }
    }

    /* Now lets see the suported modes:
       First test the word no 53 */
    memcpy(&W, &Data[106], sizeof(U16));
    if (W & (BIT_1 | BIT_2))
    {
        /* PIO Modes */
        memcpy(&W, &Data[128], sizeof(U16));

        if (W & BIT_0)
        {
            Dummy= 1<< STATAPI_PIO_MODE_3 |
                   1<< STATAPI_PIO_MODE_2 |
                   1<< STATAPI_PIO_MODE_1 |
                   1<< STATAPI_PIO_MODE_0;
            Params->CurrentPioMode= STATAPI_PIO_MODE_3;
        }
        Params->SupportedPioModes = Dummy;

        if (W & BIT_1)
        {
            Dummy= 1<<STATAPI_PIO_MODE_4;
            Params->CurrentPioMode= STATAPI_PIO_MODE_4;
        }
        Params->SupportedPioModes |= Dummy;

        /*DMA  Modes*/
        memcpy(&W, &Data[126], sizeof(U16));
        if (W & BIT_2)
        {
            Dummy= 1<<STATAPI_DMA_MWDMA_MODE_2;
        }
        Params->SupportedDmaModes = Dummy;
        if (W & BIT_1)
        {
            Dummy= 1<<STATAPI_DMA_MWDMA_MODE_1;
        }
        Params->SupportedDmaModes |= Dummy;
        if (W & BIT_0)
        {
            Dummy= 1<<STATAPI_DMA_MWDMA_MODE_0;
        }
        Params->SupportedDmaModes |= Dummy;
        if (W & (BIT_2 | BIT_1 | BIT_0))
            DevHndl->DmaEnabled = TRUE;

        memcpy(&W,&Data[176],sizeof(U16));
        if (W & BIT_5)
        {
            Dummy= 1 << STATAPI_DMA_UDMA_MODE_5;
        }
        Params->SupportedDmaModes |= Dummy;
        if (W & BIT_4)
        {
            Dummy= 1<<STATAPI_DMA_UDMA_MODE_4;
        }
        Params->SupportedDmaModes |= Dummy;
        if (W & BIT_3)
        {
            Dummy= 1<<STATAPI_DMA_UDMA_MODE_3;
        }
        Params->SupportedPioModes |= Dummy;
        if (W & BIT_2)
        {
            Dummy= 1<<STATAPI_DMA_UDMA_MODE_2;
        }
        Params->SupportedDmaModes |= Dummy;
        if (W & BIT_1)
        {
            Dummy= 1<<STATAPI_DMA_UDMA_MODE_1;
        }
        Params->SupportedDmaModes |= Dummy;
        if (W & BIT_0)
        {
            Dummy= 1<<STATAPI_DMA_UDMA_MODE_0;
        }
        Params->SupportedDmaModes |= Dummy;
        if (W & (BIT_4 | BIT_3 | BIT_2 | BIT_1 | BIT_0))
            DevHndl->DmaEnabled = TRUE;

        /*--------- Now lets see the currently selected DMA modes -------*/
        memcpy(&W,&Data[126],sizeof(U16));
        if (W & BIT_10)
        {
            Params->CurrentDmaMode= STATAPI_DMA_MWDMA_MODE_2;
        }

        if (W & BIT_9)
        {
            Params->CurrentDmaMode= STATAPI_DMA_MWDMA_MODE_1;
        }

        if (W & BIT_8)
        {
            Params->CurrentDmaMode= STATAPI_DMA_MWDMA_MODE_0;
        }

        W= Data[176];

        if (W & BIT_13)
        {
            Params->CurrentDmaMode= STATAPI_DMA_UDMA_MODE_5;
        }

        if (W & BIT_12)
        {
            Params->CurrentDmaMode= STATAPI_DMA_UDMA_MODE_4;
        }

        if (W & BIT_11)
        {
            Params->CurrentDmaMode= STATAPI_DMA_UDMA_MODE_3;
        }

        if (W & BIT_10)
        {
            Params->CurrentDmaMode= STATAPI_DMA_UDMA_MODE_2;
        }

        if (W & BIT_9)
        {
            Params->CurrentDmaMode= STATAPI_DMA_UDMA_MODE_1;
        }

        if (W & BIT_8)
        {
           Params->CurrentDmaMode= STATAPI_DMA_UDMA_MODE_0;
        }

        /* <essay>Now we know what the drive's current DMA and PIO modes
         * are, if it has them, make sure we match. Ideally, the drive
         * should still be in the modes we think it is - however, some
         * disks after a hard reset have been known to change (e.g.
         * UDMA4 to MWDMA2).  So this is to guard against that. It's
         * less than ideal, but the user should be explicitly setting
         * the mode they want after a hard reset anyway.</essay>
         */
        Error = FALSE;
 		if(ATAHandle->DeviceType != STATAPI_EMI_PIO4)  
 		{
	        if (!IsOutsideRange(Params->CurrentDmaMode,
	                           STATAPI_DMA_MWDMA_MODE_0, STATAPI_DMA_UDMA_MODE_5))
	        {
	            if (ATAPI_SetDmaMode(ATAHandle,DevHndl, Params->CurrentDmaMode) == TRUE)
	                Error = TRUE;
	        }
	    }
        if (!IsOutsideRange(Params->CurrentPioMode,
                            STATAPI_PIO_MODE_0, STATAPI_PIO_MODE_4))
        {
            if (ATAPI_SetPioMode(ATAHandle,DevHndl, Params->CurrentPioMode) == TRUE)
                Error = TRUE;
        }
    }

    return Error;
}

/****************************************************************************
Name         : GetSizeRequired()

Description  : Looks at the command, and returns the buffer-size
               required to support it.

Parameters   : The command structure.

Return Value : Buffer-size required (0...U32)
 ****************************************************************************/
static U32 GetSizeRequired(const STATAPI_Cmd_t *Cmd)
{
    U32 i;

    if (Cmd->CmdCode == STATAPI_CMD_SMART_IN)
    {
        /* If doing a SMART READ DATA, it returns a fixed size (512
         * byte) block. SMART READ LOG, however, does not. So we have to
         * have a special case.
         *
         * If not doing a SMART READ DATA, this will fall through to the
         * sector calculation at the end of the function.
         */
        if (Cmd->Features == 0xd0)
            return 512;
    }
    else
    {
        /* Check for predefined length */
        for (i = 0; i < NUM_CMD_FIXED_LENGTH; i++)
        {
            if (Cmd->CmdCode == CmdFixedLength[i])
                return CmdFixedLengthValues[i];
        }
    }

    /* If we got here, we didn't find it listed in the exceptions table
     * above, so fall back on calculating.
     */
    return(SECTOR_BSIZE * ((Cmd->SectorCount == 0)?256:Cmd->SectorCount));
}

/****************************************************************************
Name         : RegisterEvents()

Description  : Makes a call to STEVT_Open() and the registers the
               STATAPI events.

Parameters   : ST_DeviceName_t EvtHandlerName    Event handler name
               STEVT_Handle_t  EvtHndl           Event Handle

Return Value : ST_NO_ERROR      Successful Open() and Registers()
               Error            Error code from Open() or Resgister()
 ****************************************************************************/

static ST_ErrorCode_t RegisterATAEvents(const ST_DeviceName_t DeviceName,
										const ST_DeviceName_t EvtHandlerName,
                                        STEVT_Handle_t *EvtHndl)
{
    STEVT_OpenParams_t STEVT_OpenParams;
    ST_ErrorCode_t     Error;
    ata_ControlBlock_t *ATAHandle;
    
    /* Obtain the control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);
    
    if (NULL == ATAHandle)
        return ST_ERROR_UNKNOWN_DEVICE;
    if (strcmp(DeviceName, ATAHandle->DeviceName))
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }        

    /* Avoids lint warnings */
    STEVT_OpenParams.dummy = 0;
    Error = STEVT_Open(EvtHandlerName, &STEVT_OpenParams, EvtHndl);
    if( Error != ST_NO_ERROR )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Open() failed"));
        return( Error );
    }

    Error = STEVT_RegisterDeviceEvent(*EvtHndl, ATAHandle->DeviceName,
                           STATAPI_HARD_RESET_EVT,
                           &ATAHandle->EvtIds[STATAPI_HARD_RESET_ID]);
    if( Error != ST_NO_ERROR )
    {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_RegisterDeviceEvent(STATAPI_HARD_RESET_EVT) failed"));
        return( Error );
    }

    Error = STEVT_RegisterDeviceEvent(*EvtHndl, ATAHandle->DeviceName,
                           STATAPI_SOFT_RESET_EVT,
                           &ATAHandle->EvtIds[STATAPI_SOFT_RESET_ID]);
    if( Error != ST_NO_ERROR )
    {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_RegisterDeviceEvent(STATAPI_SOFT_RESET_EVT) failed"));
        return( Error );
    }

    Error = STEVT_RegisterDeviceEvent(*EvtHndl, ATAHandle->DeviceName,
                           STATAPI_CMD_COMPLETE_EVT,
                           &ATAHandle->EvtIds[STATAPI_CMD_COMPLETE_ID]);
    if( Error != ST_NO_ERROR )
    {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_RegisterDeviceEvent(STATAPI_CMD_COMPLETE_EVT) failed"));
        return( Error );
    }
    Error = STEVT_RegisterDeviceEvent(*EvtHndl, ATAHandle->DeviceName,
                           STATAPI_PKT_COMPLETE_EVT,
                           &ATAHandle->EvtIds[STATAPI_PKT_COMPLETE_ID]);
    if( Error != ST_NO_ERROR )
    {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_RegisterDeviceEvent(STATAPI_PKT_COMPLETE_EVT) failed"));
        return( Error );
    }

    return( Error );
}

/****************************************************************************
Name         : UnregisterATAEvents()

Description  : Unregisters the STATAPI events and then Closes() the event handler.

Parameters   : None

Return Value : ST_NO_ERROR      Successful Unregisters() and Close()
               Error            Error code from Close() or Unregister()
 ****************************************************************************/

static ST_ErrorCode_t UnregisterATAEvents( const ST_DeviceName_t DeviceName,
										   STEVT_Handle_t EvtHndl )
{
    ST_ErrorCode_t     Error;
    ata_ControlBlock_t *ATAHandle;

    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);
    
    if (NULL == ATAHandle)
        return ST_ERROR_UNKNOWN_DEVICE;
        
    if (strcmp(DeviceName, ATAHandle->DeviceName))
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    
    Error = STEVT_UnregisterDeviceEvent( EvtHndl, ATAHandle->DeviceName,
                             STATAPI_HARD_RESET_EVT );
    if( Error != ST_NO_ERROR )
    {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_Unregister(STATAPI_HARD_RESET_EVT) failed"));
        return( Error );
    }

    Error = STEVT_UnregisterDeviceEvent( EvtHndl, ATAHandle->DeviceName,
                             STATAPI_SOFT_RESET_EVT );
    if( Error != ST_NO_ERROR )
    {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_Unregister(STATAPI_SOFT_RESET_EVT) failed"));
        return( Error );
    }

    Error = STEVT_UnregisterDeviceEvent( EvtHndl, ATAHandle->DeviceName,
                             STATAPI_CMD_COMPLETE_EVT );
    if( Error != ST_NO_ERROR )
    {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_Unregister(STATAPI_CMD_COMPLETE_EVT) failed"));
        return( Error );
    }

    Error = STEVT_UnregisterDeviceEvent( EvtHndl, ATAHandle->DeviceName,
                             STATAPI_PKT_COMPLETE_EVT );
    if( Error != ST_NO_ERROR )
    {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_Unregister(STATAPI_PKT_COMPLETE_EVT) failed"));
        return( Error );
    }

    Error = STEVT_Close(EvtHndl);
    if( Error != ST_NO_ERROR )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Close() failed"));
        return( Error );
    }

    return( Error );
}

/****************************************************************************
Name         : IsOutsideRange()

Description  : Checks if U32 formal parameter 1 is outside the range of
               bounds (U32 formal parameters 2 and 3).

Parameters   : Value for checking, followed by the two bounds, preferably
               the lower bound followed by the upper bound.

Return Value : TRUE  if parameter is outside given range
               FALSE if parameter is within  given range
 ****************************************************************************/

static BOOL IsOutsideRange( U32 ValuePassed,
                            U32 LowerBound,
                            U32 UpperBound )
{
    U32 Temp;

    /* ensure bounds are the correct way around */
    if( UpperBound < LowerBound ) /* bounds require swapping */
    {
        Temp       = LowerBound;
        LowerBound = UpperBound;
        UpperBound = Temp;
    }

    if( ( ValuePassed < LowerBound ) ||
        ( ValuePassed > UpperBound ) )
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
}
/*****************************************************************************
ATAPI_GetControlBlockFromName()
Description:
    Runs through the queue of initialized ATAPI devices and checks for
    the ATAPI with the specified device name.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    DeviceName, text string indicating the device name to check.
Return Value:
    NULL, end of queue encountered - invalid device name.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    ATAPI_IsInit()
*****************************************************************************/
ata_ControlBlock_t *ATAPI_GetControlBlockFromName(const ST_DeviceName_t DeviceName)
{
    ata_ControlBlock_t *qp = ATAPIQueueHead_p; /* Global queue head pointer */

    while(qp != NULL)
    {
        /* Check the device name for a match with the item in the queue */
        if(strcmp(qp->DeviceName,DeviceName) != 0)
        {
            /* Next ATAPI in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The ATA entry has been found */
            break;
        }
    }
    /* Return the ATA control block (or NULL) to the caller */
    return qp;
} /* ATAPI_GetControlBlockFromName() */

/*****************************************************************************
ATAPI_GetControlBlockFromHandle()
Description:
    Runs through the queue of initialized ATAPI devices and checks for
    the ATAPI with the specified handle.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    Handle, an open handle.
Return Value:
    NULL, end of queue encountered - invalid handle.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    ATAPI_GetControlBlockFromName()
*****************************************************************************/
ata_ControlBlock_t *ATAPI_GetControlBlockFromHandle(STATAPI_Handle_t Handle)
{
    ata_ControlBlock_t *qp = ATAPIQueueHead_p; /* Global queue head pointer */

    /* Check handle */
    if((void *)Handle != NULL)
    {
        while(qp != NULL)
    	{
       		 /* Check the device name for a match with the item in the queue */
	        if((Handle!= &qp->Handles[0]) && (Handle!= &qp->Handles[1]))
	        {
	            /* Next ATA in the queue */
	            qp = qp->Next_p;
	        }
	        else
	        {
	            /* The ATA entry has been found */
	            break;
	        }
   		}
   	}
   	
    /* Return the ATA control block (or NULL) to the caller */
    return qp;

} /* ATAPI_GetControlBlockFromHandle() */

/*****************************************************************************
ATAPI_IsInit()
Description:
    Runs through the queue of initialized devices and checks that
    the ATAPI with the specified device name has not already been added.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    DeviceName, text string indicating the device name to check.
Return Value:
    TRUE, the device has already been initialized.
    FALSE, the device is not in the queue and therefore is not initialized.
See Also:
    STATAPI_Init()
*****************************************************************************/
static BOOL ATAPI_IsInit(const ST_DeviceName_t DeviceName)
{
    BOOL Initialized = FALSE;
    
    ata_ControlBlock_t *qp = ATAPIQueueHead_p; /* Global queue head pointer */

    while(qp != NULL)
    {
        /* Check the device name for a match with the item in the queue */
        if(strcmp(qp->DeviceName, DeviceName) != 0)
        {
            /* Next ATAPI in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The ATAPI is already initialized */
            Initialized = TRUE;
            break;
        }
    }
    /* Boolean return value reflecting whether the device is initialized */
    return Initialized;
} /* ATAPI_IsInit() */

/*****************************************************************************
ATAPI_QueueAdd()
Description:
    This routine appends an allocated ATAPI control block to the
    ATAPI queue.

    NOTE: The interrupt lock must be held when calling this routine.

Parameters:
    QueueItem_p, the control block to append to the queue.
Return Value:
    Nothing.
See Also:
    ATAPI_QueueRemove()
*****************************************************************************/

static void ATAPI_QueueAdd(ata_ControlBlock_t *QueueItem_p)
{
    ata_ControlBlock_t *qp;

    /* Check the base element is defined, otherwise, append to the end of the
     * linked list.
     */
    if(ATAPIQueueHead_p == NULL)
    {
        /* As this is the first item, no need to append */
        ATAPIQueueHead_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
    else
    {
        /* Iterate along the list until we reach the final item */
        qp = ATAPIQueueHead_p;
        while(qp != NULL && qp->Next_p)
        {
            qp = qp->Next_p;
        }

        /* Append the new item */
        qp->Next_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }

} /* ATAPI_QueueAdd() */

/*****************************************************************************
ATAPI_QueueRemove()
Description:
    This routine removes an allocated ATAPI control block from the
    ATAPI queue.

    NOTE: The interrupt lock must be held when calling this routine.
    Use ATAPI_IsInit() or ATAPI_GetControlBlockFromName() to verify whether
    or not a particular item is in the queue prior to calling this routine.

Parameters:
    QueueItem_p, the control block to remove from the queue.
Return Value:
    Nothing.
See Also:
    ATAPI_QueueAdd()
*****************************************************************************/
static void ATAPI_QueueRemove(ata_ControlBlock_t *QueueItem_p)
{
    ata_ControlBlock_t *this_p, *last_p;

    /* Check the base element is defined, otherwise quit as no items are
     * present on the queue.
     */
    if(ATAPIQueueHead_p != NULL)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = ATAPIQueueHead_p;

        /* Iterate over each queue element until we find the required
         * queue item to remove.
         */
        while(this_p != NULL && this_p != QueueItem_p)
        {
            last_p = this_p;
            this_p = this_p->Next_p;
        }

        /* Check we found the queue item */
        if(this_p == QueueItem_p)
        {
            /* Unlink the item from the queue */
            if(last_p != NULL)
            {
                last_p->Next_p = this_p->Next_p;
            }
            else
            {
                /* Recalculate the new head of the queue i.e.,
                 * we have just removed the queue head.
                 */
                ATAPIQueueHead_p = this_p->Next_p;
            }
        }
    }
} /* ATAPI_QueueRemove() */
/* End of module */
