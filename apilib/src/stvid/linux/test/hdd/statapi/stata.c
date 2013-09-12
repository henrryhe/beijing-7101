/*******************************************************************************

File name   : stata.c

Description : ST api mode pio4 interface

COPYRIGHT (C) ST-Microelectronics 1999-2000.

Date               Modification                          Name
----               ------------                          ----
 02/11/00          Created                               FR
 08/01/00          Adapted for STVID (STAPI)             AN
 25 May 01         Rename statapi.h in stata.h           FQ

*****************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/*#define TRACE_DECODE*/
/* Includes ----------------------------------------------------------------- */
#include <string.h>
#include <interrup.h>
#include <semaphor.h>
#include <ostime.h>

#include "stddefs.h"
#include "sttbx.h"

#ifdef DVD_TRANSPORT_STPTI
#include "stpti.h"
#else
#include "pti.h"
#endif
#ifdef DVD_TRANSPORT_LINK /* workaround for link 2.0.1 */
#include "ptilink.h"
#endif

#include "statareg.h"
#include "stata.h"

/* Enable the block move DMA. */
#ifndef ST_5514
    /* does not exists on 5514, and since this is legacy only, we don't care */
    #define BMDMA_ENABLE
#endif


/* Private Types ------------------------------------------------------------ */
typedef struct
{
    U32             InterruptNumber;
    U32             InterruptLevel;
    partition_t*    CPUPartition_p;
    U32             CallBackEvents;
    void            (*CallBackFunction) (U32 Type, U32 Value);
    semaphore_t     InterruptSemaphore;
    semaphore_t     Mutex;
    semaphore_t     BlockMoveMutex;
    BOOL            WantAnInterrupt;
#ifdef BMDMA_ENABLE
    semaphore_t     BMDMA_IntSemaphore;
#endif
}   ATA_Handle_t;

/* Private Constants -------------------------------------------------------- */
#define ATATIMEOUT 500000

/* Private Variables (static)------------------------------------------------ */
static BOOL ATA_FirstInit = TRUE;

/* Global Variables --------------------------------------------------------- */
ATA_Handle_t    ATA_Handle;

/* Private Macros ----------------------------------------------------------- */
#define DRV_BSY         ( ( ATA_REG_STATUS & ATA_REG_STATUS_BSY ) == ATA_REG_STATUS_BSY )
#define DRV_RDY         ( ( ATA_REG_STATUS & ATA_REG_STATUS_RDY ) == ATA_REG_STATUS_RDY )
#define DRV_ERR         ( ( ATA_REG_STATUS & ATA_REG_STATUS_ERR ) == ATA_REG_STATUS_ERR )
#define DRV_DRQ         ( ( ATA_REG_STATUS & ATA_REG_STATUS_DRQ ) == ATA_REG_STATUS_DRQ )
#define DRV_SKC         ( ( ATA_REG_STATUS & ATA_REG_STATUS_SKC ) == ATA_REG_STATUS_SKC )


#ifndef BMDMA_ENABLE  /* 5514 for example */
void ReadBytes ( U32 Address, U32 Size )
{
    /*
    U16 * PStart = (U16*) (Address);
    U16 * PEnd   = (U16*) (Address+Size);
    for (  ; PStart!=PEnd ; *PStart++ = *(DU16*) ATA_REG_DATA );
    */
    U32 Lines = Size / 2;

    __optasm { ldabc Lines,2,0; move2dinit;
               ldabc 2, Address, (U16)ATA_REG_DATA; move2dall; }


}
void WriteBytes ( U32 Address, U32 Size )
{
    /*
    U16 * PStart = (U16*) (Address);
    U16 * PEnd   = (U16*) (Address+Size);
    for (  ; PStart!=PEnd ; *(DU16*) ATA_REG_DATA = *PStart++ );
    */
    U32 Lines = Size / 2;

    __optasm { ldabc Lines,0,2;               move2dinit;
               ldabc 2,ATA_REG_DATA, Address; move2dall;  }
}
#endif

#define ATA_WaitCondition(TimeOut,Cond) {       \
U16 Dummy;                                      \
U32 TOut = (U32) TimeOut;                       \
while ( (!(Cond))  &&  (TOut>0) ) { TOut --; Dummy = ATA_REG_ALTSTAT; }  \
if (TOut==0) {STTBX_Report((STTBX_REPORT_LEVEL_INFO, "TIME OUT : %x\r\n",ATA_REG_STATUS)); return (STATA_ERROR_TIMEOUT); }\
}\


#ifdef BMDMA_ENABLE
#define    BLOCK_MOVE_INT_LEVEL     6
#ifndef BLOCK_MOVE_INTERRUPT
#define    BLOCK_MOVE_INTERRUPT     15
#endif

#define BMDMA_SrcAddress  0x20026000
#define BMDMA_DestAddress 0x20026004
#define BMDMA_Count       0x20026008
#define BMDMA_IntEn       0x2002600C
#define BMDMA_Status      0x20026010
#define BMDMA_IntAck      0x20026014
#endif


/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t ATA_HardReset ( void );
static ST_ErrorCode_t ATA_SoftReset(void);
static ST_ErrorCode_t ATA_InitInterface ( void );
static ST_ErrorCode_t ATA_DeviceSelect(void);
static ST_ErrorCode_t ATA_SetFeature(U8 Command, U8 SubCommand);
static ST_ErrorCode_t ATA_SetMultipleMode (U8 SectorNumber);
static ST_ErrorCode_t ATA_WaitForInterrupt(void);
static ST_ErrorCode_t ATA_ClearInterrupt (void);
static void ATA_InterruptHandler(void);

#ifdef BMDMA_ENABLE
static void ata_BlockMoveIntHandler(void);
static void ATA_BMDMA(void *Source, void *Destination, U32 Size);
#endif

/* Public Functions --------------------------------------------------------- */


/*******************************************************************************
Name        : STATA_GetRevision
Description : Returns the current version of the ATA driver
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_Revision_t   STATA_GetRevision( void )
{
    static char     Revision[] = "ATA PIO mode driver 0.0.0";
    return ((ST_Revision_t) Revision);
} /* end of STATA_GetRevision() */

/*******************************************************************************
Name        : STATA_Init
Description : Initializes the current ATA driver
Parameters  : Init parameters
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t  STATA_Init( STATA_InitParams_t *InitParams_p)
{
    ST_ErrorCode_t ErrCode;

    if ( ATA_FirstInit == TRUE )
    {
        semaphore_init_fifo(&ATA_Handle.Mutex, 1);
#ifdef ST_5514
        /* clear IT mask */
        HDD_DMA_ITM  = 0;
        ErrCode = HDD_DMA_ITS;
#endif

        ATA_Handle.InterruptNumber  = InitParams_p->InterruptNumber;
        ATA_Handle.InterruptLevel   = InitParams_p->InterruptLevel;
        ATA_Handle.CPUPartition_p   = InitParams_p->CPUPartition_p;
        ATA_Handle.CallBackEvents   = InitParams_p->CallBackEvents;
        ATA_Handle.CallBackFunction = InitParams_p->CallBackFunction;
        ATA_Handle.WantAnInterrupt  = FALSE;
        interrupt_install( ATA_Handle.InterruptNumber,
                           ATA_Handle.InterruptLevel,
                           (void(*)(void*))ATA_InterruptHandler,
                           NULL);
#if defined(ST_5514)
        /* interrupt_trigger_mode_number  ( ATAHndl.InterruptNumber, interrupt_trigger_mode_high_level ); */
        interrupt_enable_number        ( ATA_Handle.InterruptNumber );
#else
        interrupt_enable ( ATA_Handle.InterruptLevel );
#endif

#ifdef ST_5514
        /* clear IT mask */
        HDD_DMA_ITM  = 5; /* only get HDD interrupt */
#endif

        /* Semaphore for interrupt handler signal */
        semaphore_init_fifo_timeout (&ATA_Handle.InterruptSemaphore, 0);
        ATA_FirstInit = FALSE;

#ifdef BMDMA_ENABLE
        /*create interrupt semaphore*/
        semaphore_init_fifo_timeout(&ATA_Handle.BMDMA_IntSemaphore,0);
        /*install the interrupt handler*/
        if(interrupt_install( BLOCK_MOVE_INTERRUPT,BLOCK_MOVE_INT_LEVEL,
                            (void(*)(void*))ata_BlockMoveIntHandler,NULL))

        interrupt_enable(BLOCK_MOVE_INT_LEVEL );
#endif
    }

    /* then hardware */
    ErrCode = ATA_InitInterface();

    return ( ErrCode );
} /* end of STATA_Init() */


/*******************************************************************************
Name        : STATA_Term
Description : Terminates the ATA instance
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t  STATA_Term( void )
{
    /* delete semaphores */
    semaphore_delete(&ATA_Handle.Mutex);
    semaphore_delete(&ATA_Handle.InterruptSemaphore);
    /* uninstall the interrupt handlers */
    interrupt_uninstall(ATA_Handle.InterruptNumber, ATA_Handle.InterruptLevel);
    ATA_FirstInit = TRUE;

  #ifdef BMDMA_ENABLE
    /* Delete interrupt semaphore*/
     semaphore_delete(&ATA_Handle.BMDMA_IntSemaphore);
    /* uninstall the interrupt handler*/
    if(interrupt_uninstall( BLOCK_MOVE_INTERRUPT,BLOCK_MOVE_INT_LEVEL))
    {
        return TRUE;
    }
   #endif

    return ( ST_NO_ERROR );
} /* end of STATA_Term() */


/*******************************************************************************
Name        : ATA_HardReset
Description : Applies a hard reset of ATA interface
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t ATA_HardReset ( void )
{

#if defined (mb282b) || defined (mb314)
    ATA_HRD_RST = ATA_HRD_RST_ASSERT;
    task_delay ( 10 );
    ATA_HRD_RST = ATA_HRD_RST_DEASSERT;

    if ( (ATA_HRD_RST & ATA_HRD_RST_DEASSERT ) != ATA_HRD_RST_DEASSERT )
    {
        return ( STATA_HARD_RESET_FAILURE );
    }
#endif

    return ( ST_NO_ERROR );
} /* end of ATA_HardReset() */

/*******************************************************************************
Name        : ATA_SoftReset
Description : Applies a soft reset of ATA interface
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t ATA_SoftReset(void)
{
    ATA_REG_CONTROL = 0x0C;
    task_delay  (1000);
    ATA_REG_CONTROL = 0x08;/* enable interrupt */
    return ( ST_NO_ERROR );
} /* end of ATA_SoftReset() */

/*******************************************************************************
Name        : ATA_InitInterface
Description : Local function to Initialize the ATA hardware
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t ATA_InitInterface ( void )
{
    int Error;

    Error = ATA_HardReset ( );
    if ( Error != ST_NO_ERROR )
    {
        return ( Error );
    }

    Error = ATA_SoftReset();
    if ( Error != ST_NO_ERROR )
    {
        return ( Error );
    }

    Error = ATA_DeviceSelect();
    if ( Error != ST_NO_ERROR )
    {
        return ( Error );
    }

    Error = ATA_SetFeature(FEATURE_ENABLE_WRITE_CACHE, 0);
    if ( Error != ST_NO_ERROR )
    {
        return ( Error );
    }

    Error = ATA_SetFeature(FEATURE_ENABLE_READ_LOOK_AHEAD_FEATURE, 0 );
    if ( Error != ST_NO_ERROR )
    {
        return ( Error );
    }

    Error = ATA_SetFeature(FEATURE_DISABLE_REVERTING_TO_POWER_ON_DEFAULTS, 0);
    if ( Error != ST_NO_ERROR )
    {
        return ( Error );
    }

    Error = ATA_SetFeature(FEATURE_DISABLE_ADVANCED_POWER_MANAGEMENT, 0);
    if ( Error != ST_NO_ERROR )
    {
        return ( Error );
    }
    Error = ATA_SetFeature(FEATURE_SET_TRANSFER_MODE, PIO_MODE_4);
    if ( Error != ST_NO_ERROR )
    {
        return ( Error );
    }

    Error = ATA_SetMultipleMode(MULTIPLE_SETTING);
    if ( Error != ST_NO_ERROR )
    {
        return ( Error );
    }

    STTBX_Print(("ATAPI    : ATA Interface         Initialized.\n"));

    return ( Error );

} /* end of ATA_InitInterface() */
/*******************************************************************************
Name        : ATA_DeviceSelect
Description : Selects the device operation
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t ATA_DeviceSelect(void)
{
    /* first wait for not busy */
    ATA_WaitCondition(ATATIMEOUT, (!DRV_BSY) && (!DRV_DRQ) );
    semaphore_wait ( &ATA_Handle.Mutex );
    ATA_REG_DEVHEAD = 0;
    semaphore_signal ( &ATA_Handle.Mutex );
    ATA_WaitCondition(ATATIMEOUT, (!DRV_BSY) && (!DRV_DRQ) );
    return ( ST_NO_ERROR );
} /* end of ATA_DeviceSelect() */

/*******************************************************************************
Name        : ATA_InterruptHandler
Description : ATA interrupt handler
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
void ATA_InterruptHandler(void)
{
    U8  Dummy;
    /* clear interrupt = read status register */
#ifdef ST_5514
    /* clear on 5514 */
    Dummy = HDD_DMA_ITS;
#endif

    Dummy = ATA_REG_ALTSTAT;
    Dummy = ATA_REG_STATUS;

    if ( ATA_Handle.WantAnInterrupt == TRUE )
    {
        semaphore_signal(&ATA_Handle.InterruptSemaphore);
        ATA_Handle.WantAnInterrupt = FALSE;
    }
} /* end of ATA_InterruptHandler() */

/*******************************************************************************
Name        : ATA_WaitForInterrupt
Description : Wait for interrupt reached
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t ATA_WaitForInterrupt(void)
{
    clock_t         TimeOut;
    ST_ErrorCode_t   Error = ST_NO_ERROR ;
/*    TimeOut = time_plus(time_now(), 15625*2);*/
    TimeOut = time_plus(time_now(), 15625 * 20);
    if(semaphore_wait_timeout(&ATA_Handle.InterruptSemaphore, &TimeOut))
    {
        Error = STATA_ERROR_TIMEOUT;
        ATA_Handle.WantAnInterrupt = FALSE;
    }
    return (Error);
} /* end of ATA_WaitForInterrupt() */

/*******************************************************************************
Name        : ATA_ClearInterrupt
Description : Routine that clears the pending Interrupts, and reset the
                semaphore counter
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t ATA_ClearInterrupt (void)
{
    U8  Dummy;
    Dummy = ATA_REG_ALTSTAT;
    Dummy = ATA_REG_STATUS;
    ATA_Handle.WantAnInterrupt = TRUE;

    return(TRUE);
} /* end of ATA_ClearInterrupt() */

/*******************************************************************************
Name        : STATA_PerformDiagnostic
Description : Performs a device diagnostic, and return the result.
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t STATA_PerformDiagnostic ( void )
{
    ST_ErrorCode_t Error;
    U8      AtaError;

    ATA_WaitCondition ( ATATIMEOUT, (!DRV_BSY) && (!DRV_DRQ));
    semaphore_wait      (&ATA_Handle.Mutex);
    ATA_ClearInterrupt  ( );
    ATA_REG_DEVHEAD     = 0;
    ATA_REG_COMMAND     = COMMAND_EXECUTE_DEVICE_DIAGNOSTIC;
    Error = ATA_WaitForInterrupt();
    if( Error == ST_NO_ERROR )
    {
        AtaError = ATA_REG_ERROR;
        if ((AtaError != DEVICE_0_PASSED_1_PASSED) &&
            (AtaError != DEVICE_0_PASSED_1_FAILED))
        {
            Error = STATA_DEVICE_FAILED;
        }
    }
    semaphore_signal    (&ATA_Handle.Mutex);
    return (Error);
} /* end of STATA_PerformDiagnostic() */

/*******************************************************************************
Name        : STATA_IdentifyDevice
Description : Gets Id buffer from ATA interface.
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t  STATA_IdentifyDevice ( U16 *Info )
{
    ST_ErrorCode_t  Error;
    U32 Loop;

    ATA_WaitCondition   (ATATIMEOUT, (!DRV_BSY) && (!DRV_DRQ));
    semaphore_wait      (&ATA_Handle.Mutex);
    ATA_ClearInterrupt  ( );
    ATA_REG_DEVHEAD     = 0;
    ATA_REG_COMMAND     = COMMAND_IDENTIFY_DEVICE;
    if ( (Error = ATA_WaitForInterrupt()) == ST_NO_ERROR )
    {
        for ( Loop=0; Loop < 256; Loop ++)
        {
            Info[Loop] = *(volatile U16*) ATA_REG_DATA ;
        }
    }
    semaphore_signal    (&ATA_Handle.Mutex);
    return ( Error );
} /* end of STATA_IdentifyDevice() */

/*******************************************************************************
Name        : STATA_WriteMultiple
Description : Write several blocks to the ATA device.
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t STATA_WriteMultiple ( U32 Source, U32 LBA, U32 BlockNumber)
{
    ST_ErrorCode_t Error;
    U32 Loop;
    U16* Offset_p;
    U32 j;

    /* Just check if BlockNumber is available */
    if (BlockNumber)
    {
        ATA_WaitCondition ( ATATIMEOUT, (!DRV_BSY) && (!DRV_DRQ));
        semaphore_wait           (&ATA_Handle.Mutex);
        LBA = LBA * MULTIPLE_SETTING;
        ATA_REG_SECCOUNT    = (U8) (MULTIPLE_SETTING * BlockNumber);
        ATA_REG_SECNUM      = 0xFF & (LBA);
        ATA_REG_CYLLOW      = 0xFF & (LBA>>8);
        ATA_REG_CYLHIGH     = 0xFF & (LBA>>16);
        ATA_REG_DEVHEAD     = 0x40 | (0x0F & LBA>>24);/* LBA, device = 0 */
        ATA_REG_COMMAND     = COMMAND_WRITE_MULTIPLE;
        /* first wait for not busy and data request */
        ATA_WaitCondition( ATATIMEOUT, (!DRV_BSY) && (DRV_DRQ) );
        ATA_ClearInterrupt          ( );
        for( Loop = 0 ; Loop<BlockNumber  ; Loop++)
        {
            ATA_Handle.WantAnInterrupt = TRUE;

    #ifdef USE_BLOCK_MOVE_DMA
            BMDMA_SOURCE            = Source + ( Loop * ATA_BLOCK_SIZE );
            BMDMA_DEST              = ATA_REG_DATA;
            BMDMA_COUNT             = ATA_BLOCK_SIZE;
            semaphore_wait ( &ATAHndl.ATABlockMoveSemaphore );
    #else
    #if defined (ST_5514)
            Offset_p = (U16*)((char*)Source + ( Loop * ATA_BLOCK_SIZE ));
            for (j = 0; j < (ATA_BLOCK_SIZE / 2); j++)
            {
                ATA_REG_DATA = (STSYS_DU32)*(Offset_p + j);
                /*memcpy ( (char*) ATA_REG_DATA , (char*) (Source + ( Loop * ATA_BLOCK_SIZE )), ATA_BLOCK_SIZE );*/
            }
    #else
            memcpy ( (char*) ATA_REG_DATA , (char*) (Source + ( Loop * ATA_BLOCK_SIZE )), ATA_BLOCK_SIZE );
            /*WriteBytes ( (Source + ( Loop * ATA_BLOCK_SIZE )), ATA_BLOCK_SIZE );*/
    #endif
    #endif
            if((Error = ATA_WaitForInterrupt()) != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"HDD_WRITE_ERROR\r\n"));
                break;
            }
        }
        semaphore_signal            (&ATA_Handle.Mutex);
    }
    else
    {
        Error=ST_NO_ERROR;
    }
    return(Error);
}

#if defined(ST_5514)
#if defined HDDI_DMA
/*******************************************************************************
Name        : STATA_ReadMultipleDMA
Description : Read several sectors from ATA interface through DMA.
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
STATA_Status_t STATA_ReadMultipleDMA( U32 Destination, U32 LBA, U8 SectorNumber)
{
    U8  Dummy;
    ST_ErrorCode_t Error=ST_NO_ERROR;
    U32 Loop;
    U16* Offset_p;
    U16* Offset16_p;
    U32 j;
    U16 Buf;

    /* Just check if BlockNumber is available */
    if (BlockNumber)
    {
        ATA_WaitCondition ( ATATIMEOUT, (!DRV_BSY) && (!DRV_DRQ));
        semaphore_wait          (&ATA_Handle.Mutex);

        ATA_ClearInterrupt      ( );
        LBA = LBA * SizeOfABlock;
        ATA_REG_SECCOUNT    = SectorNumber ;
        ATA_REG_SECNUM      = 0xFF & (LBA);
        ATA_REG_CYLLOW      = 0xFF & (LBA>>8);
        ATA_REG_CYLHIGH     = 0xFF & (LBA>>16);
        ATA_REG_DEVHEAD     = 0x40 | (0x0F & (LBA>>24));
        ATA_REG_COMMAND     = COMMAND_READ_DMA_0;
        /*Ata_In.Feature     = 0;*/
        /*Ata_In.SecCount    = nb_sector;*/
        /*Ata_In.SectorToRW  = nb_sector;*/
        /*Ata_In.DataBuffer  = Destination;*/

        for ( Loop = 0 ; (Loop < BlockNumber) && (Error == ST_NO_ERROR) ; Loop ++ )
        {
            if ( (Error = ATA_WaitForInterrupt()) != ST_NO_ERROR )
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "HDD READ ERROR on block %d out of %d", Loop, BlockNumber));
                break;
            }
            if ( Loop < BlockNumber - 1 )
            {   /* want to catch an interrupt on next time, except for last time
                where we don't care, since we are not going to read further */
                ATA_Handle.WantAnInterrupt = TRUE;
            }

            /* Now, we test the destination of datas.*/
            if (Destination)
            {
                HDD_DMA_SA  = Destination ;     /* Configure the DMA */
                HDD_DMA_SI  = HDD_DMA_SI_32BYTES ;
                HDD_DMA_WC  = SectorNumber<<8 ;
                HDD_DMA_C   = HDD_DMA_C_STARTBIT | HDD_DMA_C_DMAENABLE |  /* Set Start bit */   HDD_DMA_C_READNOTWRITE ;

    if ( ATA0_Info.Current_Mode == MW_MODE )
    {
        STSYS_WriteRegDev32LE( HDDI_BASE + HDDI_MODE, HDDI_MODE_MWDMA );  /* Switch to the selected mode */
    }
    if ( ATA0_Info.Current_Mode == UDMA_MODE )
    {
        STSYS_WriteRegDev32LE( HDDI_BASE + HDDI_MODE, HDDI_MODE_UDMA );
    }

    Temp = ( STSYS_ReadRegDev32LE( HDDI_BASE + HDDI_DMA_C ) & HDDI_DMA_C_MASK ) & ~HDDI_DMA_C_STARTBIT;
    STSYS_WriteRegDev32LE( HDDI_BASE + HDDI_DMA_C, Temp & HDDI_DMA_C_MASK );  /* Remove the Start bit */

    if ( CachedOption == WantToPause )
    {
        task_delay( 8/*DELAYBEFOREPAUSE*/ );
        Temp = ( STSYS_ReadRegDev32LE( HDDI_BASE + HDDI_DMA_C ) & HDDI_DMA_C_MASK ) & ~HDDI_DMA_C_DMAENABLE;
        STSYS_WriteRegDev32LE( HDDI_BASE + HDDI_DMA_C, Temp & HDDI_DMA_C_MASK );  /* Pause */
        Temp = STSYS_ReadRegDev32LE( HDDI_BASE + HDDI_DMA_CB ) & HDDI_DMA_CB_MASK;
        nb_byte_transfered = ( Ata_In->SectorToRW * SECTOR_SIZE ) - Temp;
        HDDI_Trace(( "HDDI:Pause:byte transfered = 0x%x\r\n", nb_byte_transfered));
        task_delay( 2/*DELAYFORPAUSE*/ );
        Temp = ( STSYS_ReadRegDev32LE( HDDI_BASE + HDDI_DMA_C ) & HDDI_DMA_C_MASK ) | HDDI_DMA_C_DMAENABLE;
        STSYS_WriteRegDev32LE( HDDI_BASE + HDDI_DMA_C, Temp & HDDI_DMA_C_MASK );  /* Restart */
    }

    if ( CachedOption == WantToAbort )
    {
/*        task_delay( DELAYBEFOREABORT );*/
        Temp = ( STSYS_ReadRegDev32LE( HDDI_BASE + HDDI_DMA_C ) & HDDI_DMA_C_MASK ) | HDDI_DMA_C_STOPBIT;
        STSYS_WriteRegDev32LE( HDDI_BASE + HDDI_DMA_C, Temp & HDDI_DMA_C_MASK );  /* Abort */
        Temp = STSYS_ReadRegDev32LE( HDDI_BASE + HDDI_DMA_CB ) & HDDI_DMA_CB_MASK;
        nb_byte_transfered = ( Ata_In->SectorToRW * SECTOR_SIZE ) - Temp;
        report(( severity_info, "HDDI:Abort:byte transfered = 0x%x\r\n", nb_byte_transfered));
	}

    if ( ATA_WaitForInterrupt( &HDDI_ITS ) != HDDI_NO_ERROR )    /* HDMA2 */
  	{
        return HDDI_INT_TIMEOUT;
    }

    STSYS_WriteRegDev32LE( HDDI_BASE + HDDI_MODE, HDDI_MODE_PIOREG );
    ATA_WaitCondition( ATATIMEOUT, (!DRV_BSY) & (!DRV_DRQ), Status );   /* HDMA0 */
    if ( Status != HDDI_NO_ERROR )
    {
        return Status;
    }
            }
        }


    Status = STATA_Dma( &Ata_In, CachedOption );
    STATA_TraceError( Status );
    STATA_ReadStatus( &Ata_Out );
    nb_byte_read = ( nb_sector * SECTOR_SIZE ) - ( STSYS_ReadRegDev32LE( HDDI_BASE + HDDI_DMA_CB ) & HDDI_DMA_CB_MASK );
/*    report((severity_info, "\nHDDI:READDMA:Transfer %d bytes\r\n", nb_byte_read));*/
    HDDI_Trace(("HDDI:READDMA:Transfer %d bytes\r\n", nb_byte_read));
    if ((( Ata_Out.Status & HDDI_ATA_SR_BSY ) == HDDI_ATA_SR_BSY ) || (( Ata_Out.Status & HDDI_ATA_SR_DRDY ) != HDDI_ATA_SR_DRDY ) ||
        (( Ata_Out.Status & HDDI_ATA_SR_DF ) == HDDI_ATA_SR_DF ) || (( Ata_Out.Status & HDDI_ATA_SR_DRQ ) == HDDI_ATA_SR_DRQ ) ||
        (( Ata_Out.Status & HDDI_ATA_SR_ERR ) == HDDI_ATA_SR_ERR ))
    {
        HDDI_Trace(("HDDI:READDMA:Error = 0x%x\r\n", Ata_Out.Error));
        HDDI_Trace(("HDDI:READDMA:LBA = 0x%x\r\n", (U32) 0x0FFFFFFF&( Ata_Out.SecNum | ( Ata_Out.CylLow << 8 ) |
                                                          ( Ata_Out.CylHigh << 16 ) | ( Ata_Out.DevHead << 24 ))));
        HDDI_Trace(("HDDI:READDMA:Status = 0x%x\r\n", Ata_Out.Status));
    }
    semaphore_signal( &ATAMutex );
    return Status;
}
#elif
/*******************************************************************************
Name        : STATA_ReadMultiple
Description : Read several sectors from ATA interface.
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t  STATA_ReadMultiple         ( U32 Destination, U32 LBA, U32 BlockNumber, U8 SizeOfABlock)
{
    U8  Dummy;
    ST_ErrorCode_t Error=ST_NO_ERROR;
    U32 Loop;
    U16* Offset_p;
    U16* Offset16_p;
    U32 j;
    U16 Buf;

#ifdef DVD_TRANSPORT_STPTI
    STPTI_DMAParams_t       STDMA_Params;
#endif  /* of #ifdef DVD_TRANSPORT_STPTI */

    /* Just check if BlockNumber is available */
    if (BlockNumber)
    {
        ATA_WaitCondition ( ATATIMEOUT, (!DRV_BSY) && (!DRV_DRQ));
        semaphore_wait          (&ATA_Handle.Mutex);

        ATA_ClearInterrupt      ( );
        LBA = LBA * SizeOfABlock;
        ATA_REG_SECCOUNT    = MULTIPLE_SETTING * BlockNumber ;
        ATA_REG_SECNUM      = 0xFF & (LBA);
        ATA_REG_CYLLOW      = 0xFF & (LBA>>8);
        ATA_REG_CYLHIGH     = 0xFF & (LBA>>16);
        ATA_REG_DEVHEAD     = 0x40 | (0x0F & (LBA>>24));
        ATA_REG_COMMAND     = COMMAND_READ_MULTIPLE;

        for ( Loop = 0 ; (Loop < BlockNumber) && (Error == ST_NO_ERROR) ; Loop ++ )
        {
            if ( (Error = ATA_WaitForInterrupt()) != ST_NO_ERROR )
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "HDD READ ERROR on block %d out of %d", Loop, BlockNumber));
                break;
            }
            if ( Loop < BlockNumber - 1 )
            {   /* want to catch an interrupt on next time, except for last time
                where we don't care, since we are not going to read further */
                ATA_Handle.WantAnInterrupt = TRUE;
            }

            /* Now, we test the destination of datas.*/
            if (!Destination)
            {
#ifndef DVD_TRANSPORT_STPTI
                /* The stream of data is directly injected to decoder */
                pti_video_dma_inject ( (unsigned char*)(ATA_REG_DATA), (int) (SECTOR_SIZE * SizeOfABlock) );
                pti_video_dma_synchronize ( );
#else   /* of #ifndef DVD_TRANSPORT_STPTI */

                STDMA_Params.Destination = VIDEO_CD_FIFO;
                STDMA_Params.Holdoff = 1;
                STDMA_Params.WriteLength = 1;
                STDMA_Params.CDReqLine = STPTI_CDREQ_VIDEO;

                for (j = 0; j < ((SECTOR_SIZE * SizeOfABlock) / 2); j++)
                {
                    Buf = (U16)ATA_REG_DATA;
                    Error = STPTI_UserDataWrite((U8 *)Buf, 2, &STDMA_Params);
                    if ( Error != ST_NO_ERROR )
                    {
                        STTBX_Print(("STPTI_UserDataWrite(): error=%d\n", Error));
                    }
                    else
                    {
                        Error = STPTI_UserDataSynchronize(&STDMA_Params);
                        if ( Error != ST_NO_ERROR )
                        {
                            STTBX_Print(("STPTI_UserDataSynchronize() : error=%d\n", Error));
                        }
                    }
                }
#endif  /* of #ifndef DVD_TRANSPORT_STPTI */
            }
            else
            {
#ifdef BMDMA_ENABLE
                /* The stream of data copied via block move module */
                ATA_BMDMA( (char*) ATA_REG_DATA, (char*) (Destination + ( Loop * SECTOR_SIZE * SizeOfABlock )), SECTOR_SIZE * SizeOfABlock);
#else
                /* The stream of data copied to destination buffer */
                Offset_p = (U16*)((char*)Destination + ( Loop * SECTOR_SIZE * SizeOfABlock ));
                for (j = 0; j < (ATA_BLOCK_SIZE / 2); j++)
                {
                    *(Offset_p + j) = (U16)ATA_REG_DATA;
                }
#endif
            }
        }
        Dummy = ATA_REG_ALTSTAT;
        Dummy = ATA_REG_STATUS;

/*        ATA_WaitForInterrupt();*/
        semaphore_signal(&ATA_Handle.Mutex);
    }
    else
    {
        Error=ST_NO_ERROR;
    }

    return(Error);
} /* end of STATA_ReadMultiple() */
#endif /* ST_5514 */
#endif /* HDDI_DMA */

#if !defined(ST_5514)
/*******************************************************************************
Name        : STATA_ReadMultiple
Description : Read several sectors from ATA interface.
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t  STATA_ReadMultiple         ( U32 Destination, U32 LBA, U32 BlockNumber, U8 SizeOfABlock)
{
    U8  Dummy;
    ST_ErrorCode_t Error=ST_NO_ERROR;
    U32 Loop;
    U16* Offset_p;
    U16* Offset16_p;
    U32 j;
    U16 Buf;

#ifdef DVD_TRANSPORT_STPTI
    STPTI_DMAParams_t       STDMA_Params;
#endif  /* of #ifdef DVD_TRANSPORT_STPTI */

    /* Just check if BlockNumber is available */
    if (BlockNumber)
    {
        ATA_WaitCondition ( ATATIMEOUT, (!DRV_BSY) && (!DRV_DRQ));
        semaphore_wait          (&ATA_Handle.Mutex);

        ATA_ClearInterrupt      ( );
        LBA = LBA * SizeOfABlock;
        ATA_REG_SECCOUNT    = MULTIPLE_SETTING * BlockNumber ;
        ATA_REG_SECNUM      = 0xFF & (LBA);
        ATA_REG_CYLLOW      = 0xFF & (LBA>>8);
        ATA_REG_CYLHIGH     = 0xFF & (LBA>>16);
        ATA_REG_DEVHEAD     = 0x40 | (0x0F & (LBA>>24));
        ATA_REG_COMMAND     = COMMAND_READ_MULTIPLE;

        for ( Loop = 0 ; (Loop < BlockNumber) && (Error == ST_NO_ERROR) ; Loop ++ )
        {
            if ( (Error = ATA_WaitForInterrupt()) != ST_NO_ERROR )
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "HDD READ ERROR on block %d out of %d", Loop, BlockNumber));
                break;
            }
            if ( Loop < BlockNumber - 1 )
            {   /* want to catch an interrupt on next time, except for last time
                where we don't care, since we are not going to read further */
                ATA_Handle.WantAnInterrupt = TRUE;
            }

            /* Now, we test the destination of datas.*/
            if (!Destination)
            {
#ifndef DVD_TRANSPORT_STPTI
                /* The stream of data is directly injected to decoder */
                pti_video_dma_inject ( (unsigned char*)(ATA_REG_DATA), (int) (SECTOR_SIZE * SizeOfABlock) );
                pti_video_dma_synchronize ( );

#else   /* of #ifndef DVD_TRANSPORT_STPTI */

#if defined(ST_7015) || defined(ST_7020)
                STDMA_Params.Destination = VIDEO_CD_FIFO1;
#else   /* of #if defined(ST_7015) || defined(ST_7020)*/
                STDMA_Params.Destination = VIDEO_CD_FIFO;
#endif  /* of #if defined(ST_7015) || defined(ST_7020)*/

                STDMA_Params.Holdoff = 1;
                STDMA_Params.WriteLength = 1;
                STDMA_Params.CDReqLine = STPTI_CDREQ_VIDEO;
                Error = STPTI_UserDataWrite((U8 *)ATA_REG_DATA, (U32)(SECTOR_SIZE * SizeOfABlock), &STDMA_Params);
                if ( Error != ST_NO_ERROR )
                {
                    STTBX_Print(("STPTI_UserDataWrite(): error=%d\n", Error));
                }
                else
                {
                    Error = STPTI_UserDataSynchronize(&STDMA_Params);
                    if ( Error != ST_NO_ERROR )
                    {
                        STTBX_Print(("STPTI_UserDataSynchronize() : error=%d\n", Error));
                    }
                }

#endif  /* of #ifndef DVD_TRANSPORT_STPTI */
            }
            else
            {
#ifdef BMDMA_ENABLE
                /* The stream of data copied via block move module */
                ATA_BMDMA( (char*) ATA_REG_DATA, (char*) (Destination + ( Loop * SECTOR_SIZE * SizeOfABlock )), SECTOR_SIZE * SizeOfABlock);
#else
                /* The stream of data copied to destination buffer */
                memcpy ( (char*) (Destination + ( Loop * SECTOR_SIZE * SizeOfABlock )) , (char*) ATA_REG_DATA , SECTOR_SIZE * SizeOfABlock );

#endif
            }
        }
        Dummy = ATA_REG_ALTSTAT;
        Dummy = ATA_REG_STATUS;


/*        ATA_WaitForInterrupt();*/

        semaphore_signal(&ATA_Handle.Mutex);
    }
    else
    {
        Error=ST_NO_ERROR;
    }

    return(Error);
} /* end of STATA_ReadMultiple() */
#endif /* !ST_5514 */


/*******************************************************************************
Name        : ATA_SetFeature
Description : set feature for ATA interface.
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t  ATA_SetFeature(U8 Command, U8 SubCommand)
{
    ST_ErrorCode_t  Error;

    ATA_WaitCondition ( ATATIMEOUT, (!DRV_BSY) && (!DRV_DRQ));
    semaphore_wait ( &ATA_Handle.Mutex );
    ATA_ClearInterrupt ( );
    ATA_REG_FEATURE     = Command;/* disable reverting to power on default */
    ATA_REG_SECCOUNT    = SubCommand;
    ATA_REG_SECNUM      = 0;
    ATA_REG_CYLLOW      = 0;
    ATA_REG_CYLHIGH     = 0;
    ATA_REG_DEVHEAD     = 0;
    ATA_REG_COMMAND     = COMMAND_SET_FEATURES;/* set feature command */
    Error = ATA_WaitForInterrupt ( );
    semaphore_signal ( &ATA_Handle.Mutex );
    return (Error);
} /* end of ATA_SetFeature() */

/*******************************************************************************
Name        : ATA_SetMultipleMode
Description : Sets multiple mode for the device.
Parameters  : The sector number
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t ATA_SetMultipleMode (U8 SectorNumber)
{
    ST_ErrorCode_t Error;
    ATA_WaitCondition  ( ATATIMEOUT, (!DRV_BSY) && (!DRV_DRQ));
    semaphore_wait     ( &ATA_Handle.Mutex );
    ATA_ClearInterrupt ( );
    ATA_REG_SECCOUNT    = SectorNumber;
    ATA_REG_COMMAND     = COMMAND_SET_MULTIPLE_MODE;
    Error = ATA_WaitForInterrupt ( );
    semaphore_signal ( &ATA_Handle.Mutex );
    return (Error);
} /* end of ATA_SetMultipleMode() */


/****************************************************************************
   Name:        ata_BlockMoveIntHandler
   Description: routine that clears the pending Interrupts, and reset the
                semaphore counter
****************************************************************************/
#ifdef BMDMA_ENABLE

static void ata_BlockMoveIntHandler(void)
{
    volatile U32 data;
  U32 valid_bits = (1<<2)-1;
  U32 expected_value = 0x3;

    #pragma ST_device (BMDMAReg)
	volatile U8* BMDMAReg;


	BMDMAReg = (volatile U8*)BMDMA_Status;
	data = *BMDMAReg;


  if ((data & valid_bits) == expected_value)
  {
 		BMDMAReg = (volatile U8*)BMDMA_IntAck;
		*BMDMAReg = 1;

		BMDMAReg = (volatile U8*)BMDMA_Status;
		data = *BMDMAReg;

    semaphore_signal(&ATA_Handle.BMDMA_IntSemaphore);
  }
  else
  {
    semaphore_signal(&ATA_Handle.BMDMA_IntSemaphore);
  }
}

/***************************************************************
Name: ATA_BMDMA
Description: Performs Block Move DMA
Parameters: Source address, Destination address, and Size of
 						data to be moved
****************************************************************/
static void ATA_BMDMA(void *Source, void *Destination, U32 Size)
{
  #pragma ST_device (BMDMAReg)
  volatile U8* BMDMAReg;


      BMDMAReg = (volatile U8*) BMDMA_DestAddress;
  	 *BMDMAReg = (U32) (Destination);
  	  BMDMAReg = (volatile U8*)BMDMA_SrcAddress;
	 *BMDMAReg = (U32) Source;
	  BMDMAReg = (volatile U8*)BMDMA_IntEn;
	 *BMDMAReg = 1;
	  BMDMAReg = (volatile U8*)BMDMA_Count;
	 *BMDMAReg = Size;

    semaphore_wait(&ATA_Handle.BMDMA_IntSemaphore);

}
#endif



/* ------------------------------- End of file ---------------------------- */
