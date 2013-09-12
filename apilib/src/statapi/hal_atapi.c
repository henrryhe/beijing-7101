/************************************************************************

Source file name : hal_atapi.c

Description: HAL implementation of the ATA/ATAPI driver for the
             ATA Host Interface

COPYRIGHT (C) STMicroelectronics  2000

************************************************************************/

/*Includes-------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "stlite.h"
#include "stos.h"
#include "stsys.h"
#include "statapi.h"
#include "hal_atapi.h"
#include "ata.h"
#include "stdevice.h"
#if defined(ATAPI_FDMA)
#include "stfdma.h"
#elif defined(STATAPI_CRYPT)
#include "stcrypt.h"
#endif

#include "sttbx.h"

#if defined(ST_OS20)
#include "debug.h"
#else
#include "os21debug.h"
#endif

/*Private Constants----------------------------------------------------*/

#define  COUNT_WAIT_LIMIT       1000

#define  MAX_TRACE              100

/*---------------HW specific ---------------------------*/
#define     ATA_HRD_RST_ASSERT      0x0
#define     ATA_HRD_RST_DEASSERT    0x1

/*
** Note:
** The code has not been ported to OS21 & ST40 for BMDMA_ENABLE
** as the option will not be used for OS21 & ST40 builds.
*/
#ifdef BMDMA_ENABLE
/* BMDMA details */

#ifndef     BLOCK_MOVE_INT_LEVEL
#define     BLOCK_MOVE_INT_LEVEL    6
#endif
#ifndef     BLOCK_MOVE_INTERRUPT
#define     BLOCK_MOVE_INTERRUPT    15
#endif

#define     BMDMA_SrcAddress        0x20026000
#define     BMDMA_DestAddress       0x20026004
#define     BMDMA_Count             0x20026008
#define     BMDMA_IntEn             0x2002600C
#define     BMDMA_Status            0x20026010
#define     BMDMA_IntAck            0x20026014

#endif

/* Exported variables ------------------------------------------------ */

#if defined(ATAPI_DEBUG)
BOOL    ATAPI_Verbose = FALSE;
#elif defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
/* Cheating, but the DMA code uses this in so many places... */
static BOOL ATAPI_Verbose = FALSE;
#endif

#if defined(ATAPI_DEBUG)
U32     atapi_intcount = 0;
U32     atapi_inttrace[MAX_TRACE];
#endif

#if defined(ST_5514)
U32     WriteRecoverGood = 0;
U32     WriteRecoverBad = 0;
#endif
/* Private variables ------------------------------------------------- */

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
static   U32    HDDI_ITS_Value = 0;
#endif

static   STATAPI_PioTiming_t  CurrentPioTiming;
static   STATAPI_DmaTiming_t  CurrentDmaTiming;

#if defined(ATAPI_FDMA)
static   STFDMA_ChannelId_t   ATAPIi_FDMA_ChannelId;

#elif defined(STATAPI_CRYPT)
static   STEVT_Handle_t       CRYPTEvtHandle;
static   semaphore_t         *CRYPTTransferCompleteSem;
static   BOOL                 CRYPTStartedDMA;
static void hal_CryptCallback(STEVT_CallReason_t Reason,
							  STEVT_EventConstant_t Event,
							  const void *EventData);
#endif

/*DMA_PRESENT*/
#if defined(ST_5514) || defined(ST_5528) ||  defined(ST_7100) || defined(ST_7109) || defined(ST_8010)
static const STATAPI_Capability_t CurrentCapabilities =
                            {
                              STATAPI_HDD_UDMA4,
                              1 << STATAPI_PIO_MODE_4,

#if defined(ST_5528) || defined(ST_8010)
                              /* Can only do mode 5 on 28+ */
                              (1 << STATAPI_DMA_UDMA_MODE_5) |
#endif
                              (1 << STATAPI_DMA_UDMA_MODE_4) |
                              (1 << STATAPI_DMA_UDMA_MODE_2) |
                              (1 << STATAPI_DMA_UDMA_MODE_0) |
                              (1 << STATAPI_DMA_MWDMA_MODE_0) |
                              (1 << STATAPI_DMA_MWDMA_MODE_2)
                            };
#else
static const STATAPI_Capability_t CurrentCapabilities =
                            {
                              STATAPI_EMI_PIO4,
                              1 << STATAPI_PIO_MODE_4,
                              0
                            };
#endif

/* Left non-static because if BMDMA is defined cmmd_ata.c needs to
 * access them.
 */
 
/*PATA Regmasks*/

#if defined (ST_5512)
U32 RegsMasks[STATAPI_NUM_REG_MASKS]={ (aCS1 | nCS0 | aDA2 | aDA1 | nDA0),
                                       (nCS1 | aCS0 | nDA2 | nDA1 | nDA0),
                                       (nCS1 | aCS0 | nDA2 | nDA1 | aDA0),
                                       (nCS1 | aCS0 | nDA2 | nDA1 | aDA0),
                                       (nCS1 | aCS0 | nDA2 | aDA1 | nDA0),
                                       (nCS1 | aCS0 | nDA2 | aDA1 | aDA0),
                                       (nCS1 | aCS0 | aDA2 | nDA1 | nDA0),
                                       (nCS1 | aCS0 | aDA2 | nDA1 | aDA0),
                                       (nCS1 | aCS0 | aDA2 | aDA1 | nDA0),
                                       (nCS1 | aCS0 | aDA2 | aDA1 | aDA0),
                                       (nCS1 | aCS0 | aDA2 | aDA1 | aDA0),
                                       (aCS1 | nCS0 | aDA2 | aDA1 | nDA0)
                                      };

#elif defined (ST_5508) || defined (ST_5518)
U32 RegsMasks[STATAPI_NUM_REG_MASKS]={(0x1c0000),
                                      (0x200000),
                                      (0x220000),
                                      (0x220000),
                                      (0x240000),
                                      (0x260000),
                                      (0x280000),
                                      (0x2a0000),
                                      (0x2c0000),
                                      (0x2e0000),
                                      (0x2e0000),
                                      (0x1c0000)
                                     };
#elif defined(mb361)
U32 RegsMasks[STATAPI_NUM_REG_MASKS]={(0x150000), /* ASR */
                                      (0x080000), /* Data */
                                      (0x0a0000), /* ERR */
                                      (0x0a0000), /* FEATURE */
                                      (0x090000), /* SC */
                                      (0x0b0000), /* SN */
                                      (0x0c0000), /* CL */
                                      (0x0e0000), /* CH */
                                      (0x0d0000), /* DH */
                                      (0x0f0000), /* STA */
                                      (0x0f0000), /* COMMAND */
                                      (0x150000)  /* CTL */
                                     };

#elif defined(mb382)
U32 RegsMasks[STATAPI_NUM_REG_MASKS]={(0x0e0000), /* ASR */
                                      (0x100000), /* Data */
                                      (0x110000), /* ERR */
                                      (0x110000), /* FEATURE */
                                      (0x120000), /* SC */
                                      (0x130000), /* SN */
                                      (0x140000), /* CL */
                                      (0x150000), /* CH */
                                      (0x160000), /* DH */
                                      (0x170000), /* STA */
                                      (0x170000), /* COMMAND */
                                      (0x0e0000)  /* CTL */
                                     };
#elif defined(mb390)
U32 RegsMasks[STATAPI_NUM_REG_MASKS]={(0x1c0000), /* ASR */
                                      (0x200000), /* Data */
                                      (0x220000), /* ERR */
                                      (0x220000), /* FEATURE */
                                      (0x240000), /* SC */
                                      (0x260000), /* SN */
                                      (0x280000), /* CL */
                                      (0x2a0000), /* CH */
                                      (0x2c0000), /* DH */
                                      (0x2e0000), /* STA */
                                      (0x2e0000), /* COMMAND */
                                      (0x1c0000)  /* CTL */
                                     };

#elif defined(mb391)
U32 RegsMasks[STATAPI_NUM_REG_MASKS]={(0x8c0000), /* ASR */
                                      (0x000000), /* Data */
                                      (0x020000), /* ERR */
                                      (0x020000), /* FEATURE */
                                      (0x040000), /* SC */
                                      (0x060000), /* SN */
                                      (0x080000), /* CL */
                                      (0x0a0000), /* CH */
                                      (0x0c0000), /* DH */
                                      (0x0e0000), /* STA */
                                      (0x0e0000), /* COMMAND */
                                      (0x8c0000)  /* CTL */
                                     };

#elif defined(mb395)
U32 RegsMasks[STATAPI_NUM_REG_MASKS]={(0x2c0000), /* ASR */
                                      (0x100000), /* Data */
                                      (0x120000), /* ERR */
                                      (0x120000), /* FEATURE */
                                      (0x140000), /* SC */
                                      (0x160000), /* SN */
                                      (0x180000), /* CL */
                                      (0x1a0000), /* CH */
                                      (0x1c0000), /* DH */
                                      (0x1e0000), /* STA */
                                      (0x1e0000), /* COMMAND */
                                      (0x2c0000)  /* CTL */
                                     };

#elif defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
/* Not actually masks in this case; offsets from base address for the
 * relevant registers
 */
U32 RegsMasks[STATAPI_NUM_REG_MASKS]={ (0x38),
                                       (0x40),
                                       (0x44),
                                       (0x44),
                                       (0x48),
                                       (0x4c),
                                       (0x50),
                                       (0x54),
                                       (0x58),
                                       (0x5c),
                                       (0x5c),
                                       (0x38)
                                     };
#elif defined(ST_7100) || defined(ST_7109) || defined(ST_5525)
                                         

U32 RegsMasks[STATAPI_NUM_REG_MASKS]={(0x1c0000), /* ASR */
                                      (0x200000), /* Data */
                                      (0x220000), /* ERR */
                                      (0x220000), /* FEATURE */
                                      (0x240000), /* SC */
                                      (0x260000), /* SN */
                                      (0x280000), /* CL */
                                      (0x2a0000), /* CH */
                                      (0x2c0000), /* DH */
                                      (0x2e0000), /* STA */
                                      (0x2e0000), /* COMMAND */
                                      (0x1c0000)  /* CTL */
                                     };
#endif

/*SATA RegMasks*/
#if defined(ST_7100) || defined(ST_7109)

U32 SATARegsMasks[STATAPI_NUM_REG_MASKS]={(0x820), /* ASR */
                                      (0x800), /* Data */
                                      (0x804), /* ERR */
                                      (0x804), /* FEATURE */
                                      (0x808), /* SC */
                                      (0x80c), /* SN */
                                      (0x810), /* CL */
                                      (0x814), /* CH */
                                      (0x818), /* DH */
                                      (0x81c), /* STA */
                                      (0x81c), /* COMMAND */
                                      (0x820)  /* CTL */
                                     };
#endif

/* Need to know what mode we should be in */
static STATAPI_DmaMode_t    CurrentDmaMode;
static STATAPI_PioMode_t    CurrentPioMode;

#if defined(ST_5514) || defined(ST_5528) ||  defined(ST_7100) || defined(ST_7109) || defined(ST_8010)
/* Simplifies cases in DmaDataBlock */
static BOOL                 DMAIsUDMA = FALSE;
#endif

/*Private functions prototypes-----------------------------------------*/

#if (ATAPI_USING_INTERRUPTS)
	STOS_INTERRUPT_DECLARE(ata_InterruptHandler,ATAHandle_p);
#endif

void SetPIOTiming(volatile U32 *Base, STATAPI_PioTiming_t *Timing);

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
static BOOL hal_ChangeIntMask(hal_Handle_t *HalHndl_p,
                              U32 Interrupts_to_set);
static BOOL hal_WaitDMAEngines(U32 Base, BOOL DMAIsUDMA, BOOL Read);
#endif
#if defined(HDDI_5514_CUT_3_1)
static BOOL DoUDMAWriteRecover(hal_Handle_t *HalHndl_p,
                               U32 StartAddress, U32 WordCount);
#endif

#if defined(ST_7100) || defined(ST_7109)
static void hal_SATAAHB2STBus_PC_Init(U32 BaseAddress);
static void hal_SATAAHB2STBus_PC_Reset(U32 BaseAddress);
void hal_SATA_Int_Clear_Mask(U32 BaseAddress);
static void hal_SATA_Int_Clear(U32 BaseAddress);
#endif

#ifdef BMDMA_ENABLE
static INTERRUPT_RETURN_TYPE ata_BlockMoveIntHandler (void *);
#endif

/*Functions------------------------------------------------------------*/
#if defined(STATAPI_CRYPT) && !defined(ATAPI_FDMA)
static void hal_CryptCallback(STEVT_CallReason_t Reason,
							  STEVT_EventConstant_t Event,
							  const void *EventData)
{
    /* Wake up the dma function */
    if(Event == STCRYPT_TRANSFER_COMPLETE_EVT)
    {
        if(CRYPTStartedDMA == TRUE)
        {
            CRYPTStartedDMA = FALSE;
            STOS_SemaphoreSignal(CRYPTTransferCompleteSem);
        }
    }
}
#endif/*STATAPI_CRYPT*/

/************************************************************************
Name: hal_Init

Description:
    This function initializes the structures needed to manage the silicon
    Basically it allocates the hal handle and installs the interrupt handler

Parameters:
************************************************************************/
BOOL hal_Init (const ST_DeviceName_t DeviceName,const STATAPI_InitParams_t *params_p,
														    hal_Handle_t **HalHndl_p)
{
 	ata_ControlBlock_t *ATAHandle;
#if (ATAPI_USING_INTERRUPTS)
    int osResult = 0;
    int inumber;
#endif
 
    /* Allocate handle */
    *HalHndl_p = (hal_Handle_t*) memory_allocate (params_p->DriverPartition,sizeof (hal_Handle_t));
    
    if(*HalHndl_p == NULL)
        return TRUE;

    (*HalHndl_p)->BaseAddress     = params_p->BaseAddress;
    (*HalHndl_p)->HWResetAddress  = params_p->HW_ResetAddress;
    (*HalHndl_p)->InterruptNumber = params_p->InterruptNumber;
    (*HalHndl_p)->InterruptLevel  =  params_p->InterruptLevel;
    (*HalHndl_p)->DriverPartition = params_p->DriverPartition;

#if defined(STATAPI_SET_EMIREGISTER_MAP)
    (*HalHndl_p)->RegMap 	      = params_p->RegisterMap_p;
#endif
     ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);
    (*HalHndl_p)->DeviceType  =  ATAHandle->DeviceType ;

#if defined(ST_7100) || defined(ST_7109)     
	 if(ATAHandle->DeviceType == STATAPI_SATA)
	     hal_SATAAHB2STBus_PC_Init((U32)(*HalHndl_p)->BaseAddress);
#endif

#if defined(ATAPI_FDMA) 
    (*HalHndl_p)->NCachePartition = params_p->NCachePartition;
    if (NULL == params_p->NCachePartition)
    {
        STTBX_Print(("Warning, built for FDMA but NULL ncachepartition\n"));
        (*HalHndl_p)->Node_p = NULL;
    }
    else
    {
        /* Scratch variable later on */
        STFDMA_Node_t *Node_p;

        /* +31 since we may have to slide the pointer up a bit */
        (*HalHndl_p)->OriginalNode_p = (STFDMA_Node_t*)(memory_allocate(params_p->NCachePartition,
                                                        sizeof(STFDMA_Node_t) + 31));

        if (NULL == ((*HalHndl_p)->OriginalNode_p))
        {
            STOS_MemoryDeallocate((*HalHndl_p)->DriverPartition,*HalHndl_p);
            HalHndl_p = NULL;
            return TRUE;
        }

        /* Adjust for alignment restriction (32-byte aligned) */
        (*HalHndl_p)->Node_p =
            (STFDMA_Node_t *)(((U32)(*HalHndl_p)->OriginalNode_p + 31) & ~31);


        /* Try to lock an high bandwith FDMA channel */
        {
            ST_ErrorCode_t ErrCode=ST_NO_ERROR;

            ErrCode=STFDMA_LockChannelInPool(STFDMA_HIGH_BANDWIDTH_POOL,&ATAPIi_FDMA_ChannelId,STFDMA_1);

            if (ErrCode!=ST_NO_ERROR)
            {
                STOS_MemoryDeallocate((*HalHndl_p)->NCachePartition,(*HalHndl_p)->OriginalNode_p);
                STOS_MemoryDeallocate((*HalHndl_p)->DriverPartition,*HalHndl_p);
                *HalHndl_p = NULL;
                return TRUE;
            }
        }

        /* Shorthand */
        Node_p = (*HalHndl_p)->Node_p;

        /* Just copy 1D to 1D, nothing special, just get on with it...
         * these values don't change, so we can program them here.
         */
        Node_p->Next_p = NULL;
        Node_p->SourceStride = 0;
        Node_p->DestinationStride = 0;
        Node_p->NodeControl.PaceSignal = STFDMA_REQUEST_SIGNAL_NONE;
        Node_p->NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
        Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
        Node_p->NodeControl.NodeCompleteNotify = FALSE;
        Node_p->NodeControl.NodeCompletePause = FALSE;
        Node_p->NodeControl.Reserved = 0;
    }
#elif defined(STATAPI_CRYPT)
    {
    	STEVT_OpenParams_t      OpenParams;
        STEVT_SubscribeParams_t SubscribeParams;

	    /* Initialise STCRYPT Event */
        OpenParams.dummy = 0;
        if(STEVT_Open(STCRYPT_DEVICE_NAME, &OpenParams, &CRYPTEvtHandle) != ST_NO_ERROR)
        {
            return TRUE;
        }

        /* Register for the CRYPT event */
        SubscribeParams.NotifyCallback = hal_CryptCallback;
        if(STEVT_Subscribe(CRYPTEvtHandle, STCRYPT_TRANSFER_COMPLETE_EVT, &SubscribeParams) != ST_NO_ERROR)
        {
            STEVT_Close(CRYPTEvtHandle);
            return TRUE;
        }

        /* Create Transfer complete semaphore */
        CRYPTTransferCompleteSem = STOS_SemaphoreCreateFifo(NULL, 0);
	    
    }
#endif/*ATAPI_FDMA*/

#ifdef BMDMA_ENABLE
    /* create interrupt semaphore */
    (*HalHndl_p)->BMDMA_IntSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL , 0);

    /* install the interrupt handler */
    inumber = (int)BLOCK_MOVE_INTERRUPT;
    if(STOS_SUCCESS != STOS_InterruptInstall(inumber,
                               BLOCK_MOVE_INT_LEVEL,
                               ata_BlockMoveIntHandler,
                               "STATAPI_BLOCKInterruptHandler",
                               NULL))
    {
        STTBX_Print(("Failed to install handler for interrupt %i\n", inumber));
        osResult++;
    }                          
    if (0 != osResult)
    {   /* Error: clean and exit */
        STTBX_Print(("Failed to install interrupt %i\n", inumber));
        STOS_SemaphoreDelete(NULL,(*HalHndl_p)->BMDMA_IntSemaphore_p);
#if defined(ATAPI_FDMA)
        STOS_MemoryDeallocate((*HalHndl_p)->NCachePartition,
                          (*HalHndl_p)->OriginalNode_p);
#endif
        STOS_MemoryDeallocate((*HalHndl_p)->DriverPartition,*HalHndl_p);
        return TRUE;
    }

    /* Enable interrupts in the host */
    if(STOS_SUCCESS != STOS_InterruptEnable((*HalHndl_p)->InterruptNumber,
                                            (*HalHndl_p)->InterruptLevel))
    {
        /* Enabling failed */
         STTBX_Print(("ERROR: Could not enable interrupt handler\n"));
         osResult++;
    }

    if (0 != osResult)
    {   /* Error: clean and exit */
        STTBX_Print(("Failed to enable interrupt %i\n", inumber));
        /* We may want to uninstall interrupt here? */
        STOS_SemaphoreDelete(NULL,((*HalHndl_p)->BMDMA_IntSemaphore_p);
#if defined(ATAPI_FDMA)
        STOS_MemoryDeallocate((*HalHndl_p)->NCachePartition,
                          (*HalHndl_p)->OriginalNode_p);
#endif
        STOS_MemoryDeallocate((*HalHndl_p)->DriverPartition,*HalHndl_p);
        return TRUE;
    }

#endif  /* BMDMA_ENABLE */


#if ATAPI_USING_INTERRUPTS

    /* create interrupt semaphore */
    (*HalHndl_p)->InterruptSemaphore_p = STOS_SemaphoreCreateFifoTimeOut((*HalHndl_p)->DriverPartition,0);

    if (NULL == (*HalHndl_p)->InterruptSemaphore_p)
    {   /* Error: clean and exit */
#if defined(ATAPI_FDMA)
        STOS_MemoryDeallocate((*HalHndl_p)->NCachePartition,
                              (*HalHndl_p)->OriginalNode_p);
#endif
        STOS_MemoryDeallocate((*HalHndl_p)->DriverPartition,HalHndl_p);
        *HalHndl_p = NULL;
        return TRUE;
    }

    /* install the interrupt handler */
    osResult = 0;
    inumber = (int)(*HalHndl_p)->InterruptNumber;


    if(STOS_SUCCESS != STOS_InterruptInstall( inumber,
                             				 (*HalHndl_p)->InterruptLevel,
                               				  ata_InterruptHandler,
                              				  "STATAPI_InterruptHandler",
                              				  ATAHandle))
    {
        STTBX_Print(("Failed to install handler for interrupt %i\n", inumber));
        osResult++;
    }
  	


    if (0 != osResult)
    {   /* Error: clean and exit */
        STTBX_Print(("Failed to install interrupt %i\n", inumber));
        STOS_SemaphoreDelete(NULL,(*HalHndl_p)->InterruptSemaphore_p);
        
#if defined(ATAPI_FDMA)
        STOS_MemoryDeallocate((*HalHndl_p)->NCachePartition,
                          (*HalHndl_p)->OriginalNode_p);
#endif
        STOS_MemoryDeallocate((*HalHndl_p)->DriverPartition,*HalHndl_p);
        *HalHndl_p = NULL;
        return TRUE;
    }

    /* Enable interrupts in the host */
    if(STOS_SUCCESS != STOS_InterruptEnable((*HalHndl_p)->InterruptNumber,
                                            (*HalHndl_p)->InterruptLevel))
    {
        /* Enabling failed */
         STTBX_Print(("ERROR: Could not enable interrupt handler\n"));
         osResult++;
    }

    if (0 != osResult)
    {   /* Error: clean and exit */
        STTBX_Print(("Failed to enable interrupt %i\n", inumber));
        /* We may want to uninstall interrupt here? */
        STOS_SemaphoreDelete(NULL,(*HalHndl_p)->InterruptSemaphore_p);
#if defined(ATAPI_FDMA)
        STOS_MemoryDeallocate((*HalHndl_p)->NCachePartition,
                          (*HalHndl_p)->OriginalNode_p);
#endif
        STOS_MemoryDeallocate((*HalHndl_p)->DriverPartition,*HalHndl_p);
        *HalHndl_p = NULL;
        return TRUE;
    }

#endif /*ATAPI_USING_INTERRUPTS*/

#if defined(ST_7100) || defined(ST_7109)  
    if(ATAHandle->DeviceType == STATAPI_SATA)
    {
#if defined(STATAPI_HOSTCERROR_INTERRUPT_ENABLE)    	
    	(*HalHndl_p)->SATAError = 0;
#endif    	
	    hal_SATA_Int_Clear_Mask((U32)(*HalHndl_p)->BaseAddress);
	    if (ReadReg((*HalHndl_p)->BaseAddress + STATUS_BLOCK) & DMAC_CHANNEL0_STATUS_TFRMASK)
		{
			STTBX_Print(("Status STATUS_BLOCK not cleared !!!!!\n"));
		}
	    else
	    {
		    STTBX_Print(("Status STATUS_BLOCK cleared Successfully!!!!!\n"));
	 	}
    }
#endif /*7100*/    

    if(ATAHandle->DeviceType != STATAPI_EMI_PIO4)
    {
	    (*HalHndl_p)->DmaTransfer = FALSE;
	    (*HalHndl_p)->DmaAborted = FALSE;
	    (*HalHndl_p)->StoredByteCount = 0;
	}

  return FALSE;
} /* hal_Init*/

/************************************************************************
Name: hal_Term

Description:
    This function deallocates all the variables created by the init function

Parameters:
    HalHdl      pointer to the main structure
************************************************************************/
BOOL hal_Term(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p)
{
#if (ATAPI_USING_INTERRUPTS)
    int osResult = 0;
    int inumber;
#ifdef ST_OS20
    interrupt_status_t IntStatus;
#endif /*ST_OS20*/
#endif
	ata_ControlBlock_t *ATAHandle;
	ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);

#ifdef BMDMA_ENABLE
    /* Delete interrupt semaphore*/
    STOS_SemaphoreDelete(NULL,HalHndl_p->BMDMA_IntSemaphore_p);

    /* uninstall the interrupt handler*/
    inumber = (int)BLOCK_MOVE_INTERRUPT;
    /* Remove interrupt handler */
    if(STOS_SUCCESS != STOS_InterruptUninstall(inumber,
                                               BLOCK_MOVE_INT_LEVEL,
                            				   NULL)
    {
        STTBX_Print(("Failed to uninstall handler for interrupt %i\n", inumber));
        osResult++;
    }
#endif

#if ATAPI_USING_INTERRUPTS
    /*Delete interrupt semaphore*/
    STOS_SemaphoreDelete(NULL,HalHndl_p->InterruptSemaphore_p);

#if defined(ST_OS20)
 			/*Bug GNBvd55795 WA*/
            /* Obtain number of active handlers still installed */
            if(interrupt_status(HalHndl_p->InterruptLevel,
                                 &IntStatus) == 0)
            {
                /* Disable interrupt level if no handler are installed */
                if(IntStatus.interrupt_numbers == 0)
                {
                    /* Disable interrupts at this level */
				    if(STOS_SUCCESS != STOS_InterruptDisable(HalHndl_p->InterruptNumber,
				                                            HalHndl_p->InterruptLevel))
				    {
				        /* Disabling failed */
				         STTBX_Print(("ERROR: Could not disable interrupt\n"));
				         return TRUE;
				    }
                }
            }
            else
            {
                /* Unable to obtain interrupt level status */
                    return TRUE;
            }
#endif /* ST_OS20 */

    /*unistall the int. handler*/
    osResult = 0;
    inumber = (int)HalHndl_p->InterruptNumber;

    /* Remove interrupt handler */
    if(STOS_SUCCESS != STOS_InterruptUninstall(inumber,
                           					   HalHndl_p->InterruptLevel,
                            				   ATAHandle))
    {
        STTBX_Print(("Failed to uninstall handler for interrupt %i\n", inumber));
        osResult++;
    }

    if (0 != osResult)
    {
        STTBX_Print(("Failed to uninstall handler for interrupt %i\n", inumber));
        return TRUE;
    }
#endif/*ATAPI_USING_INTERRUPTS*/

#if defined(ATAPI_FDMA)
    /* unlock the FDMA channel */

    if(STFDMA_UnlockChannel(ATAPIi_FDMA_ChannelId,STFDMA_1) != ST_NO_ERROR)

    {
        STTBX_Print(("Failed to Unlock FDMA channel %d\n", ATAPIi_FDMA_ChannelId));
        return TRUE;
    }

    STOS_MemoryDeallocate(HalHndl_p->NCachePartition, HalHndl_p->OriginalNode_p);
#elif defined(STATAPI_CRYPT)
   
    /* Unsubscribe Transfer compelete event */
    STEVT_Unsubscribe(CRYPTEvtHandle, STCRYPT_TRANSFER_COMPLETE_EVT);
    STEVT_Close(CRYPTEvtHandle);
    /* Delete Transfer complete semaphore */
    STOS_SemaphoreDelete(NULL,CRYPTTransferCompleteSem);
#endif/*ATAPI_FDMA*/

    /* Deallocate handle */
    STOS_MemoryDeallocate (HalHndl_p->DriverPartition, HalHndl_p);

    return FALSE;
}/*end hal_Term*/

/*Functions specific to SATA*/

#if defined(ST_7100) || defined(ST_7109)
/************************************************************************
Name: hal_SATA_Int_Clear_Mask
Description:

Parameters:
    HalHndl     Handle of the HAL (for the base address)
    regNo       register number
    data        data value to write to register
*********************************************************************/
void hal_SATA_Int_Clear_Mask(U32 BaseAddress)
{
	U32 ErrorVal = 0; 	
    /* CLEAR_TFR
  	-- Clear All channel Interrupts by writing '1' in respectives bits */
   WriteReg(((U32)BaseAddress + CLEAR_TFR), 0xFF);
    /* CLEAR_BLOCK*/
   WriteReg(((U32)BaseAddress + CLEAR_BLOCK), 0xFF);
  	/* CLEAR_SRCTRAN*/
   WriteReg(((U32)BaseAddress + CLEAR_SRCTRAN), 0xFF);
    /* CLEAR_DSTTRAN*/
   WriteReg(((U32)BaseAddress + CLEAR_DSTTRAN), 0xFF);
   /* CLEAR_ERR*/
   WriteReg(((U32)BaseAddress + CLEAR_ERR), 0xFF);

   /* DMAC -- DMACINTTFR Interrrupt Clearing & Masking */
  	/* MASK_TFR
  	-- Mask channels 7:1 by writing '0' in respectives bits
  	-- Unmask channel 0 by writing '1' in bit '0' */
   WriteReg(((U32)BaseAddress + MASK_TFR), 0xFF00);
  	/* MASK_BLOCK
  	-- Mask All channels by writing '0' in respectives bits */
   WriteReg(((U32)BaseAddress + MASK_BLOCK), 0xFF01);
  	/* MASK_SRCTRAN*/
   WriteReg(((U32)BaseAddress + MASK_SRCTRAN), 0xFF00);
  	/* MASK_DSTTRAN
  	-- Mask All channels by writing '0' in respectives bits */
   WriteReg(((U32)BaseAddress + MASK_DSTTRAN), 0xFF00);
  	/* MASK_ERR*/
   WriteReg(((U32)BaseAddress + MASK_ERR), 0xFF01);
 
   ErrorVal = ReadReg((U32)BaseAddress + SCR1);
   WriteReg(((U32)BaseAddress + SCR1), ErrorVal);	/* clearing the SCR1 register*/
   
#if defined(STATAPI_HOSTCERROR_INTERRUPT_ENABLE)
  /* Enable SATA host SError Interrupts */
   WriteReg(((U32)BaseAddress  + ERRMR), 0xFFFFFFFF);
  /* Enable SATA Host Error Interrupt: bits DMATM, NEWFPM,PMABORTM,ERRM,NEWBISTM */
   WriteReg(((U32)BaseAddress  + INTMR), SATA_SERROR_INTERRUPT_ENABLE);
   
   ErrorVal = ReadReg((U32)BaseAddress + SCR1);
   WriteReg(((U32)BaseAddress + SCR1), ErrorVal);	/* clearing the SCR1 register*/

#endif   

}

/*********************************************************************
Name: SATA_Int_Clear
Description:

Parameters:
    HalHndl     Handle of the HAL (for the base address)
    regNo       register number
    data        data value to write to register
*********************************************************************/

static void hal_SATA_Int_Clear(U32 BaseAddress)
{
  WriteReg(((U32)BaseAddress + SERROR_REG),ReadReg(BaseAddress +  SERROR_REG));
	/* CLEAR_TFR
  	-- Clear All channel Interrupts by writing '1' in respectives bits */
  WriteReg(((U32)BaseAddress + CLEAR_BLOCK), 0xFF);
  /* CLEAR_ERR
  -- Clear All channel Interrupts by writing '1' in respectives bits */
  WriteReg(((U32)BaseAddress + CLEAR_ERR), 0xFF);
}

/*********************************************************************
Name: hal_SATAAHB2STBus_PC_Init
Description:Initialize the AHB to STbus protocol converter registers so as to have max.
            performance
Parameters:
    HalHndl     Handle of the HAL (for the base address)
*********************************************************************/
static void hal_SATAAHB2STBus_PC_Init(U32 BaseAddress)
{
    hal_SATAAHB2STBus_PC_Reset(BaseAddress);

    STTBX_Print(("\n SATA_AHB2STBUS_PC_STATUS  %x", STSYS_ReadRegDev32LE(BaseAddress  + SATA_AHB2STBUS_PC_STATUS)));
    STSYS_WriteRegDev32LE((BaseAddress  + SATA_AHB2STBUS_STBUS_OPC), 0x3);    /* DMA Read, write posting always = 0*/
    STSYS_WriteRegDev32LE((BaseAddress  + SATA_AHB2STBUS_MESSAGE_SIZE_CONFIG), 0x3);
    STSYS_WriteRegDev32LE((BaseAddress  + SATA_AHB2STBUS_CHUNK_SIZE_CONFIG), 0x2);
#if defined(ST_7109)/*for 7109 cut2.0*/
    STSYS_WriteRegDev32LE((BaseAddress + PC_GLUE_LOGIC), 0x100ff);
#else
    STSYS_WriteRegDev32LE((BaseAddress + PC_GLUE_LOGIC), 0x1ff);
#endif
	STTBX_Print(("\n SATA Host AHB2STBus Protocol Converter Init done, OK.\n"));
}

/*------------------------------------------------------
 * This function Resets the AHB2STBus Protocol Converter
 *------------------------------------------------------*/
static void hal_SATAAHB2STBus_PC_Reset(U32 BaseAddress)
{
    STSYS_WriteRegDev32LE((BaseAddress + SATA_AHB2STBUS_SW_RESET), 0x1);
    STSYS_WriteRegDev32LE((BaseAddress + SATA_AHB2STBUS_SW_RESET), 0x0);
	STTBX_Print(("\n\n   -------   SATA Host AHB2STBus Protocol Converter Reset done  ------- \n\n"));

}
#endif/*#if defined(ST_7100) || defined(ST_7109)*/

/************************************************************************
Name: hal_RegOutByte
Description:
    This function works as interface to the ATA registers of the device,
    allowing the user to write data in these registers.
    No bus acquisition procedure is done. User must manage this

Parameters:
    HalHndl     Handle of the HAL (for the base address)
    regNo       register number
    data        data value to write to register
*********************************************************************/
void hal_RegOutByte (hal_Handle_t *HalHndl_p, ATA_Register_t regNo, U8 data)
{
#if defined(ST_7100) || defined(ST_7109)

   if(HalHndl_p->DeviceType == STATAPI_SATA)
   {
    	DU32  *addr;
    	addr = (DU32 *)((U32)HalHndl_p->BaseAddress | SATARegsMasks[regNo]);
       *addr = (U32)data;
   }
   else
   {
   	    DU8     *addr;
        addr = (DU8 *)((U32)HalHndl_p->BaseAddress | RegsMasks[regNo]);
        *addr = data;
   }
#else
    DU8     *addr;
    addr = (DU8 *)((U32)HalHndl_p->BaseAddress | RegsMasks[regNo]);
    *addr = data;
#endif

}
/************************************************************************
Name: hal_RegInByte
Description:
    This function works as interface to the ATA registers of the device,
    allowing the user to read data in these registers. No bus acquisition
    procedure is done. User must manage this

Parameters:
    HalHndl     Handle of the HAL (for the base address)
    regNo       register number
************************************************************************/
U8 hal_RegInByte (hal_Handle_t *HalHndl_p, ATA_Register_t regNo)
{
#if defined(ST_7100) || defined(ST_7109)

   if(HalHndl_p->DeviceType == STATAPI_SATA)
   {
	    DU32 *addr;
	    addr =(DU32 *)((U32)HalHndl_p->BaseAddress | SATARegsMasks[regNo]);
	    return (U8)(*addr);
   }
   else
   {
   	    DU8 *addr;
    	addr = (DU8 *)((U32)HalHndl_p->BaseAddress | RegsMasks[regNo]);
    	return *addr;
   }
#else
    DU8 *addr;
    addr = (DU8 *)((U32)HalHndl_p->BaseAddress | RegsMasks[regNo]);
    return *addr;
#endif

}

/************************************************************************
Name: hal_RegOutWord
Description:
    This function works as an interface to the device's ATA DATA register
    allowing the user to  WRITE data in this register. No bus acquisition
    procedure is done. User must manage this

Parameters:
    HalHndl     Handle of the HAL (for the base address)
    data        data value to write to register
*********************************************************************/
void hal_RegOutWord (hal_Handle_t *HalHndl_p, U16 data)
{
#if defined(ST_7100)
   if(HalHndl_p->DeviceType == STATAPI_SATA)
   {
       DU32 *addr;
   	   addr =(DU32 *)((U32)HalHndl_p->BaseAddress | SATARegsMasks[ATA_REG_DATA]);
       *addr = data;
   }
   else
   {
   	   DU16 *addr;
       addr = (DU16 *)((U32)HalHndl_p->BaseAddress | RegsMasks[ATA_REG_DATA]) ;
      *addr = data;
   }
#else
    DU16    *addr;
    addr = (DU16 *)((U32)HalHndl_p->BaseAddress | RegsMasks[ATA_REG_DATA]) ;
    *addr = data;
#endif

}

/************************************************************************
Name: hal_RegInWord
Description:
    This function works as interface to the device's ATA DATA register allowing
    the user to  READ data in these registers. No bus acquisition procedure is
    done. User must manage this

Parameters:
    HalHndl     Handle of the HAL (for the base address)
************************************************************************/
U16 hal_RegInWord (hal_Handle_t *HalHndl_p)
{

#if defined(ST_7100)
   if(HalHndl_p->DeviceType == STATAPI_SATA)
   {
    	DU32 *addr;
    	addr =(DU32 *)((U32)HalHndl_p->BaseAddress | SATARegsMasks[ATA_REG_DATA]);
   		return (U16)(*addr);
   }
   else
   {
	   	DU16  *addr;
	    addr = (DU16 *)((U32)HalHndl_p->BaseAddress | RegsMasks[ATA_REG_DATA]) ;
	    return (U16)(*addr);
   }
#else
    DU16    *addr;
    addr = (DU16 *)((U32)HalHndl_p->BaseAddress | RegsMasks[ATA_REG_DATA]) ;
    return (U16)(*addr);
#endif

}

/************************************************************************
Name: hal_GetCapabilites

Description:
    This function returns the capabilities of the current host hardware
    interface.

Parameters:
    HalHndl         Handle of the HAL (for the base address)
    Capabilites     pointer to a previously allocated structure to fill with
                    the return values
Returns:
    FALSE       All OK
    TRUE        Something wrong
************************************************************************/
BOOL hal_GetCapabilities(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl,
											STATAPI_Capability_t *Capabilities)
{
    ata_ControlBlock_t *ATAHandle;
    
    /* Obtain the ATAPI control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);
    
    /* Checking the handle and the output parameter */
    if ((Capabilities != NULL) && (HalHndl == ATAHandle->HalHndl))
    					 		  
    {

        Capabilities->SupportedPioModes = CurrentCapabilities.SupportedPioModes;
        Capabilities->SupportedDmaModes = CurrentCapabilities.SupportedDmaModes;
        Capabilities->DeviceType = CurrentCapabilities.DeviceType;
    }
    else
    {
        return TRUE;
    }

    return FALSE;
}

/************************************************************************
Name: hal_EnableInts
Description:
    Enable interrupts in the device

Parameters:
    HalHndl     Handle of the HAL (for the base address)
************************************************************************/
void hal_EnableInts (hal_Handle_t *HalHndl_p)
{
    /* Clear nIEN bit */
    hal_RegOutByte (HalHndl_p, ATA_REG_CONTROL, nIEN_CLEARED);
}

/************************************************************************
Name:hal_DisableInts
Description:
    Disable interrupts in the device

Parameters:
    HalHndl     Handle of the HAL (for the base address)
************************************************************************/
void hal_DisableInts (hal_Handle_t *HalHndl_p)
{
    /* Set nIEN bit */
    hal_RegOutByte (HalHndl_p,ATA_REG_CONTROL,nIEN_SET);
}

/************************************************************************
Name:   hal_AwaitInt()
Description:
    This function waits for an interrupt generated any of the ata devices

Parameters:
    HalHndl     Handle of the HAL (for the base address)
    Timeout     Timeout value for (internal) semaphore
************************************************************************/

BOOL hal_AwaitInt(hal_Handle_t *HalHndl_p,U32 Timeout)
{
    clock_t     TO;
    BOOL        Error = FALSE;

    TO = time_plus(time_now(), Timeout);
   
	if (STOS_SemaphoreWaitTimeOut(HalHndl_p->InterruptSemaphore_p, &TO))
    {
        STTBX_Print(("No DMA interrupt received\n"));
        STTBX_Print(("Waited for %i clocks\n", Timeout));
        Error = TRUE;
	}
	return (Error);
}

#if (ATAPI_USING_INTERRUPTS)
/************************************************************************
Name: ata_InterruptHandler
Description:

Parameters:
    Not used
************************************************************************/
STOS_INTERRUPT_DECLARE(ata_InterruptHandler,ATAHandle_p)
{
#if defined(ST_5514) || defined(ST_5528)|| defined(ST_8010)
    U32 intvalue = 0;
#elif defined(ST_7100) || defined(ST_7109) 
    U32 Status_Block = 0, Status_Error = 0;
    U8 Temp;
    U8 interrupt_source = 0;
    volatile U32 Dummy;
#else
    volatile U32 Dummy;
#endif

     ata_ControlBlock_t *ATAHandle;
     hal_Handle_t *The_HalHandle_p = NULL;
     
     ATAHandle = ATAHandle_p;
     The_HalHandle_p = ATAHandle->HalHndl;

#if defined(ST_7100) || defined(ST_7109) 

    if(ATAHandle->DeviceType == STATAPI_SATA)
    {
	    Temp = hal_RegInByte (The_HalHandle_p,ATA_REG_STATUS);
	    Temp = hal_RegInByte (The_HalHandle_p,ATA_REG_ALTSTAT);
	
	   	/* Check for DMACINTTFR -- to be updated later, see DMAC*/
	    if (ReadReg((U32)The_HalHandle_p->BaseAddress + STATUS_BLOCK ) & DMAC_CHANNEL0_STATUS_TFRMASK) 
	    /*DMAC whole Transfer Complete*/
		{
			Status_Block = ReadReg(((U32)The_HalHandle_p->BaseAddress + STATUS_BLOCK));
	        WriteReg(((U32)The_HalHandle_p->BaseAddress + CLEAR_BLOCK), 0x01);
	        interrupt_source = 1;
	    }
	
	   	if (ReadReg((U32)The_HalHandle_p->BaseAddress + STATUS_ERR ) & DMAC_CHANNEL0_STATUS_TFRMASK)    
	   	/*DMAC Block Transfer Complete*/
		{
			Status_Error = ReadReg(((U32)The_HalHandle_p->BaseAddress + STATUS_ERR));
			WriteReg(((U32)The_HalHandle_p->BaseAddress + CLEAR_ERR), 0x01);
	    }
#if defined(STATAPI_HOSTCERROR_INTERRUPT_ENABLE)
		{
			U32 Test  = 0;
			U32 Value = 0;
			
			Value = ReadReg((U32)The_HalHandle_p->BaseAddress + INTPR);
			
			/* INTPR[ERR] is set*/
		    if (( Value & SATA_SERROR_INTERRUPT_ENABLE) == SATA_SERROR_INTERRUPT_ENABLE)	 
		    {
		    	 Test = ReadReg((U32)The_HalHandle_p->BaseAddress + SCR1);
		    	 The_HalHandle_p->SATAError = Test;
		    	 
		    	 if((Test & SATA_AHB_ILLEGALACCESS_ERROR) | (Test & SATA_DATA_INTEGRITY_ERROR) \
		    	 	 | (Test & SATA_10b8b_DECODER_ERROR) | (Test & SATA_DISPARITY_ERROR))
		    	 {
		    	 		interrupt_source = 2;
		    	 }
	
			  	 /* clearing the SCR1 register*/    	
		     	 WriteReg(((U32)The_HalHandle_p->BaseAddress + SCR1), Test);	
		      	/* clearing the INTPR register*/      		
		     	 WriteReg(((U32)The_HalHandle_p->BaseAddress + INTPR), SATA_SERROR_INTERRUPT_ENABLE);	
			}
		}
#endif	  	    
	 }

#endif
#if defined(ATAPI_DEBUG)
    /* Mark handler entry */
    if (atapi_intcount < MAX_TRACE)
        atapi_inttrace[atapi_intcount] = 0x100;
    atapi_intcount++;
#endif  /* ATAPI_DEBUG */

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
#if defined(ATAPI_DEBUG)
    /* Store current DMA_STA for use in any debugging */
    intvalue = ReadReg((U32)(The_HalHandle_p->BaseAddress) + HDDI_DMA_STA);
    if (atapi_intcount < MAX_TRACE)
        atapi_inttrace[atapi_intcount] = intvalue;
    atapi_intcount++;

    /* Store current ITS for debugging */
    intvalue = ReadReg((U32)(The_HalHandle_p->BaseAddress) + HDDI_DMA_ITS);
    if (atapi_intcount < MAX_TRACE)
        atapi_inttrace[atapi_intcount] = intvalue;
    atapi_intcount++;
#else
    intvalue = ReadReg((U32)(The_HalHandle_p->BaseAddress) + HDDI_DMA_ITS);
#endif  /* ATAPI_DEBUG */

    HDDI_ITS_Value = intvalue;
#endif  /* ST_5514 || ST_5528  || defined(ST_8010)*/

    /* clear interrupt = read status register */
#if !defined(ST_5514) && !defined(ST_5528) && !defined(ST_8010)
    if(ATAHandle->DeviceType != STATAPI_SATA)
    {
	    Dummy = hal_RegInByte (The_HalHandle_p,ATA_REG_ALTSTAT);
	    Dummy = hal_RegInByte (The_HalHandle_p,ATA_REG_STATUS);
	    Dummy = hal_RegInByte (The_HalHandle_p, ATA_REG_SECCOUNT); /* ?! */
	}
#endif

#if defined(ST_8010)
    if ( HDDI_ITS_Value & HDDI_DMA_ITS_IRQ )
    {
        STOS_SemaphoreSignal(The_HalHandle_p->InterruptSemaphore_p);
    }
#elif defined(ST_7100) || defined(ST_7109)
    if(ATAHandle->DeviceType == STATAPI_SATA)
    {
	    if(interrupt_source == 1)
	    {
	        STOS_SemaphoreSignal(The_HalHandle_p->InterruptSemaphore_p);
	        interrupt_source = 0;
	    }
	    else if(interrupt_source == 2)
	    {
	    	STOS_SemaphoreSignal(The_HalHandle_p->InterruptSemaphore_p);
	        interrupt_source = 0;
	    }
    }
	else
	        STOS_SemaphoreSignal(The_HalHandle_p->InterruptSemaphore_p);
	         
#else
    STOS_SemaphoreSignal(The_HalHandle_p->InterruptSemaphore_p);
#endif

    STOS_INTERRUPT_EXIT(STOS_SUCCESS);
}

#endif /* ATAPI_USING_INTERRUPTS */

/************************************************************************
Name: hal_RegOutBlock

Description:
    This function works as interface to the  device's ATA  DATA register
    allowing the user to  WRITE data in this register. No bus acquisition
    procedure is done. User must manage this

Parameters:
    HalHndl     Handle of the HAL (for the base address)
    data        data buffer to write
    Size        Number of bytes to write
    UseDMA      Whether a DMA engine should be used to transfer the
                data to the data port (if available)
*********************************************************************/
void __inline hal_RegOutBlock (hal_Handle_t *HalHndl_p,
                                         U32 *data,
                                         U32 Size,
                                         BOOL UseDMA,
                                         BOOL EnableCrypt)
{
#if defined(ST_5514)  || defined(ST_5528) || defined(ST_8010)\
|| defined(ST_7100)|| defined(ST_7109) || defined(ST_5525)
    U32 loop;
    U16 temp;
    U16 *pui16StartAddress;

#endif
#if defined(ATAPI_FDMA)
    STFDMA_Node_t                  *Node_p = NULL;
    STFDMA_TransferParams_t         TransferParams;
    STFDMA_TransferId_t             TransferId;
    ST_ErrorCode_t                  ErrCode=ST_NO_ERROR;
#endif
   
   if(HalHndl_p->DeviceType == STATAPI_SATA)
   {
#if defined(ST_7100) || defined(ST_7109)   	   	
#if defined (ST_7109)
    	DU16  *addr;
   	    addr = (DU16 *)((U32)HalHndl_p->BaseAddress | SATARegsMasks[ATA_REG_DATA]);
#elif defined (ST_7100)
    	DU32  *addr;
    	addr = (DU32 *)((U32)HalHndl_p->BaseAddress | SATARegsMasks[ATA_REG_DATA]);
#endif
	    pui16StartAddress = ( U16* )data;
	    loop = Size >> 1;
	    for (; loop > 0; loop--)
	    {
	        temp = *pui16StartAddress;
	        *addr = temp;
	        pui16StartAddress++;
	    }
	    if ( Size & 0x01 )
	    {
	    	/* If the size is in odd, write the last byte*/
	    	*addr = *(U8*)pui16StartAddress;
	    }
#endif/*7100*/	    
   }
   else
   {
   		DU16  *addr;
	    /* Work out where to send the data */
	    addr = (DU16 *)((U32)HalHndl_p->BaseAddress | RegsMasks[ATA_REG_DATA]);

#if defined(ATAPI_FDMA)
    /* Grab the node we already allocate (depending on higher layers to
     * keep one operation per bus.
     */
    	Node_p = HalHndl_p->Node_p;
    	if ((UseDMA == TRUE) && (NULL != Node_p) && (((U32)data & 0x1F)==0)) /* transfer should be aligned to 32 bytes */
    	{
	        STTBX_Print(("ATA: FDMA - Write %d bytes from 0x%08x to 0x%08x\n",Size,data,addr));
	        /* Set up the node elements that aren't preconfigured */
	        Node_p->NumberBytes = Size;
#if defined (ARCHITECTURE_ST40)
	        Node_p->SourceAddress_p = (void *)TRANSLATE_PHYS_ADD(data);
	        Node_p->DestinationAddress_p = (void *)TRANSLATE_PHYS_ADD(addr);
#else
	        Node_p->SourceAddress_p = (void *)(data);
	        Node_p->DestinationAddress_p = (void *)(addr);
#endif
	        Node_p->Length = Size;
	        Node_p->NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
	        Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_STATIC;
	
	        TransferParams.ChannelId         = ATAPIi_FDMA_ChannelId;
	        /* Blocking transfer */
	        TransferParams.CallbackFunction  = NULL;
#if defined (ARCHITECTURE_ST40)
        	TransferParams.NodeAddress_p     = (STFDMA_Node_t*)TRANSLATE_PHYS_ADD(Node_p);
#else
        	TransferParams.NodeAddress_p     = (Node_p);
#endif
	        TransferParams.BlockingTimeout   = 0;
	        TransferParams.ApplicationData_p = NULL;
	        TransferParams.FDMABlock         = STFDMA_1;
	
	        ErrCode=STFDMA_StartTransfer(&TransferParams, &TransferId);
	        if (ErrCode!=ST_NO_ERROR)
	        {
	            STTBX_Print(("ATA: FDMA - Write has failed !\n"));
	        }
        }
        else
        {
	        /* Built for FDMA, but NULL ncachepartition (in which case the
	         * user gets a warning already, but we don't want to crash).
	         */
	        STTBX_Print(("ATA: CPU - Write %d bytes from 0x%08x to 0x%08x\n",Size,data,addr));
	        memcpy((void *)addr, data, Size);
    	}
#elif defined(STATAPI_CRYPT)
	{
	    /* With DES DMA */
	    STCRYPT_DmaSetup_t  DmaSetup;
	    ST_ErrorCode_t      ErrCode=ST_NO_ERROR;
	
	    /* Transfer should be aligned to 32 bytes */
	    if ((UseDMA == TRUE) && (((U32)data & 0x1F)==0))
	    {
	        STTBX_Print(("ATA: DESDMA - Write %d bytes from 0x%08x to 0x%08x\n",Size,data,addr));
	
	        /* Start the crypto-core */
	        DmaSetup.Algorithm     = STCRYPT_ALGORITHM_AES;
	        DmaSetup.Mode          = (EnableCrypt == TRUE) ? STCRYPT_ENGINE_MODE_ENCRYPT : STCRYPT_ENGINE_MODE_BYPASS;
	        DmaSetup.Key           = STCRYPT_KEY_IS_CHANNEL_0;
	        DmaSetup.Source_p      = (void *)data;
	        DmaSetup.Destination_p = (void *)addr;
	        DmaSetup.Size          = Size;
	
	        CRYPTStartedDMA = TRUE;
	
	        ErrCode = STCRYPT_StartDma(&DmaSetup);
	        if (ErrCode != ST_NO_ERROR)
	        {
	            STTBX_Print(("ATA: DESDMA - Write has failed !\n"));
	        }
	        else
	        {
	            /* Wait for end of transfer */
	            STOS_SemaphoreWait(CRYPTTransferCompleteSem);
	        }
	    }
	    /* Else simple memcpy */
	    else
	    {
	        STTBX_Print(("ATA: CPU - Write %d bytes from 0x%08x to 0x%08x\n",Size,data,addr));
	        memcpy((void *)addr,data,Size);
	    }
	}
#elif defined(ST_5514) || defined(ST_5528) || defined(ST_8010)\
|| defined(ST_7100) || defined(ST_7109) || defined(ST_5525)
	    pui16StartAddress = ( U16* )data;
	    loop = Size >> 1;
	    for (; loop > 0; loop--)
	    {
	        temp = *pui16StartAddress;
	        *addr = temp;
	        pui16StartAddress++;
	    }
	    if ( Size & 0x01 )
	    {
	    	/* If the size is in odd, write the last byte*/
	    	*addr = *(U8*)pui16StartAddress;
	    }
#else
    memcpy((void *)addr, data, Size);
#endif
   }
}

/************************************************************************
Name: hal_RegInBlock

Description:
    This function works as interface to the device's ATA DATA register allowing
    the user to  READ data in these registers. No bus acquisition procedure is
    done. User must manage this

Parameters:
    HalHndl     Handle of the HAL (for the base address)
    data        data buffer to fill
    Size        Number of bytes to read
    UseDMA      Whether a DMA engine should be used to transfer the data
                from the data port, if available
************************************************************************/
void __inline hal_RegInBlock (hal_Handle_t *HalHndl_p,
                              U32 *data,
                              U32 Size,
                              BOOL UseDMA,
                              BOOL EnableCrypt)
{

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)\
|| defined(ST_7100) || defined(ST_7109)  || defined(ST_5525)
    U32 loop;
    U16 *pui16StartAddress;
#endif

#if defined(ATAPI_FDMA)
    STFDMA_Node_t                  *Node_p = NULL;
    STFDMA_TransferParams_t         TransferParams;
    STFDMA_TransferId_t             TransferId;
    ST_ErrorCode_t                  ErrCode=ST_NO_ERROR;
#endif

   if(HalHndl_p->DeviceType == STATAPI_SATA)
   {
#if defined(ST_7100) || defined(ST_7109)   	
#if defined (ST_7109)
    	DU16  *addr;
   	    addr = (DU16 *)((U32)HalHndl_p->BaseAddress | SATARegsMasks[ATA_REG_DATA]);
#elif defined (ST_7100)
    	DU32  *addr;
    	addr = (DU32 *)((U32)HalHndl_p->BaseAddress | SATARegsMasks[ATA_REG_DATA]);
#endif
	    pui16StartAddress = ( U16* )data;
	    loop = Size >> 1;
	    for (; loop > 0; loop--)
	    {
	        *pui16StartAddress = *addr;
	        pui16StartAddress++;
	    }
	
	    if ( Size & 0x01 )
	    {
	    	/* If the size is in odd, read the last byte*/
	        *(U8*)pui16StartAddress = *addr;
	    }
#endif	    
	}
	else 
	{
			DU16  *addr;
	    	/* Work out where to send the data */
	    	addr = (DU16 *)((U32)HalHndl_p->BaseAddress | RegsMasks[ATA_REG_DATA]);
	
#if defined(ATAPI_FDMA)
		    /* Grab the node we already allocate (depending on higher layers to
		     * keep one operation per bus.
		     */
		    Node_p = HalHndl_p->Node_p;
		    if ((UseDMA == TRUE) && (NULL != Node_p) && (((U32)data & 0x1F)==0)) /* transfer should be aligned to 32 bytes */
		    {
		        	STTBX_Print(("ATA: FDMA - Read %d bytes from 0x%08x to 0x%08x\n",Size,
		        																addr,data));
		       		/* Set up the node elements that aren't preconfigured */
		        	Node_p->NumberBytes = Size;
#if defined (ARCHITECTURE_ST40)
			        Node_p->SourceAddress_p = (void *)TRANSLATE_PHYS_ADD(addr);
			        Node_p->DestinationAddress_p = (void *)TRANSLATE_PHYS_ADD(data);
#else
			        Node_p->SourceAddress_p = (void *)(addr);
			        Node_p->DestinationAddress_p = (void *)(data);
#endif
	        		Node_p->Length = Size;
			
			        Node_p->NodeControl.SourceDirection      = STFDMA_DIRECTION_STATIC;
			        Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
			
			        TransferParams.ChannelId         = ATAPIi_FDMA_ChannelId;
			        /* Blocking transfer */
			        TransferParams.CallbackFunction  = NULL;
#if defined (ARCHITECTURE_ST40)
       				TransferParams.NodeAddress_p     = (STFDMA_Node_t*)TRANSLATE_PHYS_ADD(Node_p);
#else
        			TransferParams.NodeAddress_p     = Node_p;
#endif
			        TransferParams.BlockingTimeout   = 0;
			        TransferParams.ApplicationData_p = NULL;
			        TransferParams.FDMABlock         = STFDMA_1;
			
			        ErrCode=STFDMA_StartTransfer(&TransferParams,&TransferId);
			        if (ErrCode!=ST_NO_ERROR)
			        {
			          STTBX_Print(("ATA: FDMA - Read has failed !\n"));
			        }
            }
    		else
		    {
		        	/* Built for FDMA, but NULL ncachepartition (in which case the
		        	 * user gets a warning already, but we don't want to crash).
		        	 */
		        	STTBX_Print(("ATA: CPU - Read %d bytes from 0x%08x to 0x%08x\n",Size,addr,data));
		        	memcpy(data, (void *)addr, Size);
		    }
#elif defined(STATAPI_CRYPT)
	{    
	    STCRYPT_DmaSetup_t  DmaSetup;
	    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
	    
	    /* Transfer should be aligned to 32 bytes */
	    if ((UseDMA == TRUE) && (((U32)data & 0x1F)==0))
	    {
	        STTBX_Print(("ATA: DESDMA - Read %d bytes from 0x%08x to 0x%08x\n",Size,addr,data));
	
	        /* Start the crypto-core */
	        DmaSetup.Algorithm     = STCRYPT_ALGORITHM_AES;
	        DmaSetup.Mode          = (EnableCrypt == TRUE) ? STCRYPT_ENGINE_MODE_DECRYPT : STCRYPT_ENGINE_MODE_BYPASS;
	        DmaSetup.Key           = STCRYPT_KEY_IS_CHANNEL_0;
	        DmaSetup.Source_p      = (void *)addr;
	        DmaSetup.Destination_p = (void *)data;
	        DmaSetup.Size          = Size;
	
	        CRYPTStartedDMA = TRUE;
	
	        ErrCode = STCRYPT_StartDma(&DmaSetup);
	        if (ErrCode != ST_NO_ERROR)
	        {
	            STTBX_Print(("ATA: DESDMA - Read has failed !\n"));
	        }
	        else
	        {
	            /* Wait for end of transfer */
	            STOS_SemaphoreWait(CRYPTTransferCompleteSem);
	        }
	    }
	    /* Else simple memcpy */
	    else
	    {
	        STTBX_Print(("ATA: CPU - Read %d bytes from 0x%08x to 0x%08x\n",Size,addr,data));
	        memcpy(data,(void *)addr,Size);
	    }
	}
#elif defined(ST_5514) || defined(ST_5528) || defined(ST_8010)\
|| defined(ST_7100) || defined(ST_7109) || defined(ST_5525)

	    pui16StartAddress = ( U16* )data;
	    loop = Size >> 1;
	    for (; loop > 0; loop--)
	    {
	        *pui16StartAddress = *addr;
	        pui16StartAddress++;
	    }
	
	    if ( Size & 0x01 )
	    {
	    	/* If the size is in odd, read the last byte*/
	        *(U8*)pui16StartAddress = *addr;
	    }
#else
	    memcpy(data, (void *)addr, Size);
#endif
	}/*if*/
}

/****************************************************************************
Name: ata_BlockMoveIntHandler
Description:
    routine that clears the pending Interrupts, and reset the semaphore
    counter
****************************************************************************/
#ifdef BMDMA_ENABLE
static INTERRUPT_RETURN_TYPE ata_BlockMoveIntHandler (void *param)
{
    volatile U32 data;
    U32         valid_bits = (1<<2)-1;
    U32         expected_value = 0x3;

#pragma ST_device (BMDMAReg)
	volatile U8 *BMDMAReg;

    param = param;  /* Not used */

    BMDMAReg = (volatile U8*)BMDMA_Status;
    data = *BMDMAReg;

    if ((data & valid_bits) == expected_value)
    {
        BMDMAReg = (volatile U8*)BMDMA_IntAck;
        *BMDMAReg = 1;

		BMDMAReg = (volatile U8*)BMDMA_Status;
		data = *BMDMAReg;

        STOS_SemaphoreSignal(The_HalHandle_p->BMDMA_IntSemaphore_p);
    }
    else
    {
        STOS_SemaphoreSignal(The_HalHandle_p->BMDMA_IntSemaphore_p);
    }

    return (INTERRUPT_RETURN_OK);
}

/***************************************************************
Name: ATA_BMDMA
Description:
    Performs Block Move DMA

Parameters:
    Source              Source address
    Destination         Destination address
    Size                Amount of data to be moved
****************************************************************/

void ATA_BMDMA (void *Source, void *Destination, U32 Size)
{
#pragma ST_device (BMDMAReg)
    volatile U8 *BMDMAReg;

    BMDMAReg = (volatile U8*) BMDMA_DestAddress;
  	*BMDMAReg = (U32) (Destination);
  	BMDMAReg = (volatile U8*)BMDMA_SrcAddress;
	*BMDMAReg = (U32) Source;
	BMDMAReg = (volatile U8*)BMDMA_IntEn;
	*BMDMAReg = 1;
	BMDMAReg = (volatile U8*)BMDMA_Count;
	*BMDMAReg = Size;

    STOS_SemaphoreWait(The_HalHandle_p->BMDMA_IntSemaphore_p);
}
#endif  /* BMDMA_ENABLE */


/************************************************************************
Name:   hal_SoftReset()
Description:
    Performs a soft reset on all the devices, according to section 9.2
    of ATA-5

Parameters:
    HalHndl     Handle of the HAL (for the base address)
************************************************************************/
BOOL hal_SoftReset(hal_Handle_t *HalHndl_p)
{
    BOOL error = FALSE;

#if defined (ATAPI_DEBUG)
	DU8 status, error_reg;
#endif

    /* Assert SRST and disable interrupts */
    hal_RegOutByte(HalHndl_p, ATA_REG_CONTROL, SRST_SET | nIEN_SET);

    /* Need to ensure a gap between assert and de-assert of >= 5us */
    task_delay(TWO_MS);

    /*Deassert SRTS and enable interrupts if necessary*/
#if ATAPI_USING_INTERRUPTS
    hal_RegOutByte(HalHndl_p, ATA_REG_CONTROL, 0x00);
#else
    hal_RegOutByte(HalHndl_p, ATA_REG_CONTROL, nIEN_SET);
#endif

    /* Wait t > 2ms */
    task_delay(TWO_MS);

#if !defined (ATAPI_DEBUG)
    /* Wait for drive to clear BSY */
    error = WaitForBit(HalHndl_p, ATA_REG_STATUS, BSY_BIT_MASK, 0);
#else
    do
    {
        /* Now check the status*/
        WAIT400NS;
        status = hal_RegInByte(HalHndl_p, ATA_REG_STATUS);
		if ( (status&ERR_BIT_MASK)!=0)
		{
			/* There is an error, check it */
        	error_reg = hal_RegInByte(HalHndl_p, ATA_REG_ERROR);
			STTBX_Print(("ata_cmd_PioOut(%08x) : !! ERROR=%02x !!\n",error_reg));
			if ((error_reg & 0x4)!=0)
				{ STTBX_Print(("ata_cmd_PioOut(%08x) : !! ABORT !!\n")); }
			if ((error_reg & 0x10)!=0)
				{ STTBX_Print(("ata_cmd_PioOut(%08x) : !! OUT OF RANGE !!\n")); }
			if ((error_reg & 0x40)!=0)
				{ STTBX_Print(("ata_cmd_PioOut(%08x) : !! UNRECOVERABLE !!\n")); }
			error = TRUE;
			break;
		}
    } while((status & BSY_BIT_MASK)!=0);

#endif

#if defined(ATAPI_DEBUG)
    if (TRUE == ATAPI_Verbose)
    {
        STTBX_Print(("hal_SoftReset(): done reset, %s\n",
                    (error == 0)?"all okay":"failed"));
    }
#endif

    /* Caller can now check signature, assuming success */
    return error;
}

/************************************************************************
Name:   hal_HardReset()
Description:
    Performs a hardware reset on all the devices

Parameters:
    HalHndl     Handle of the HAL (for the base address)
************************************************************************/
BOOL hal_HardReset (hal_Handle_t *HalHndl_p)
{
    BOOL Error = FALSE;

#if defined (ST_5512)
    *(HalHndl_p->HWResetAddress) = ATA_HRD_RST_ASSERT;
    task_delay (TWO_MS);
    *(HalHndl_p->HWResetAddress) = ATA_HRD_RST_DEASSERT;
    Error = FALSE;

#elif defined(ST_5514) || defined(ST_5528) || defined(ST_8010)

    WriteReg((U32)(HalHndl_p->BaseAddress) + HDDI_ATA_RESET, 1);

    task_delay( TWO_MS / 4);       /* Wait 500us */

    /* Clear the reset signal */
    WriteReg((U32)(HalHndl_p->BaseAddress) + HDDI_ATA_RESET, 0);

    /* Delay period? */
    task_delay( TWO_MS * 4 );      /* Wait 8ms  */

    if (WaitForBit(HalHndl_p, ATA_REG_ALTSTAT, BSY_BIT_MASK,0))
    {
        STTBX_Print(("Timed out waiting for busy to be clear\n"));
    }

    Error = FALSE;

#elif defined(ST_7100) || defined(ST_7109)
   
   if(HalHndl_p->DeviceType == STATAPI_SATA)
   {
	   U32 Test;
	  
	   WriteReg((U32)HalHndl_p->BaseAddress + PHYCR,0x388fc);/*for cut 2.x,3.x*/
	  /* WriteReg((U32)HalHndl_p->BaseAddress + PHYCR,0x0013704A);*/  /*for cut 1.x*/
	   STTBX_Print(("PHYCR:Default value for Cut 1.3  = %x \n", ReadReg((U32)HalHndl_p->BaseAddress + PHYCR)));
	   task_delay( TWO_MS /4);
	
	   STTBX_Print(("CDR7 : Status Reg  Value = %x \n",ReadReg((U32)HalHndl_p->BaseAddress + RegsMasks[ATA_REG_STATUS])));
	
	   WriteReg((U32)HalHndl_p->BaseAddress + SCR2, 0x1);
	   task_delay( TWO_MS / 4);       /* Wait 50us */
	   WriteReg((U32)HalHndl_p->BaseAddress + SCR2, 0x000);
	   task_delay( TWO_MS * 4);      /* Wait 8ms  */
	
	
	   STTBX_Print(("SCR0 : SStatus  Value = %x \n",ReadReg((U32)HalHndl_p->BaseAddress+ SCR0)));
	   Test =ReadReg((U32)HalHndl_p->BaseAddress + SCR1);
	   WriteReg(((U32)HalHndl_p->BaseAddress + SCR1), Test);	/* clearing the SCR1 register*/
	   STTBX_Print(("SCR1 : SError Value = %x \n",ReadReg((U32)HalHndl_p->BaseAddress + SCR1)));
	   STTBX_Print(("DMA_ID_REG Value = %x \n",ReadReg((U32)HalHndl_p->BaseAddress + DMA_ID_REG)));
	    if (WaitForBit(HalHndl_p, ATA_REG_ALTSTAT, BSY_BIT_MASK,0))
	    {
	        STTBX_Print(("Timed out waiting for busy to be clear\n"));
	    }
	   hal_SATAAHB2STBus_PC_Init((U32)(HalHndl_p)->BaseAddress);
	     Error = FALSE;
	}
	else
	{
		DU8 status;
		do
		{
		     /* Now check the status*/
		     WAIT400NS;
		     status = hal_RegInByte(HalHndl_p, ATA_REG_STATUS);
		} while((status & BSY_BIT_MASK)!=0);
	
	    /* Instead a Hard Reset we perform another SW reset */
	    Error = hal_SoftReset(HalHndl_p);
	}

#else
    /* i.e. not 5512, 5514 , 5528 and SATA */
    DU8 status;
   do
   {
       /* Now check the status*/
       WAIT400NS;
       status = hal_RegInByte(HalHndl_p, ATA_REG_STATUS);
   } while((status & BSY_BIT_MASK)!=0);

    /* Instead a Hard Reset we perform another SW reset */
    Error = hal_SoftReset(HalHndl_p);
#endif

    return Error;
}

/************************************************************************
Name:
    hal_SetDmaMode()

Description:
    Sets the HAL handle to use the passed mode

Parameters:
    Mode        DMA mode to use for future transfers
************************************************************************/
BOOL hal_SetDmaMode (hal_Handle_t *HalHndl_p, STATAPI_DmaMode_t Mode)
{
    CurrentDmaMode = Mode;

    if(HalHndl_p->DeviceType != STATAPI_EMI_PIO4)
    {
#if defined(ST_5514) || defined(ST_5528) ||  defined(ST_7100) || defined(ST_7109) || defined(ST_8010)        	
	    if (Mode <= STATAPI_DMA_MWDMA_MODE_2)
	        DMAIsUDMA = FALSE;
	    else
	        DMAIsUDMA = TRUE;
#endif	        
    }

    return FALSE;
}

/************************************************************************
Name:
    hal_SetPIOMode()

Description:
    Sets the HAL handle to use the passed mode

Parameters:
    Mode        PIO mode to use for future transfers
************************************************************************/
BOOL hal_SetPioMode (hal_Handle_t *HalHndl_p, STATAPI_PioMode_t Mode)
{
    CurrentPioMode = Mode;
    return FALSE;
}

/************************************************************************
Name:   hal_GetDmaTiming()
Description:
    retrieves the current Timing setings of the DMA Mode
Parameters:
************************************************************************/
BOOL hal_GetDmaTiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p,
													STATAPI_DmaTiming_t *Time)
{
    ata_ControlBlock_t *ATAHandle;
    
    /* Obtain the ATAPI control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);
    
    /* Check parameters */
    if ((Time == NULL) || (HalHndl_p != ATAHandle->HalHndl))
     return TRUE;
    /* Copy data - won't actually be useful unless this has a HDDI, but
     * it is always present...
     */
	if(HalHndl_p->DeviceType != STATAPI_SATA)
    	memcpy(Time, &CurrentDmaTiming, sizeof(STATAPI_DmaTiming_t));
    else
        memset(Time,0x00, sizeof(STATAPI_DmaTiming_t));
    return FALSE;

}

/************************************************************************
Name:   hal_GetPioTiming()
Description:
    retrieves the current Timing setings of the Pio Mode
Parameters:
************************************************************************/
BOOL hal_GetPioTiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p,
													STATAPI_PioTiming_t *Time)
{
    ata_ControlBlock_t *ATAHandle;
    
    /* Obtain the ATAPI control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);
    
    /* Check parameters */
    if ((Time == NULL) || (HalHndl_p != ATAHandle->HalHndl))
    	return TRUE;
    	
	if(HalHndl_p->DeviceType != STATAPI_SATA)
    	memcpy(Time, &CurrentPioTiming, sizeof(STATAPI_PioTiming_t));
	else
     	memset(Time,0x00, sizeof(STATAPI_PioTiming_t));

    return FALSE;
}

BOOL hal_SetDmaTiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p,
													   STATAPI_DmaTiming_t *Time)
{  
    BOOL Error = FALSE;
    ata_ControlBlock_t *ATAHandle;
    
    /* Obtain the ATAPI control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);
    
    /* Check parameters */
    if ((Time == NULL) || (HalHndl_p != ATAHandle->HalHndl))
        return TRUE;

#if defined(ST_5514) || defined(ST_5528) ||  defined(ST_7100) || defined(ST_7109) || defined(ST_8010)
    CurrentDmaTiming = *Time;
    
#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
    /* Plug values into the registers */
    switch (CurrentDmaMode)
    {
        case STATAPI_DMA_MWDMA_MODE_0:
        case STATAPI_DMA_MWDMA_MODE_1:
        case STATAPI_DMA_MWDMA_MODE_2:
                        hal_SetMWDMATiming(DeviceName,HalHndl_p,
                                &Time->DmaTimingParams.MwDmaTimingParams);
                        break;

        case STATAPI_DMA_UDMA_MODE_0:
        case STATAPI_DMA_UDMA_MODE_1:
        case STATAPI_DMA_UDMA_MODE_2:
        case STATAPI_DMA_UDMA_MODE_3:
        case STATAPI_DMA_UDMA_MODE_4:
        case STATAPI_DMA_UDMA_MODE_5:
                        hal_SetUDMATiming(DeviceName,HalHndl_p,
                                &Time->DmaTimingParams.UltraDmaTimingParams);
                        break;

        case STATAPI_DMA_NOT_SUPPORTED:
        default:
                        Error = TRUE;
                        break;
    }
#endif   /* defined(ST_5514) || defined(ST_5528) || defined(ST_8010)*/ 
#else
    /* Doesn't contain *DMA stuff */
    Error = TRUE;
#endif /* defined DMA_PRESENT */

    return Error;
}

BOOL hal_SetPioTiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p,
													STATAPI_PioTiming_t *Time)
{
    BOOL Error = FALSE;
    ata_ControlBlock_t *ATAHandle;
    
    /* Obtain the ATAPI control block associated with the device name */
    ATAHandle = ATAPI_GetControlBlockFromName(DeviceName);
    
    /* Check parameters */
    if ((Time == NULL) || (HalHndl_p != ATAHandle->HalHndl))
        return TRUE;

    CurrentPioTiming = *Time;

    /* Then set the appropriate registers */
    SetPIOTiming(HalHndl_p->BaseAddress, Time);

    return Error;
}

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
/* Worker routines, used by hal_setdmatiming and hal_setpiotiming */
void hal_SetMWDMATiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p, STATAPI_MwDmaTiming_t *Timing)
{

	if(HalHndl_p->DeviceType == STATAPI_HDD_UDMA4)
	{
	    U32 *Base = (U32 *)HalHndl_p->BaseAddress;
	
	    WriteReg((U32)Base + HDDI_MWDMA_TD, Timing->NotDIoRwAssertedT);
	    WriteReg((U32)Base + HDDI_MWDMA_TH, Timing->WriteDataHoldT);
	    WriteReg((U32)Base + HDDI_MWDMA_TJ, Timing->DIoRwToDMAckHoldT);
	    WriteReg((U32)Base + HDDI_MWDMA_TKR, Timing->NotDIoRNegatedT);
	    WriteReg((U32)Base + HDDI_MWDMA_TKW, Timing->NotDIoWNegatedT);
	    WriteReg((U32)Base + HDDI_MWDMA_TM, Timing->CsToNotDIoRwT);
#if !defined (ST_8010) /* Known Bug in 8010 - register not accessible */
        WriteReg((U32)Base + HDDI_MWDMA_TN, Timing->CsHoldT);
#endif
#if defined(ATAPI_DEBUG)
	    if (TRUE == ATAPI_Verbose)
	    {
	        STTBX_Print(("HDDI_MWDMA_TD: %i\n", Timing->NotDIoRwAssertedT));
	        STTBX_Print(("HDDI_MWDMA_TH: %i\n", Timing->WriteDataHoldT));
	        STTBX_Print(("HDDI_MWDMA_TJ: %i\n", Timing->DIoRwToDMAckHoldT));
	        STTBX_Print(("HDDI_MWDMA_TKR: %i\n", Timing->NotDIoRNegatedT));
	        STTBX_Print(("HDDI_MWDMA_TKW: %i\n", Timing->NotDIoWNegatedT));
	        STTBX_Print(("HDDI_MWDMA_TM: %i\n", Timing->CsToNotDIoRwT));
#if !defined (ST_8010) /* Known Bug in 8010 - register not accessible */
	        STTBX_Print(("HDDI_MWDMA_TN: %i\n", Timing->CsHoldT));
#endif
	    }
#endif  /* ATAPI_DEBUG */
	}
}
void hal_SetUDMATiming(const ST_DeviceName_t DeviceName,hal_Handle_t *HalHndl_p, STATAPI_UltraDmaTiming_t *Timing)
{
	if(HalHndl_p->DeviceType == STATAPI_HDD_UDMA4)
	{
	    U32 *Base = (U32 *)HalHndl_p->BaseAddress;
	
	    WriteReg((U32)Base + HDDI_UDMA_RP, Timing->MinAssertStopNegateT);
	    WriteReg((U32)Base + HDDI_UDMA_ACK, Timing->AckT);
	    WriteReg((U32)Base + HDDI_UDMA_ENV, Timing->InitEnvelopeT);
#if defined(ST_5514)
	    WriteReg((U32)Base + HDDI_UDMA_TLI, Timing->LimitedInterlockT);
	    WriteReg((U32)Base + HDDI_UDMA_RFS, Timing->ReadyToFinalStrobeT);
#elif defined(ST_5528) || defined(ST_8010)
	    WriteReg((U32)Base + HDDI_UDMA_CVS, Timing->CRCSetupT);
#else
#error Unknown device!
#endif
	    WriteReg((U32)Base + HDDI_UDMA_SS, Timing->HostStrobeToStopSetupT);
	    WriteReg((U32)Base + HDDI_UDMA_ML, Timing->MinInterlockT);
	    WriteReg((U32)Base + HDDI_UDMA_DVS, Timing->DataOutSetupT);
	    WriteReg((U32)Base + HDDI_UDMA_DVH, Timing->DataOutHoldT);
	
#if defined(ATAPI_DEBUG)
	    if (TRUE == ATAPI_Verbose)
	    {
	        STTBX_Print(("HDDI_UDMA_RP: %i\n", Timing->MinAssertStopNegateT));
	        STTBX_Print(("HDDI_UDMA_ACK: %i\n", Timing->AckT));
	        STTBX_Print(("HDDI_UDMA_ENV: %i\n", Timing->InitEnvelopeT));
#if defined(ST_5514)
	        STTBX_Print(("HDDI_UDMA_TLI: %i\n", Timing->LimitedInterlockT));
	        STTBX_Print(("HDDI_UDMA_RFS: %i\n", Timing->ReadyToFinalStrobeT));
#elif defined(ST_5528)
	        STTBX_Print(("HDDI_UDMA_CVS: %i\n", Timing->CRCSetupT));
#endif
	        STTBX_Print(("HDDI_UDMA_SS: %i\n", Timing->HostStrobeToStopSetupT));
	        STTBX_Print(("HDDI_UDMA_ML: %i\n", Timing->MinInterlockT));
	        STTBX_Print(("HDDI_UDMA_DVS: %i\n", Timing->DataOutSetupT));
	        STTBX_Print(("HDDI_UDMA_DVH: %i\n", Timing->DataOutHoldT));
	    }
#endif  /* ATAPI_DEBUG */
	}
}
#endif/*#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)*/

void SetPIOTiming(volatile U32 *Base, STATAPI_PioTiming_t *Timing)
{
#if defined(ST_5514) || defined(ST_5528)|| defined(ST_8010)
    WriteReg((U32)Base + HDDI_DPIO_I, Timing->InitSequenceDelay);
    WriteReg((U32)Base + HDDI_DPIO_IORDY, Timing->IoRdySetupDelay);
    WriteReg((U32)Base + HDDI_DPIO_WR, Timing->WriteDelay);
    WriteReg((U32)Base + HDDI_DPIO_RD, Timing->ReadDelay);
    WriteReg((U32)Base + HDDI_DPIO_WREN, Timing->WriteEnableOutDelay);
    WriteReg((U32)Base + HDDI_DPIO_AH, Timing->AddressHoldDelay);
    WriteReg((U32)Base + HDDI_DPIO_WRRE, Timing->WriteRecoverDelay);
    WriteReg((U32)Base + HDDI_DPIO_RDRE, Timing->ReadRecoverDelay);

#if defined(ATAPI_DEBUG)
    if (TRUE == ATAPI_Verbose)
    {
        STTBX_Print(("HDDI_DPIO_I: %i\n", Timing->InitSequenceDelay));
        STTBX_Print(("HDDI_DPIO_IORDY: %i\n", Timing->IoRdySetupDelay));
        STTBX_Print(("HDDI_DPIO_WR: %i\n", Timing->WriteDelay));
        STTBX_Print(("HDDI_DPIO_RD: %i\n", Timing->ReadDelay));
        STTBX_Print(("HDDI_DPIO_WREN: %i\n", Timing->WriteEnableOutDelay));
        STTBX_Print(("HDDI_DPIO_AH: %i\n", Timing->AddressHoldDelay));
        STTBX_Print(("HDDI_DPIO_WRRE: %i\n", Timing->WriteRecoverDelay));
        STTBX_Print(("HDDI_DPIO_RDRE: %i\n", Timing->ReadRecoverDelay));
    }
#endif  /* ATAPI_DEBUG */
#endif
}


BOOL hal_ClearInterrupt(hal_Handle_t *HalHandle)
{
    BOOL ATA_SemSet = FALSE, ITS_Set = FALSE;
    hal_Handle_t    *The_HalHandle_p = NULL;
    
    The_HalHandle_p = HalHandle;
#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
 {
    U32 ITSVal;

    /* Read ITS to clear all pending interrupts */
    ITSVal = ReadReg((U32)HalHandle->BaseAddress + HDDI_DMA_ITS);
    if (ITSVal != 0)
    {
        if (ATAPI_Verbose)
        {
            STTBX_Print(("hal_ClearInterrupt(): read ITS value 0x%04x\n",
                        ITSVal));
        }
        ITS_Set = TRUE;
    }

    /* Call STOS_SemaphoreWaitTimeOut with immediate timeout.  This will
     * clear the semaphore, or time out if no interrupt was pending.
     */
    if (STOS_SemaphoreWaitTimeOut(The_HalHandle_p->InterruptSemaphore_p,
                               TIMEOUT_IMMEDIATE) == 0)
    {
        if (ATAPI_Verbose)
        {
            STTBX_Print(("hal_ClearInterrupt(): interrupt semaphore waiting\n"));
        }
        ATA_SemSet = TRUE;
    }
    else
    {
        /* Timeout - no semaphore was set */
        ATA_SemSet = FALSE;
    }
    }
#endif

    return (ATA_SemSet || ITS_Set);
}

/* ========================================================================
   Name:        HDDI_Change_Int_Mask
   Description: Change interrupt in HDDI_ITM register
				Note: First read ITS to clear any pending interrupts,
				otherwise CPU interrupt could be triggered by unmasking.
   ======================================================================== */

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
static BOOL hal_ChangeIntMask(hal_Handle_t *HalHndl_p,
                              U32 Interrupts_to_set)
{
	U32 Interrupt_Status;

    if (ATAPI_Verbose)
    {
        STTBX_Print(("hal_ChangeIntMask: setting mask to %i\n",
                    (~Interrupts_to_set) & 0x07));
    }
    Interrupt_Status = ReadReg((U32)HalHndl_p->BaseAddress + HDDI_DMA_ITS);
    WriteReg ((U32)HalHndl_p->BaseAddress + HDDI_DMA_ITM,
              (~Interrupts_to_set) & 0x7) ;

	if (Interrupt_Status != 0)
	{
        if (ATAPI_Verbose)
        {
            STTBX_Print(("HDDI: Change_Int_Mask. Note: ITS non-zero\r\n")) ;
        }
		return TRUE;
	}

	return FALSE;
}
#endif

#if defined(HDDI_5514_CUT_3_1)
static BOOL DoUDMAWriteRecover(hal_Handle_t *HalHndl_p,
                               U32 StartAddress, U32 WordCount)
{
    BOOL    Error = FALSE;
    U32     Base;
    U32     NewWordCount;   /* 16-bit words */
    U32     NewAddress;     /* 64 bytes from the end. */
    U32     DMAFlags;

    STTBX_Print(("Doing UDMA write workaround\n"));

    Base = (U32)HalHndl_p->BaseAddress;

    /* Workaround is to start 64 bytes from the end (since that's what
     * dropped), but program the DMA to send 96 bytes. Why this in
     * particular should work, I have no idea. See GNBvd14055 for people
     * to ask.
     */
    NewWordCount = 48;
    NewAddress = (StartAddress + (WordCount << 1)) - 64;

    WriteReg(Base + HDDI_DMA_SA, NewAddress);
    WriteReg(Base + HDDI_DMA_SI, HDDI_DMA_SI_32BYTES);
    WriteReg(Base + HDDI_DMA_WC, NewWordCount);

    /* Change ITM so we only receive IRQ */
    hal_ChangeIntMask(HalHndl_p, HDDI_DMA_ITS_IRQ);

    /* Enable DMA. We know we're not reading. */
    DMAFlags = HDDI_DMA_C_DMAENABLE | HDDI_DMA_C_STARTBIT;

    WriteReg(Base + HDDI_DMA_C, DMAFlags);

    /* Set this, so things (eg interrupt handler) know the transfer type */
    HalHndl_p->DmaTransfer = TRUE;

    /* And set the mode - this triggers the start of the transfer */
    WriteReg(Base + HDDI_MODE,
             (DMAIsUDMA == TRUE)?HDDI_MODE_UDMA:HDDI_MODE_MWDMA);

    /* Remove start bit from control, else transfer terminates
     * prematurely
     */
    DMAFlags &= ~HDDI_DMA_C_STARTBIT;
    WriteReg(Base + HDDI_DMA_C, DMAFlags);

    /* Wait for another interrupt for 100us */
    Error = hal_AwaitInt(HalHndl_p, ST_GetClocksPerSecond() / 100);
    if (Error == FALSE)
    {
        STTBX_Print(("Successful recovery\n"));
        WriteRecoverGood++;
    }
    else
    {
        STTBX_Print(("Unsuccessful recovery\n"));
        WriteRecoverBad++;
    }

    return Error;
}
#endif /* HDDI_5514_CUT_3_1 */

BOOL hal_DmaDataBlock(hal_Handle_t *HalHndl_p, U8 DevCtrl, U8 DevHead,
                      U16 *StartAddress, U32 WordCount, U32 BufferSize,
                      U32 *BytesRW, BOOL Read, BOOL *CrcError)
{
    BOOL Error = FALSE;

#if defined(ST_5514) || defined(ST_5528) ||  defined(ST_7100) || defined(ST_7109) || defined(ST_8010)
    U32             IntFlags = 0;
    U32             Count;          /* Used for limiting waitloops */
    U32             DMAFlags = 0;
    U32             BytesLeft = 0;
    U32             Base;
    U32             Status;
    U32             Dummy;
    U32             InternalWordCount;
    U32             StatusVal;
    BOOL 			RetError=FALSE;/* Bug 50151 STSA*/
    hal_Handle_t    *The_HalHandle_p = NULL;

    Base = (U32)HalHndl_p->BaseAddress;
    The_HalHandle_p = HalHndl_p;

#if defined(ST_7100) || defined(ST_7109)

   if(HalHndl_p->DeviceType == STATAPI_SATA)
   {
   	 	U32  data = 0;
   	 	U32  *tempBuffer;
#if defined(ARCHITECTURE_ST40)
    	tempBuffer = (U32 *)((U32)StartAddress & ~0xe0000000);
#endif

		/*enable the DMAC*/
		WriteReg(((U32)Base + DMA_CFG_REG), DMA_CFG_REG_ENABLE);
		hal_SATA_Int_Clear((U32) Base);
		WordCount = WordCount / 2 ;
		*CrcError = FALSE;

	    if(Read==TRUE)/*in case of DMAin*/
	    {
	         WriteReg(((U32)Base + SAR0),((U32)Base + HOST_BASE_ADDRESS));
	         WriteReg(((U32)Base + DAR0),tempBuffer);
	         WriteReg(((U32)Base + CTL0L), DMA_READ_CONTROL_L);
	         data=WordCount & 0x0fff;
	         WriteReg(((U32)Base + CTL0H),data);
	         WriteReg(((U32)Base + DBTSR), DMA_DBTSR);
	         WriteReg(((U32)Base + CFG0L), DMA_READ_CONFIGURATION_L);
	         WriteReg((U32)Base + DMACR, DMA_DMACR);/*enable transmit channel
	                                          for DMA transfer*/
	    }
   	    else /* DMA Write*/
    	{

	         WriteReg(((U32)Base + SAR0),tempBuffer);
	         WriteReg(((U32)Base + DAR0),((U32)Base + HOST_BASE_ADDRESS));
	         WriteReg(((U32)Base + CTL0L),DMA_WRITE_CONTROL_L);
	         data=WordCount & 0x0fff;
	         WriteReg(((U32)Base + CTL0H),data);
	         WriteReg(((U32)Base + DBTSR), DMA_DBTSR);
	         WriteReg(((U32)Base + CFG0L), DMA_WRITE_CONFIGURATION_L);
	         WriteReg(((U32)Base + DMACR), DMA_DMACR);/*enable receive channel
	                                             for DMA transfer*/
    	}
	    /*Configuration Register*/
	    WriteReg(((U32)Base + CFG0H), DMA_CONFIGURATION_H);
   	   /* Enable DMA interrupts*/
	    WriteReg(((U32)Base + CH_EN_REG),DMA_CHANNEL_ENABLE);
        HalHndl_p->DmaTransfer = TRUE;
        
        Error = hal_AwaitInt(HalHndl_p, INT_TIMEOUT );
		    	
   		if (Error == TRUE)
        {
	        STTBX_Print(("Interrupt timeout\n"));
	        if(ReadReg((U32)Base + SCR1) != 0)
	        {
		         /*Check for any CRC error*/
		     	 /*read the SERROR Register*/
				 Status = ReadReg((U32)Base + SCR1);
				 if ((Status & SATA_CRC_ERROR) == SATA_CRC_ERROR)
		         	 *CrcError = TRUE;
				 WriteReg(((U32)Base + SCR1), Status);	/* clearing the SCR1 register*/
				 hal_DmaAbort(The_HalHandle_p);
 	        } 
        }

#if defined(STATAPI_HOSTCERROR_INTERRUPT_ENABLE)	
        STTBX_Print(("SATA Error Register:SCR1 Value = 0x%x \n",HalHndl_p->SATAError));
        Status = HalHndl_p->SATAError;	         
        if((Status & SATA_AHB_ILLEGALACCESS_ERROR) == SATA_AHB_ILLEGALACCESS_ERROR)
        {
        	Error = TRUE; 
            STTBX_Print(("SATA AHB Bus Error has occured = 0x%x \n", Status));
            return Error;

        }
        if((Status & SATA_DATA_INTEGRITY_ERROR) == SATA_DATA_INTEGRITY_ERROR)
        {
        	Error = TRUE;
        	STTBX_Print(("Disparity,8b/10 decoder error,CRC errors = 0x%x \n", Status));
        	return Error;
        }   
     
        if((Status & SATA_DISPARITY_ERROR) == SATA_DISPARITY_ERROR)
        {
        	Error = TRUE;
        	STTBX_Print(("Disparity errors = 0x%x \n", Status));
        	return Error;
        }
        if((Status & SATA_10b8b_DECODER_ERROR) == SATA_10b8b_DECODER_ERROR)
        {
        	Error = TRUE;
        	STTBX_Print(("8b/10b Decoder errors = 0x%x \n", Status));
        	return Error;
        }
 
#endif           
   		*BytesRW +=(( ReadReg((U32)Base + CTL0H))& 0x0fff) * 4 ;
   		 return Error;
   	}/*if*/

#else /*not SATA*/

    while (hal_ClearInterrupt(HalHndl_p)) { }

    /* Figure out which interrupts are required... */
#if defined(HDDI_5514_CUT_3_1)
    IntFlags = HDDI_DMA_ITM_IRQ;
    if ((DMAIsUDMA == TRUE) && (Read == FALSE))
        IntFlags |= HDDI_DMA_ITM_DEND;
#else
    /* DEND is unreliable for 3.2; don't remember for earlier cuts.
     * GNBvd13621, GNBvd15299.
     */
    /* Also that way for 5528, apparently */
    IntFlags = HDDI_DMA_ITM_IRQ;
#endif

    /* Note: we're trusting the cell is in PIO mode at function entry.  */
    /* Added when poring for 8010 */

    WriteReg(Base + HDDI_MODE, HDDI_MODE_PIOREG );

    hal_ChangeIntMask(HalHndl_p, IntFlags);

    HalHndl_p->StoredByteCount = 0;
    HalHndl_p->DmaAborted = FALSE;
    *CrcError = FALSE;

    /* Take a local copy of this, in case we have to modify it below */
    InternalWordCount = WordCount;

#if defined(HDDI_5514_CUT_3_1) || defined(HDDI_5514_CUT_3_2)
    /* Compensate for GNBvd15724, "MWDMA does not terminate for certain
     * devices". This is done by adding an extra 128 words for each
     * MWDMA write. These words are not sent, but prevent the HDDI
     * terminating too early. See the DDTS for details.
     */
    if ((DMAIsUDMA == FALSE) && (Read == FALSE))
    {
        InternalWordCount += 128;
    }
#endif

    /* Command is already programmed by the layer above us; we just have
     * to set up the DMA.
     */
    if (ATAPI_Verbose)
    {
        STTBX_Print(("Writing 0x%08x to HDDI_DMA_SA\n", StartAddress));
        STTBX_Print(("Writing %i to HDDI_DMA_WC\n", InternalWordCount));
    }
#if defined(ARCHITECTURE_ST40)
    StartAddress = (U16 *)((U32)StartAddress & ~0xe0000000);
#endif
    WriteReg(Base + HDDI_DMA_SA, StartAddress);
    WriteReg(Base + HDDI_DMA_SI, HDDI_DMA_SI_32BYTES);
    WriteReg(Base + HDDI_DMA_WC, InternalWordCount);
#if defined(ST_5528)
    WriteReg(Base + HDDI_DMA_CHUNK, HDDI_DMA_CHUNK_1PACKET);
#endif

    /* Enable DMA */
    DMAFlags = HDDI_DMA_C_DMAENABLE | HDDI_DMA_C_STARTBIT;
    if (Read == TRUE)
    {
        DMAFlags |= HDDI_DMA_C_READNOTWRITE;
    }

    if (ATAPI_Verbose)
    {
        STTBX_Print(("Writing 0x%08x to HDDI_DMA_C\n", DMAFlags));
#if defined(ATAPI_DEBUG)
        STTBX_Print(("Interrupt count: %i\n", atapi_intcount));
#endif
        STTBX_Print(("Setting HDDI_MODE to %s\n",
                    (DMAIsUDMA == TRUE)?"HDDI_MODE_UDMA":"HDDI_MODE_MWDMA"));

    }
    WriteReg(Base + HDDI_DMA_C, DMAFlags);

    /* Set this, so things (eg interrupt handler) know the transfer type */
    HalHndl_p->DmaTransfer = TRUE;

    /* And set the mode - this triggers the start of the transfer */
    WriteReg(Base + HDDI_MODE,
             (DMAIsUDMA == TRUE)?HDDI_MODE_UDMA:HDDI_MODE_MWDMA);

    /* Remove start bit from control, else transfer terminates
     * prematurely
     */
    DMAFlags &= ~HDDI_DMA_C_STARTBIT;
    WriteReg(Base + HDDI_DMA_C, DMAFlags);

    /* Now let's see if we get an interrupt. */
    Error = hal_AwaitInt(HalHndl_p, INT_TIMEOUT * 3);
    if (ATAPI_Verbose)
    {
        STTBX_Print(("DMA status: 0x%04x\n",
                    ReadReg(Base + HDDI_DMA_STA)));
        STTBX_Print(("Bytes remaining: %i\n",
                    ReadReg(Base + HDDI_DMA_CB)));
#if defined(ATAPI_DEBUG)
        STTBX_Print(("Interrupts received: %i\n", atapi_intcount));
#endif
        STTBX_Print(("IEN: 0x%04x\n", ReadReg(Base + HDDI_DMA_ITM)));
    }

    /* Switch back to PIO/register mode, if we timed out rather than got an
     * interrupt.
     */
    if (Error == TRUE)
    {
        STTBX_Print(("Interrupt timeout\n"));
#if defined(ATAPI_DEBUG)
        if (ATAPI_Verbose)
        {
            U32 i;
            for (i = 0; i < atapi_intcount; i++)
            {
                STTBX_Print(("%i %04x\n", i, atapi_inttrace[i]));
            }
            STTBX_Print(("DMA status: 0x%04x\n",
                    ReadReg((U32)HalHndl_p->BaseAddress + HDDI_DMA_STA)));
        }
#endif
        Count = 0;
        while ((ReadReg(Base + HDDI_MODE) != HDDI_MODE_PIOREG) &&
               (Count < COUNT_WAIT_LIMIT))
        {
#if defined(ATAPI_DEBUG)
            if (ATAPI_Verbose)
            {
                STTBX_Print(("Writing %08x to %08x\n", HDDI_MODE_PIOREG,
                            Base + HDDI_MODE));
            }
#endif
            WriteReg(Base + HDDI_MODE, HDDI_MODE_PIOREG);
            Count++;
        }

        if (Count >= COUNT_WAIT_LIMIT)
        {
            /* Now unsafe to access any device registers... */
            STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
                         "!!! Failed to reset to PIO mode - 0x%x !!!\n",
                         ReadReg(Base + HDDI_MODE)));
            Error = TRUE;
            HalHndl_p->DmaTransfer = FALSE;
            /* Slightly ugly, but reduces nesting substantially below.
             * Note: should we leave DmaTransfer as true, to indicate
             * that you can't access device registers?
             */
            goto dmablock_end;
        }
    }

#if defined(HDDI_5514_CUT_3_1)
    /* Got interrupt, so check how everything's going. */
    if ((HDDI_ITS_Value & (HDDI_DMA_ITS_DEND | HDDI_DMA_ITS_IRQ)) ==
            (HDDI_DMA_ITS_DEND | HDDI_DMA_ITS_IRQ))
    {
        /* Got both interrupts */
        if (ATAPI_Verbose)
        {
            STTBX_Print(("Received DEND and IRQ\n"));
        }
    }
    else
    {
        if (HDDI_ITS_Value & HDDI_DMA_ITS_DEND)
        {
            /* If we're doing UDMA write, check CB value */
            if ((DMAIsUDMA == TRUE) && (Read == FALSE))
            {
                if (ReadReg(Base + HDDI_DMA_CB) > 0)
                {
                    /* Bug GNBvd14055, 64bytes short. Note: the byte
                     * count is short one word, hence 0x3e.
                     */
                    if (ReadReg(Base + HDDI_DMA_CB) == 0x3e)
                    {
                        Error = DoUDMAWriteRecover(HalHndl_p,
                                    (U32)StartAddress, WordCount);
                        if (Error == TRUE)
                        {
                            STTBX_Print(("Write recovery failed\n"));
                        }
                    }
                    else
                    {
                        /* Normally would raise error, but may be
                         * another bug in CB counting - need to
                         * check. (GNBvd14826)
                         */
                    }
                }
            } /* UDMA write? */
        } /* DEND interrupt? */

        if (HDDI_ITS_Value & HDDI_DMA_ITS_IRQ)
        {
            /* Currently, do nothing. */
        }
    } /* Got both DEND and IRQ? */
#endif

    /* Required for both 5528 and 5514, but they need us to wait for
     * different things. Split into another function to make it clearer.
     * hal_WaitDMAEngines returns a value to say whether it's all okay
     * or not, but realistically there's nothing we can do if it isn't.
     */
     /* Bug 50151 STSA*/
     RetError= hal_WaitDMAEngines(Base, DMAIsUDMA, Read);

#if defined(HDDI_5514_CUT_3_2)
    /* Wait for the CA to become zero, to ensure HDDI has finished. All
     * MWDMA transfers, and UDMA write. (Bug 15302 means it's not safe
     * to use CA for UDMA read.)
     */
    if ((DMAIsUDMA == FALSE) ||
        (DMAIsUDMA == TRUE) && (Read == FALSE))
    {
        Count = 0;
        if (ATAPI_Verbose)
        {
            STTBX_Print(("Waiting for HDDI_DMA_C to clear\n"));
        }
        while ((ReadReg(Base + HDDI_DMA_CA) != 0) &&
               (Count < COUNT_WAIT_LIMIT))
        {
            Count++;
        }
        if (ATAPI_Verbose)
        {
            STTBX_Print(("HDDI_DMA_CA: %i; Count: %i\n",
                        ReadReg(Base + HDDI_DMA_CA), Count));
        }

        if (Count >= COUNT_WAIT_LIMIT)
        {
            STTBX_Print(("Error - HDDI_DMA_CA non-zero\n"));
        }
    }
#endif

    /* Now safe to switch back to PIO mode */
    Count = 0;
    while ((ReadReg(Base + HDDI_MODE) != HDDI_MODE_PIOREG) &&
           (Count < COUNT_WAIT_LIMIT))
    {
        if (ATAPI_Verbose)
        {
            STTBX_Print(("Writing %08x to %08x\n", HDDI_MODE_PIOREG,
                        Base + HDDI_MODE));
        }
        WriteReg(Base + HDDI_MODE, HDDI_MODE_PIOREG);
        Count++;
    }

    if (Count >= COUNT_WAIT_LIMIT)
    {
        /* Now unsafe to access any device registers... */
        STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
                     "!!! Failed to reset to PIO mode - 0x%x !!!\n",
                     ReadReg(Base + HDDI_MODE)));
        Error = TRUE;
    }
    else
    {
        HalHndl_p->DmaTransfer = FALSE;

        /* Read (and discard) ASR - this satisfies the ATA spec
         * requiring a delay of at least 1 PIO mode cycle before reading
         * status register
         */
        Dummy = hal_RegInByte (The_HalHandle_p,ATA_REG_ALTSTAT);

        /* Check if an error or similar occurred */
        Status = hal_RegInByte (The_HalHandle_p, ATA_REG_STATUS);
        if ((Status & HDDI_ATA_SR_ERR) == HDDI_ATA_SR_ERR)
        {
            STTBX_Print(("Status: 0x%02x\n", Status));
            Status = hal_RegInByte(The_HalHandle_p, ATA_REG_ERROR);
             /*STSA*/
            if((Status & (HDDI_ATA_ERR_UNC)) != 0)
            {
                 HalHndl_p->DmaAborted = TRUE;
            }
            STTBX_Print(("Error: 0x%02x\n", Status));

            /* Set CRC flag if required, so caller can know whether or not
             * retries are an option.
             */
            if ((Status & HDDI_ATA_ERR_ICRC) == HDDI_ATA_ERR_ICRC)
            {
                *CrcError = TRUE;
                 Error = TRUE;
            }
        }

        /* Read bytes left to go, and work out how many were
         * transferred. Note that since the register is reset to 0 when
         * DMA is stopped or reset, then it will indicate no bytes went,
         * even if they have.
         */
        BytesLeft = ReadReg((U32)HalHndl_p->BaseAddress + HDDI_DMA_CB);
        if (BytesLeft > 0)
        {
            STTBX_Print(("Bytes left: %i\n", BytesLeft));
        }

#if   defined(HDDI_5514_CUT_3_0)
        /* Bug in MWDMA implementation on cut 3 - CB doesn't return true value
         * when writing. Adjust bytes down by 2(N-1), if the remainder is
         * that high.
         */
        if ((DMAIsUDMA == FALSE) && (Read == FALSE))
        {
            U32 temp = 2 * ((WordCount / 256) - 1);
            if (BytesLeft >= temp)
                BytesLeft -= temp;
        }
#elif defined(HDDI_5514_CUT_3_1)
        /* Another bug found since means that we cannot rely at all on
         * the byte count value for MWDMA. We're going to have to zero
         * the bytes left, and assume the user is checking error values.
         * (Every time the device pauses the transfer, the HDDI
         * miscounts a word.) Note: could maybe check CA?
         */
        if (DMAIsUDMA == FALSE)
            BytesLeft = 0;
#elif defined(HDDI_5514_CUT_3_2)
        /* The application note for 3.2 would seem to indicate (p7, p9,
         * GNBvd10320, GNBvd14826) that the byte counter is unreliable
         * for both MWDMA *and* UDMA writes. This may need checking.
         */
        BytesLeft = 0;
#endif

        /* If the interrupt routine (or Abort) signalled a DMA abort,
         * then set the error flag.
         */
         if (HalHndl_p->DmaAborted == TRUE)
         {
            Error = TRUE;
            /*STSA*/
            StatusVal  = ReadReg((U32)Base + HDDI_DMA_C);
            StatusVal |= HDDI_DMA_C_STOPBIT;
            WriteReg(Base + HDDI_DMA_C, StatusVal);
            hal_ResetInterface();
            if (ATAPI_Verbose)
            {
                STTBX_Print(("DMA Abort detected\n"));
            }
         }

        *BytesRW = (WordCount * 2) - (BytesLeft + HalHndl_p->StoredByteCount);
        if (ATAPI_Verbose)
        {
            STTBX_Print(("Bytes transferred: %i\n", *BytesRW));
        }

    }

    if ((hal_ClearInterrupt(HalHndl_p) != FALSE) && (ATAPI_Verbose == TRUE))
    {
        STTBX_Print(("Note - HDDI interrupt and/or semaphore needed clearing\n"));
    }

dmablock_end:
#endif /*sata*/

#else
    Error = TRUE;
#endif

    return Error;
}

void hal_AfterDma (hal_Handle_t *HalHndl_p)
{
    /* Not required for current interfaces (all exit paths should switch
     * back to PIO mode if required), but might be useful in the future.
     */
}

/* Write to HDDI control registers to pause DMA */
void hal_DmaPause (hal_Handle_t *HalHndl_p)
{
#if defined(HDDI_5514_CUT_3_2)
    STTBX_Print(("Call to hal_DmaPause - unsafe, GNBvd07905, 07918\n"));
#elif defined(ST_5514) || defined(ST_5528)  || defined(ST_8010)
    U32 ByteCount;
    volatile U32 Status;

    ByteCount = ReadReg((U32)HalHndl_p->BaseAddress + HDDI_DMA_CB);
    HalHndl_p->StoredByteCount += ByteCount;

    Status = ReadReg((U32)HalHndl_p->BaseAddress + HDDI_DMA_C);
    Status &= (~HDDI_DMA_C_DMAENABLE & HDDI_DMA_C_MASK);
    WriteReg((U32)HalHndl_p->BaseAddress + HDDI_DMA_C, Status);

#if defined(HDDI_5514_CUT_3_0) || \
    defined(HDDI_5514_CUT_3_1) || \
    defined(HDDI_5514_CUT_3_2)
    /* Bug in MWDMA implementation on cut 3 - CB doesn't return true value
     * when writing. Note: bug found since means that we can't rely at
     * all on CB, but I don't really see we have any choice here, we'll
     * have to at least make a stab.
     */
    if ((DMAIsUDMA == FALSE) && ((Status & HDDI_DMA_C_READNOTWRITE) == 0))
    {
        U32 temp = 2 * ((ByteCount / SECTOR_BSIZE) - 1);
        HalHndl_p->StoredByteCount += temp;
    }
#endif

#endif
}

/* Write to HDDI control registers to resume DMA */
void hal_DmaResume (hal_Handle_t *HalHndl_p)
{
#if defined(HDDI_5514_CUT_3_2)
    STTBX_Print(("Call to hal_DmaPause - unsafe, GNBvd07905, 07918\n"));
#elif defined(ST_5514) || defined(ST_5528)  || defined(ST_8010)
    volatile U32 Status;

    Status = ReadReg((U32)HalHndl_p->BaseAddress + HDDI_DMA_C);
    Status |= HDDI_DMA_C_DMAENABLE;
    WriteReg((U32)HalHndl_p->BaseAddress + HDDI_DMA_C, Status);
#endif
}

/* Abort DMA transfer - requires transfer to have been paused first */
BOOL hal_DmaAbort (hal_Handle_t *HalHndl_p)
{
    BOOL            Error = FALSE;

#if defined(ST_5514) || defined(ST_5528) || defined(ST_8010)
    {
	    volatile U32 Status;
	    HalHndl_p->StoredByteCount = 0;
	    Status = ReadReg((U32)HalHndl_p->BaseAddress + HDDI_DMA_C);
	    if ((Status & HDDI_DMA_C_DMAENABLE) != 0)
	    {
	        /* Must be paused before you can terminate. */
	        Error = TRUE;
	    }
	    else
	    {
	        Status |= HDDI_DMA_C_STOPBIT;
	        WriteReg((U32)HalHndl_p->BaseAddress + HDDI_DMA_C, Status);
	        HalHndl_p->DmaAborted = TRUE;
	    }
	}
#elif defined(ST_7100) || defined(ST_7109)
	if(HalHndl_p->DeviceType == STATAPI_SATA)
	{
		WriteReg(((U32)HalHndl_p->BaseAddress + DMA_CFG_REG), DMA_CFG_REG_DISABLE);
	    while ((ReadReg((U32)HalHndl_p->BaseAddress  + CH_EN_REG) & CHANNEL_ENABLED) == CHANNEL_ENABLED)	/*Channel is not disbled*/
	   	  WriteReg(((U32)HalHndl_p->BaseAddress + CH_EN_REG), 0x100);
	      /*disable the receive channel if it is enabled*/
	    WriteReg(((U32)HalHndl_p->BaseAddress + DMACR), DMACR_DISABLE);
	    HalHndl_p->DmaAborted = TRUE;
    }
    else
         Error = TRUE;
     
#else
         Error = TRUE;
#endif         
    return Error;
}


/* Resets the HDD interface, if one is available; else does nothing.
 *
 * ==NOTE==
 *
 * This function is not portable to different chips without changing the
 * hardcoded values. Yes, it's somewhat evil, but at this time probably
 * a better alternative than pulling in STCFG purely for a HAL function.
 */
void hal_ResetInterface(void)
{
    /* XXX This should be supported on 5528 also - write me */
#if defined(ST_5514)
#pragma ST_device (CfgReg)
    /* We want this particular offset */
    U32 *CfgReg = (U32 *)(CFG_BASE_ADDRESS + 0x0c);

    /* Pulse HDDI_RESET high for >= 1us */
    *CfgReg |= 0x40000;
    /* Wait for a bit */
    task_delay(5);
    /* Set bit low */
    *CfgReg &= ~0x40000;
#endif
}
/****************************************************************************
Name         : hal_SetRegisterMap

Description  : Overrides the default register map for this platform

Parameters   :
    RegMap      The register map to use instead

Return Value :
    None.
****************************************************************************/
void hal_SetRegisterMap(const STATAPI_RegisterMap_t *RegMap)
{
    RegsMasks[0]  = RegMap->AlternateStatus;
    RegsMasks[1]  = RegMap->Data;
    RegsMasks[2]  = RegMap->Error;
    RegsMasks[3]  = RegMap->Error;              /* Feature */
    RegsMasks[4]  = RegMap->SectorCount;
    RegsMasks[5]  = RegMap->SectorNumber;
    RegsMasks[6]  = RegMap->CylinderLow;
    RegsMasks[7]  = RegMap->CylinderHigh;
    RegsMasks[8]  = RegMap->DeviceHead;
    RegsMasks[9]  = RegMap->Status;
    RegsMasks[10] = RegMap->Status;             /* Command */
    RegsMasks[11] = RegMap->AlternateStatus;    /* Control */
}

/****************************************************************************
Name         : hal_GetRegisterMap

Description  : retrieves the default register map for this platform

Parameters   :
    RegMap      The register map to use instead

Return Value :
    None.
****************************************************************************/
void hal_GetRegisterMap(STATAPI_RegisterMap_t *RegMap)
{
    RegMap->AlternateStatus = RegsMasks[0];
    RegMap->Data            = RegsMasks[1];
    RegMap->Error           = RegsMasks[2];
    RegMap->SectorCount     = RegsMasks[4];
    RegMap->SectorNumber    = RegsMasks[5];
    RegMap->CylinderLow     = RegsMasks[6];
    RegMap->CylinderHigh    = RegsMasks[7];
    RegMap->DeviceHead      = RegsMasks[8];
    RegMap->Status          = RegsMasks[9];
}

#if defined(ST_5514) || defined(ST_5528) ||  defined(ST_7100) || defined(ST_7109) || defined(ST_8010)
static BOOL hal_WaitDMAEngines(U32 Base, BOOL DMAIsUDMA, BOOL Read)
{
    BOOL Error = FALSE;
    U32 Count;

#if defined(ST_5514)
    /* If doing a UDMA read, we have to make sure the HDDI has finished
     * transferring data. Enter yet another workaround for bug
     * GNBvd13621.
     */
    if ((DMAIsUDMA == TRUE) && (Read == TRUE))
    {
        Count = 0;
        /* Wait for bits 8:10 to become 0 */
        while ((ReadReg(Base + HDDI_DMA_STA) & 0x700) &&
               (Count < COUNT_WAIT_LIMIT))
        {
            Count++;
        }

        if (Count >= COUNT_WAIT_LIMIT)
        {
            STTBX_Print(("Error - HDDI_DMA_STA 10:8 non-zero\n"));
            Error = TRUE;
        }
    }
#elif defined(ST_5528)
    /* Wait for bits 15:5 to become 0. The ones which weren't involved
     * in the current transfer should be 0 to start with; the others
     * should become 0 when everything's finished.
     */

    Count = 0;
    while ((ReadReg(Base + HDDI_DMA_STA) & 0xffe0) &&
           (Count < COUNT_WAIT_LIMIT))
    {
        Count++;
    }

    if (Count >= COUNT_WAIT_LIMIT)
    {
        STTBX_Print(("Error - HDDI_DMA_STA 15:5 non-zero\n"));
        Error = TRUE;
    }
#endif
    return Error;
}
#endif
/*end of hal_atapi.c --------------------------------------------------*/
