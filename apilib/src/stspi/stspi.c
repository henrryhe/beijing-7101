/******************************************************************************

File name       : stspi.c

Description     : Driver code for STAPI SPI interface

Reference       : SSC4 document, ssc4.pdf(ADCS 7532916)
Revision History:

COPYRIGHT (C) STMicroelectronics 2005

******************************************************************************/

/* Includes------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "stcommon.h"
#include "stddefs.h"
#include "stspi.h"
#include "stsys.h"
#include "stlite.h"
#include "stpio.h"
#include "sttbx.h"

#ifdef STSPI_MASTER_SLAVE
#include "stevt.h"
#endif

#include "stdevice.h"

/* Constants ----------------------------------------------------------------*/

#if defined(ST_5100)||defined(ST_7100)||defined(ST_5301)||defined(ST_8010)||defined(ST_7109)
#define NUM_SSC 3
#elif defined(ST_5105) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107)
#define NUM_SSC 2
#endif

#define MAGIC_NUM 0x08080808

/* --- SSC offsets in 32-bit chunks --- */
#define SSC_BAUD_RATE                  0
#define SSC_TX_BUFFER                  1
#define SSC_RX_BUFFER                  2
#define SSC_CONTROL                    3
#define SSC_INT_ENABLE                 4
#define SSC_STATUS                     5
#define SSC_I2C                        6
#define SSC_SLAD                       7
#define SSC_CLR                       32
#define SSC_GLITCH_WIDTH              64
#define SSC_PRE_SCALER                65
#define SSC_GLITCH_WIDTH_DATAOUT      66
#define SSC_PRE_SCALER_DATAOUT        67

/* ---  The SSC control register bits --- */
#define SSC_CON_DATA_WIDTH_7     0x06
#define SSC_CON_DATA_WIDTH_8     0x07
#define SSC_CON_DATA_WIDTH_9     0x08
#define SSC_CON_HEAD_CONTROL     0x10
#define SSC_CON_CLOCK_PHASE      0x20
#define SSC_CON_CLOCK_POLARITY   0x40
#define SSC_CON_RESET            0x80
#define SSC_CON_MASTER_SELECT    0x100
#define SSC_CON_ENABLE           0x200
#define SSC_CON_LOOP_BACK        0x400
#define SSC_CON_ENB_TX_FIFO      0x800
#define SSC_CON_ENB_RX_FIFO      0x1000
#define SSC_CON_ENB_CLST_RX      0x2000

/* ---  The status register bits --- */
#define SSC_STAT_RX_FULL         0x01
#define SSC_STAT_TX_EMPTY        0x02
#define SSC_STAT_TX_ERROR        0x04
#define SSC_STAT_RX_ERROR        0x08
#define SSC_STAT_PHASE_ERROR     0x10
#define SSC_STAT_STRETCH         0x20
#define SSC_STAT_AAS             0x40
#define SSC_STAT_STOP            0x80
#define SSC_STAT_ARBL            0x100
#define SSC_STAT_BUSY            0x200
#define SSC_STAT_NACK            0x400
#define SSC_STAT_REPSTRT         0x800
#define SSC_STAT_TX_HALF_EMPTY   0x1000
#define SSC_STAT_TX_FULL         0x2000
#define SSC_STAT_RX_HALF_FULL    0x4000

/* ---  The interrupt enable register bits --- */
#define SSC_IEN_RX_FULL          0x01
#define SSC_IEN_TX_EMPTY         0x02
#define SSC_IEN_TX_ERROR         0x04
#define SSC_IEN_RX_ERROR         0x08
#define SSC_IEN_PHASE_ERROR      0x10
#define SSC_IEN_AAS              0x40
#define SSC_IEN_STOP             0x80
#define SSC_IEN_ARBL             0x100
#define SSC_IEN_NACK             0x400
#define SSC_IEN_REPSTRT          0x800
#define SSC_IEN_TX_HALF_EMPTY    0x1000
#define SSC_IEN_TX_FULL          0x2000
#define SSC_IEN_RX_HALF_FULL     0x4000

#if defined(ST_5100) || defined(ST_5105) || defined(ST_7100)|| defined(ST_5301) ||\
defined(ST_8010) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107)
/* Timing registers; 5528+ only */
#define SSC_REP_START_HOLD_TIME     8   /* base + 0x20   */
#define SSC_START_HOLD_TIME         9   /* base + 0x24   */
#define SSC_REP_START_SETUP_TIME   10   /* base + 0x28   */
#define SSC_DATA_SETUP_TIME        11   /* base + 0x2c   */
#define SSC_STOP_SETUP_TIME        12   /* base + 0x30   */
#define SSC_BUS_FREE_TIME          13   /* base + 0x34   */
#define SSC_TX_FSTAT               14   /* base + 0x38   */
#define SSC_RX_FSTAT               15   /* base + 0x3c   */
#define SSC_0_PRSC_BRG             16   /* base + 0x40   */
#define SSC_0_CLRSTAT              32   /* base + 0x80   */
#define SSC_0_AGFR                 40   /* base + 0x100  */
#define SSC_0_PRSC                 41   /* base + 0x104  */
#define SSC_0_AGFR_DATAOUT         42   /* base + 0x108  */
#define SSC_0_PRSC_DATAOUT         43   /* base + 0x10c  */
#endif

/* --- Default values for SSC4 for various registers --- */
#define DEFAULT_CONTROL   SSC_CON_ENABLE

/* --- misc constants & defines --- */
#define MILLISEC_PER_SEC   1000
#define MICROSEC_PER_SEC   1000000

/* Typedefs -----------------------------------------------------------------*/
#pragma ST_device (DU16)
typedef volatile U16       DU16;
#pragma ST_device (DU32)
typedef volatile U32       DU32;

typedef enum spi_context_e
{
    READING,
    WRITING
} spi_context_t;

/* Driver state - note order is important so we can do < & > comparisons */
typedef enum STSPI_DriverState_e
{
    STSPI_IDLE,
    STSPI_MASTER_WRITING,
    STSPI_MASTER_READING,
    STSPI_SLAVE_TRANSMIT,
    STSPI_SLAVE_RECEIVE
} STSPI_DriverState_t;

typedef struct spi_param_block_s
{
    STSPI_InitParams_t         InitParams;
    ST_DeviceName_t            Name;
    U32                        OpenCnt;
    U16                        InterruptMask;
#ifndef ST_OS21
    semaphore_t                BusAccessSemaphore;
    semaphore_t                IOSemaphore;
#endif
    semaphore_t                *BusAccessSemaphore_p;
    semaphore_t                *IOSemaphore_p;
    STSPI_DriverState_t        State;
    U16                        *Buffer_p;
    S32                        BufferLen;
    S32                        BufferCnt;
    STPIO_Handle_t             SCLHandle;
    STPIO_Handle_t             MTSRHandle;
    STPIO_Handle_t             MRSTHandle;
    STPIO_Handle_t             MasterCSHandle;

#ifdef STSPI_MASTER_SLAVE
    STEVT_Handle_t             EVTHandle;
    STEVT_EventID_t            TxNotifyId;
    STEVT_EventID_t            RxNotifyId;
    STEVT_EventID_t            TxByteId;
    STEVT_EventID_t            RxByteId;
    STEVT_EventID_t            TxCompleteId;
    STEVT_EventID_t            RxCompleteId;
    STPIO_Handle_t             SlaveCSHandle;
#endif

    struct spi_open_block_s    *OpenBlockPtr;
    BOOL                       InUse;
    BOOL                       First_Loop;
} spi_param_block_t;

static  spi_param_block_t CtrlBlockArray[NUM_SSC];

typedef struct spi_open_block_s
{
    U32                         MagicNumber;
    STSPI_OpenParams_t          OpenParams;
    spi_param_block_t           *CtrlBlkPtr;
} spi_open_block_t;


/* Externs ------------------------------------------------------------------*/

/* Used to ensure exclusive access to global control list */
#ifndef ST_OS21
    semaphore_t         g_DriverAtomic;
#endif
    semaphore_t         *g_DriverAtomic_p = NULL;

/* Statics ------------------------------------------------------------------*/

static   const ST_Revision_t  g_Revision  = "STSPI-REL_1.0.6";

static void Spi_Deallocate(  spi_param_block_t    *CtrlBlkPtr_p);
static ST_ErrorCode_t BusAccess ( STSPI_Handle_t    Handle,
                                  U16               *Buffer_p,
                                  U32               BufferLen,
                                  U32               TimeOutMS,
                                  U32               *ActLen_p,
                                  spi_context_t     Context );

void SPI_PioHandler(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);
static void SpiReset(spi_param_block_t *Params_p);
#if defined(STI2C_STSPI_SAME_SSC_SUPPORT)
ST_ErrorCode_t Spi_GetSSC(spi_param_block_t *Params_p);
ST_ErrorCode_t Spi_FreeSSC(spi_param_block_t *Params_p);
#endif
static BOOL First_Init = TRUE;

/* Debug Functions ----------------------------------------------------------------*/

static __inline void WriteReg(DU32 *SpiRegPtr, U16 Index, U16 Value)
{
    SpiRegPtr[Index] = Value;
}

static __inline DU32 ReadReg(DU32 *SpiRegPtr, U16 Index)
{
    DU32 Value;
    Value = SpiRegPtr[Index];
    return Value;
}
#if defined(ST_8010)
/******************************************************************************
Name        : stspi_Setup
Description :

Returns     :

******************************************************************************/
void stspi_Setup( void )
{
   /* configure SSC0 SCL/SDA pins */
   *(volatile U32*)(0x50003010) |= 0x07000000;
   *(volatile U32*)(0x50003014) |= 0xe0000;

   /* configure SSC1 SCL/SDA pins */
   *(volatile U32*)(0x47000028) |= 0x773c00;
}
#endif
/******************************************************************************
Name        : IsAlreadyInitialised
Description : Performs some tests prior to initialising a new control block
Parameters  : 1) A pointer to the name of the device about to be initalised
              2) A pointer to the parameter block of the device to be
                 initialised
******************************************************************************/

static BOOL IsAlreadyInitialised( const ST_DeviceName_t      Name,
                                  const STSPI_InitParams_t   *Params_p )
{
    BOOL  Match = FALSE;
    U32   CtrBlkIndex = 0;

    while ((CtrBlkIndex < NUM_SSC) && (Match == FALSE))
    {
        if ((strcmp(CtrlBlockArray[CtrBlkIndex].Name, Name)) == 0 ||
           (CtrlBlockArray[CtrBlkIndex].InitParams.BaseAddress == Params_p->BaseAddress) ||
           (CtrlBlockArray[CtrBlkIndex].InitParams.InterruptNumber == Params_p->InterruptNumber )
          )
        {
            Match = TRUE;
        }
        else
        {
            CtrBlkIndex++;
        }
    }
    return( Match );
}


/******************************************************************************
Name        : GetPioPin
Description : Takes a PIO bitmask for one PIO pin and resolves it to a pin
              number between zero and seven. Returns UCHAR_MAX on error.
Parameters  : A bitmask for a PIO pin
******************************************************************************/

static S32 GetPioPin( U8 Value )
{
    S32 i = 0;

    while ((Value != 1) && (i < 8))
    {
        i++;
        Value >>= 1;
    }

    if ( i > 7 )
    {
        i = UCHAR_MAX;
    }

    return( i );
}

static void SpiReset(spi_param_block_t *Params_p)
{
    DU32   *SpiRegPtr  = (DU32 *) Params_p->InitParams.BaseAddress;

    SpiRegPtr[SSC_CONTROL] = DEFAULT_CONTROL | SSC_CON_RESET;
    SpiRegPtr[SSC_CONTROL] = DEFAULT_CONTROL;
}

#ifdef STSPI_MASTER_SLAVE
/******************************************************************************
Name        : RegisterEvents
Description : Called from STSPI_Init() when driver has to operate in slave
              mode. Opens the specified Event Handler & registers the
              events which can be notified in slave mode.
Parameters  :
******************************************************************************/

static ST_ErrorCode_t RegisterEvents( const ST_DeviceName_t  EvtHandlerName,
                                      spi_param_block_t      *InitBlock_p )
{
    STEVT_OpenParams_t   EVTOpenParams;

    /* Keeps lint quiet. */
    EVTOpenParams.dummy = 0;

    /* First open the specified event handler */
    if (STEVT_Open (EvtHandlerName, &EVTOpenParams,
                    &InitBlock_p->EVTHandle) != ST_NO_ERROR)
    {
        return (STSPI_ERROR_EVT_REGISTER);
    }

    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                                   STSPI_TRANSMIT_NOTIFY_EVT,
                                   &InitBlock_p->TxNotifyId) != ST_NO_ERROR)
    {
        return (STSPI_ERROR_EVT_REGISTER);
    }

    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                                   STSPI_RECEIVE_NOTIFY_EVT,
                                   &InitBlock_p->RxNotifyId) != ST_NO_ERROR)
    {
        return (STSPI_ERROR_EVT_REGISTER);
    }

    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                                   STSPI_TRANSMIT_BYTE_EVT,
                                   &InitBlock_p->TxByteId) != ST_NO_ERROR)
    {
        return (STSPI_ERROR_EVT_REGISTER);
    }
    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                                   STSPI_RECEIVE_BYTE_EVT,
                                   &InitBlock_p->RxByteId) != ST_NO_ERROR)
    {
        return (STSPI_ERROR_EVT_REGISTER);
    }

    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                                   STSPI_TRANSMIT_COMPLETE_EVT,
                                   &InitBlock_p->TxCompleteId) != ST_NO_ERROR)
    {
        return (STSPI_ERROR_EVT_REGISTER);
    }

    if (STEVT_RegisterDeviceEvent (InitBlock_p->EVTHandle, InitBlock_p->Name,
                                   STSPI_RECEIVE_COMPLETE_EVT,
                                   &InitBlock_p->RxCompleteId) != ST_NO_ERROR)
    {
        return (STSPI_ERROR_EVT_REGISTER);
    }

    return (ST_NO_ERROR);

}/* RegisterEvents */

#endif /* STSPI_MASTER_SLAVE */

/* Interrupt handler in which actual transfer take place */
#ifdef ST_OS21
static int  SpiHandler (void *Param )
#else
static void SpiHandler (void *Param )
#endif
{
    spi_param_block_t   *CtrlBlk_p  = Param;
    DU32                *SpiRegPtr  = (DU32*) CtrlBlk_p->InitParams.BaseAddress;
    U16                 IntEnables  = CtrlBlk_p->InterruptMask;
    U16                 Status,NoToRead;
    U16                 RxData;
    U16                 TxData, i;


    /* Turn off interrupts while in ISR */
    SpiRegPtr[SSC_INT_ENABLE] = 0;

    /* Get h/w status for status bit */
    Status = SpiRegPtr[SSC_STATUS];

    switch (CtrlBlk_p->State)
    {
        case STSPI_MASTER_WRITING:

            if (Status & SSC_STAT_TX_ERROR)
            {
                if (CtrlBlk_p->BufferCnt >= CtrlBlk_p->BufferLen)
                {
#ifdef STSPI_MASTER_SLAVE
                    STPIO_Set(CtrlBlk_p->MasterCSHandle,CtrlBlk_p->OpenBlockPtr->OpenParams.PIOforCS.BitMask);
#endif
                    CtrlBlk_p->State = STSPI_IDLE;
                }
                else
                {
                    /* Not the last byte, so place bytes in the Tx Fifo */
                    for(i=0; i<8 && (CtrlBlk_p->BufferCnt < CtrlBlk_p->BufferLen); i++)
                    {
                        TxData = (*(CtrlBlk_p->Buffer_p++));
                        SpiRegPtr[SSC_TX_BUFFER] = (U16)TxData;
                        RxData = SpiRegPtr[SSC_RX_BUFFER];
                        CtrlBlk_p->BufferCnt++;
                    }

                    /* Enable appropriate interrupts */
                    IntEnables &= ~SSC_IEN_RX_FULL;
                    IntEnables |= SSC_IEN_TX_ERROR;
                }
            }
            break;

        case STSPI_MASTER_READING:

            if (Status & SSC_STAT_TX_ERROR)
            {
                if (CtrlBlk_p->First_Loop)
                {
                    RxData = SpiRegPtr[SSC_RX_BUFFER];
#ifndef STSPI_MASTER_SLAVE
                    *(CtrlBlk_p->Buffer_p++) = (U16) RxData;
                    CtrlBlk_p->BufferCnt++;
#endif
                    CtrlBlk_p->First_Loop = FALSE;
                }
                else
                {
                    /* Read data bytes and place them in the user buffer */
                    for (i=0; i<8; i++)
                    {
                        RxData = SpiRegPtr[SSC_RX_BUFFER];
                        if (CtrlBlk_p->BufferCnt < CtrlBlk_p->BufferLen)
                        {
                            *(CtrlBlk_p->Buffer_p++) = RxData;
                            CtrlBlk_p->BufferCnt++;
                        }
                    }
                }
                /* If last byte has been read, clean up */
                if (CtrlBlk_p->BufferCnt >= CtrlBlk_p->BufferLen )
                {
                    /* Change the control block state to idle */
                    CtrlBlk_p->State = STSPI_IDLE;
                    IntEnables |= SSC_IEN_TX_ERROR;
                }
                else
                {
                    for (i=0; i<8;i++)
                    {
                        SpiRegPtr[SSC_TX_BUFFER] = 0xff;
                    }
#ifdef STSPI_MASTER_SLAVE
                    if ((CtrlBlk_p->BufferCnt + 8) >= CtrlBlk_p->BufferLen)
                    {
                        Status = SpiRegPtr[SSC_STATUS];
                        while(!(Status &  SSC_STAT_TX_ERROR))
                        {
                            Status = SpiRegPtr[SSC_STATUS];
                        }
                        STPIO_Set(CtrlBlk_p->MasterCSHandle,CtrlBlk_p->OpenBlockPtr->OpenParams.PIOforCS.BitMask);
                    }
#endif
                    /* Enable appropriate interrupts */
                    IntEnables |= SSC_IEN_TX_ERROR;
                    IntEnables |= SSC_IEN_TX_EMPTY;
                    IntEnables |= SSC_IEN_RX_FULL;
                }
            }
            break;

        case STSPI_IDLE:

            if ((Status & SSC_STAT_RX_FULL) || (Status & SSC_STAT_TX_ERROR))
            {
                if (CtrlBlk_p->InitParams.MasterSlave == STSPI_SLAVE)
                {
#ifdef STSPI_MASTER_SLAVE
                    STPIO_Compare_t cmp = { 0x80, 0x00 };
                    CtrlBlk_p->MasterCSHandle = 0x00;
                    RxData = SpiRegPtr[SSC_RX_BUFFER];

                    /* Set the default parameters */
                    SpiRegPtr[SSC_CONTROL] |= SSC_CON_RESET;
                    SpiRegPtr[SSC_CONTROL] &= ~SSC_CON_RESET;

                    if (RxData & 0x01)
                    {
                        SpiRegPtr[SSC_CONTROL] |= SSC_CON_ENB_TX_FIFO;
                        SpiRegPtr[SSC_CONTROL] |= SSC_CON_ENB_RX_FIFO;

                        /*Move to slave transmit state,notify user*/
                        CtrlBlk_p->State = STSPI_SLAVE_TRANSMIT;
                        STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxNotifyId,NULL);
                        for (i=0; i<8; i++)
                        {
                            STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxByteId,(void *)&TxData);
                            SpiRegPtr[SSC_TX_BUFFER] = TxData;
                        }
                        /* Enable appropriate interrupts */
                        IntEnables |= SSC_IEN_RX_FULL;
                        IntEnables &= ~SSC_IEN_TX_EMPTY;
                        IntEnables &= ~SSC_IEN_TX_ERROR;
                    }
                    else
                    {
                        /* Enable the fifo mode */
                        SpiRegPtr[SSC_CONTROL]  |= SSC_CON_ENB_TX_FIFO;
                        SpiRegPtr[SSC_CONTROL]  |= SSC_CON_ENB_RX_FIFO;

                        /*Move to slave receiver state,notify user*/
                        CtrlBlk_p->State = STSPI_SLAVE_RECEIVE;
                        STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->RxNotifyId,NULL);

                        /* Enable appropriate interrupts */
                        IntEnables |= SSC_IEN_RX_HALF_FULL;
                        IntEnables &= ~SSC_IEN_TX_ERROR;
                        IntEnables &= ~SSC_IEN_TX_EMPTY;
                    }
                    STPIO_SetCompare(CtrlBlk_p->SlaveCSHandle, &cmp);
#endif
                }
                else
                {
                    /* Clear the Rx buffer before disabling the fifos */
                    if (Status & SSC_STAT_RX_FULL)
                    {
                        for(i=0; i<8; i++)
                        {
                            RxData = SpiRegPtr[SSC_RX_BUFFER];
                        }
                    }
                    else
                    {
                        NoToRead = SpiRegPtr[SSC_RX_FSTAT];
                        for (i=0; i<NoToRead; i++)
                        {
                            RxData = SpiRegPtr[SSC_RX_BUFFER];
                        }
                    }
                    /* Disable the Tx and Rx fifo */
                    SpiRegPtr[SSC_CONTROL] &= ~SSC_CON_ENB_TX_FIFO;
                    SpiRegPtr[SSC_CONTROL] &= ~SSC_CON_ENB_RX_FIFO;
                    semaphore_signal( CtrlBlk_p->IOSemaphore_p );
                    IntEnables |= SSC_IEN_RX_FULL;
                    IntEnables &= ~SSC_IEN_TX_ERROR;
                    IntEnables &= ~SSC_IEN_TX_EMPTY;
                }
            }
            break;

#ifdef STSPI_MASTER_SLAVE

        case STSPI_SLAVE_TRANSMIT:

             if ((Status & SSC_STAT_TX_ERROR)||(Status & SSC_STAT_RX_FULL))
            {
                /* not last byte, so get next 8 bytes & send */
                for(i=0; i<8; i++)
                {
                    RxData = SpiRegPtr[SSC_RX_BUFFER];
                    STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxByteId, (void *)&TxData);
                    SpiRegPtr[SSC_TX_BUFFER] = (U16)TxData;
                }
                /* Enable appropriate interrupts */
                IntEnables &= ~SSC_IEN_TX_ERROR;
                IntEnables &= ~SSC_IEN_TX_EMPTY;
                IntEnables |= SSC_IEN_RX_FULL;
            }
            break;

        case STSPI_SLAVE_RECEIVE:

            if (Status & SSC_STAT_RX_FULL)
            {
                /* Receive data bytes and notify the user */
                for(i=0; i<8; i++)
                {
                    RxData = SpiRegPtr[SSC_RX_BUFFER];
                    STEVT_Notify(CtrlBlk_p->EVTHandle, CtrlBlk_p->RxByteId, &RxData);
                }
            }
            else
            {
                /* Less than 8 data bytes, receive them and notify the user */
                NoToRead = SpiRegPtr[SSC_RX_FSTAT];
                for (i=0; i<NoToRead; i++)
                {
                    RxData = SpiRegPtr[SSC_RX_BUFFER];
                    STEVT_Notify(CtrlBlk_p->EVTHandle, CtrlBlk_p->RxByteId, &RxData);
                }
            }
            /* Enable appropriate interrupts */
            IntEnables |= SSC_IEN_RX_HALF_FULL;
            IntEnables &= ~SSC_IEN_TX_ERROR;
            IntEnables &= ~SSC_IEN_TX_EMPTY;
            break;
#endif

    }

    SpiRegPtr[SSC_INT_ENABLE]=IntEnables;

#ifdef ST_OS21
    return(OS21_SUCCESS);
#endif

} /* SpiHandler */

#ifdef STSPI_MASTER_SLAVE
/******************************************************************************
Function Name : SPI_PioIntHandler
  Description : Handler for PIO compare interrupt
   Parameters :
******************************************************************************/

void SPI_PioHandler(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits)
{
    spi_param_block_t    *InitBlock_p = NULL;
    spi_param_block_t    *CtrlBlk_p   = NULL;
    DU32                 *SpiRegPtr   = NULL;
    U16                  IntEnables;
    U16                  NumberToRead, i, RxData;
    U32                  CtrBlkIndex = 0, Status;
    STPIO_Compare_t cmp = { 0x00, 0x00 };

    /* Find the control block of the slave */
    for( ; ((InitBlock_p = &(CtrlBlockArray[CtrBlkIndex])) != NULL) && (CtrBlkIndex < NUM_SSC); CtrBlkIndex++)
    {
        if(InitBlock_p->SlaveCSHandle)
        {
            SpiRegPtr = (DU32*) InitBlock_p->InitParams.BaseAddress;
        }
        break;
    }

    CtrlBlk_p = InitBlock_p->OpenBlockPtr->CtrlBlkPtr;
    IntEnables  = CtrlBlk_p->InterruptMask;

    /* Read the status register of the slave */
    Status = SpiRegPtr[SSC_STATUS];
    STPIO_SetCompare(CtrlBlk_p->SlaveCSHandle, &cmp);

    if(CtrlBlk_p->State == STSPI_SLAVE_TRANSMIT)
    {
        /* Clear the Rx Buffer */
        if (Status & SSC_STAT_RX_FULL)
        {
            for(i=0;i<8;i++)
            {
                RxData = SpiRegPtr[SSC_RX_BUFFER];
            }
        }
        else
        {
            NumberToRead = SpiRegPtr[SSC_RX_FSTAT];
            for(i=0; i<NumberToRead; i++)
            {
                RxData = SpiRegPtr[SSC_RX_BUFFER];
            }
        }
        /* Change the control block state to idle and notify user */
        CtrlBlk_p->State = STSPI_IDLE;
        STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->TxCompleteId, NULL);
    }
    else
    {
        NumberToRead = SpiRegPtr[SSC_RX_FSTAT];

        /* Receive data bytes and notify the user */
        for(i=0; i<NumberToRead; i++)
        {
            RxData = SpiRegPtr[SSC_RX_BUFFER];
            STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->RxByteId, &RxData);
        }
        /* Change the control block state to idle and notify user */
        CtrlBlk_p->State =  STSPI_IDLE;
        STEVT_Notify (CtrlBlk_p->EVTHandle, CtrlBlk_p->RxCompleteId, NULL);
    }

    /* Disable the Tx and Rx fifo */
    SpiRegPtr[SSC_CONTROL]   &= ~SSC_CON_ENB_TX_FIFO;
    SpiRegPtr[SSC_CONTROL]   &= ~SSC_CON_ENB_RX_FIFO;

    /* Enable appropriate interrupts */
    IntEnables      &= ~SSC_IEN_TX_ERROR;
    IntEnables      |=  SSC_IEN_RX_FULL;
    SpiRegPtr[SSC_INT_ENABLE] = IntEnables;

}/* SPI_PioHandler */

#endif /* STSPI_MASTER_SLAVE */

/******************************************************************************
Name        : ConvertMStoTicks
Description : Converts a timeout in milliseconds to the appropiate number of
              clock ticks. Returns a pointer to the result. Takes care of
              special case 'infinity'. Also takes care of overflow by treating
              as infinity.

Parameters  : 1) The number of milliseconds to convert to ticks
              2) The number of clock ticks per second
              3) Pointer to result (not used if returns infinity)

Returns     : Pointer to result. This will either be pResult or
              TIMEOUT_INFINITY.
******************************************************************************/

static clock_t * ConvertMStoTicks( U32 TimeOutMS, U32 TicksPerSec,
                                   clock_t *pResult_p )
{
    int      Div = 0;
    clock_t  Timeout;

    /* Deal with Infinity as special case */
    if (TimeOutMS == STSPI_TIMEOUT_INFINITY)
    {
        return (clock_t *)(TIMEOUT_INFINITY);
    }

    /* Make sure the conversion will not overflow */
    while (TimeOutMS > INT_MAX/TicksPerSec)
    {
        Div++;
        TimeOutMS /= 10;
    }

    /* Convert from ms to clock ticks, with rounding */
    Timeout = (TimeOutMS * TicksPerSec + 500) / 1000;

    /* Now multiply up again, but if overflow giving infinity */
    while (Div > 0)
    {
        if (Timeout > INT_MAX/10)
        {
            return (clock_t *)(TIMEOUT_INFINITY);
        }
        Div--;
        Timeout *= 10;
    }

    *pResult_p = Timeout;
    return pResult_p;

}/* ConvertMStoTicks */

/*****************************************************************************
STSPI_Close
Description:
    Closes an open handle to an spi device.
Parameters  : 1) The handle to be closed

Return Value:
    ST_NO_ERROR,                       no errors.
    ST_ERROR_BAD_PARAMETER             One or more of the parameters was invalid
    ST_ERROR_UNKNOWN_DEVICE            Invalid device name
    ST_ERROR_FEATURE_NOT_SUPPORTED     Error while trying to initialise in slave mode if driver
                                       has been built in STSPI_MASTER_ONLY mode.
    ST_ERROR_NO_FREE_HANDLES           The maximum number of allocated handles are
                                       in use; cannot allocate another handle

See Also:
    STSPI_Open()
*****************************************************************************/

ST_ErrorCode_t STSPI_Close( STSPI_Handle_t Handle )
{

    spi_param_block_t    *CtrlBlkPtr_p = 0;
    spi_open_block_t     *OpenBlkPtr_p = 0;
    ST_ErrorCode_t       ReturnCode    = ST_NO_ERROR;

    if ((spi_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    task_lock();
    if (TRUE == First_Init)
    {
        task_unlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    task_unlock();

    /* Take global atomic */
    semaphore_wait(g_DriverAtomic_p);
    OpenBlkPtr_p = (spi_open_block_t*)Handle;

    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
        /* Release Busaccess Semaphore */
        semaphore_signal(g_DriverAtomic_p);
        return (ST_ERROR_INVALID_HANDLE);
    }

    CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;

    /* Take Bus access to protect the open list  */
    semaphore_wait (CtrlBlkPtr_p->BusAccessSemaphore_p);

    /* Release global Semaphore */
    semaphore_signal (g_DriverAtomic_p);

    /* Decrement the control structure open count */
    CtrlBlkPtr_p->OpenCnt--;

    /* Clear the magic value to mark it availabe for reuse*/
    OpenBlkPtr_p->MagicNumber = 0;

    /* Release Busaccess Semaphore */
    semaphore_signal (CtrlBlkPtr_p->BusAccessSemaphore_p);

    return (ReturnCode);

}/* STSPI_Close */

/*****************************************************************************
STSPI_GetParams
Description:
    Returns the current operating parameters
Parameters  : 1) A handle to the spi device being read from
              2) A pointer for returning the current parameters
Return Value:
    ST_NO_ERROR,                 no errors.
    ST_ERROR_BAD_PARAMETER       One or more of the parameters was invalid
    ST_ERROR_UNKNOWN_DEVICE      Device has not been initialized
    ST_ERROR_INVALID_HANDLE      Device handle invalid.

See Also:
    STSPI_SetParams()
*****************************************************************************/

ST_ErrorCode_t STSPI_GetParams( STSPI_Handle_t Handle,
                                STSPI_Params_t *GetParams_p )
{

    spi_open_block_t   *OpenBlkPtr_p;
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;

    if ((spi_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    if (GetParams_p == NULL)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    task_lock();
    if (TRUE == First_Init)
    {
        task_unlock();
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    task_unlock();

    /* Take global atomic */
    semaphore_wait(g_DriverAtomic_p);

    /* Set up the pointers to the open block  */
    OpenBlkPtr_p = (spi_open_block_t*)Handle;

    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
        /* Release Busaccess Semaphore */
        semaphore_signal(g_DriverAtomic_p);
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Copy the parameters from open block to user defined structure*/
    memcpy(GetParams_p, &OpenBlkPtr_p->OpenParams, sizeof(STSPI_OpenParams_t));

    /* Release the access semaphore */
    semaphore_signal(g_DriverAtomic_p);

    return (ErrorCode);

}/* STSPI_GetParams */

/******************************************************************************
Name        : STSPI_GetRevision
Description : Returns a pointer to the string containing the version number of
              this code.
Parameters  : None
******************************************************************************/

ST_Revision_t STSPI_GetRevision ( void )
{
    return g_Revision;

}/* STSPI_GetRevision */

/*****************************************************************************
STSPI_Init()
Description:
    Initialises an spi device and its control structure.
Parameters  : 1) A pointer to the devicename
              2) A pointer to an initialisation parameter block
Return Value:
    ST_NO_ERROR,                       no errors.
    ST_ERROR_ALREADY_INITIALIZED       Another user has already initialized the device
    ST_ERROR_BAD_PARAMETER             One or more of the parameters was invalid
    ST_ERROR_NO_MEMORY                 Unable to allocate memory for internal data structures
    ST_ERROR_INTERRUPT_INSTALL         Error installing interrupts for the APIs internal interrupt handler
    STSPI_ERROR_PIO                    Error accessing specified PIO port (see below)
    STSPI_ERROR_NO_FREE_SSC            Error if all avaible SSCs have been already initialised
See Also:
    STSPI_Close()
    STSPI_Open()
*****************************************************************************/

ST_ErrorCode_t STSPI_Init( const ST_DeviceName_t Name,const STSPI_InitParams_t *InitParams_p )
{

    DU32                 *SpiRegPtr   = NULL;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;
    spi_param_block_t    *InitBlock_p = NULL;
#if !defined(STI2C_STSPI_SAME_SSC_SUPPORT)    
    U16                  ConValue     = 0x0000;
    S32                  IntReturn;
#endif    
    S32                  PinforSCL, PinforMRST, PinforMTSR;
    STPIO_OpenParams_t   SCLOpenParams, MRSTOpenParams, MTSROpenParams;
    ST_ErrorCode_t       PIOReturnCode;
    U32                  OpenBlkIndex = 0,CtrBlkIndex = 0;


    if ((Name == 0)||(strlen(Name) >= ST_MAX_DEVICE_NAME )   ||
        (Name[0] == '\0')      ||    /* NUL Name string? */
        (InitParams_p == NULL) || (InitParams_p->ClockFrequency == 0)||
        (InitParams_p->MaxHandles == 0))
    {
        return (ST_ERROR_BAD_PARAMETER) ;
    }

    /* Check for valid base address */
    if (InitParams_p->BaseAddress == NULL)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check partition has been specified */
    if (InitParams_p->DriverPartition == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Check PIO Pin values are valid */
#if defined(ST_8010)
    if ( InitParams_p->BaseAddress == (U32*)SSC_2_BASE_ADDRESS)
#endif
    {
        PinforSCL  = GetPioPin (InitParams_p->PIOforSCL.BitMask);
        PinforMTSR = GetPioPin (InitParams_p->PIOforMTSR.BitMask);
        PinforMRST = GetPioPin (InitParams_p->PIOforMRST.BitMask);
        if ((PinforSCL >= UCHAR_MAX) || (PinforMTSR >= UCHAR_MAX) || (PinforMRST >= UCHAR_MAX))
        {
            return ST_ERROR_BAD_PARAMETER;
        }
    }
    task_lock();
    if (First_Init == TRUE)
    {
        /* Initialised the global semaphore */
#ifdef ST_OS21
        g_DriverAtomic_p = semaphore_create_fifo_timeout(1);
#else
        g_DriverAtomic_p = &g_DriverAtomic;
        semaphore_init_fifo_timeout(g_DriverAtomic_p, 1);
#endif

        for (CtrBlkIndex = 0; CtrBlkIndex < NUM_SSC; CtrBlkIndex++)
        {
            CtrlBlockArray[CtrBlkIndex].InUse = 0;
        }
        First_Init = FALSE;
    }
    task_unlock();

    /* Take this to protect global control list */
    semaphore_wait(g_DriverAtomic_p);

    if (IsAlreadyInitialised (Name, InitParams_p) == TRUE)
    {
        /* release driver atomic */
        semaphore_signal(g_DriverAtomic_p);
        return ST_ERROR_ALREADY_INITIALIZED;
    }

    /* Find a free control block structure*/
    for(CtrBlkIndex = 0; CtrBlkIndex < NUM_SSC; CtrBlkIndex ++)
    {
        if (CtrlBlockArray[CtrBlkIndex].InUse == FALSE)
        {
            InitBlock_p = &CtrlBlockArray[CtrBlkIndex];
            break;
        }
    }

    /* If no free control block has been found,return error*/
    if (InitBlock_p == NULL)
    {
        /* release driver atomic */
        semaphore_signal (g_DriverAtomic_p);
        return STSPI_ERROR_NO_FREE_SSC;
    }

    strcpy( InitBlock_p->Name, Name );

    /* Set up the control structures */
    memcpy(&InitBlock_p->InitParams, InitParams_p, sizeof(STSPI_InitParams_t));
    memcpy(InitBlock_p->InitParams.PIOforMTSR.PortName, InitParams_p->PIOforMTSR.PortName, sizeof(ST_DeviceName_t));
    memcpy(InitBlock_p->InitParams.PIOforMRST.PortName, InitParams_p->PIOforMRST.PortName, sizeof(ST_DeviceName_t));
    memcpy(InitBlock_p->InitParams.PIOforSCL.PortName,  InitParams_p->PIOforSCL.PortName,  sizeof(ST_DeviceName_t));


    InitBlock_p->InitParams.DefaultParams = memory_allocate(InitParams_p->DriverPartition, sizeof(STSPI_Params_t));
    InitBlock_p->OpenCnt                  = 0;
    InitBlock_p->InterruptMask            = 0x01;
    InitBlock_p->State                    = STSPI_IDLE;
    InitBlock_p->Buffer_p                 = NULL;
    InitBlock_p->BufferLen                = 0;
    InitBlock_p->BufferCnt                = 0;
    CtrlBlockArray[CtrBlkIndex].InUse     = TRUE;

    if (InitParams_p->DefaultParams != NULL)
    {
        InitBlock_p->InitParams.DefaultParams->MSBFirst     = InitParams_p->DefaultParams->MSBFirst;
        InitBlock_p->InitParams.DefaultParams->ClkPhase     = InitParams_p->DefaultParams->ClkPhase;
        InitBlock_p->InitParams.DefaultParams->Polarity     = InitParams_p->DefaultParams->Polarity;
        InitBlock_p->InitParams.DefaultParams->DataWidth    = InitParams_p->DefaultParams->DataWidth;
        InitBlock_p->InitParams.DefaultParams->BaudRate     = InitParams_p->DefaultParams->BaudRate;
        InitBlock_p->InitParams.DefaultParams->GlitchWidth  = InitParams_p->DefaultParams->GlitchWidth;
    }
    else /* store some logical values */
    {
        InitBlock_p->InitParams.DefaultParams->BaudRate     = STSPI_RATE_NORMAL;
        InitBlock_p->InitParams.DefaultParams->MSBFirst     = TRUE;
        InitBlock_p->InitParams.DefaultParams->ClkPhase     = TRUE;
        InitBlock_p->InitParams.DefaultParams->Polarity     = TRUE;
        InitBlock_p->InitParams.DefaultParams->GlitchWidth  = 0;
        InitBlock_p->InitParams.DefaultParams->DataWidth    = 7;
    }

#ifdef STSPI_MASTER_SLAVE
    ReturnCode = RegisterEvents (InitParams_p->EvtHandlerName, InitBlock_p);
#endif

    /*Initialise the semaphores*/
#ifdef ST_OS21
    InitBlock_p->IOSemaphore_p = semaphore_create_fifo_timeout(0);
    InitBlock_p->BusAccessSemaphore_p = semaphore_create_priority_timeout(1);
#else
    InitBlock_p->IOSemaphore_p = &InitBlock_p->IOSemaphore;
    InitBlock_p->BusAccessSemaphore_p = &InitBlock_p->BusAccessSemaphore;
    semaphore_init_fifo_timeout  ( InitBlock_p->IOSemaphore_p, 0 );
    semaphore_init_priority_timeout  ( InitBlock_p->BusAccessSemaphore_p, 1);
#endif

    if (ReturnCode == ST_NO_ERROR)
    {
        /* Reset SPI Control block */
        SpiRegPtr = (DU32*)InitBlock_p->InitParams.BaseAddress ;

        /* Turn off interrupts while programming block */
        SpiRegPtr[SSC_INT_ENABLE] = 0;
#if defined(ST_8010)
        if (( InitParams_p->BaseAddress == (U32*)SSC_0_BASE_ADDRESS)
            || ( InitParams_p->BaseAddress == (U32*)SSC_1_BASE_ADDRESS))
        {
            stspi_Setup();
        }
        else if ( InitParams_p->BaseAddress == (U32*)SSC_2_BASE_ADDRESS)
#endif
        {
            /* Initialise PIO for setup for SCL */
            SCLOpenParams.ReservedBits = InitParams_p->PIOforSCL.BitMask;

#ifdef STSPI_MASTER_SLAVE
            SCLOpenParams.BitConfigure[PinforSCL] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
#else
            SCLOpenParams.BitConfigure[PinforSCL] = STPIO_BIT_ALTERNATE_OUTPUT;
#endif

            PIOReturnCode = STPIO_Open (InitParams_p->PIOforSCL.PortName,
                                        &SCLOpenParams, &InitBlock_p->SCLHandle);

            if (PIOReturnCode != ST_NO_ERROR)
            {
                ReturnCode = STSPI_ERROR_PIO;
            }
            else
            {
                /* Configure MRST */
                MRSTOpenParams.ReservedBits = InitBlock_p->InitParams.PIOforMRST.BitMask;

#ifdef STSPI_MASTER_SLAVE
                MRSTOpenParams.BitConfigure[PinforMRST] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
#else
                MRSTOpenParams.BitConfigure[PinforMRST] = STPIO_BIT_INPUT;
#endif
                PIOReturnCode = STPIO_Open (InitParams_p->PIOforMRST.PortName,
                                            &MRSTOpenParams, &InitBlock_p->MRSTHandle);

                if (PIOReturnCode != ST_NO_ERROR)
                {
                    ReturnCode = STSPI_ERROR_PIO;
                }
                else
                {
                    /* Configure MTSR */
                    MTSROpenParams.ReservedBits = InitBlock_p->InitParams.PIOforMTSR.BitMask;

#ifdef STSPI_MASTER_SLAVE
                    MTSROpenParams.BitConfigure[PinforMTSR] = STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
#else
                    MTSROpenParams.BitConfigure[PinforMTSR] = STPIO_BIT_ALTERNATE_OUTPUT;
#endif
                    PIOReturnCode = STPIO_Open (InitParams_p->PIOforMTSR.PortName,
                                                  &MTSROpenParams, &InitBlock_p->MTSRHandle);

                    if (PIOReturnCode != ST_NO_ERROR)
                    {
                        ReturnCode = STSPI_ERROR_PIO;
                    }
                
                }
            }
        }

        if ( ReturnCode == ST_NO_ERROR)
        {
#if !defined (STI2C_STSPI_SAME_SSC_SUPPORT)
            /* Try to register and enable the interrupt handler */
            IntReturn = interrupt_install ( InitBlock_p->InitParams.InterruptNumber,
                                        InitBlock_p->InitParams.InterruptLevel,
                                        SpiHandler, InitBlock_p );
            if (IntReturn == 0)
            {

#ifdef ST_OS21
                IntReturn=interrupt_enable_number(InitBlock_p->InitParams.InterruptNumber);
#else

#ifdef STAPI_INTERRUPT_BY_NUMBER
                /* interrupt_enable() will be redundant after change will be performed in STBOOT */
                IntReturn =interrupt_enable(InitBlock_p->InitParams.InterruptLevel);
                if (IntReturn == 0)
                {
                    IntReturn=interrupt_enable_number(InitBlock_p->InitParams.InterruptNumber);
                }

#else

                IntReturn = interrupt_enable(InitBlock_p->InitParams.InterruptLevel);

#endif
#endif
                if (IntReturn == 0)
                {
#endif                    
                    /* Add the correct number of open blocks to the list */
                    InitBlock_p->OpenBlockPtr = (spi_open_block_t*)memory_allocate ( InitParams_p->DriverPartition,
                                                  (sizeof(spi_open_block_t) *(InitParams_p->MaxHandles)));

                    /* If pointer value not NULL the allocate suceeded */
                    if (InitBlock_p->OpenBlockPtr != 0)
                    {
                        /* Set block values to defaults */
                        for (OpenBlkIndex = 0; OpenBlkIndex < InitParams_p->MaxHandles; OpenBlkIndex++)
                        {
                            InitBlock_p->OpenBlockPtr[OpenBlkIndex].MagicNumber = 0;
                            InitBlock_p->OpenBlockPtr[OpenBlkIndex].CtrlBlkPtr  = InitBlock_p;
                        }
#if !defined(STI2C_STSPI_SAME_SSC_SUPPORT)
                        /* Perform bus reset -soft reset */
                        SpiReset(InitBlock_p);

                        /* Set Baud Rate */
                        WriteReg(SpiRegPtr, SSC_BAUD_RATE,
                        InitBlock_p->InitParams.ClockFrequency / ( 15 * InitBlock_p->InitParams.DefaultParams->BaudRate));
                        WriteReg(SpiRegPtr, SSC_GLITCH_WIDTH, InitBlock_p->InitParams.DefaultParams->GlitchWidth);

                        /* Set the default parameters */
                        if (InitBlock_p->InitParams.DefaultParams->MSBFirst)
                        {
                            ConValue |= SSC_CON_HEAD_CONTROL;
                        }
                        else
                        {
                            ConValue &= ~SSC_CON_HEAD_CONTROL;
                        }

                        if (InitBlock_p->InitParams.DefaultParams->ClkPhase)
                        {
                            ConValue |= SSC_CON_CLOCK_PHASE;
                        }
                        else
                        {
                            ConValue &= ~SSC_CON_CLOCK_PHASE;
                        }

                        if (InitBlock_p->InitParams.DefaultParams->Polarity)
                        {
                            ConValue |= SSC_CON_CLOCK_POLARITY;
                        }
                        else
                        {
                            ConValue &= ~SSC_CON_CLOCK_POLARITY;
                        }

                        ConValue |= InitBlock_p->InitParams.DefaultParams->DataWidth;
                        ConValue |= DEFAULT_CONTROL;

                        /* Program the control register */
                        WriteReg(SpiRegPtr, SSC_CONTROL, ConValue);

                        WriteReg(SpiRegPtr, SSC_PRE_SCALER,
                                (InitBlock_p->InitParams.ClockFrequency / 10000000));

                        /* Read spi buffer to clear spurious data */
                        (void)ReadReg(SpiRegPtr, SSC_RX_BUFFER);

                        /* enable interrupts from SPI registers */
                        WriteReg(SpiRegPtr, SSC_INT_ENABLE, InitBlock_p->InterruptMask);
#endif /*STI2C_STSPI_SAME_SSC_SUPPORT*/
                    }
                    else  /* Allocate open block failed */
                    {
                        ReturnCode = ST_ERROR_NO_MEMORY;
                    }
#if !defined (STI2C_STSPI_SAME_SSC_SUPPORT)
                }
                else  /* Interrupt enable failed */
                {
                    ReturnCode = ST_ERROR_BAD_PARAMETER;
                }
            }
            else
            {
                ReturnCode = ST_ERROR_INTERRUPT_INSTALL;
            }
#endif
        }
    }

    if (ReturnCode != ST_NO_ERROR)
    {
        /* Deallocate the open blocks */
        Spi_Deallocate(InitBlock_p);

        /* Clear the data in the control block*/
        memset (&CtrlBlockArray[CtrBlkIndex], 0, sizeof(spi_param_block_t));
        CtrlBlockArray[CtrBlkIndex].InUse = 0;

        /* Reset the devices */
        SpiRegPtr = (DU32*)InitBlock_p->InitParams.BaseAddress;
        SpiRegPtr[SSC_INT_ENABLE] = 0;
        SpiRegPtr[SSC_CONTROL] = 0;
    }
    
    /* Release global semaphore */
    semaphore_signal (g_DriverAtomic_p);

    return (ReturnCode);

}/* STSPI_Init */


/*****************************************************************************
STSPI_Open
Description:
    Create and return a handle to an initialised spi device.
Parameters  : 1) A pointer to the name of a device control block
              2) A pointer to parameter block
              3) A pointer to an empty handle
Return Value:
    ST_NO_ERROR,                       no errors.
    ST_ERROR_BAD_PARAMETER             One or more of the parameters was invalid
    ST_ERROR_UNKNOWN_DEVICE            Invalid device name
    ST_ERROR_NO_FREE_HANDLES           The maximum number of allocated handles are
                                       in use; cannot allocate another handle

See Also:
    STSPI_Close()
    STSPI_Init()
    STSPI_Read()
*****************************************************************************/

ST_ErrorCode_t STSPI_Open(    const ST_DeviceName_t      Name,
                              const STSPI_OpenParams_t   *OpenParams_p,
                              STSPI_Handle_t             *Handle)
{
    spi_param_block_t    *CtrlBlkPtr_p    = NULL;
    spi_open_block_t     *OpenBlkPtr_p    = NULL;
    U32                  CtrBlkIndex      = 0, OpenBlkIndex = 0;
#ifdef STSPI_MASTER_SLAVE
    STPIO_OpenParams_t   CSOpenParams;
    S32                  PinforCS;
    ST_ErrorCode_t       PIOReturnCode = ST_NO_ERROR;
#endif

    task_lock();
    if(Handle != NULL)
    {
        *Handle = 0;
    }

    if (TRUE == First_Init)
    {
        task_unlock();
        return (ST_ERROR_UNKNOWN_DEVICE);
    }
    task_unlock();

    if ((Name == 0) ||(strlen(Name) >= ST_MAX_DEVICE_NAME ) ||
        (Handle == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
#ifdef STSPI_MASTER_SLAVE
    PinforCS   = GetPioPin (OpenParams_p->PIOforCS.BitMask);
    if (PinforCS >= UCHAR_MAX)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
#endif
    /* Take the  global semaphore to make safe access to global control list */
    semaphore_wait (g_DriverAtomic_p);

    /* Walk the static array of Ctrl blocks until we find the right name */
    while (CtrBlkIndex < NUM_SSC)
    {
       if (strcmp( CtrlBlockArray[CtrBlkIndex].Name, Name ) == 0)
       {
            CtrlBlkPtr_p = &CtrlBlockArray[CtrBlkIndex];
            break;
       }
       else
       {
            CtrBlkIndex++;
       }
    }

    /* If no match was found, this device name has not been init'ed */
    if (CtrlBlkPtr_p == NULL)
    {
        /* release driver atomic */
        semaphore_signal (g_DriverAtomic_p);
        return (ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Take bus access. Note that we still have the global semaphore. */
    semaphore_wait (CtrlBlkPtr_p->BusAccessSemaphore_p);

    /* Release the global semaphore so that other func call can take place*/
    semaphore_signal (g_DriverAtomic_p);

    /* Check OpenCnt is less than MaxHandles */
    if (CtrlBlkPtr_p->OpenCnt >= CtrlBlkPtr_p ->InitParams.MaxHandles)
    {
        /* release semaphores */
        semaphore_signal(CtrlBlkPtr_p->BusAccessSemaphore_p);
        return (ST_ERROR_NO_FREE_HANDLES);
    }

    /* Search a free Open Block to assign to this Handle */
    while ((OpenBlkIndex < CtrlBlkPtr_p->InitParams.MaxHandles))
    {
        if (CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].MagicNumber == 0)
        {
            break;
        }
        else
        {
            OpenBlkIndex ++;
        }
    }

    if (OpenBlkIndex == CtrlBlkPtr_p->InitParams.MaxHandles)
    {
        /* release semaphores */
        semaphore_signal (CtrlBlkPtr_p->BusAccessSemaphore_p);
#ifndef STSPI_NO_TBX
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                     "STSPI: !! Got \"no free handles\" after checking open count !!\n"));
#endif
        return (ST_ERROR_NO_FREE_HANDLES);    /* Should never get this! */
    }

    OpenBlkPtr_p = &(CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex]);

    /* Increment the handle count in the control block */
    CtrlBlkPtr_p->OpenCnt++;

    memcpy(&OpenBlkPtr_p->OpenParams.PIOforCS,&OpenParams_p->PIOforCS,sizeof(STPIO_PIOBit_t));

    /* Configure CS */
#ifdef STSPI_MASTER_SLAVE
    CSOpenParams.ReservedBits  = OpenParams_p->PIOforCS.BitMask;
    CSOpenParams.IntHandler    = SPI_PioHandler;

    if (CtrlBlkPtr_p->InitParams.MasterSlave == STSPI_MASTER)
    {
        CSOpenParams.BitConfigure[PinforCS] = STPIO_BIT_OUTPUT;
        PIOReturnCode =  STPIO_Open (OpenParams_p->PIOforCS.PortName,
                                     &CSOpenParams, &CtrlBlkPtr_p->MasterCSHandle);
    }
    else
    {
        CSOpenParams.BitConfigure[PinforCS] = STPIO_BIT_INPUT;
        PIOReturnCode =  STPIO_Open (OpenParams_p->PIOforCS.PortName,
                                     &CSOpenParams, &CtrlBlkPtr_p->SlaveCSHandle);
    }
    if (PIOReturnCode != ST_NO_ERROR)
    {
        return (STSPI_ERROR_PIO);
    }
    else
    {
        if (CtrlBlkPtr_p->InitParams.MasterSlave == STSPI_MASTER)
        {
             CtrlBlkPtr_p->SlaveCSHandle = 0x00;
        }
        else
        {
             CtrlBlkPtr_p->MasterCSHandle = 0x00;
        }
    }
#endif

    /* Handle value is expected to be the memory address of associated open block*/
    *Handle = (STSPI_Handle_t)OpenBlkPtr_p;

    /* Place the driver Id as magic value to mark this block as in-use and valid*/
    OpenBlkPtr_p->MagicNumber = MAGIC_NUM;

    /* Release the access semaphore */
    semaphore_signal (CtrlBlkPtr_p->BusAccessSemaphore_p);

    return (ST_NO_ERROR);

}/* STSPI_Open */

/*****************************************************************************
STSPI_Read
Description:
    Called to initiate a read on the spi bus
Parameters  : 1) A handle to the spi device being read from
              2) A pointer to the buffer being read into
              3) The number of bytes to read
              4) The timeout for the whole transfer
              5) A pointer to an empty U32 which will have the actual number
                 of bytes read when the read completes.
Return Value:
    ST_NO_ERROR,                 no errors.
    ST_ERROR_INVALID_HANDLE      Device handle invalid.
    ST_ERROR_DEVICE_BUSY         The SPI device is in use by another application
    ST_ERROR_TIMEOUT             Time-out on read operation
    ST_ERROR_UNKNOWN_DEVICE      Device has not been initialized

See Also:
   STSPI_Write()
*****************************************************************************/

ST_ErrorCode_t STSPI_Read (   STSPI_Handle_t Handle,
                              U16            *Buffer_p,
                              U32            NumberToRead,
                              U32            Timeout,
                              U32            *ActLen_p)
{

    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    if ((spi_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    if ((Buffer_p == NULL) || (ActLen_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if (NumberToRead == 0)
    {
        *ActLen_p = 0;
        return (ST_NO_ERROR);
    }


    ErrorCode = BusAccess(Handle, Buffer_p, NumberToRead, Timeout, ActLen_p, READING);


    return (ErrorCode);

}/* STSPI_Read */

/*****************************************************************************
STSPI_SetParams
Description:
    Returns the current operating parameters
Parameters  : 1) A handle to the spi device being read from
              2) Pointer to the SPI parameter data structure
Return Value:
    ST_NO_ERROR,                 no errors.
    ST_ERROR_BAD_PARAMETER       One or more of the parameters was invalid
    ST_ERROR_UNKNOWN_DEVICE      Device has not been initialized
    ST_ERROR_INVALID_HANDLE      Device handle invalid.

See Also:
   STSPI_GetParams()
*****************************************************************************/

ST_ErrorCode_t STSPI_SetParams( STSPI_Handle_t       Handle,
                                const STSPI_Params_t *SetParams_p )
{
    spi_open_block_t    *OpenBlkPtr_p = NULL;
    spi_param_block_t   *CtrlBlkPtr_p = NULL;
    ST_ErrorCode_t      ErrorCode     = ST_NO_ERROR;
    DU32                *SpiRegPtr;
    U16                 ConValue      = 0x0000;

    if ((spi_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    if ((SetParams_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER );
    }

    if (SetParams_p->DataWidth == 0)
    {
        return (STSPI_ERROR_DATAWIDTH);
    }

    task_lock();
    if (TRUE == First_Init)
    {
        task_unlock();
        return (ST_ERROR_UNKNOWN_DEVICE);
    }
    task_unlock();

    /* Take global atomic */
    semaphore_wait(g_DriverAtomic_p);

    /* Set up the pointers to the open block  */
    OpenBlkPtr_p = (spi_open_block_t*)Handle;

    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
        /* Release Busaccess Semaphore */
        semaphore_signal(g_DriverAtomic_p);
        return (ST_ERROR_INVALID_HANDLE);
    }

    CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;

    SpiRegPtr = (DU32*)CtrlBlkPtr_p->InitParams.BaseAddress;

    /* Set baud rate */
    WriteReg(SpiRegPtr, SSC_BAUD_RATE,
              CtrlBlkPtr_p->InitParams.ClockFrequency / (2 * SetParams_p->BaudRate));

    /* Set glitch width */
    WriteReg(SpiRegPtr, SSC_GLITCH_WIDTH, SetParams_p->GlitchWidth);

    ConValue = 0x0000;
    if (SetParams_p->MSBFirst)
    {
        ConValue |= SSC_CON_HEAD_CONTROL;
    }
    else
    {
        ConValue &= ~SSC_CON_HEAD_CONTROL;
    }

    if (SetParams_p->ClkPhase)
    {
        ConValue |= SSC_CON_CLOCK_PHASE;
    }
    else
    {
        ConValue &= ~SSC_CON_CLOCK_PHASE;
    }

    if (SetParams_p->Polarity)
    {
        ConValue |= SSC_CON_CLOCK_POLARITY;
    }
    else
    {
        ConValue &= ~SSC_CON_CLOCK_POLARITY;
    }
    ConValue |= (SetParams_p->DataWidth-1);
    ConValue |= DEFAULT_CONTROL;

    /* Program the control register */
    WriteReg(SpiRegPtr, SSC_CONTROL, ConValue);

    /* Release the access semaphore */
    semaphore_signal(g_DriverAtomic_p);

    return (ErrorCode);

}/* STSPI_SetParams */

/*****************************************************************************
STSPI_Term
Description:
    Puts an init()ed spi device into a stable shut down state and
    deletes its associated control structures.
Parameters  : 1) A pointer to the name of a device to be shut down
              2) A pointer to the termination parameter block
Return Value:
    ST_NO_ERROR,                       no errors.
    ST_ERROR_INTERRUPT_UNINSTALL       Error Uninstalling interrupts for the APIs internal interrupt handler
    ST_ERROR_UNKNOWN_DEVICE            Device has not been initialized
    ST_ERROR_OPEN_HANDLE               Could not terminate driver; not all handles have been closed

See Also:
    STSPI_Close()
    STSPI_Init()
    STSPI_Open()
*****************************************************************************/

ST_ErrorCode_t STSPI_Term( const ST_DeviceName_t      Name,
                           const STSPI_TermParams_t   *TermParams_p )
{
    DU32                 *SpiRegPtr    = NULL;
    ST_ErrorCode_t       ReturnCode    = ST_NO_ERROR;
    spi_param_block_t    *CtrlBlkPtr_p = NULL;
    U32                  OpenBlkIndex  = 0, CtrBlkIndex = 0;

    if ((Name == 0) || (strlen(Name) >= ST_MAX_DEVICE_NAME ) ||
        (TermParams_p == NULL) )
    {
        return (ST_ERROR_BAD_PARAMETER) ;
    }

    task_lock();
    if (TRUE == First_Init)
    {
        task_unlock();
        return (ST_ERROR_UNKNOWN_DEVICE);
    }
    task_unlock();

    /* Take the  global semaphore to make safe access to global control list */
    semaphore_wait(g_DriverAtomic_p);

    /* Walk the static array of Ctrl blocks until we find the right name */
    while (CtrBlkIndex < NUM_SSC)
    {
        if (strcmp( CtrlBlockArray[CtrBlkIndex].Name, Name ) == 0)
        {
            CtrlBlkPtr_p = &CtrlBlockArray[CtrBlkIndex];
            break;
        }
        else
        {
            CtrBlkIndex++;
        }
    }

    /* If no match was found, this device name has not been init'ed */
    if (CtrlBlkPtr_p == NULL)
    {
        /* release driver atomic */
        semaphore_signal(g_DriverAtomic_p);
        return (ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Take bus access. Note that we still have the global semaphore.
       This is done  to ensure that nobody else is waiting on it when it gets deleted */
    semaphore_wait(CtrlBlkPtr_p->BusAccessSemaphore_p);

    /*Check the opencount */
    if ((CtrlBlkPtr_p->OpenCnt != 0) && (TermParams_p->ForceTerminate == FALSE))
    {
        semaphore_signal (g_DriverAtomic_p);
        semaphore_signal (CtrlBlkPtr_p->BusAccessSemaphore_p);
        return (ST_ERROR_OPEN_HANDLE);
    }

    /* Do Force termination of the block if required */
    while ((OpenBlkIndex < CtrlBlkPtr_p->InitParams.MaxHandles) && (CtrlBlkPtr_p->OpenCnt != 0))
    {
        if (CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].MagicNumber == MAGIC_NUM)
        {
            /* Decrement the control structure open count */
            CtrlBlkPtr_p->OpenCnt--;
            /* clear the magic value to mark it available for reuse */
            CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].MagicNumber = 0;
        }
        else
        {
            OpenBlkIndex++;
        }
    }

    SpiRegPtr = (DU32*)CtrlBlkPtr_p->InitParams.BaseAddress;
#if !defined(STI2C_STSPI_SAME_SSC_SUPPORT)
    /* Disable the Interrupt */
    SpiRegPtr[SSC_INT_ENABLE] = 0;

    /* Uninstall the interrupt handler for this port. Only disable
     * interrupt if no other handlers connected on the same level.
     * Note this needs to be MT-safe.
     */

    interrupt_lock();
    if (interrupt_uninstall(CtrlBlkPtr_p->InitParams.InterruptNumber,
                            CtrlBlkPtr_p->InitParams.InterruptLevel ) != 0)
    {
        ReturnCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }
#ifdef ST_OS21
    interrupt_disable_number(CtrlBlkPtr_p->InitParams.InterruptNumber);
#else

#ifdef STAPI_INTERRUPT_BY_NUMBER
    /* interrupt_enable() will be redundant after change will be performed in STBOOT */
    interrupt_disable (CtrlBlkPtr_p->InitParams.InterruptLevel);
    interrupt_disable_number(CtrlBlkPtr_p->InitParams.InterruptNumber);
#else
    interrupt_disable (CtrlBlkPtr_p->InitParams.InterruptLevel);
#endif /*STAPI_INTERRUPT_BY_NUMBER*/

#endif /*ST_OS21*/

    interrupt_unlock();       /* end of interrupt uninstallation */
#endif /*STI2C_STSPI_SAME_SSC_SUPPORT*/
    /* Deallocate all open blocks associated with this device */
    memory_deallocate (CtrlBlkPtr_p->InitParams.DriverPartition, CtrlBlkPtr_p->OpenBlockPtr);

    /* Deallocate memory allocated for default params */
    memory_deallocate (CtrlBlkPtr_p->InitParams.DriverPartition, CtrlBlkPtr_p->InitParams.DefaultParams);

    /* Reset the devices */
    SpiRegPtr[SSC_CONTROL] = 0;

    /* Deallocate the semaphores */
    semaphore_delete (CtrlBlkPtr_p->IOSemaphore_p);
    semaphore_delete (CtrlBlkPtr_p->BusAccessSemaphore_p);

    if ((CtrlBlkPtr_p->InitParams.PIOforMTSR.PortName != NULL) ||
        (CtrlBlkPtr_p->InitParams.PIOforMRST.PortName != NULL) ||
        (CtrlBlkPtr_p->InitParams.PIOforSCL.PortName != NULL))
    {
        (void)STPIO_Close (CtrlBlkPtr_p->MTSRHandle);
        (void)STPIO_Close (CtrlBlkPtr_p->MRSTHandle);
        (void)STPIO_Close (CtrlBlkPtr_p->SCLHandle);
        (void)STPIO_Close (CtrlBlkPtr_p->MasterCSHandle);
    }

#ifdef STSPI_MASTER_SLAVE
        (void)STPIO_Close (CtrlBlkPtr_p->SlaveCSHandle);
#endif

    /* clear the data in the control block*/
    memset (&CtrlBlockArray[CtrBlkIndex], 0, sizeof(spi_param_block_t ));

    /* We've now finished with the atomic operations */
    semaphore_signal(g_DriverAtomic_p);

    return (ReturnCode);

} /* STSPI_Term */

/*****************************************************************************
STSPI_Write
Description:
    Called to initiate a write on the spi bus
Parameters  : 1) A handle to the spi device being written to
              2) A pointer to the buffer being written to
              3) The number of bytes to write
              4) The timeout for the whole transfer
              5) A pointer to an empty U32 which will have the actual number
                 of bytes written when the write completes.
Return Value:
    ST_NO_ERROR,                 no errors.
    ST_ERROR_INVALID_HANDLE      Device handle invalid.
    ST_ERROR_DEVICE_BUSY         The SPI device is in use by another application
    ST_ERROR_TIMEOUT             Time-out on read operation
    ST_ERROR_UNKNOWN_DEVICE      Device has not been initialized

See Also:
   STSPI_Read()
*****************************************************************************/

ST_ErrorCode_t STSPI_Write( STSPI_Handle_t Handle,
                            U16            *Buffer_p,
                            U32            NumberToWrite,
                            U32            Timeout,
                            U32            *ActLen_p)
{
    ST_ErrorCode_t   ErrorCode = ST_NO_ERROR;
    if ((spi_open_block_t*)Handle == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    if ((Buffer_p == NULL) || (ActLen_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if (NumberToWrite == 0)
    {
        *ActLen_p = 0;
        return (ST_NO_ERROR);
    }

    ErrorCode = BusAccess(Handle, Buffer_p, NumberToWrite,
                               Timeout, ActLen_p, WRITING);

    return (ErrorCode);

}/* STSPI_Write */

/* BusAccess function actually initiates read/write Op.
 * Interrupt Handler is called from within this function.
 */
static ST_ErrorCode_t BusAccess( STSPI_Handle_t    Handle,
                                 U16               *Buffer_p,
                                 U32               BufferLen,
                                 U32               TimeOutMS,
                                 U32               *ActLen_p,
                                 spi_context_t     Context )
{
    const U32           TicksPerSec = ST_GetClocksPerSecond();
    DU32                *SpiRegPtr;
    clock_t             BusTimeOut, *pBusTimeOut_p;
    ST_ErrorCode_t      ReturnCode = ST_NO_ERROR;
    clock_t             *pTransferTimeOut, TimeOut;
    spi_param_block_t   *CtrlBlkPtr_p;
    spi_open_block_t    *OpenBlkPtr_p;
    S32                 SemReturn;
    U16                 TxData, RxData;

    /* Set up the pointers to the open block  */
    OpenBlkPtr_p = (spi_open_block_t*)Handle;
    if (OpenBlkPtr_p->MagicNumber != MAGIC_NUM)
    {
        /* Return error */
        return (ST_ERROR_INVALID_HANDLE);
    }

    /* Set bus access timeout */
    pBusTimeOut_p = ConvertMStoTicks (OpenBlkPtr_p->CtrlBlkPtr->InitParams.BusAccessTimeout, TicksPerSec,
                                     &BusTimeOut);
    if (pBusTimeOut_p != TIMEOUT_INFINITY)
    {
        *pBusTimeOut_p = time_plus (*pBusTimeOut_p, time_now());
    }
    task_lock();

    if (TRUE == First_Init)
    {
        task_unlock();
        return (ST_ERROR_UNKNOWN_DEVICE);
    }
    task_unlock();

    /* Take global atomic */
    SemReturn = semaphore_wait_timeout(g_DriverAtomic_p, pBusTimeOut_p);
    if (SemReturn != 0)
    {
        return (ST_ERROR_TIMEOUT);
    }

    /* Take CtrlBlkPtr and SpiRegptr */
    CtrlBlkPtr_p = OpenBlkPtr_p->CtrlBlkPtr;
    SpiRegPtr    = (DU32*)CtrlBlkPtr_p->InitParams.BaseAddress;
#if defined(STI2C_STSPI_SAME_SSC_SUPPORT)
    /* Get SSC to start operation */
    ReturnCode = Spi_GetSSC(CtrlBlkPtr_p);
    if(ReturnCode != ST_NO_ERROR)
    {
        semaphore_signal(g_DriverAtomic_p);
        return ReturnCode;
    }
#endif    
#ifdef STSPI_MASTER_SLAVE
    CtrlBlkPtr_p->SlaveCSHandle = 0x00;
#endif

    /* Check bus access has not been claimed already */
    SemReturn = semaphore_wait_timeout( (CtrlBlkPtr_p->BusAccessSemaphore_p),
                                        pBusTimeOut_p );
    if (SemReturn != 0)
    {
        semaphore_signal(g_DriverAtomic_p);

        /* Failed to claim bus access */
        return (ST_ERROR_DEVICE_BUSY);
    }

    /* Release global Semaphore so that other driver function can run */
    semaphore_signal(g_DriverAtomic_p);

    /* Tidy up the IOSemaphore as it may be out of sync. Note the routine
     * can exit with spurious events still on the semaphore
     */
    while (semaphore_wait_timeout (CtrlBlkPtr_p->IOSemaphore_p,
                                       TIMEOUT_IMMEDIATE) == 0)
    {}/* do nothing */

    /* Setup the control structure */
    CtrlBlkPtr_p->Buffer_p          = Buffer_p;
    CtrlBlkPtr_p->BufferLen         = BufferLen;
    CtrlBlkPtr_p->BufferCnt         = 0;

    /* Enable Master mode bearing in mind the soft reset restriction*/
    WriteReg(SpiRegPtr, SSC_CONTROL,
        SpiRegPtr[SSC_CONTROL]|(SSC_CON_MASTER_SELECT | SSC_CON_RESET));

    WriteReg(SpiRegPtr, SSC_CONTROL,
         SpiRegPtr[SSC_CONTROL]& ~SSC_CON_RESET);

    /* Make sure control has master-select*/
    WriteReg(SpiRegPtr, SSC_CONTROL,
                SpiRegPtr[SSC_CONTROL] | SSC_CON_MASTER_SELECT);

    /* Set the baud-rate to use */
    WriteReg(SpiRegPtr, SSC_BAUD_RATE,
             CtrlBlkPtr_p->InitParams.ClockFrequency / (2 * CtrlBlkPtr_p->InitParams.DefaultParams->BaudRate));

    /* Convert transfer timeout in ms to ticks */
    pTransferTimeOut = ConvertMStoTicks(TimeOutMS, TicksPerSec, &TimeOut );

    /* Derive timeout, if specified timeout not infinity */
    if (pTransferTimeOut != TIMEOUT_INFINITY)
    {
        *pTransferTimeOut = time_plus (*pTransferTimeOut, time_now());
    }

    /* Enable Tx and Rx side Fifo */
    SpiRegPtr[SSC_CONTROL] |= SSC_CON_ENB_TX_FIFO;
    SpiRegPtr[SSC_CONTROL] |= SSC_CON_ENB_RX_FIFO;

    if (Context == WRITING)
    {
#ifdef STSPI_MASTER_SLAVE
        /* Clear the CS line*/
        STPIO_Clear(CtrlBlkPtr_p->MasterCSHandle, CtrlBlkPtr_p->OpenBlockPtr->OpenParams.PIOforCS.BitMask);
#endif
        CtrlBlkPtr_p->State = STSPI_MASTER_WRITING;
        TxData = (*(CtrlBlkPtr_p->Buffer_p++));
        CtrlBlkPtr_p->BufferCnt++;
        SpiRegPtr[SSC_TX_BUFFER] = TxData;
        RxData = SpiRegPtr[SSC_RX_BUFFER];
    }
    else
    {
#ifdef STSPI_MASTER_SLAVE
        STPIO_Clear(CtrlBlkPtr_p->MasterCSHandle, CtrlBlkPtr_p->OpenBlockPtr->OpenParams.PIOforCS.BitMask);
#endif
        CtrlBlkPtr_p->First_Loop = TRUE;
        CtrlBlkPtr_p->State = STSPI_MASTER_READING;
        TxData = 0xff;
        SpiRegPtr[SSC_TX_BUFFER] = TxData;
    }

    /* Enable Appropriate SPI interrupts */
    CtrlBlkPtr_p->InterruptMask = SSC_IEN_TX_ERROR;
    SpiRegPtr[SSC_INT_ENABLE] = CtrlBlkPtr_p->InterruptMask;

    /* Now wait for transfer to complete */
    SemReturn = semaphore_wait_timeout(CtrlBlkPtr_p->IOSemaphore_p, pTransferTimeOut);

    /* Test semaphore for timeout */
    if (SemReturn != 0)
    {
       /* Force the driver to ideal mode*/
        CtrlBlkPtr_p->State = STSPI_IDLE;
        ReturnCode = ST_ERROR_TIMEOUT;
    }

    /* See how many bytes were transferred */
    *ActLen_p = CtrlBlkPtr_p->BufferCnt;
#if defined(STI2C_STSPI_SAME_SSC_SUPPORT)    
    /*Free the SSC to stop operation*/
    Spi_FreeSSC(CtrlBlkPtr_p);
#endif
    semaphore_signal(CtrlBlkPtr_p->BusAccessSemaphore_p);

    return (ReturnCode);

} /* BusAccess */

/******************************************************************************
Name        : Spi_Deallocate
Description : Deallocate
Parameters  : A pointer to the control structure for the spi device
Return      : None
******************************************************************************/

static void Spi_Deallocate( spi_param_block_t *CtrlBlkPtr_p )
{
    U32  OpenBlkIndex = 0;

    /* Clear the data in the open blocks that are currently being used  */
    while((OpenBlkIndex < CtrlBlkPtr_p->InitParams.MaxHandles) &&
          (CtrlBlkPtr_p->OpenCnt != 0))
    {
        if (CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].MagicNumber == MAGIC_NUM)
        {
            /* Decrement the control structure open count */
            CtrlBlkPtr_p->OpenCnt--;

            /* Clear the entries in the open block  */
            CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].CtrlBlkPtr = NULL;

            /* Clear the magic value to mark it availabe for reuse*/
            CtrlBlkPtr_p->OpenBlockPtr[OpenBlkIndex].MagicNumber = 0;
        }
        else
        {
            OpenBlkIndex++;
        }
    }

    /* Uninstall the interrupt handler */
    interrupt_lock();
    interrupt_uninstall (CtrlBlkPtr_p->InitParams.InterruptNumber,
                         CtrlBlkPtr_p->InitParams.InterruptLevel);
#ifdef ST_OS21
    interrupt_disable_number (CtrlBlkPtr_p->InitParams.InterruptNumber);
#else

#ifdef STAPI_INTERRUPT_BY_NUMBER
    /* interrupt_enable() will be redundant after change will be performed in STBOOT */
    interrupt_disable (CtrlBlkPtr_p->InitParams.InterruptLevel);
    interrupt_disable_number (CtrlBlkPtr_p->InitParams.InterruptNumber);
#else
    interrupt_disable (CtrlBlkPtr_p->InitParams.InterruptLevel);
#endif /*STAPI_INTERRUPT_BY_NUMBER*/

#endif /*ST_OS21*/

    interrupt_unlock();       /* end of interrupt uninstallation */

    /* Deallocate all open blocks associated with this device */
    memory_deallocate (CtrlBlkPtr_p->InitParams.DriverPartition, CtrlBlkPtr_p->OpenBlockPtr);

    /* deallocate the semaphores */
    semaphore_delete (CtrlBlkPtr_p->IOSemaphore_p);
    semaphore_delete (CtrlBlkPtr_p->BusAccessSemaphore_p);

    /* Close the PIO Handle  */
    if ((CtrlBlkPtr_p->InitParams.PIOforMTSR.PortName!= NULL) ||
        (CtrlBlkPtr_p->InitParams.PIOforMRST.PortName!= NULL) || (CtrlBlkPtr_p->InitParams.PIOforSCL.PortName!= NULL))
    {
        (void)STPIO_Close (CtrlBlkPtr_p->SCLHandle );
        (void)STPIO_Close (CtrlBlkPtr_p->MTSRHandle);
        (void)STPIO_Close (CtrlBlkPtr_p->MRSTHandle);
        (void)STPIO_Close (CtrlBlkPtr_p->MasterCSHandle);
    }

#ifdef STSPI_MASTER_SLAVE
    (void)STPIO_Close (CtrlBlkPtr_p->SlaveCSHandle );
#endif

}/* Spi_Deallocate */

#if defined(STI2C_STSPI_SAME_SSC_SUPPORT)
/*****************************************************************************
Spi_GetSSC
Description:
    Get SSC block by installing the interrupt handler.
******************************************************************************/
ST_ErrorCode_t Spi_GetSSC(spi_param_block_t *Params_p)
{
    DU32                 *SpiRegPtr   = NULL;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;
    U16                  ConValue     = 0x0000;
    spi_param_block_t   *CtrlBlkPtr_p;
    S32                  IntReturn;    
    
    CtrlBlkPtr_p = (spi_param_block_t *)Params_p;

    SpiRegPtr = (DU32*)CtrlBlkPtr_p->InitParams.BaseAddress ; 
    
    interrupt_lock();
               
    if(SpiRegPtr[SSC_STATUS] & SSC_STAT_BUSY)
    {
        interrupt_unlock();
        return ST_ERROR_DEVICE_BUSY;   
    }
    else
    {
        /* Try to register and enable the interrupt handler */
        IntReturn = interrupt_install ( CtrlBlkPtr_p->InitParams.InterruptNumber,
                                    CtrlBlkPtr_p->InitParams.InterruptLevel,
                                    SpiHandler, CtrlBlkPtr_p );

        if (IntReturn == 0)
        {
#ifndef ST_OS21
                /* interrupt_enable() will be redundant after change will be performed in STBOOT */
            IntReturn =interrupt_enable (CtrlBlkPtr_p->InitParams.InterruptLevel);
            if (IntReturn == 0)
#endif
            {
                IntReturn=interrupt_enable_number(CtrlBlkPtr_p->InitParams.InterruptNumber);
            }
            
            interrupt_unlock();
            
            SpiRegPtr[SSC_INT_ENABLE]= 0x01;
            if (IntReturn != 0)
           /* Interrupt enable failed */
            {
                ReturnCode = ST_ERROR_BAD_PARAMETER;
            }
            /* Perform bus reset -soft reset */
            SpiReset(CtrlBlkPtr_p);

            /* Set Baud Rate */
            WriteReg(SpiRegPtr, SSC_BAUD_RATE,
            CtrlBlkPtr_p->InitParams.ClockFrequency / ( 15 * CtrlBlkPtr_p->InitParams.DefaultParams->BaudRate));
            WriteReg(SpiRegPtr, SSC_GLITCH_WIDTH, CtrlBlkPtr_p->InitParams.DefaultParams->GlitchWidth);

            /* Set the default parameters */
            if (CtrlBlkPtr_p->InitParams.DefaultParams->MSBFirst)
            {
                ConValue |= SSC_CON_HEAD_CONTROL;
            }
            else
            {
                ConValue &= ~SSC_CON_HEAD_CONTROL;
            }

            if (CtrlBlkPtr_p->InitParams.DefaultParams->ClkPhase)
            {
                ConValue |= SSC_CON_CLOCK_PHASE;
            }
            else
            {
                ConValue &= ~SSC_CON_CLOCK_PHASE;
            }

            if (CtrlBlkPtr_p->InitParams.DefaultParams->Polarity)
            {
                ConValue |= SSC_CON_CLOCK_POLARITY;
            }
            else
            {
                ConValue &= ~SSC_CON_CLOCK_POLARITY;
            }

            ConValue |= CtrlBlkPtr_p->InitParams.DefaultParams->DataWidth;
            ConValue |= DEFAULT_CONTROL;

            /* Program the control register */
            WriteReg(SpiRegPtr, SSC_CONTROL, ConValue);

            WriteReg(SpiRegPtr, SSC_PRE_SCALER,
                    (CtrlBlkPtr_p->InitParams.ClockFrequency / 10000000));

            /* Read spi buffer to clear spurious data */
            (void)ReadReg(SpiRegPtr, SSC_RX_BUFFER);

            /* enable interrupts from SPI registers */
            WriteReg(SpiRegPtr, SSC_INT_ENABLE, CtrlBlkPtr_p->InterruptMask);            
        }
        else
        {
            interrupt_unlock();
            ReturnCode = ST_ERROR_INTERRUPT_INSTALL;
        }

    }
    return ( ReturnCode );
}
#endif

#if defined(STI2C_STSPI_SAME_SSC_SUPPORT)
/*****************************************************************************
Spi_FreeSSC
Description:
    Free SSC Block by Uninstalling the interrupt handler.
*****************************************************************************/
ST_ErrorCode_t Spi_FreeSSC(spi_param_block_t *Params_p)
{
    DU32                 *SpiRegPtr   = NULL;
    ST_ErrorCode_t       ReturnCode   = ST_NO_ERROR;
    spi_param_block_t    *CtrlBlkPtr_p;
    S32                  IntReturn;
    
    CtrlBlkPtr_p = (spi_param_block_t *)Params_p;  
    
    SpiRegPtr = (DU32*)CtrlBlkPtr_p->InitParams.BaseAddress;
    /* Disable the Interrupt */
    SpiRegPtr[SSC_INT_ENABLE] = 0;

    /* Uninstall the interrupt handler for this port. Only disable
     * interrupt if no other handlers connected on the same level.
     * Note this needs to be MT-safe.
     */
    interrupt_lock();
    if(SpiRegPtr[SSC_STATUS] & SSC_STAT_BUSY)
    {
       interrupt_unlock();     
       return ST_ERROR_DEVICE_BUSY;
    }
    else
    {
        IntReturn = interrupt_uninstall(CtrlBlkPtr_p->InitParams.InterruptNumber,
                            CtrlBlkPtr_p->InitParams.InterruptLevel );
        if (IntReturn == 0)                    
        {                    
#ifndef ST_OS21
            /* interrupt_enable() will be redundant after change will be performed in STBOOT */
            IntReturn =interrupt_disable (CtrlBlkPtr_p->InitParams.InterruptLevel);
            if (IntReturn == 0)
#endif
            {
                IntReturn=interrupt_disable_number(CtrlBlkPtr_p->InitParams.InterruptNumber);
            } 

            interrupt_unlock();
            
            if ( IntReturn != 0 )
            {
                ReturnCode = ST_ERROR_BAD_PARAMETER;
            }            
    
            SpiRegPtr[SSC_CONTROL] = 0;
        }
        else
        {
            interrupt_unlock();
            ReturnCode = ST_ERROR_INTERRUPT_UNINSTALL;
        }
    }    
    return( ReturnCode );
}
#endif



