/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2007.  All rights reserved.

File Name:   tsgdma.c
Description: API functions to control TSGDMA IP block


******************************************************************************/
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif

/* Includes ---------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX)  */

#include "stsys.h"
#include "stcommon.h"
#include "stos.h"
#include "osx.h"

#include "stpti.h"

#if defined (STPTI_FRONTEND_HYBRID)

#include "tsgdma.h"
#include "tsgdma_regs.h"

/* Exported Variables ------------------------------------------------------ */
semaphore_t * TSGDMA_DataEmpty[TSGDMA_NUM_SWTS];
semaphore_t * TSGDMA_SWTSLock[TSGDMA_NUM_SWTS];

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Private Variables ------------------------------------------------------- */
static U32 TSGDMA_ISRInstallCount = 0;
/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */
STOS_INTERRUPT_DECLARE(TSGDMA_Int_Handler, Param);

/* Functions --------------------------------------------------------------- */

/******************************************************************************
Function Name : stpti_InitialiseTSGDMA
  Description : Initialises the PTI TSGDMA not the 1394 0r TSout TSGDMA.
   Parameters :
******************************************************************************/
void TsgdmaHAL_SlimCoreInit(void)
{
    static BOOL First = TRUE;

    U32 i=0;

    STTBX_Print(("TsgdmaHAL_SlimCoreInit\n"));

    if (TRUE == First)
    {
        /* Clear any pending interrupt(s) */
        STSYS_WriteRegDev32LE((void*)TSGDMA_MAILBOX1_MASK, 0 );
        STSYS_WriteRegDev32LE((void*)TSGDMA_MAILBOX1_CLEAR, 0xFFFFFFFF );

        /* Create semaphores for the handling of swts data */
        for (i=0;i<TSGDMA_NUM_SWTS;i++)
        {
            TSGDMA_DataEmpty[i] = STOS_SemaphoreCreateFifo( NULL, 0 );
            TSGDMA_SWTSLock[i]  = STOS_SemaphoreCreateFifo( NULL, 1 );
        }
#if 0
        U32 CpuRun;

        STTBX_Print(("Stopping the SLIM core\n"));
        STSYS_WriteRegDev32LE((void*)TSGDMA_SLIM_CPU_RUN, 0);
        /*Check the core is stopped*/
        while ( !(0x02 & (CpuRun = STSYS_ReadRegDev32LE((void*)TSGDMA_SLIM_CPU_RUN))));
#endif
        First = FALSE;

        /* Set up TSGDMA dataram with the two blocks of initialisation data */
        STTBX_Print(("Loading TSGDMA_TDA1 @ 0x%08X\n", (U32 )(TSGDMA_DMEM_MEM_BASE+TSGDMA_TDA1_BASE_ADDR) ));
        STTBX_Print(("Loading TSGDMA_DATA @ 0x%08X\n", (U32 )(TSGDMA_DMEM_MEM_BASE+TSGDMA_DATA_BASE_ADDR) ));
        STTBX_Print(("Loading TSGDMA_PROG @ 0x%08X\n", (U32 )(TSGDMA_IMEM_BASE) ));

        memcpy((U32 *)(TSGDMA_DMEM_MEM_BASE+(TSGDMA_TDA1_BASE_ADDR*4)),TSGDMA_TDA1,sizeof(TSGDMA_TDA1));
        memcpy((U32 *)(TSGDMA_DMEM_MEM_BASE+(TSGDMA_DATA_BASE_ADDR*4)),TSGDMA_Data,sizeof(TSGDMA_Data));
        /* Set up the TSGDMA program code */
        memcpy((U32 *)(TSGDMA_IMEM_BASE),TSGDMA_Prog,sizeof(TSGDMA_Prog));

        /* Write 1 to STBUS Sync, to allow host to write to TSGDMA DMEM or IMEM once slimcore is running */
        STSYS_WriteRegDev32LE( (void*)TSGDMA_STBUS_SYNC, 1 );

        /* Write to enable dma_req[5:0] : all channels enabled */
        STSYS_WriteRegDev32LE( (void*)TSGDMA_DMAREQMASK, 0x3F );
#if defined (STTBX_PRINT)
        {
            volatile U32* addr=(U32*)(TSGDMA_DMEM_MEM_BASE+TSGDMA_DATA_BASE_ADDR);
            int i,j;

            for (i=0;i<30;i++)
            {
                STTBX_Print(("0x%08x    :", (U32)addr));
                for (j=0;j<6;j++)
                {
                STTBX_Print(("0x%08x ",*(addr++) ));

                }
                STTBX_Print(("\n"));
            }
        }
#endif

        /* IMPORTANT !! : COUNTER writes must be done in this order*/
        /* Reset the free running 32 bit time counter to a 1ms window @266MHz */
        /* We need to check this value for portability issues */
        /* PJW Doesn't this trigger immediately as counter = trigger val ?*/
        STSYS_WriteRegDev32LE( (void*)TSGDMA_COUNTER_TRIGGER_VAL, 0x00040f10 );
        STSYS_WriteRegDev32LE( (void*)TSGDMA_COUNTER, 0x00040f10) ;

        for (i=0; i < TSGDMA_NUM_SW_CHANNELS; i++ )
        {
            /*setting TSGDMA_BYTES_THRESHOLD to MAX for SWTS channel */
            STSYS_WriteRegDev32LE((void*) TSGDMA_BYTES_THRESHOLD(i), 0x7FFFFFFF);
        }
        /* start the slimcore */
        STSYS_WriteRegDev32LE( (void*)TSGDMA_SLIM_CPU_RUN, 0x01);
        STTBX_Print(("Starting the SLIM core\n"));
        STTBX_Print(("TSGDMA_SLIM_CPU_RUN 0x%08X\n", *(U32 *)(TSGDMA_SLIM_CPU_RUN) ));
        {
            U8 cnt;
            for ( cnt=0;cnt<5;cnt++)
            {
                STTBX_Print(("TSGDMA_SLIM_CPU_PC  0x%08X\n",  *(U32 *)(TSGDMA_SLIM_CPU_PC) ));
            }
        }
    }/*if (TRUE == First)*/
}

/******************************************************************************
Function Name : TsgdmaHAL_ReadDREQStatus
  Description :
   Parameters : None
******************************************************************************/
U32 TsgdmaHAL_ReadDREQStatus(void)
{
    return STSYS_ReadRegDev32LE(TSGDMA_DMAREQSTATUS);
}


/******************************************************************************
Function Name : TsgdmaHAL_WritePTIDestination
  Description :
   Parameters : StreamID
                DestinationPTI
******************************************************************************/
void TsgdmaHAL_WritePTILiveDestination(STPTI_StreamID_t StreamID, STPTI_DevicePtr_t DestinationPTI)
{
    U32 Dest;
    U32 Tmp = 0;

    switch ( (U32)DestinationPTI )
    {
#if defined(ST_OSLINUX)
        case 1:
#endif
        case PTIB_BASE_ADDRESS:
            {
                if ( (U32)StreamID & TSGDMA_REP_BIT )
                {
                    Tmp = TSGDMA_DEST_PTI1_M;
                    STTBX_Print(("TsgdmaHAL_WritePTILiveDestination - Routing StreamID 0x%04x to PTI_B with Rep Bit set\n", StreamID ));
                }
                else
                {
                    Tmp = TSGDMA_DEST_PTI1;
                    STTBX_Print(("TsgdmaHAL_WritePTILiveDestination - Routing StreamID 0x%04x to PTI_B\n", StreamID ));
                }
            }
            break;
#if defined(ST_OSLINUX)
        case 0:
#endif
        case PTIA_BASE_ADDRESS:
        default :
            {
                if ( (U32)StreamID & TSGDMA_REP_BIT )
                {
                    Tmp = TSGDMA_DEST_PTI0_M;
                    STTBX_Print(("TsgdmaHAL_WritePTILiveDestination - Routing StreamID 0x%04x to PTI_A with Rep Bit set\n", StreamID ));
                }
                else
                {
                    Tmp = TSGDMA_DEST_PTI0;
                    STTBX_Print(("TsgdmaHAL_WritePTILiveDestination - Routing StreamID 0x%04x to PTI_A\n", StreamID ));
                }
            }
            break;
    }

    Dest = STSYS_ReadRegDev32LE(TSGDMA_LIVE_DEST((StreamID & 0x3F)));
    Dest = ((Dest & 0x0000000F) | Tmp);

    STSYS_WriteRegDev32LE(TSGDMA_LIVE_DEST((StreamID & 0x3F)), Dest);
}


/******************************************************************************
Function Name : TsgdmaHAL_ClearPTILiveDestination
  Description :
   Parameters : StreamID
                DestinationPTI
******************************************************************************/
void TsgdmaHAL_ClearPTILiveDestination(STPTI_StreamID_t StreamID, STPTI_DevicePtr_t DestinationPTI)
{
    U32 Dest;
    U32 Tmp = 0;

    switch ( (U32)DestinationPTI )
    {
#if defined(ST_OSLINUX)
        case 1:
#endif
        case PTIB_BASE_ADDRESS:
            {
                if ( (U32)StreamID & TSGDMA_REP_BIT )
                {
                    Tmp = ~((U32)TSGDMA_DEST_PTI1_M);
                    STTBX_Print(("TsgdmaHAL_ClearPTILiveDestination - Stopping StreamID 0x%04x to PTI_B with Rep Bit set\n", StreamID ));
                }
                else
                {
                    Tmp = ~((U32)TSGDMA_DEST_PTI1);
                    STTBX_Print(("TsgdmaHAL_ClearPTILiveDestination - Stopping StreamID 0x%04x to PTI_B\n", StreamID ));
                }
            }
            break;
#if defined(ST_OSLINUX)
        case 0:
#endif
        case PTIA_BASE_ADDRESS:
        default :
            {
                if ( (U32)StreamID & TSGDMA_REP_BIT )
                {
                    Tmp = ~((U32)TSGDMA_DEST_PTI0_M);
                    STTBX_Print(("TsgdmaHAL_ClearPTILiveDestination - Stopping StreamID 0x%04x to PTI_A with Rep Bit set\n", StreamID ));
                }
                else
                {
                    Tmp = ~((U32)TSGDMA_DEST_PTI0);
                    STTBX_Print(("TsgdmaHAL_ClearPTILiveDestination - Stopping StreamID 0x%04x to PTI_A\n", StreamID ));
                }
            }
            break;
    }

    Dest = STSYS_ReadRegDev32LE(TSGDMA_LIVE_DEST((StreamID & 0x3F)));
    Dest = ((Dest & 0x0000000F) & Tmp);

    STSYS_WriteRegDev32LE(TSGDMA_LIVE_DEST((StreamID & 0x3F)), Dest);
}


/******************************************************************************
Function Name : TsgdmaHAL_StartLiveChannel
  Description :
   Parameters :
******************************************************************************/
void TsgdmaHAL_StartLiveChannel(STPTI_StreamID_t StreamID)
{
    U32 bit_location;
    U32 clear_value_mbx0;
    U32 LoopCnt;

    bit_location = (0x1 << (StreamID & 0x3F));
    clear_value_mbx0 = STSYS_ReadRegDev32LE(TSGDMA_MAILBOX0_STATUS);

    clear_value_mbx0 = clear_value_mbx0 | bit_location;

    STTBX_Print(("TSGDMA: Request a new channel configuration \n"));
    STSYS_WriteRegDev32LE((void*)TSGDMA_MAILBOX0_CLEAR, clear_value_mbx0);

    for( LoopCnt=0;LoopCnt<10;LoopCnt++)
    {
        if ( bit_location & STSYS_ReadRegDev32LE( TSGDMA_MAILBOX0_STATUS ) )
        {
            STOS_TaskDelayUs(10);
        }
        else
        {
            break;
        }
    }
    STTBX_Print(("TsgdmaHAL_StartLiveChannel - LoopCnt = %u\n", LoopCnt));
}

/******************************************************************************
Function Name : TsgdmaHAL_StartSWTSChannel
  Description :
   Parameters : StreamID = The stream ID of the SWTS channel
                LinkedList_p = A pointer to the Linked list used by TSGDMA

******************************************************************************/
void TsgdmaHAL_StartSWTSChannel(STPTI_StreamID_t StreamID, TSGDMA_SWTSNode_t *LinkedList_p)
{
    U32 BitLocation;

    STSYS_WriteRegDev32LE((void*)TSGDMA_NEXT_NODE( (StreamID & 0x3F) ), (U32) STOS_VirtToPhys( LinkedList_p ));

    BitLocation = (0x1 << (StreamID&0x3F));

    STTBX_Print(("TSGDMA: TsgdmaHAL_StartSWTSChannel StreamID=0x%02x\n LinkedList_p->NextNode_p=0x%x\n LinkedList_p->CtrlWord=0x%x\n LinkedList_p->TSData_p=0x%x\nLinkedList_p->ActualSize=0x%x\n LinkedList_p->PacketLength=0x%x\n LinkedList_p->Channel_Dest=0x%x\n LinkedList_p->TagHeader=0x%x\n LinkedList_p->Stuffing=0x%x\n BitLocation=%d\n",StreamID, LinkedList_p->NextNode_p,LinkedList_p->CtrlWord,LinkedList_p->TSData_p,LinkedList_p->ActualSize,LinkedList_p->PacketLength,LinkedList_p->Channel_Dest,LinkedList_p->TagHeader,LinkedList_p->Stuffing,BitLocation ));

    /* Note: Starting the injection BEFORE enabling the interrupt here is deliberate, due to the behavour of the TSGDMA.

             The interrupt is level triggered, based on an AND of the corresponding bits in the STATUS and MASK registers.

             As a result, if the MASK register bit is set BEFORE clearing the STATUS register bit, an interrupt occurs
             immediately, which is interpreted by the system as a spurious interrupt.

             There is no need to worry about the interrupt being generated whilst the MASK bit is clear, because as soon
             as the MASK bit is enabled, if the TSGDMA engine has completed the transfer already, the STATUS register bit
             will be set, resulting in an interrupt firing.
    */

    /* Start the injection */
    STSYS_WriteRegDev32LE((void*)TSGDMA_MAILBOX1_CLEAR, BitLocation);

    /* Enable the interrupt - atomic operation */
    stpti_AtomicSetBit( (void *)TSGDMA_MAILBOX1_MASK, StreamID & 0x3F );

    /* Wait for completion signal from the ISR */
    STOS_SemaphoreWait(TSGDMA_DataEmpty[StreamID&0x3F]);
}

/******************************************************************************
Function Name : TsgdmaHAL_WriteLiveDMA
  Description : Configure DMA params for live stream
   Parameters : StreamID
                Base
                Size
                PktSize
******************************************************************************/
void TsgdmaHAL_WriteLiveDMA(STPTI_StreamID_t StreamID, U32 Base, U32 Size, U32 PktLen)
{
    STTBX_Print(("TsgdmaHAL_WriteLiveDMA StreamID 0x%04x Base 0x%08x Size 0x%08x Pktlen 0x%08x\n", (U32)StreamID, Base, Size, PktLen));

    STSYS_WriteRegDev32LE(TSGDMA_LIVE_BUFFERBASE   ((StreamID & 0x3F)), Base   );
    STSYS_WriteRegDev32LE(TSGDMA_LIVE_BUFFERSIZE   ((StreamID & 0x3F)), Size   );
    STSYS_WriteRegDev32LE(TSGDMA_LIVE_PACKET_LENGTH((StreamID & 0x3F)), PktLen);
}

/******************************************************************************
Function Name : TsgdmaHAL_WriteSWTSPace
  Description : Configure Pacing for the SWTS TSGDMA interface
   Parameters : StreamID
                PaceBps
******************************************************************************/
void TsgdmaHAL_WriteSWTSPace (STPTI_StreamID_t StreamID, U32 PaceBps)
{
    U32 Kbps = 0x7FFFFFFF;

    /* If the pace is set to zero then set to the default max rate */
    if ( PaceBps != 0 )
    {
        Kbps = PaceBps/(U32)1000;
    }
    STTBX_Print(("TsgdmaHAL_WriteSWTSPace StreamID 0x%04x Rate %u\n",StreamID, Kbps));

    STSYS_WriteRegDev32LE((void*) TSGDMA_BYTES_THRESHOLD((StreamID & 0x3F)), Kbps);
}

/******************************************************************************
Function Name : TsgdmaHAL_RegisterDump
  Description :
   Parameters :
******************************************************************************/
void TsgdmaHAL_RegisterDump(STPTI_StreamID_t StreamID)
{
    U32 Sw_Ch_No, Live_Ch_No;

    Sw_Ch_No = Live_Ch_No = StreamID;

    if ( (STPTI_STREAM_IDTYPE_TSIN << 8) & Live_Ch_No )
    {
    STTBX_Print(("TSGDMA_DMAREQHOLDOFF                        0x%08X\n",                     *(U32 *)TSGDMA_DMAREQHOLDOFF      ((Live_Ch_No & 0x3F))  ));
    }
    STTBX_Print(("TSGDMA_STBUS_SYNC                           0x%08X\n",                     *(U32 *)TSGDMA_STBUS_SYNC                                ));
    STTBX_Print(("TSGDMA_STBUS_ACCESS                         0x%08X\n",                     *(U32 *)TSGDMA_STBUS_ACCESS                              ));
    STTBX_Print(("TSGDMA_STBUS_ADDRESS                        0x%08X\n",                     *(U32 *)TSGDMA_STBUS_ADDRESS                             ));
    STTBX_Print(("TSGDMA_COUNTER                              0x%08X\n",                     *(U32 *)TSGDMA_COUNTER                                   ));
    STTBX_Print(("TSGDMA_COUNTER_TRIGGER_VAL                  0x%08X\n",                     *(U32 *)TSGDMA_COUNTER_TRIGGER_VAL                       ));
    STTBX_Print(("TSGDMA_COUNTER_TRIGGER                      0x%08X\n",                     *(U32 *)TSGDMA_COUNTER_TRIGGER                           ));
    STTBX_Print(("TSGDMA_PACE_COUNTER                         0x%08X\n",                     *(U32 *)TSGDMA_PACE_COUNTER                              ));
    STTBX_Print(("TSGDMA_PACE_COUNTER_TRIGGER_VAL             0x%08X\n",                     *(U32 *)TSGDMA_PACE_COUNTER_TRIGGER_VAL                  ));
    STTBX_Print(("TSGDMA_PACE_COUNTER_TRIGGER                 0x%08X\n",                     *(U32 *)TSGDMA_PACE_COUNTER_TRIGGER                      ));
    STTBX_Print(("TSGDMA_PACING_MULTIPLE                      0x%08X\n",                     *(U32 *)TSGDMA_PACING_MULTIPLE                           ));
    STTBX_Print(("TSGDMA_T2INIT_DATA_FLAG                     0x%08X\n",                     *(U32 *)TSGDMA_T2INIT_DATA_FLAG                          ));
    STTBX_Print(("TSGDMA_TSCOPRO_IDLE_FLAG                    0x%08X\n",                     *(U32 *)TSGDMA_TSCOPRO_IDLE_FLAG                         ));
    STTBX_Print(("TSGDMA_1394_CLOCK_SELECT0                   0x%08X\n",                     *(U32 *)TSGDMA_1394_CLOCK_SELECT0                        ));
    STTBX_Print(("TSGDMA_1394_CLOCK_SELECT1                   0x%08X\n",                     *(U32 *)TSGDMA_1394_CLOCK_SELECT1                        ));
    STTBX_Print(("TSGDMA_MAILBOX0_STATUS                      0x%08X\n",                     *(U32 *)TSGDMA_MAILBOX0_STATUS                           ));
    STTBX_Print(("TSGDMA_MAILBOX0_SET                         0x%08X\n",                     *(U32 *)TSGDMA_MAILBOX0_SET                              ));
    STTBX_Print(("TSGDMA_MAILBOX0_CLEAR                       0x%08X\n",                     *(U32 *)TSGDMA_MAILBOX0_CLEAR                            ));
    STTBX_Print(("TSGDMA_MAILBOX0_MASK                        0x%08X\n",                     *(U32 *)TSGDMA_MAILBOX0_MASK                             ));
    STTBX_Print(("TSGDMA_MAILBOX1_STATUS                      0x%08X\n",                     *(U32 *)TSGDMA_MAILBOX1_STATUS                           ));
    STTBX_Print(("TSGDMA_MAILBOX1_SET                         0x%08X\n",                     *(U32 *)TSGDMA_MAILBOX1_SET                              ));
    STTBX_Print(("TSGDMA_MAILBOX1_CLEAR                       0x%08X\n",                     *(U32 *)TSGDMA_MAILBOX1_CLEAR                            ));
    STTBX_Print(("TSGDMA_MAILBOX1_MASK                        0x%08X\n",                     *(U32 *)TSGDMA_MAILBOX1_MASK                             ));
    STTBX_Print(("TSGDMA_DMAREQSTATUS                         0x%08X\n",                     *(U32 *)TSGDMA_DMAREQSTATUS                              ));
    STTBX_Print(("TSGDMA_DMAREQMASK                           0x%08X\n",                     *(U32 *)TSGDMA_DMAREQMASK                                ));
    STTBX_Print(("TSGDMA_DMAREQDIV                            0x%08X\n",                     *(U32 *)TSGDMA_DMAREQDIV                                 ));
    STTBX_Print(("TSGDMA_DMAREQDIVCOUNT                       0x%08X\n",                     *(U32 *)TSGDMA_DMAREQDIVCOUNT                            ));
    STTBX_Print(("TSGDMA_DMAREQTEST                           0x%08X\n",                     *(U32 *)TSGDMA_DMAREQTEST                                ));
    STTBX_Print(("TSGDMA_GPOUT                                0x%08X\n",                     *(U32 *)TSGDMA_GPOUT                                     ));
    STTBX_Print(("TSGDMA_T2_INIT                              0x%08X\n",                     *(U32 *)TSGDMA_T2_INIT                                   ));
    STTBX_Print(("TSGDMA_CP_IDLE                              0x%08X\n",                     *(U32 *)TSGDMA_CP_IDLE                                   ));
    STTBX_Print(("TSGDMA_CHANNEL_STALL_ENABLE    0x%08X       0x%08X\n", TSGDMA_CHANNEL_STALL_ENABLE, *(U32 *)TSGDMA_CHANNEL_STALL_ENABLE             ));
    STTBX_Print(("TSGDMA_CHANNEL_0_CFG           0x%08X       0x%08X\n", TSGDMA_CHANNEL_0_CFG,        *(U32 *)TSGDMA_CHANNEL_0_CFG                    ));
    STTBX_Print(("TSGDMA_CHANNEL_1_CFG           0x%08X       0x%08X\n", TSGDMA_CHANNEL_1_CFG,        *(U32 *)TSGDMA_CHANNEL_1_CFG                    ));
    if ( (STPTI_STREAM_IDTYPE_SWTS << 8) & Sw_Ch_No )
    {
    STTBX_Print(("TSGDMA_SWTS_REG_BASE      (Sw_Ch_No: %u)    0x%08X\n", (Sw_Ch_No & 0x3F),   *(U32 *)TSGDMA_SWTS_REG_BASE       ((Sw_Ch_No & 0x3F))  ));
    STTBX_Print(("TSGDMA_BYTES_THRESHOLD    (Sw_Ch_No: %u)    0x%08X\n", (Sw_Ch_No & 0x3F),   *(U32 *)TSGDMA_BYTES_THRESHOLD     ((Sw_Ch_No & 0x3F))  ));
    STTBX_Print(("TSGDMA_CURRENT_NODE       (Sw_Ch_No: %u)    0x%08X\n", (Sw_Ch_No & 0x3F),   *(U32 *)TSGDMA_CURRENT_NODE        ((Sw_Ch_No & 0x3F))  ));
    STTBX_Print(("TSGDMA_NEXT_NODE          (Sw_Ch_No: %u)    0x%08X\n", (Sw_Ch_No & 0x3F),   *(U32 *)TSGDMA_NEXT_NODE           ((Sw_Ch_No & 0x3F))  ));
    STTBX_Print(("TSGDMA_CTRL               (Sw_Ch_No: %u)    0x%08X\n", (Sw_Ch_No & 0x3F),   *(U32 *)TSGDMA_CTRL                ((Sw_Ch_No & 0x3F))  ));
    STTBX_Print(("TSGDMA_SOFT_BUFFERBASE    (Sw_Ch_No: %u)    0x%08X\n", (Sw_Ch_No & 0x3F),   *(U32 *)TSGDMA_SOFT_BUFFERBASE     ((Sw_Ch_No & 0x3F))  ));
    STTBX_Print(("TSGDMA_SOFT_BUFFERSIZE    (Sw_Ch_No: %u)    0x%08X\n", (Sw_Ch_No & 0x3F),   *(U32 *)TSGDMA_SOFT_BUFFERSIZE     ((Sw_Ch_No & 0x3F))  ));
    STTBX_Print(("TSGDMA_SOFT_PACKET_LENGTH (Sw_Ch_No: %u)    0x%08X\n", (Sw_Ch_No & 0x3F),   *(U32 *)TSGDMA_SOFT_PACKET_LENGTH  ((Sw_Ch_No & 0x3F))  ));
    STTBX_Print(("TSGDMA_SOFT_DEST          (Sw_Ch_No: %u)    0x%08X\n", (Sw_Ch_No & 0x3F),   *(U32 *)TSGDMA_SOFT_DEST           ((Sw_Ch_No & 0x3F))  ));
    STTBX_Print(("TSGDMA_TAG_HEADER         (Sw_Ch_No: %u)    0x%08X\n", (Sw_Ch_No & 0x3F),   *(U32 *)TSGDMA_TAG_HEADER          ((Sw_Ch_No & 0x3F))  ));
    }
    else if ( (STPTI_STREAM_IDTYPE_TSIN << 8) & Live_Ch_No )
    {
    STTBX_Print(("TSGDMA_LIVE_REG_BASE      (Live_Ch_No: %u)  0x%08X\n", (Live_Ch_No & 0x3F),  (U32  )TSGDMA_LIVE_REG_BASE       ((Live_Ch_No & 0x3F)) ));
    STTBX_Print(("TSGDMA_LIVE_BUFFERBASE    (Live_Ch_No: %u)  0x%08X\n", (Live_Ch_No & 0x3F), *(U32 *)TSGDMA_LIVE_BUFFERBASE     ((Live_Ch_No & 0x3F)) ));
    STTBX_Print(("TSGDMA_LIVE_BUFFERSIZE    (Live_Ch_No: %u)  0x%08X\n", (Live_Ch_No & 0x3F), *(U32 *)TSGDMA_LIVE_BUFFERSIZE     ((Live_Ch_No & 0x3F)) ));
    STTBX_Print(("TSGDMA_LIVE_PACKET_LENGTH (Live_Ch_No: %u)  0x%08X\n", (Live_Ch_No & 0x3F), *(U32 *)TSGDMA_LIVE_PACKET_LENGTH  ((Live_Ch_No & 0x3F)) ));
    STTBX_Print(("TSGDMA_LIVE_DEST          (Live_Ch_No: %u)  0x%08X\n", (Live_Ch_No & 0x3F), *(U32 *)TSGDMA_LIVE_DEST           ((Live_Ch_No & 0x3F)) ));

    STTBX_Print(("TSGDMA_LIVE_PRV_BUFFER_BASE                 0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x04) ));
    STTBX_Print(("TSGDMA_LIVE_PRV_BUFFER_SIZE                 0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x08) ));
    STTBX_Print(("TSGDMA_LIVE_PRV_PKT_LENGTH                  0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x0C) ));
    STTBX_Print(("TSGDMA_LIVE_PRV_DEST                        0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x10) ));

    STTBX_Print(("TSGDMA_LIVE_PRV_HDR_TAG_USED                0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x14) ));
    STTBX_Print(("TSGDMA_LIVE_PRV_RD_PTR                      0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x18) ));
    STTBX_Print(("TSGDMA_LIVE_PRV_DEST_PREV                   0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x1C) ));
    STTBX_Print(("TSGDMA_LIVE_PRV_NUM_PKT                     0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x20) ));
    STTBX_Print(("TSGDMA_LIVE_PRV_PREV_WRD                    0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x24) ));
    STTBX_Print(("TSGDMA_LIVE_PRV_INLD32_SHFT                 0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x28) ));
    STTBX_Print(("TSGDMA_LIVE_PRV_PKT_DEST                    0x%08X\n",  *(U32 *)(TSGDMA_LIVE_DEST       ((Live_Ch_No & 0x3F)) + 0x2C) ));
    {
        U8 cnt;
        for ( cnt=0;cnt<5;cnt++)
        {
            STTBX_Print(("TSGDMA_SLIM_CPU_PC                          0x%08X\n",  *(U32 *)(TSGDMA_SLIM_CPU_PC) ));
        }
    }

    }
#if defined (STTBX_PRINT)
            {
            volatile U32* addr=(U32*)(TSGDMA_DMEM_MEM_BASE+TSGDMA_DATA_BASE_ADDR);
            int i,j;

            for (i=0;i<30;i++)
            {
                STTBX_Print(("0x%08x    :", (U32)addr));
                for (j=0;j<6;j++)
                {
                STTBX_Print(("0x%08x ",*(addr++) ));

                }
                STTBX_Print(("\n"));
            }
        }
#endif

}


/******************************************************************************
Function Name : TSGDMA_InterruptUnInstall
  Description : UnInstall the interrupt handler for SWTS injection
   Parameters : None
******************************************************************************/
ST_ErrorCode_t TSGDMA_InterruptUnInstall(void)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTBX_Print(("Calling TSGDMA_InterruptUnInstall\n"));

    if ( TSGDMA_ISRInstallCount > 0)
    {
        --TSGDMA_ISRInstallCount;
        if ( TSGDMA_ISRInstallCount == 0)
        {
            /* Make sure the interrupt is disabled first */
            if ( STOS_SUCCESS != STOS_InterruptDisable( TSGDMA_MAILBOX_FLAG_INTERRUPT, 0 ) )
            {
                STTBX_Print(("STOS_InterruptDisable Error\n"));
                Error = ST_ERROR_INTERRUPT_UNINSTALL;
            }
            else if ( STOS_SUCCESS != STOS_InterruptUninstall( TSGDMA_MAILBOX_FLAG_INTERRUPT, 0, NULL))
            {
                STTBX_Print(("TSGDMA_InterruptUnInstall Calling STOS_InterruptUninstall\n"));
                Error = ST_ERROR_INTERRUPT_UNINSTALL;
            }
        }
    }
    return Error;
}

/******************************************************************************
Function Name : TSGDMA_InterruptInstall
  Description : Install the interrupt handler for SWTS injection
   Parameters : None
******************************************************************************/
ST_ErrorCode_t TSGDMA_InterruptInstall(void)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTBX_Print(("Calling TSGDMA_InterruptInstall\n"));

    if ( 0 == TSGDMA_ISRInstallCount)
    {
        if ( STOS_SUCCESS != STOS_InterruptDisable( TSGDMA_MAILBOX_FLAG_INTERRUPT, 0 ) )
        {
            STTBX_Print(("STOS_InterruptDisable Error\n"));
            Error = ST_ERROR_INTERRUPT_INSTALL;
        }
        else if ( STOS_SUCCESS != STOS_InterruptInstall( TSGDMA_MAILBOX_FLAG_INTERRUPT, 0, STOS_INTERRUPT_CAST(TSGDMA_Int_Handler), "TSGDMA_SWTS", NULL))
        {
            STTBX_Print(("STOS_InterruptInstall Error\n"));
            Error = ST_ERROR_INTERRUPT_INSTALL;
        }
        else if ( STOS_SUCCESS != STOS_InterruptEnable( TSGDMA_MAILBOX_FLAG_INTERRUPT, 0 ))
        {
            STTBX_Print(("STOS_InterruptEnable Error\n"));
            Error = ST_ERROR_INTERRUPT_INSTALL;
        }
        else
        {
            STTBX_Print(("TSGDMA_InterruptInstall Calling STOS_InterruptInstall\n"));
            TSGDMA_ISRInstallCount++;
        }
    }
    else
    {
        TSGDMA_ISRInstallCount++;
    }

    return Error;
}


/******************************************************************************
Function Name : TSGDMA_Int_Handler
  Description : Interrupt handler
   Parameters : None
******************************************************************************/
STOS_INTERRUPT_DECLARE(TSGDMA_Int_Handler, Param)
{
    U32 Value, Mask, ChannelIndex;

    Value = STSYS_ReadRegDev32LE((void*)TSGDMA_MAILBOX1_STATUS);
    Mask  = STSYS_ReadRegDev32LE((void*)TSGDMA_MAILBOX1_MASK);

    while(( Value & Mask ) & TSGDMA_SW_CHANNEL_MASK )
    {
        /* Service the pending interrupt(s) */
        for( ChannelIndex = 0; ChannelIndex < TSGDMA_NUM_SWTS; ChannelIndex++ )
        {
            U32 ChannelBitMask = 1 << ChannelIndex;

            if(( Value & Mask ) & ChannelBitMask )
            {
                STOS_SemaphoreSignal( TSGDMA_DataEmpty[ChannelIndex] );
            }
        }

        /* Clear the pending interrupt(s) */
        STSYS_WriteRegDev32LE((void*)TSGDMA_MAILBOX1_MASK, (Mask & ~TSGDMA_SW_CHANNEL_MASK) | (~Value & TSGDMA_SW_CHANNEL_MASK));

        /* Re-check the interrupt status */
        Value = STSYS_ReadRegDev32LE((void*)TSGDMA_MAILBOX1_STATUS);
        Mask  = STSYS_ReadRegDev32LE((void*)TSGDMA_MAILBOX1_MASK);
    }

    STOS_INTERRUPT_EXIT( STOS_SUCCESS );
}


#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */

/* End of tsgdma.c --------------------------------------------------------- */
