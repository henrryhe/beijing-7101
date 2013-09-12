#define STTBX_PRINT 1   /*Always required on */
#if defined (ST_OSLINUX)
#include <linux/dmapool.h>
#include <linux/kthread.h>
#else
#include <stdlib.h>
#include <string.h>
#if defined (ST_OS20)
#include <stdio.h>
#endif
#endif

#include "stdevice.h"
#include "stfdma.h"
#include "fdmatst.h"

#include "audiopaced.h"


/*#define CRYSTAL_FREQUENCY_27MHZ 1*/
#define CRYSTAL_FREQUENCY_30MHZ 1

#define TEST_DURATION 0.01               /* Test duration in hours */
#define LOOP_COUNT    1

#define SPDIF_NODE_PAUSE_INTERVAL 200
#define SPDIF_NODE_INTERRUPT_INTERVAL 100


#if defined (ST_OSLINUX)
#define STTBX_Print(x) printk x
#else
#define STTBX_Print(x) printf x
#endif


FrequencySynthesizerClockParameters_t FSCLockValues[] =
{/* SDIV    PE      MD */
#if defined (CRYSTAL_FREQUENCY_27MHZ)
	{0x6,  0x5100,  0xFA},  /*Freq =  2.048   MHz :		256*8kHz       */
	{0x6,  0x6F05,  0xF3},  /*Freq =  2.8224  MHz :		256*11.025kHz  */
	{0x6,  0x3600,  0xF1},  /*Freq =  3.072   MHz :		256*12kHz      */
	{0x5,  0x5100,  0xFA},  /*Freq =  4.096   MHz :		256*16kHz      */
	{0x5,  0x6F05,  0xF3},  /*Freq =  5.6448  MHz :		256*22.05kHz   */
	{0x5,  0x3600,  0xF1},  /*Freq =  6.144   MHz :		256*24kHz      */
	{0x4,  0x5100,  0xFA},  /*Freq =  8.192   MHz :		256*32kHz      */
	{0x4,  0x6F05,  0xF3},  /*Freq = 11.2896  MHz :	    256*44.1kHz    */
	{0x4,  0x3600,  0xF1},  /*Freq = 12.288   MHz :		256*48kHz      */
	{0x3,  0x5100,  0xFA},  /*Freq = 16.384   MHz :		256*64kHz      */
	{0x3,  0x6974,  0xF3},  /*Freq = 22.528   MHz :		256*88kHz      */
	{0x3,  0x3600,  0xF1},  /*Freq = 24.576   MHz :		256*96kHz      */
	{0x2,  0x3600,  0xF1},  /*Freq = 49.152   MHz :		256*192kHz     */
#elif defined (CRYSTAL_FREQUENCY_30MHZ)
	{0x6,  0x5A00,  0xFD},  /*Freq =  2.048   MHz :		256*8kHz       */
	{0x6,  0x5EE9,  0xF5},  /*Freq =  2.8224  MHz :		256*11.025kHz  */
	{0x6,  0x3C00,  0xF3},  /*Freq =  3.072   MHz :		256*12kHz      */
	{0x5,  0x5A00,  0xFD},  /*Freq =  4.096   MHz :		256*16kHz      */
	{0x5,  0x5EE9,  0xF5},  /*Freq =  5.6448  MHz :		256*22.05kHz   */
	{0x5,  0x3C00,  0xF3},  /*Freq =  6.144   MHz :		256*24kHz      */
	{0x4,  0x5A00,  0xFD},  /*Freq =  8.192   MHz :		256*32kHz      */
	{0x4,  0x5EE9,  0xF5},  /*Freq = 11.2896  MHz :   	256*44.1kHz    */
	{0x4,  0x3C00,  0xF3},  /*Freq = 12.288   MHz :		256*48kHz      */
	{0x3,  0x5A00,  0xFD},  /*Freq = 16.384   MHz :		256*64kHz      */
	{0x3,  0x58BA,  0xF5},  /*Freq = 22.528   MHz :		256*88kHz      */
	{0x3,  0x3C00,  0xF3},  /*Freq = 24.576   MHz :		256*96kHz      */
	{0x2,  0x3C00,  0xF3},  /*Freq = 49.152   MHz :		128*192kHz     */
#endif
};

extern partition_t *system_partition;

static semaphore_t *pLaunchNextTaskSemap, *pInitFDMASemap;
static semaphore_t *pSPDIFFeedSemap, *pPCM0FeedSemap, *pPCM1FeedSemap;
static semaphore_t *pSPDIFPlaySemap, *pPCM0PlaySemap, *pPCM1PlaySemap;
static semaphore_t *pSPDIFStopSemap, *pPCM0StopSemap, *pPCM1StopSemap;
static semaphore_t *pSPDIFAbortSemap, *pPCM0AbortSemap, *pPCM1AbortSemap;

static semaphore_t *pMTOMTaskContSemap, *pMTOMTaskStopSemap, *pMTOMAbortSemap;

static U8 *pSPDIFTaskStack, *pPCM0TaskStack, *pPCM1TaskStack, *pMTOMTaskStack;
static tdesc_t *pSPDIFTaskDescriptor, *pPCM0TaskDescriptor, *pPCM1TaskDescriptor, *pMTOMTaskDescriptor;
#if defined (ST_OS20) || defined (ST_OS21)
static task_t *pSPDIFTask, *pPCM0Task, *pPCM1Task, *pMTOMTask;
#elif defined (ST_OSLINUX)
static struct task_struct *pSPDIFTask, *pPCM0Task, *pPCM1Task, *pMTOMTask;
#endif

STFDMA_TransferId_t SPDIFTransferId, PCM0TransferId, PCM1TransferId, MTOMTransferId;

int AC3Stream = 0, DTSStreamType1 = 0, MPEG1Layer1 = 0, MPEG1Layer2 = 0, MPEG1Layer3 = 0;
U32 FrameSize;

TestType_t TestType;
STOS_Clock_t StartTime, TestDuration;
static semaphore_t *pTestTimeUpSemap;
STFDMA_SPDIFNode_t *NextPausedToBe_p[SPDIF_NODE_COUNT/SPDIF_NODE_PAUSE_INTERVAL];
int LastPaused = 0, NextPaused = 0;

#if defined (ST_OSLINUX)
extern struct dma_pool* g_NodePool;
dma_addr_t SPDIFNodePhysicalAddress[SPDIF_NODE_COUNT];
dma_addr_t PCM0NodePhysicalAddress[4];
dma_addr_t PCM1NodePhysicalAddress[4];
#endif

char InterruptHandlerName[50] = "";

void DetermineStreamType(StreamType_t StreamType)
{
	U32 sync_value = 0, sync_value1 = 0, sync_value2 = 0;

	AC3Stream      = 0;
	DTSStreamType1 = 0;
	MPEG1Layer1    = 0;
	MPEG1Layer2    = 0;
	MPEG1Layer3    = 0;

	sync_value  = GET_REG_VAL(SPDIF_PLAYER_BUFFER_ADDRESS);
	sync_value  = (sync_value << 16) | ( sync_value >> 16);
	sync_value1 = sync_value;
	sync_value  = (sync_value) & 0xFFFF0000;

	/* For AC3: syncword 0x0B77. CRCL 16 bit. Sample Rate Codes 00 48 kHz 01 44.1 kHz 10 32 kHz 11 reserved */
	if(sync_value == 0x0B770000)
	{
		AC3Stream = 1;
		STTBX_Print(("AC3Sync: 0x%08x\n", sync_value));

        sync_value2 = GET_REG_VAL(SPDIF_PLAYER_BUFFER_ADDRESS + 4);
    	sync_value2 = (sync_value2 << 16) | ( sync_value2 >> 16);

		if((sync_value2 & 0xc0000000) == 0x00000000)
			STTBX_Print(("\nIt's AC3 Stream of 48 KHz\n"));

		if((sync_value2 & 0xc0000000) == 0x40000000)
			STTBX_Print(("\nIt's AC3 Stream of 44.1 KHz\n"));

		if((sync_value2 & 0xc0000000) == 0x80000000)
			STTBX_Print(("\nIt's AC3 Stream of 32 KHz\n"));
	}

	/* For DTSType1: Sampling frequency 48 kHz only */
	else if(sync_value == 0x7FFE0000)
	{
		DTSStreamType1 = 1;
		STTBX_Print(("\nDTS(Type1) Sync: 0x%08x\n", sync_value));
		STTBX_Print(("\nIt's DTS(Type1) Stream of 48 KHz\n"));
	}

	/* For MPEG 32 bit Header : Sync word [1111 1111 1111], ID [1], Layer ["11" Layer I "10" Layer II "01" Layer III "00" reserved], Protection_bit [1],
	                            Bit rate index [0000], Sampling frequency ['00' 44.1 kHz '01' 48 kHz '10' 32 kHz '11' reserved], Padding bit [1], Private bit [1], Mode [00],
	                            Mode extension [00], Copyright [1], Original/Home [1], Emphasis [00] */
	else if(((sync_value >> 20) == 0x00000FFF) && ((sync_value & 0x00060000) == 0x00060000))
	{
		MPEG1Layer1 = 1;
		STTBX_Print(("\nMPEG1(Layer1) Sync: 0x%08x\n", sync_value1));

	    sync_value2 = GET_REG_VAL(SPDIF_PLAYER_BUFFER_ADDRESS);
		sync_value2 = (sync_value2 << 16) | ( sync_value2 >> 16);

		if((sync_value2 & 0x00000c00) == 0x00000400)
			STTBX_Print(("\nIt's MPEG1(Layer1) Stream of 48 KHz\n"));

		if((sync_value2 & 0x00000c00) == 0x00000000)
			STTBX_Print(("\nIt's MPEG1(Layer1) Stream of 44.1 KHz\n"));

		if((sync_value2 & 0x00000c00) == 0x00000800)
			STTBX_Print(("\nIt's MPEG1(Layer1) Stream of 32 KHz\n"));
	}

	else if(((sync_value >> 20) == 0x00000FFF) && ((sync_value & 0x00060000) == 0x00040000))
	{
		MPEG1Layer2 = 1;
		STTBX_Print(("\nMPEG1_Layer2_Sync: 0x%08x\n", sync_value1));

	    sync_value2 = GET_REG_VAL(SPDIF_PLAYER_BUFFER_ADDRESS);
		sync_value2 = (sync_value2 << 16) | ( sync_value2 >> 16);

		if((sync_value2 & 0x00000c00) == 0x00000400)
			STTBX_Print(("\nIt's MPEG1(Layer2) Stream of 48 KHz\n"));

		if((sync_value2 & 0x00000c00) == 0x00000000)
			STTBX_Print(("\nIt's MPEG1(Layer2) Stream of 44.1 KHz\n"));

		if((sync_value2 & 0x00000c00) == 0x00000800)
			STTBX_Print(("\nIt's MPEG1(Layer2) Stream of 32 KHz\n"));
	}

	else if(((sync_value >> 20) == 0x00000FFF) && ((sync_value & 0x00060000) == 0x00020000))
	{
		MPEG1Layer3 = 1;
		STTBX_Print(("\nMPEG1_Layer3_Sync: 0x%08x\n", sync_value1));

	    sync_value2 = GET_REG_VAL(SPDIF_PLAYER_BUFFER_ADDRESS);
		sync_value2 = (sync_value2 << 16) | ( sync_value2 >> 16);

		if((sync_value2 & 0x00000c00) == 0x00000400)
			STTBX_Print(("\nIt's MPEG1(Layer3) Stream of 48 KHz\n"));

		if((sync_value2 & 0x00000c00) == 0x00000000)
			STTBX_Print(("\nIt's MPEG1(Layer3) Stream of 44.1 KHz\n"));

		if((sync_value2 & 0x00000c00) == 0x00000800)
			STTBX_Print(("\nIt's MPEG1(Layer3) Stream of 32 KHz\n"));
	}

	STTBX_Print(("\n"));
}

void CalculateFrameSize(StreamType_t StreamType)
{
	/* sync_word = ABCDXXXX ( First Two byte ABCD)
	   cases 1. ABCDXXXX; 2. XXABCDXX; 3. XXXXABCD; 4 XXXXXXAB and CDXXXXXX.
	*/
    U32 sync_value = 0, value =0 , value1 = 0 , sync_address = 0;
	int loop = 1;

	sync_address = SPDIF_PLAYER_BUFFER_ADDRESS;

	sync_value = GET_REG_VAL(SPDIF_PLAYER_BUFFER_ADDRESS);
	sync_value = (sync_value << 16) | ( sync_value >> 16);	    /*due to 5700 FirmWare Issue ...*/
	sync_value = (sync_value) & 0xFFFF0000;

	FrameSize = 0;

	while(loop)
	{
		sync_address = sync_address + 4;
        value   = GET_REG_VAL(sync_address);
		value = (value << 16) | (value >> 16);            /*due to 5700 FirmWare Issue ...*/

		/* case 1.*/
		if(sync_value == (value & 0xFFFF0000))
		{
			FrameSize = FrameSize + 4;
			loop = 0;
		}
		/*case 2.*/
		if(sync_value == ((value >> 8) << 16))
		{
			FrameSize = FrameSize + 5;
			loop = 0;
		}

		/*case 3.*/
		if(sync_value == (value << 16))
		{
			FrameSize = FrameSize + 6;
			loop = 0;
		}

        value1 = GET_REG_VAL(sync_address + 4);
		value1 = (value1 << 16) | (value1 >> 16);
		value1 = ((value << 24) | ((value1 >> 8) & 0x00FF0000));

		/* case 4.*/
		if(sync_value == value1)
		{
			FrameSize = FrameSize + 7;
			loop = 0;
		}

		FrameSize = FrameSize + 4;
	}
	FrameSize = FrameSize - 4;
}

#if defined (ST_OSLINUX)
void *CreateFDMANode(StreamType_t StreamType, dma_addr_t *PhysicalAddress)
#else
void *CreateFDMANode(StreamType_t StreamType)
#endif
{
    if(StreamType == SPDIF_COMPRESSED)
    {
        STFDMA_SPDIFNode_t *Node_p = NULL;

#if defined (ST_OSLINUX)
        Node_p = (STFDMA_SPDIFNode_t *)dma_pool_alloc(g_NodePool, GFP_KERNEL, PhysicalAddress);
#else
        /* Node must be 32 byte aligned */
        STFDMA_SPDIFNode_t *temp_p = NULL;

        temp_p = (STFDMA_SPDIFNode_t *)STOS_MemoryAllocateClear(fdmatst_GetNCachePartition(), 1, sizeof(STFDMA_SPDIFNode_t)+31);
        Node_p = (STFDMA_SPDIFNode_t *)(((U32)temp_p + (U32)31) & (~(U32)31));
#endif

        if (Node_p == NULL)
            STTBX_Print(("\nERROR: Node creation failed - not enough memory\n"));
#if defined (ARCHITECTURE_ST40) && defined (ST_OS21)
        else
        {
            cache_purge_data(Node_p, sizeof(STFDMA_SPDIFNode_t)+31);
            Node_p = (STFDMA_SPDIFNode_t*)ADDRESS_TO_P2(Node_p);
        }
#endif
        return (void *)Node_p;
    }
    else
    {
        STFDMA_Node_t *Node_p = NULL;

#if defined (ST_OSLINUX)
        Node_p = (STFDMA_Node_t *)dma_pool_alloc(g_NodePool, GFP_KERNEL, PhysicalAddress);
#else
        /* Node must be 32 byte aligned */
        STFDMA_Node_t *temp_p = NULL;

        temp_p = (STFDMA_Node_t *)STOS_MemoryAllocateClear(fdmatst_GetNCachePartition(), 1, sizeof(STFDMA_Node_t)+31);
        Node_p = (STFDMA_Node_t *)(((U32)temp_p + (U32)31) & (~(U32)31));
#endif

        if (Node_p == NULL)
            STTBX_Print(("\nERROR: Node creation failed - not enough memory\n"));
#if defined (ARCHITECTURE_ST40) && defined (ST_OS21)
        else
        {
            cache_purge_data(Node_p, sizeof(STFDMA_Node_t)+31);
            Node_p = (STFDMA_Node_t*)ADDRESS_TO_P2(Node_p);
        }
#endif
        return (void *)Node_p;
    }
}

ST_ErrorCode_t FDMALinkedListAllocate(StreamType_t StreamType, void **Node_p, U32 SampleCount)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if(StreamType == SPDIF_COMPRESSED)
    {
        STFDMA_SPDIFNode_t *temp_p = NULL, *temp2_p = NULL;
        int i, j;

        /* Resetting state of channel in the additional data region (3) of SPDIF */
        SET_REG_VAL(0xB92293A4, 0x0);

        for(i = 0, j = 0; i < SPDIF_NODE_COUNT; i++)
        {
#if defined (ST_OSLINUX)
            temp_p = (STFDMA_SPDIFNode_t *)CreateFDMANode(StreamType, &SPDIFNodePhysicalAddress[i]);
#else
            temp_p = (STFDMA_SPDIFNode_t *)CreateFDMANode(StreamType);
#endif

            if(temp_p == NULL)
            {
                ErrorCode = ST_ERROR_NO_MEMORY;
                break;
            }

    		temp_p->Next_p      = NULL;

    		temp_p->Extended	= STFDMA_EXTENDED_NODE;
    		temp_p->Type		= STFDMA_EXT_NODE_SPDIF;
    		temp_p->DReq		= STFDMA_REQUEST_SIGNAL_SPDIF_AUDIO;

#if defined (ST_7100)
    		temp_p->ModeData	= FALSE;
    		temp_p->Pad		    = 0;/*((unsigned int)(SPDIF_AC3_REPITION_TIME*4)- 0x8 -FrameSize); //Repetion Period - 8B(Pa,Pb,Pc,Pd)-FrameSize (Burst PayLoad)*/
#elif defined (ST_7109) || defined (ST_7200)
    		temp_p->ModeData	= FALSE;
    		temp_p->Pad		    = 0;/*((unsigned int)(SPDIF_AC3_REPITION_TIME*4)- 0x8 -FrameSize); //Repetion Period - 8B(Pa,Pb,Pc,Pd)-FrameSize (Burst PayLoad)*/
    		temp_p->Secure		= FALSE;
    		temp_p->Pad2		= 0;
#endif

    		temp_p->BurstEnd	= TRUE;/*Burst Start or Continuation*/
    		temp_p->Valid		= TRUE;/*Node is Valid*/

    		if(TestType == ALL_PLUS_MTOM && ((i+1)%SPDIF_NODE_PAUSE_INTERVAL) == 0)
    		{
    		    NextPausedToBe_p[j++]       = temp_p;
    		    temp_p->NodeCompletePause   = TRUE;
            }
    		else
    		    temp_p->NodeCompletePause   = FALSE;

    		if((TestType == ALL_PLUS_MTOM && ((i+1)%SPDIF_NODE_INTERRUPT_INTERVAL) == 0) ||\
    		    ((i+1)%200) == 0)
    		    temp_p->NodeCompleteNotify	= TRUE;
    		else
    		    temp_p->NodeCompleteNotify	= FALSE;

        	temp_p->NumberBytes				= FrameSize;    		
			temp_p->SourceAddress_p			=  PERIPH_ADDR_TRANSLATE(((void *)((U32)SPDIF_PLAYER_BUFFER_ADDRESS + i*(FrameSize/4)*4)));	
#if defined (ST_7200)
  		    temp_p->DestinationAddress_p	= (void *)(SPDIF_FIFO_DATA_REG);
#else
			temp_p->DestinationAddress_p	= PERIPH_ADDR_TRANSLATE((void *)SPDIF_FIFO_DATA_REG);
#endif

    		(temp_p->Data).CompressedMode.PreambleB	= 0x4E1F;/*Sync Word*/
    		(temp_p->Data).CompressedMode.PreambleA	= 0xF872;/*Sync Word*/
    		(temp_p->Data).CompressedMode.PreambleD	= (FrameSize*8);/*Making burst period to transfer*/

    		if(AC3Stream == 1)
    		{
    			(temp_p->Data).CompressedMode.PreambleC	= 0x01;
    			(temp_p->Data).CompressedMode.BurstPeriod = SPDIF_AC3_REPITION_TIME ;
    		}
    		else if(DTSStreamType1 == 1)
    		{
    			(temp_p->Data).CompressedMode.PreambleC	= 0x0b;
    			(temp_p->Data).CompressedMode.BurstPeriod	= SPDIF_DTS1_REPITION_TIME;
    		}
    		else if(MPEG1Layer1 == 1)
    		{
    			(temp_p->Data).CompressedMode.PreambleC	= 0x04;
    			(temp_p->Data).CompressedMode.BurstPeriod	= SPDIF_MPEG1_2_L1_REPITION_TIME;
    		}
    		else if(MPEG1Layer2 == 1)
    		{
        		(temp_p->Data).CompressedMode.PreambleC	= 0x05;
    		    (temp_p->Data).CompressedMode.BurstPeriod	= SPDIF_MPEG1_2_L1_REPITION_TIME;
    	    }
    		else if(MPEG1Layer3 == 1)
    		{
    			(temp_p->Data).CompressedMode.PreambleC	= 0x05;
    			(temp_p->Data).CompressedMode.BurstPeriod	= SPDIF_MPEG1_2_L2_L3_REPITION_TIME;
    		}

    		temp_p->Channel0.Status_0   	                    = 0x02000302;
    		temp_p->Channel0.Status.CompressedMode.Status_1   	= 0x00;
    		temp_p->Channel0.Status.CompressedMode.UserStatus 	= 0;
    		temp_p->Channel0.Status.CompressedMode.Valid     	= 1;
    		temp_p->Channel0.Status.CompressedMode.Pad        	= 0;

    		temp_p->Channel1.Status_0   	                    = 0x02000302;
    		temp_p->Channel1.Status.CompressedMode.Status_1   	= 0x00;
    		temp_p->Channel1.Status.CompressedMode.UserStatus 	= 0;
    		temp_p->Channel1.Status.CompressedMode.Valid      	= 1;
    		temp_p->Channel1.Status.CompressedMode.Pad        	= 0;

    		temp_p->Pad1[4]			        = 0x0;

    		if(i == 0)
    	    {
    		    *(STFDMA_SPDIFNode_t **)Node_p = temp_p;
    		    temp2_p = temp_p;
    		}
    		else
    		{
    		    temp2_p->Next_p = temp_p;
    		    temp2_p = temp_p;
    		}
    	}

    	if(TestType == ALL_PLUS_MTOM && ErrorCode == ST_NO_ERROR)
    	{
    	    (*(STFDMA_SPDIFNode_t **)Node_p)->NodeCompletePause = FALSE;
    	    (*(STFDMA_SPDIFNode_t **)Node_p)->NodeCompleteNotify = FALSE;
    	    temp_p->Next_p= *(STFDMA_SPDIFNode_t **)Node_p;
    	    LastPaused = j-1;
    	    NextPaused = 0;
    	}
    }
    else
    {
        STFDMA_Node_t *temp_p = NULL, *temp2_p = NULL;
        int i;

        for(i = 0; i < 4; i++)
        {
#if defined (ST_OSLINUX)
            if(StreamType == PCM_TEN_CHANNEL)
                temp_p = (STFDMA_Node_t *)CreateFDMANode(StreamType, &PCM0NodePhysicalAddress[i]);
            else /* StreamType == PCM_TWO_CHANNEL */
                temp_p = (STFDMA_Node_t *)CreateFDMANode(StreamType, &PCM1NodePhysicalAddress[i]);
#else
            temp_p = (STFDMA_Node_t *)CreateFDMANode(StreamType);
#endif

            if(temp_p == NULL)
            {
                ErrorCode = ST_ERROR_NO_MEMORY;
                break;
            }

    		temp_p->Next_p							    = NULL;

            if(StreamType == PCM_TEN_CHANNEL)
            {
#if defined (ST_7200)			
				temp_p->NodeControl.PaceSignal			    = STFDMA_REQUEST_SIGNAL_NONE;
#else
    		    temp_p->NodeControl.PaceSignal			    = STFDMA_REQUEST_SIGNAL_PCM0;
#endif
            }
    		else
    		{
#if defined (ST_7200)			
				temp_p->NodeControl.PaceSignal			    = STFDMA_REQUEST_SIGNAL_NONE;
#else    		
    		    temp_p->NodeControl.PaceSignal			    = STFDMA_REQUEST_SIGNAL_PCM1;
#endif
    		}			
			
    		temp_p->NodeControl.SourceDirection		    = STFDMA_DIRECTION_INCREMENTING;
    		temp_p->NodeControl.DestinationDirection    = STFDMA_DIRECTION_STATIC;
    		temp_p->NodeControl.Reserved				= 0;
#if defined (ST_7109) || defined (ST_7200)
    		temp_p->NodeControl.Secure  			    = TRUE;
    		temp_p->NodeControl.Reserved1				= 0;
#endif
            temp_p->NodeControl.NodeCompletePause   	= FALSE;
    		temp_p->NodeControl.NodeCompleteNotify		= TRUE;

    		if(StreamType == PCM_TEN_CHANNEL)
    		{
        		temp_p->NumberBytes						    = 10*SampleCount/4 ;    /* For 10 channels */
				temp_p->SourceAddress_p = PERIPH_ADDR_TRANSLATE((void *)(PCM_PLAYER_0_BUFFER_ADDRESS + i*temp_p->NumberBytes));		
#if !defined (ST_7200)
				temp_p->DestinationAddress_p				=   PERIPH_ADDR_TRANSLATE((void *)PCMPLAYER0_FIFO_DATA_REG);
#else
    		    temp_p->DestinationAddress_p				= (void *)(PCMPLAYER0_FIFO_DATA_REG);
#endif
        		temp_p->Length							    = 10*4;                 /* Ten channel */
        		temp_p->SourceStride						= 10*4;                 /* Ten channel */
    		}
            else
            {
        		temp_p->NumberBytes						    = 10*SampleCount/4 ;     /* For 2 channels */
				temp_p->SourceAddress_p					    = 		PERIPH_ADDR_TRANSLATE((void *)(PCM_PLAYER_1_BUFFER_ADDRESS + i*temp_p->NumberBytes));
#if !defined (ST_7200)
				temp_p->DestinationAddress_p        =    PERIPH_ADDR_TRANSLATE((void *)PCMPLAYER1_FIFO_DATA_REG);
#else
    		    temp_p->DestinationAddress_p				= (void *)(PCMPLAYER1_FIFO_DATA_REG);
#endif
        		temp_p->Length							    = 10*4;                  /* Two channel */
        		temp_p->SourceStride						= 10*4;                  /* Two channel */
    		}
    		temp_p->DestinationStride					= 0;

    		if(i == 0)
    	    {
    		    *(STFDMA_Node_t **)Node_p = temp_p;
    		    temp2_p = temp_p;
    		}
    		else if(i == 3)
    		{
    		    temp2_p->Next_p = temp_p;
/*    		    temp_p->Next_p = *(STFDMA_Node_t **)Node_p;*/
    		}
    		else
    		{
    		    temp2_p->Next_p = temp_p;
    		    temp2_p = temp_p;
    		}
        }

    	if(TestType == ALL_PLUS_MTOM && ErrorCode == ST_NO_ERROR)
    	{
    	    temp_p->NodeControl.NodeCompletePause = TRUE;
    	    temp_p->Next_p= *(STFDMA_Node_t **)Node_p;
    	}
    }

	return ErrorCode;
}

void FDMALinkedListDeallocate(StreamType_t StreamType, void **FirstNode_p)
{
    if(StreamType == SPDIF_COMPRESSED)
    {
        STFDMA_SPDIFNode_t *temp_p = *(STFDMA_SPDIFNode_t **)FirstNode_p;
        STFDMA_SPDIFNode_t **Node_p = (STFDMA_SPDIFNode_t **)FirstNode_p;
        U32 i = 0;

        for(i = 0; i < SPDIF_NODE_COUNT; i++)
        {
            if(i < SPDIF_NODE_COUNT-1)
                *Node_p = temp_p->Next_p;
#if defined (ST_OSLINUX)
            dma_pool_free(g_NodePool, temp_p, SPDIFNodePhysicalAddress[i]);
#else
            STOS_MemoryDeallocate(fdmatst_GetNCachePartition(), (void *)temp_p);
#endif
            if(i < SPDIF_NODE_COUNT-1)
                temp_p = *Node_p;
        }
    }
    else
    {
        STFDMA_Node_t *temp_p = *(STFDMA_Node_t **)FirstNode_p;
        STFDMA_Node_t **Node_p = (STFDMA_Node_t **)FirstNode_p;
        U32 i = 0;

        for(i = 0; i < 4; i++)
        {
            if(i < 3)
                *Node_p = temp_p->Next_p;
#if defined (ST_OSLINUX)
            if(StreamType == PCM_TEN_CHANNEL)
                dma_pool_free(g_NodePool, temp_p, PCM0NodePhysicalAddress[i]);
            else /* StreamType == PCM_TWO_CHANNEL */
                dma_pool_free(g_NodePool, temp_p, PCM1NodePhysicalAddress[i]);
#else
            STOS_MemoryDeallocate(fdmatst_GetNCachePartition(), (void *)temp_p);
#endif
            if(i < 3)
                temp_p = *Node_p;
        }
    }
}

void PlayerStart(StreamType_t StreamType)
{
    U32	control_start;

    if(StreamType == SPDIF_COMPRESSED)
    {
        control_start   = GET_REG_VAL(SPDIF_CONTROL_REG);
    	control_start  &= 0xFFFFFFF0;
    	control_start  |= (U32)spdif_encoded_data;

        /* Soft reset */
        SET_REG_VAL(SPDIF_BASE, 0x1);
    	STOS_TaskDelay(fdmatst_GetClocksPerSec()/30000);
        SET_REG_VAL(SPDIF_BASE, 0x0);

        /* Starting the SPDIF player */
        SET_REG_VAL(SPDIF_CONTROL_REG, control_start);
    }
    else if(StreamType == PCM_TEN_CHANNEL)
    {
#if defined (ST_7200)
        /* Enabling the internal DAC - soft reset (active low) */
        SET_REG_VAL(AUDIO_ADAC0_CTRL, 0x68);
    	STOS_TaskDelay(fdmatst_GetClocksPerSec()/30000);
        SET_REG_VAL(AUDIO_ADAC0_CTRL, 0x69);
#endif

		control_start  = GET_REG_VAL(PCMPLAYER0_CONTROL_REG);
		control_start &= 0xFFFFFFFC;                                                                   /*bitwise and to make last to bit 00 and then OR'ing   ...case for MUTE --> START*/
    	control_start |= (U32)pcm_mode_on_16;

        /* Soft reset */
        SET_REG_VAL(PCMPLAYER0_BASE, 0x1);
    	STOS_TaskDelay(fdmatst_GetClocksPerSec()/30000);
        SET_REG_VAL(PCMPLAYER0_BASE, 0x0);

        /* Starting PCM Player #0 */
        SET_REG_VAL(PCMPLAYER0_CONTROL_REG, control_start);
    }
    else /* StreamType == PCM_TWO_CHANNEL */
    {
 #if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
        /* Enabling the internal DAC - soft reset (active low) */
        SET_REG_VAL(AUDIO_ADAC_CTRL, 0x68);
#endif 
    	STOS_TaskDelay(fdmatst_GetClocksPerSec()/30000);
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
        SET_REG_VAL(AUDIO_ADAC_CTRL, 0x69);
#endif

		control_start  = GET_REG_VAL(PCMPLAYER1_CONTROL_REG);
		control_start &= 0xFFFFFFFC;                                                                   /*bitwise and to make last to bit 00 and then OR'ing   ...case for MUTE --> START*/
    	control_start |= (U32)pcm_mode_on_16;

        /* Soft reset */
        SET_REG_VAL(PCMPLAYER1_BASE, 0x1);
    	STOS_TaskDelay(fdmatst_GetClocksPerSec()/30000);
        SET_REG_VAL(PCMPLAYER1_BASE, 0x0);

        /* Starting PCM Player #1 */
        SET_REG_VAL(PCMPLAYER1_CONTROL_REG, control_start);
    }
}

void PlayerStop(StreamType_t StreamType)
{
    if(StreamType == SPDIF_COMPRESSED)
        SET_REG_VAL(SPDIF_BASE, 0x0);
    else if(StreamType == PCM_TEN_CHANNEL)
        SET_REG_VAL(PCMPLAYER0_BASE, 0x0);
    else /* StreamType == PCM_TWO_CHANNEL */
    {
        SET_REG_VAL(PCMPLAYER1_BASE, 0x0);
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 				
        SET_REG_VAL(AUDIO_ADAC_CTRL, 0x69);
#endif
    }
}

void PlayerReset(StreamType_t StreamType)
{
    if(StreamType == SPDIF_COMPRESSED)
    {
        SET_REG_VAL(SPDIF_CONTROL_REG,          0x0);
        SET_REG_VAL(SPDIF_PA_PB_REG,            0x0);
        SET_REG_VAL(SPDIF_PC_PD_REG,            0x0);
        SET_REG_VAL(SPDIF_CL1_REG,              0x0);
        SET_REG_VAL(SPDIF_CR1_REG,              0x0);
        SET_REG_VAL(SPDIF_CL2_CR2_UV_REG,       0x0);
        SET_REG_VAL(SPDIF_PAUSE_LAT_REG,        0x0);
        SET_REG_VAL(SPDIF_FRAMELGTH_BURST_REG,  0x0);
    }
    else if(StreamType == PCM_TEN_CHANNEL)
    {
        SET_REG_VAL(PCMPLAYER0_FORMAT_REG,  0x0);
        SET_REG_VAL(PCMPLAYER0_CONTROL_REG, 0x0);
    }
    else /* StreamType == PCM_TWO_CHANNEL */
    {
        SET_REG_VAL(PCMPLAYER1_FORMAT_REG,  0x0);
        SET_REG_VAL(PCMPLAYER1_CONTROL_REG, 0x0);
    }
}

void ConvertListAddressesToPeripheral(StreamType_t StreamType, void *pNode)
{
    if(StreamType == SPDIF_COMPRESSED)
    {
        STFDMA_SPDIFNode_t *pNextNode;
        STFDMA_SPDIFNode_t *pCurrentNode = (STFDMA_SPDIFNode_t *)pNode;
        STFDMA_SPDIFNode_t *pFirstNode = (STFDMA_SPDIFNode_t *)pNode;

        do
        {
            pNextNode = pCurrentNode->Next_p;
            if (NULL != pCurrentNode->Next_p)
            {
                pCurrentNode->Next_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(pCurrentNode->Next_p);
            }
            pCurrentNode->SourceAddress_p = TRANSLATE_BUFFER_ADDRESS_TO_PHYS(pCurrentNode->SourceAddress_p);
#if !defined (ST_7200)
            /* To avoid converting the FIFO addresses of the respective audio IPs wrongly */
            pCurrentNode->DestinationAddress_p = TRANSLATE_BUFFER_ADDRESS_TO_PHYS(pCurrentNode->DestinationAddress_p);
#endif
            pCurrentNode = pNextNode;
        }
        while ((pCurrentNode != NULL) && (pCurrentNode != pFirstNode));
    }
    else
    {
        STFDMA_Node_t *pNextNode;
        STFDMA_Node_t *pCurrentNode = (STFDMA_Node_t *)pNode;
        STFDMA_Node_t *pFirstNode = (STFDMA_Node_t *)pNode;

        do
        {
            pNextNode = pCurrentNode->Next_p;
            if (NULL != pCurrentNode->Next_p)
            {
                pCurrentNode->Next_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(pCurrentNode->Next_p);
            }
            pCurrentNode->SourceAddress_p = TRANSLATE_BUFFER_ADDRESS_TO_PHYS(pCurrentNode->SourceAddress_p);
#if !defined (ST_7200)
            pCurrentNode->DestinationAddress_p = TRANSLATE_BUFFER_ADDRESS_TO_PHYS(pCurrentNode->DestinationAddress_p);
#endif
            pCurrentNode = pNextNode;
        }
        while ((pCurrentNode != NULL) && (pCurrentNode != pFirstNode));
    }
}

void ConvertListAddressesToVirtual(StreamType_t StreamType, void *pNode)
{
    if(StreamType == SPDIF_COMPRESSED)
    {
        STFDMA_SPDIFNode_t *pCurrentNode = (STFDMA_SPDIFNode_t *)pNode;
        STFDMA_SPDIFNode_t *pFirstNode = (STFDMA_SPDIFNode_t *)pNode;

        do
        {
            if (NULL != pCurrentNode->Next_p)
            {
                pCurrentNode->Next_p = (STFDMA_SPDIFNode_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(pCurrentNode->Next_p);
            }
            pCurrentNode->SourceAddress_p = (void *)TRANSLATE_BUFFER_ADDRESS_TO_VIRT(pCurrentNode->SourceAddress_p);
            pCurrentNode->DestinationAddress_p = (void *)TRANSLATE_BUFFER_ADDRESS_TO_VIRT(pCurrentNode->DestinationAddress_p);
            pCurrentNode = pCurrentNode->Next_p;
        }
        while ((pCurrentNode != NULL) && (pCurrentNode != pFirstNode));
    }
    else
    {
        STFDMA_Node_t *pCurrentNode = (STFDMA_Node_t *)pNode;
        STFDMA_Node_t *pFirstNode = (STFDMA_Node_t *)pNode;

        do
        {
            if (NULL != pCurrentNode->Next_p)
            {
                pCurrentNode->Next_p = (STFDMA_Node_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(pCurrentNode->Next_p);
            }
            pCurrentNode->SourceAddress_p = (void *)TRANSLATE_BUFFER_ADDRESS_TO_VIRT(pCurrentNode->SourceAddress_p);
            pCurrentNode->DestinationAddress_p = (void *)TRANSLATE_BUFFER_ADDRESS_TO_VIRT(pCurrentNode->DestinationAddress_p);
            pCurrentNode = pCurrentNode->Next_p;
        }
        while ((pCurrentNode != NULL) && (pCurrentNode != pFirstNode));
    }
}

void SPDIFUserCallback( U32             TransferId,
					    U32             CallbackReason,
						U32             *CurrentNode_p,
						U32             NodeBytesTransfered,
						BOOL            Error,
						void            *ApplicationData_p,
                        STOS_Clock_t    InterruptTime)
{

    if(TestType == ALL_PLUS_MTOM)
        if(STOS_time_minus(InterruptTime, StartTime) >= TestDuration)
            STOS_SemaphoreSignal(pTestTimeUpSemap);

	if(CallbackReason == STFDMA_NOTIFY_TRANSFER_COMPLETE)
	{
/*		STTBX_Print(("\nSPDIF TRANSFER COMPLETE and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nSPDIF TRANSFER COMPLETE and current node address = 0x%x\n", (U32)CurrentNode_p));*/
		STOS_SemaphoreSignal(pSPDIFPlaySemap);
    }
	else if(CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING)
	{
/*		STTBX_Print(("\nSPDIF NODE_COMPLETE_DMA_CONTINUING and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nSPDIF NODE_COMPLETE_DMA_CONTINUING and current node address = 0x%x\n", (U32)CurrentNode_p));*/

		if(TestType == ALL_PLUS_MTOM)
		{
            NextPausedToBe_p[NextPaused]->NodeCompletePause  = FALSE;
            NextPausedToBe_p[NextPaused]->NodeCompleteNotify = FALSE;
            NextPausedToBe_p[LastPaused]->NodeCompletePause  = TRUE;
            NextPausedToBe_p[LastPaused]->NodeCompleteNotify = TRUE;
            LastPaused = NextPaused;
            NextPaused = (NextPaused + 1)%(SPDIF_NODE_COUNT/SPDIF_NODE_PAUSE_INTERVAL);
        }
	}
	else if(CallbackReason == STFDMA_NOTIFY_TRANSFER_ABORTED)
	{
/*		STTBX_Print(("\nSPDIF TRANSFER_ABORTED and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nSPDIF TRANSFER_ABORTED and current node address = 0x%x\n", (U32)CurrentNode_p));*/
		STOS_SemaphoreSignal(pSPDIFAbortSemap);
	}
	else if(CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED)
	{
/*		STTBX_Print(("\nSPDIF NODE_COMPLETE_DMA_PAUSED and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nSPDIF NODE_COMPLETE_DMA_PAUSED and current node address = 0x%x\n", (U32)CurrentNode_p));*/

        if(TestType == ALL_PLUS_MTOM)
        {
            STFDMA_SPDIFNode_t *temp = (STFDMA_SPDIFNode_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(CurrentNode_p);

            STTBX_Print(("\nERROR: SPDIF STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING callback missed!"));
            NextPausedToBe_p[LastPaused]->NodeCompletePause = TRUE;
            NextPausedToBe_p[LastPaused]->NodeCompleteNotify = TRUE;

            if(temp == NextPausedToBe_p[NextPaused])    /* Confirming */
            {
                LastPaused = NextPaused;
                NextPaused = (NextPaused + 1)%(SPDIF_NODE_COUNT/SPDIF_NODE_PAUSE_INTERVAL);
            }
        }

    	STTBX_Print((" Resuming SPDIF transfer!\n"));
    	STFDMA_ResumeTransfer(SPDIFTransferId);
	}
}

void PCM0UserCallback(  U32             TransferId,
					    U32             CallbackReason,
						U32             *CurrentNode_p,
						U32             NodeBytesTransfered,
						BOOL            Error,
						void            *ApplicationData_p,
                        STOS_Clock_t    InterruptTime)
{
    U32 i = 0;

    if(TestType == ALL_PLUS_MTOM)
        if(STOS_time_minus(InterruptTime, StartTime) >= TestDuration)
            STOS_SemaphoreSignal(pTestTimeUpSemap);

	if(CallbackReason == STFDMA_NOTIFY_TRANSFER_COMPLETE)
	{
		STTBX_Print(("\nPCM0 TRANSFER COMPLETE and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nPCM0 TRANSFER COMPLETE and current node address = 0x%x\n", (U32)CurrentNode_p));
		STOS_SemaphoreSignal(pPCM0PlaySemap);
    }
	else if(CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING)
	{
		STTBX_Print(("\nPCM0 NODE_COMPLETE_DMA_CONTINUING and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nPCM0 NODE_COMPLETE_DMA_CONTINUING and current node address = 0x%x\n", (U32)CurrentNode_p));

        if(TestType == ALL_PLUS_MTOM)
        {
            STFDMA_Node_t *temp = (STFDMA_Node_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(CurrentNode_p), *temp2 = temp;
            for(i = 0; i < 4; i++)
                if(temp2->NodeControl.NodeCompletePause == FALSE)
                    temp2 = (STFDMA_Node_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(temp2->Next_p);
                else
                    break;

            temp2->NodeControl.NodeCompletePause = FALSE;
            temp->NodeControl.NodeCompletePause = TRUE;
        }
	}
	else if(CallbackReason == STFDMA_NOTIFY_TRANSFER_ABORTED)
	{
/*		STTBX_Print(("\nPCM0 TRANSFER_ABORTED and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nPCM0 TRANSFER_ABORTED and current node address = 0x%x\n", (U32)CurrentNode_p));*/
		STOS_SemaphoreSignal(pPCM0AbortSemap);
	}
	else if(CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED)
	{
/*		STTBX_Print(("\nPCM0 NODE_COMPLETE_DMA_PAUSED and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nPCM0 NODE_COMPLETE_DMA_PAUSED and current node address = 0x%x\n", (U32)CurrentNode_p));*/

        if(TestType == ALL_PLUS_MTOM)
        {
            STFDMA_Node_t *temp = (STFDMA_Node_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(CurrentNode_p), *temp2 = temp;

            STTBX_Print(("\nERROR: PCM0 STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING callback missed!\n"));
            for(i = 0; i < 3; i++, temp2 = (STFDMA_Node_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(temp2->Next_p));

            if(temp->NodeControl.NodeCompletePause == TRUE)
            {
                temp2->NodeControl.NodeCompletePause = TRUE;
                temp->NodeControl.NodeCompletePause = FALSE;
            }
        }

    	STTBX_Print((" Resuming PCM0 transfer!\n"));
    	STFDMA_ResumeTransfer(PCM0TransferId);
	}
}

void PCM1UserCallback(  U32             TransferId,
					    U32             CallbackReason,
						U32             *CurrentNode_p,
						U32             NodeBytesTransfered,
						BOOL            Error,
						void            *ApplicationData_p,
                        STOS_Clock_t    InterruptTime)
{
    U32 i = 0;

    if(TestType == ALL_PLUS_MTOM)
        if(STOS_time_minus(InterruptTime, StartTime) >= TestDuration)
            STOS_SemaphoreSignal(pTestTimeUpSemap);

	if(CallbackReason == STFDMA_NOTIFY_TRANSFER_COMPLETE)
	{
		STTBX_Print(("\nPCM1 TRANSFER COMPLETE and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nPCM1 TRANSFER COMPLETE and current node address = 0x%x\n", (U32)CurrentNode_p));
		STOS_SemaphoreSignal(pPCM1PlaySemap);
    }
	else if(CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING)
	{
		STTBX_Print(("\nPCM1 NODE_COMPLETE_DMA_CONTINUING and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nPCM1 NODE_COMPLETE_DMA_CONTINUING and current node address = 0x%x\n", (U32)CurrentNode_p));

        if(TestType == ALL_PLUS_MTOM)
        {
            STFDMA_Node_t *temp = (STFDMA_Node_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(CurrentNode_p), *temp2 = temp;
            for(i = 0; i < 4; i++)
                if(temp2->NodeControl.NodeCompletePause == FALSE)
                    temp2 = (STFDMA_Node_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(temp2->Next_p);
                else
                    break;

            temp2->NodeControl.NodeCompletePause = FALSE;
            temp->NodeControl.NodeCompletePause = TRUE;
        }
	}
	else if(CallbackReason == STFDMA_NOTIFY_TRANSFER_ABORTED)
	{
/*		STTBX_Print(("\nPCM1 TRANSFER_ABORTED and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nPCM1 TRANSFER_ABORTED and current node address = 0x%x\n", (U32)CurrentNode_p));*/
		STOS_SemaphoreSignal(pPCM1AbortSemap);
	}
	else if(CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED)
	{
/*		STTBX_Print(("\nPCM1 NODE_COMPLETE_DMA_PAUSED and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nPCM1 NODE_COMPLETE_DMA_PAUSED and current node address = 0x%x\n", (U32)CurrentNode_p));*/

        if(TestType == ALL_PLUS_MTOM)
        {
            STFDMA_Node_t *temp = (STFDMA_Node_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(CurrentNode_p), *temp2 = temp;

            STTBX_Print(("\nERROR: PCM1 STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING callback missed!\n"));
            for(i = 0; i < 3; i++, temp2 = (STFDMA_Node_t *)TRANSLATE_NODE_ADDRESS_TO_VIRT(temp2->Next_p));

            if(temp->NodeControl.NodeCompletePause == TRUE)
            {
                temp2->NodeControl.NodeCompletePause = TRUE;
                temp->NodeControl.NodeCompletePause = FALSE;
            }
        }

    	STTBX_Print((" Resuming PCM1 transfer!\n"));
    	STFDMA_ResumeTransfer(PCM1TransferId);
	}
}

void MTOMUserCallback(  U32             TransferId,
					    U32             CallbackReason,
						U32             *CurrentNode_p,
						U32             NodeBytesTransfered,
						BOOL            Error,
						void            *ApplicationData_p,
                        STOS_Clock_t    InterruptTime)
{
    if(TestType == ALL_PLUS_MTOM)
        if(STOS_time_minus(InterruptTime, StartTime) >= TestDuration)
            STOS_SemaphoreSignal(pTestTimeUpSemap);

    if(CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING)
	{
/*		STTBX_Print(("\nMTOM NODE_COMPLETE_DMA_CONTINUING and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nMTOM NODE_COMPLETE_DMA_CONTINUING and current node address = 0x%x\n", (U32)CurrentNode_p));*/
	}
	else if(CallbackReason == STFDMA_NOTIFY_TRANSFER_ABORTED)
	{
/*		STTBX_Print(("\nMTOM TRANSFER_ABORTED and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nMTOM TRANSFER_ABORTED and current node address = 0x%x\n", (U32)CurrentNode_p));*/
/*      STOS_SemaphoreSignal(pMTOMAbortSemap);*/
	}
	else if(CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED)
	{
/*		STTBX_Print(("\nMTOM NODE_COMPLETE_DMA_PAUSED and transfer id = 0x%x\n", TransferId));
		STTBX_Print(("\nMTOM NODE_COMPLETE_DMA_PAUSED and current node address = 0x%x\n", (U32)CurrentNode_p));*/
    	STTBX_Print(("\nResuming FDMA transfer\n"));
    	STFDMA_ResumeTransfer(MTOMTransferId);
	}
}

STOS_INTERRUPT_DECLARE(SPDIFPlayerInterruptHandler, pParams)
{
	U32 spdif_itrp_enable = 0;
	U32 spdif_itrp_status = 0;

    spdif_itrp_enable = GET_REG_VAL(SPDIF_ITRP_ENABLE_REG);
	/* SPDIF Underflow */
    spdif_itrp_status = GET_REG_VAL(SPDIF_ITRP_STATUS_REG);
	if(((U32)spdif_itrp_enable & 0x0001) == 0x0001)
	    if(((U32)spdif_itrp_status & 0x0001) == 0x0001)
            SET_REG_VAL(SPDIF_ITRP_STATUS_CLEAR_REG, 0x1);

	/* SPDIF EoDB */
    spdif_itrp_status = GET_REG_VAL(SPDIF_ITRP_STATUS_REG);
	if(((U32)spdif_itrp_enable & 0x0002) == 0x0002)
    	if(((U32)spdif_itrp_status & 0x0002) == 0x0002)
            SET_REG_VAL(SPDIF_ITRP_STATUS_CLEAR_REG, 0x2);

	/* SPDIF EoB */
    spdif_itrp_status = GET_REG_VAL(SPDIF_ITRP_STATUS_REG);
	if(((U32)spdif_itrp_enable & 0x0004) == 0x0004)
	    if(((U32)spdif_itrp_status & 0x0004) == 0x0004)     /*Added  for SPDIF-PCM 'c' bit toggle.*/
            SET_REG_VAL(SPDIF_ITRP_STATUS_CLEAR_REG, 0x4);

	/* SPDIF EoL */
    spdif_itrp_status = GET_REG_VAL(SPDIF_ITRP_STATUS_REG);
	if(((U32)spdif_itrp_enable & 0x0008) == 0x0008)
	    if(((U32)spdif_itrp_status & 0x0008) == 0x0008)
            SET_REG_VAL(SPDIF_ITRP_STATUS_CLEAR_REG, 0x8);

	/* SPDIF EoPd */
    spdif_itrp_status = GET_REG_VAL(SPDIF_ITRP_STATUS_REG);
	if(((U32)spdif_itrp_enable & 0x0010)==0x0010)
	    if(((U32)spdif_itrp_status & 0x0010) == 0x0010)
            SET_REG_VAL(SPDIF_ITRP_STATUS_CLEAR_REG, 0x10);

	/* SPDIF Memory Block Fully Read */
    spdif_itrp_status = GET_REG_VAL(SPDIF_ITRP_STATUS_REG);
	if(((U32)spdif_itrp_enable & 0x0020)==0x0020)
    	if(((U32)spdif_itrp_status & 0x0020) == 0x0020)
            SET_REG_VAL(SPDIF_ITRP_STATUS_CLEAR_REG, 0x20);

#if defined (ST_OS21) || defined (ST_OSWINCE)
    return OS21_SUCCESS;
#elif defined (ST_OSLINUX)
        return IRQ_HANDLED;
#endif
}

STOS_INTERRUPT_DECLARE(PCMPlayer0InterruptHandler, pParams)
{
	U32 pcm0_itrp_status = 0;
	U32 pcm0_itrp_enable = 0;
/*	U32 pcm0_control_stop = 0;*/

    pcm0_itrp_enable = GET_REG_VAL(PCMPLAYER0_ITRP_ENABLE_REG);

	/* PCM0 Underflow */
    pcm0_itrp_status = GET_REG_VAL(PCMPLAYER0_ITRP_STATUS_REG);
	if((pcm0_itrp_enable & 0x0001)==0x0001)
    	if((pcm0_itrp_status & 0x0001) == 0x0001)
    	{
/*            pcm0_control_stop = GET_REG_VAL(PCMPLAYER0_CONTROL_REG);
            pcm0_control_stop &= 0xFFFFFFFC;
            SET_REG_VAL(PCMPLAYER0_CONTROL_REG, pcm0_control_stop);*/
            SET_REG_VAL(PCMPLAYER0_ITRP_STATUS_CLEAR_REG, 0x1);
        }

	/* PCM0 Memory Block Fully Read */
    pcm0_itrp_status = GET_REG_VAL(PCMPLAYER0_ITRP_STATUS_REG);
	if((pcm0_itrp_enable & 0x0002)==0x0002)
    	if((pcm0_itrp_status & 0x0002)==0x0002)
            SET_REG_VAL(PCMPLAYER0_ITRP_STATUS_CLEAR_REG, 0x2);

#if defined (ST_OS21) || defined (ST_OSWINCE)
    return OS21_SUCCESS;
#elif defined (ST_OSLINUX)
        return IRQ_HANDLED;
#endif
}

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
STOS_INTERRUPT_DECLARE(PCMPlayer1InterruptHandler, pParams)
{
	U32 pcm1_itrp_status = 0;
	U32 pcm1_itrp_enable = 0;
/*	U32 pcm1_control_stop = 0;*/

    pcm1_itrp_enable = GET_REG_VAL(PCMPLAYER1_ITRP_ENABLE_REG);

	/* PCM1 Underflow */
    pcm1_itrp_status = GET_REG_VAL(PCMPLAYER1_ITRP_STATUS_REG);
	if((pcm1_itrp_enable & 0x0001)==0x0001)
    	if((pcm1_itrp_status & 0x0001) == 0x0001)
    	{
/*            pcm1_control_stop = GET_REG_VAL(PCMPLAYER1_CONTROL_REG);
            pcm1_control_stop &= 0xFFFFFFFC;
            SET_REG_VAL(PCMPLAYER1_CONTROL_REG, pcm1_control_stop);*/
            SET_REG_VAL(PCMPLAYER1_ITRP_STATUS_CLEAR_REG, 0x1);
        }

	/* PCM1 Memory Block Fully Read */
    pcm1_itrp_status = GET_REG_VAL(PCMPLAYER1_ITRP_STATUS_REG);
	if((pcm1_itrp_enable & 0x0002)==0x0002)
    	if((pcm1_itrp_status & 0x0002)==0x0002)
            SET_REG_VAL(PCMPLAYER1_ITRP_STATUS_CLEAR_REG, 0x2);

#if defined (ST_OS21) || defined (ST_OSWINCE)
    return OS21_SUCCESS;
#elif defined (ST_OSLINUX)
        return IRQ_HANDLED;
#endif
}
#endif

ST_ErrorCode_t PlayerInterruptInstall(StreamType_t StreamType)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	U32 PlayerInterruptEnableSetRegister;
    U32 PlayerInterruptNumber, PlayerInterruptLevel;
    U32 PlayerInterruptMask;

    STOS_INTERRUPT_PROTOTYPE(PlayerInterruptHandler);

    if(StreamType == SPDIF_COMPRESSED)
    {
        PlayerInterruptEnableSetRegister = SPDIF_ITRP_ENABLE_SET_REG;
        PlayerInterruptNumber = INTERRUPT_NO_AUD_SPDIF_PLYR;
        PlayerInterruptLevel = INTERRUPT_LEVEL_AUD_SPDIF_PLYR;
        PlayerInterruptHandler = SPDIFPlayerInterruptHandler;
        strcpy(InterruptHandlerName, "SPDIFPlayerInterruptHandler");
        PlayerInterruptMask = 0x0; /*0x3;*/
    }
    else if(StreamType == PCM_TEN_CHANNEL)
    {
        PlayerInterruptEnableSetRegister = PCMPLAYER0_ITRP_ENABLE_SET_REG;
        PlayerInterruptNumber = INTERRUPT_NO_AUD_PCM_PLYR0;
        PlayerInterruptLevel = INTERRUPT_LEVEL_AUD_PCM_PLYR0;
        PlayerInterruptHandler = PCMPlayer0InterruptHandler;
        strcpy(InterruptHandlerName, "PCMPlayer0InterruptHandler");
        PlayerInterruptMask = 0x0; /*0x3;*/
    }
    else /* StreamType == PCM_TWO_CHANNEL */
    {
        PlayerInterruptEnableSetRegister = PCMPLAYER1_ITRP_ENABLE_SET_REG;
        PlayerInterruptNumber = INTERRUPT_NO_AUD_PCM_PLYR1;
        PlayerInterruptLevel = INTERRUPT_LEVEL_AUD_PCM_PLYR1;
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 				
        PlayerInterruptHandler = PCMPlayer1InterruptHandler;
        strcpy(InterruptHandlerName, "PCMPlayer1InterruptHandler");
#elif defined (ST_5162)
        PlayerInterruptHandler = PCMPlayer0InterruptHandler;
        strcpy(InterruptHandlerName, "PCMPlayer0InterruptHandler");
#endif
        PlayerInterruptMask = 0x0; /*0x3;*/
    }

	/* Setting Player interrupts */
	SET_REG_VAL(PlayerInterruptEnableSetRegister, PlayerInterruptMask);  /*Setting the data underflow and memory block fully read interrupts*/

    if (STOS_SUCCESS != STOS_InterruptInstall(PlayerInterruptNumber,
                                              PlayerInterruptLevel,
                                              PlayerInterruptHandler,
                                              InterruptHandlerName,
                                              NULL))
	{
	    /* Install failed */
		STTBX_Print(("\nERROR: Failed to install interrupt handler\n"));
		ErrorCode = ST_ERROR_INTERRUPT_INSTALL;
	}
    else
    {
        if (STOS_SUCCESS != STOS_InterruptEnable(PlayerInterruptNumber, PlayerInterruptLevel))
        {
    	    /* Install failed */
            STOS_InterruptUninstall(PlayerInterruptNumber, PlayerInterruptLevel, NULL);
            STTBX_Print(("ERROR: Could not enable interrupt at given level\n"));
            ErrorCode = ST_ERROR_INTERRUPT_INSTALL;
        }
    }

    return ErrorCode;
}

ST_ErrorCode_t PlayerInterruptUninstall(StreamType_t StreamType)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if(StreamType == SPDIF_COMPRESSED)
    {
        if(STOS_InterruptDisable(INTERRUPT_NO_AUD_SPDIF_PLYR, INTERRUPT_LEVEL_AUD_SPDIF_PLYR) != STOS_SUCCESS)
            ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
        if(STOS_InterruptUninstall(INTERRUPT_NO_AUD_SPDIF_PLYR, INTERRUPT_LEVEL_AUD_SPDIF_PLYR, NULL) != STOS_SUCCESS)
            ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }
    else if(StreamType == PCM_TEN_CHANNEL)
    {
        if(STOS_InterruptDisable(INTERRUPT_NO_AUD_PCM_PLYR0, INTERRUPT_LEVEL_AUD_PCM_PLYR0) != STOS_SUCCESS)
            ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
        if(STOS_InterruptUninstall(INTERRUPT_NO_AUD_PCM_PLYR0, INTERRUPT_LEVEL_AUD_PCM_PLYR0, NULL) != STOS_SUCCESS)
            ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }
    else /* StreamType == PCM_TWO_CHANNEL */
    {
        if(STOS_InterruptDisable(INTERRUPT_NO_AUD_PCM_PLYR1, INTERRUPT_LEVEL_AUD_PCM_PLYR1) != STOS_SUCCESS)
            ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
        if(STOS_InterruptUninstall(INTERRUPT_NO_AUD_PCM_PLYR1, INTERRUPT_LEVEL_AUD_PCM_PLYR1, NULL) != STOS_SUCCESS)
            ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }

    return (ErrorCode);
}

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
void FrequencySynthesizerConfigure(void)
{
    /* Generic setting   for 256*48Khz */
#if defined (ST_7200)
    U32 cnfgval1 = 0x000170C0;
    U32 cnfgval2 = 0x000170C1;
#else
    U32 cnfgval1 = 0x00007FFE;
    U32 cnfgval2 = 0x00007FFF;
#endif

    /* Configuring the Frequency Synthesizer for Audio IPs */

    /* cnfgval    =    0x1;               // rst_p
    cnfgval   |=  ((0x1)<<1);           //selclkin
    cnfgval   |=  ((0x7)<<2);           //pcmclk_sel    -- ext or fsynth clock selection
    cnfgval   |=  ((0xF)<<6);           //selclkout     -- synth bypass
    cnfgval   |=  ((0xF)<<10);          //nsb           -- Digital Standby (Active Low)
    cnfgval   |=  ((0x1)<<14);          //npda          -- Analog power down
    cnfgval   |=  ((0x0)<<15);          //ndiv          -- 0:27MHz   1:54MHz
    cnfgval   |=  ((0x3)<<16);          //selbw         -- PLL filter selection
    cnfgval   |=  ((0x1F)<<18);         //tst_clk_sel   -- fsyn clock sel for test jitter analysis
    cnfgval   |=  ((0x0)<<23);          //ref_clk_in    -- fsyn ref clock selection*/

#if defined (ST_7200)
    if(TestType != ALL_PLUS_MTOM && TestType != SPDIF_PLAYER_ONLY)
#endif
    {
        SET_REG_VAL(AUDIO_FSYN_CFG, cnfgval2);
        SET_REG_VAL(AUDIO_FSYN_CFG, cnfgval1);
    }

    if(TestType == ALL_PLUS_MTOM || TestType == SPDIF_PLAYER_ONLY)
    {
        /* Configuring the SPDIF Player Clock */
#if defined (ST_7200)
        SET_REG_VAL(AUDIO_FSYNB_CFG, 0x000170C1);
        SET_REG_VAL(AUDIO_FSYNB_CFG, 0x000170C0);
#endif

        SET_REG_VAL(AUDIO_FSYN_SPDIF_PROG_EN, 0x0);
        SET_REG_VAL(AUDIO_FSYN_SPDIF_MD,      FSCLockValues[FREQ_256_48MHz].MD_Val);
        SET_REG_VAL(AUDIO_FSYN_SPDIF_PE,      FSCLockValues[FREQ_256_48MHz].PE_Val);
        SET_REG_VAL(AUDIO_FSYN_SPDIF_SDIV,    FSCLockValues[FREQ_256_48MHz].SDIV_Val);
        SET_REG_VAL(AUDIO_FSYN_SPDIF_PROG_EN, 0x1);
    }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_0_ONLY)
    {
        /* Configuring the PCM Player #0 Clock */
        SET_REG_VAL(AUDIO_FSYN_PCM0_PROG_EN, 0x0);
        SET_REG_VAL(AUDIO_FSYN_PCM0_MD,      FSCLockValues[FREQ_256_48MHz].MD_Val);
        SET_REG_VAL(AUDIO_FSYN_PCM0_PE,      FSCLockValues[FREQ_256_48MHz].PE_Val);
        SET_REG_VAL(AUDIO_FSYN_PCM0_SDIV,    FSCLockValues[FREQ_256_48MHz].SDIV_Val);
        SET_REG_VAL(AUDIO_FSYN_PCM0_PROG_EN, 0x1);
    }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_1_ONLY)
    {
        /* Configuring the PCM Player #1 Clock */
        SET_REG_VAL(AUDIO_FSYN_PCM1_PROG_EN, 0x1);
        SET_REG_VAL(AUDIO_FSYN_PCM1_MD,      FSCLockValues[FREQ_256_48MHz].MD_Val);
        SET_REG_VAL(AUDIO_FSYN_PCM1_PE,      FSCLockValues[FREQ_256_48MHz].PE_Val);
        SET_REG_VAL(AUDIO_FSYN_PCM1_SDIV,    FSCLockValues[FREQ_256_48MHz].SDIV_Val);
        SET_REG_VAL(AUDIO_FSYN_PCM1_PROG_EN, 0x0);
    }
}

void FrequencySynthesizerReset(void)
{
    /* Resetting the Frequency Synthesizer */
#if defined (ST_7200)
    if(TestType != ALL_PLUS_MTOM && TestType != SPDIF_PLAYER_ONLY)
#endif
    {
        SET_REG_VAL(AUDIO_FSYN_CFG, 0x0);
    }

    if(TestType == ALL_PLUS_MTOM || TestType == SPDIF_PLAYER_ONLY)
    {
        /* Resetting the SPDIF Player clock */
#if defined (ST_7200)
        SET_REG_VAL(AUDIO_FSYNB_CFG, 0x0);
#endif
        SET_REG_VAL(AUDIO_FSYN_SPDIF_MD,      0x0);
        SET_REG_VAL(AUDIO_FSYN_SPDIF_PE,      0x0);
        SET_REG_VAL(AUDIO_FSYN_SPDIF_SDIV,    0x0);
        SET_REG_VAL(AUDIO_FSYN_SPDIF_PROG_EN, 0x1);
    }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_0_ONLY)
    {
        /* Resetting the PCM Player #0 clock */
        SET_REG_VAL(AUDIO_FSYN_PCM0_MD,      0x0);
        SET_REG_VAL(AUDIO_FSYN_PCM0_PE,      0x0);
        SET_REG_VAL(AUDIO_FSYN_PCM0_SDIV,    0x0);
        SET_REG_VAL(AUDIO_FSYN_PCM0_PROG_EN, 0x1);
    }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_1_ONLY)
    {
        /* Resetting the PCM Player #1 clock */
        SET_REG_VAL(AUDIO_FSYN_PCM1_MD,      0x0);
        SET_REG_VAL(AUDIO_FSYN_PCM1_PE,      0x0);
        SET_REG_VAL(AUDIO_FSYN_PCM1_SDIV,    0x0);
        SET_REG_VAL(AUDIO_FSYN_PCM1_PROG_EN, 0x1);
    }
}
#endif

void PlayerConfigure(StreamType_t StreamType, U32 SampleCount)
{
    if(StreamType == SPDIF_COMPRESSED)
    {
        /*** Playing ***/
        spdifplayer_control spdifcontrol;
    	U32 spdifcontrol_temp = 0;

        /* SPDIF Player control register configuration */
    	spdifcontrol.operation            	= spdif_off;
    	spdifcontrol.idlestate		   	    = idle_true;
    	spdifcontrol.rounding		   	    = no_rounding_spdif;
    	spdifcontrol.divider64             	= fs1_256_64;
    	spdifcontrol.byteswap	           	= nobyte_swap_encoded_mode;
    	spdifcontrol.stuffing_hard_soft    	= stuffing_software_encoded_mode;
    	spdifcontrol.samples_read          	= SampleCount/2;

    	spdifcontrol_temp  = (spdifcontrol.operation);
    	spdifcontrol_temp |= (spdifcontrol.idlestate<<3);
    	spdifcontrol_temp |= (spdifcontrol.rounding<<4);
    	spdifcontrol_temp |= (spdifcontrol.divider64<<5);
    	spdifcontrol_temp |= (spdifcontrol.byteswap<<13);
    	spdifcontrol_temp |= (spdifcontrol.stuffing_hard_soft<<14);
    	spdifcontrol_temp |= (spdifcontrol.samples_read<<15);

        SET_REG_VAL(SPDIF_CONTROL_REG, spdifcontrol_temp);

        /*** Mute ***/
        SET_REG_VAL(SPDIF_PA_PB_REG,            0xF8724E1F);    /*Setting the sync word Pa_Pb*/
        SET_REG_VAL(SPDIF_PC_PD_REG,            0x03001024);    /*Setting the Pause Control Word and Pause Data value (1024)*/
        SET_REG_VAL(SPDIF_CL1_REG,              0x02000100);    /*Channel status bits for SubFrame#0*/
        SET_REG_VAL(SPDIF_CR1_REG,              0x02000302);    /*Channel status bits for SubFrame#1*/
        SET_REG_VAL(SPDIF_CL2_CR2_UV_REG,       0x00000000);    /*User Status Bits and Left 35~32 Channel status Bits*/
        SET_REG_VAL(SPDIF_PAUSE_LAT_REG,        0x00030020);    /*No. of repeptions [31:16] + No. of frames after which EOL will come [15:0]*/
        SET_REG_VAL(SPDIF_FRAMELGTH_BURST_REG,  0x06000600);    /*Pause Burst Repetition period 32 IEC Frames [15:0]*/
    }
    else if(StreamType == PCM_TEN_CHANNEL)
    {
		pcmplayer_control pcm0control;
		pcmplayer_format  pcm0format;
        U32 pcm0control_temp = 0, pcm0format_temp = 0;

		/*** PCM Player format register configuration ****/
		pcm0format.bits_per_subframe = subframebit_32;
		pcm0format.data_size 	     = bit_16; /*????????*/
		pcm0format.lr_clock_polarity = leftword_lrclklow;
		pcm0format.sclk_edge   	     = risingedge_pcmsclk;
		pcm0format.padding    	     = delay_data_onebit_clk;
		pcm0format.align      	     = data_first_sclkcycle_lrclk;
		pcm0format.order      	     = data_msbfirst;

		pcm0format_temp  = pcm0format.bits_per_subframe;
		pcm0format_temp |= pcm0format.data_size<<1;
		pcm0format_temp |= pcm0format.lr_clock_polarity<<3;
		pcm0format_temp |= pcm0format.sclk_edge<<4;
		pcm0format_temp |= pcm0format.padding<<5;
		pcm0format_temp |= pcm0format.align<<6;
		pcm0format_temp |= pcm0format.order<<7;
		pcm0format_temp |= (0x5)<<8;        /* All 5 channels selected */
		pcm0format_temp |= (0xA)<<12;       /* Dreq generation per 10*4 bytes. This value holds true for 7109 cut 1.0 */
/*		pcm0format_temp |= (0x1E)<<11;*/      /* 12?????? Fifo size programmed per dreq. This value hold true for 7109 cut 2.0. Here set for 16 bytes */

        SET_REG_VAL(PCMPLAYER0_FORMAT_REG, pcm0format_temp);

		/*** PCM Player control register configuration ***/
		pcm0control.operation             = pcm_off;
		pcm0control.memory_storage_format = bits_16_0;/*bits_16_16;*/
		pcm0control.rounding              = no_rounding_pcm;
		pcm0control.divider64             = fs_256_64;
		pcm0control.wait_spdif_latency    = donot_wait_spdif_latency;
		pcm0control.samples_read          = SampleCount;

		pcm0control_temp  = pcm0control.operation;
		pcm0control_temp |= pcm0control.memory_storage_format<<2;
		pcm0control_temp |= pcm0control.rounding<<3;
		pcm0control_temp |= pcm0control.divider64<<4;
		pcm0control_temp |= pcm0control.wait_spdif_latency<<12;
      	pcm0control_temp |= (0x100)<<13;/*pcm0control.samples_read<<13;*/

        SET_REG_VAL(PCMPLAYER0_CONTROL_REG, pcm0control_temp);
    }
    else /* StreamType == PCM_TWO_CHANNEL */
    {
		pcmplayer_control pcm1control;
		pcmplayer_format  pcm1format;
        U32 pcm1control_temp = 0, pcm1format_temp = 0;

		/*** PCM Player format register configuration ****/
		pcm1format.bits_per_subframe = subframebit_32;
		pcm1format.data_size 	     = bit_16; /*????????*/
		pcm1format.lr_clock_polarity = leftword_lrclklow;
		pcm1format.sclk_edge   	     = risingedge_pcmsclk;
		pcm1format.padding    	     = delay_data_onebit_clk;
		pcm1format.align      	     = data_first_sclkcycle_lrclk;
		pcm1format.order      	     = data_msbfirst;

		pcm1format_temp  = pcm1format.bits_per_subframe;
		pcm1format_temp |= pcm1format.data_size<<1;
		pcm1format_temp |= pcm1format.lr_clock_polarity<<3;
		pcm1format_temp |= pcm1format.sclk_edge<<4;
		pcm1format_temp |= pcm1format.padding<<5;
		pcm1format_temp |= pcm1format.align<<6;
		pcm1format_temp |= pcm1format.order<<7;
		pcm1format_temp |= (0x5)<<8;        /* All 5 channels selected */
		pcm1format_temp |= (0xA)<<12;       /* Dreq generation per 10*4 bytes. This value holds true for 7109 cut 1.0 */
/*		pcm1format_temp |= (0x1E)<<11;*/      /* 12?????? Fifo size programmed per dreq. This value hold true for 7109 cut 2.0. Here set for 16 bytes */

        SET_REG_VAL(PCMPLAYER1_FORMAT_REG, pcm1format_temp);

		/*** PCM Player control register configuration ***/
		pcm1control.operation             = pcm_off;
		pcm1control.memory_storage_format = bits_16_0;/*bits_16_16;*/
		pcm1control.rounding              = no_rounding_pcm;
		pcm1control.divider64             = fs_256_64;
		pcm1control.wait_spdif_latency    = donot_wait_spdif_latency;
		pcm1control.samples_read          = SampleCount;

		pcm1control_temp  = pcm1control.operation;
		pcm1control_temp |= pcm1control.memory_storage_format<<2;
		pcm1control_temp |= pcm1control.rounding<<3;
		pcm1control_temp |= pcm1control.divider64<<4;
		pcm1control_temp |= pcm1control.wait_spdif_latency<<12;
      	pcm1control_temp |= (0x100)<<13;/*pcm1control.samples_read<<13;*/

        SET_REG_VAL(PCMPLAYER1_CONTROL_REG, pcm1control_temp);
    }

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
    /*** IO ***/
#if defined (ST_7200)
    SET_REG_VAL(AUDIO_IO_CONTROL_REG, 0x0000001f);
#else
    SET_REG_VAL(AUDIO_IO_CONTROL_REG, 0x0000000f);
#endif
#endif
}

int InjectStreamIntoMemory(StreamType_t StreamType);


#if defined (ST_OSLINUX)
static int StreamTransferPlaybackTask(void *pData)
#elif defined (ST_OS20) || defined (ST_OS21)
static void StreamTransferPlaybackTask(void *pData)
#endif
{
    U32 i;
    STFDMA_GenericNode_t *Node_p;
    STFDMA_TransferGenericParams_t TransferParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 SampleCount = 0;
	U32 SType = (U32) pData;
    StreamType_t StreamType = (StreamType_t)SType;

    STOS_TaskEnter(pData);

    if((SampleCount = InjectStreamIntoMemory(StreamType)) == 0)
        fdmatst_SetSuccessState(FALSE);
    else
    {
        if(StreamType == SPDIF_COMPRESSED)
        {
            DetermineStreamType(StreamType);		
            CalculateFrameSize(StreamType);
        }

        if(PlayerInterruptInstall(StreamType) == ST_NO_ERROR)
        {
            PlayerConfigure(StreamType, SampleCount);

            SET_REG_VAL(SPDIF_ADD_DATA_REGION_FRAME_COUNT, 0x0);
            SET_REG_VAL(SPDIF_ADD_DATA_REGION_STATUS, 0x0);
            SET_REG_VAL(SPDIF_ADD_DATA_REGION_PREC_MASK, 0xffffff00);

            ErrorCode = FDMALinkedListAllocate(StreamType, (void **)&Node_p, SampleCount);
            if(ST_NO_ERROR != ErrorCode)
            {
                STTBX_Print(("\nError: Linked list of FDMA nodes could not be set up\n"));
                fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
                return RETURN_FAIL;
            }

        	ConvertListAddressesToPeripheral(StreamType, (void *)Node_p);
          	TransferParams.ChannelId			= STFDMA_USE_ANY_CHANNEL;
        	TransferParams.NodeAddress_p		= TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        	TransferParams.BlockingTimeout		= 0;
        	TransferParams.ApplicationData_p    = NULL;
        	TransferParams.FDMABlock            = STFDMA_1;

            if(StreamType == SPDIF_COMPRESSED)
            {
            	TransferParams.Pool                 = STFDMA_SPDIF_POOL;
            	TransferParams.CallbackFunction		= SPDIFUserCallback;
            	if(TestType == ALL_PLUS_MTOM)
                	STOS_SemaphoreSignal(pLaunchNextTaskSemap);
            	else
            	    STOS_SemaphoreSignal(pInitFDMASemap);

            	STOS_SemaphoreWait(pSPDIFFeedSemap);	
                if(TestType == ALL_PLUS_MTOM)
            	    STOS_SemaphoreSignal(pPCM0FeedSemap);
            }
            else if(StreamType == PCM_TEN_CHANNEL)
            {
            	TransferParams.Pool                 = STFDMA_DEFAULT_POOL;
            	TransferParams.CallbackFunction		= PCM0UserCallback;
            	if(TestType == ALL_PLUS_MTOM)
                	STOS_SemaphoreSignal(pLaunchNextTaskSemap);
            	else
            	    STOS_SemaphoreSignal(pInitFDMASemap);

            	STOS_SemaphoreWait(pPCM0FeedSemap);
                if(TestType == ALL_PLUS_MTOM)
            	    STOS_SemaphoreSignal(pPCM1FeedSemap);
            }
            else /* StreamType == PCM_TWO_CHANNEL */
            {
            	TransferParams.Pool                 = STFDMA_DEFAULT_POOL;
            	TransferParams.CallbackFunction		= PCM1UserCallback;
            	if(TestType == ALL_PLUS_MTOM)
                	STOS_SemaphoreSignal(pLaunchNextTaskSemap);
            	else
            	    STOS_SemaphoreSignal(pInitFDMASemap);

            	STOS_SemaphoreWait(pPCM1FeedSemap);
                if(TestType == ALL_PLUS_MTOM)
            	    STOS_SemaphoreSignal(pMTOMTaskContSemap);
            }

    		for(i = 0; i < ((TestType == ALL_PLUS_MTOM)? 1: LOOP_COUNT); i++)
    		{		
                if(StreamType == SPDIF_COMPRESSED)
        		    ErrorCode = STFDMA_StartGenericTransfer(&TransferParams, &SPDIFTransferId);
                else if(StreamType == PCM_TEN_CHANNEL)
                    ErrorCode = STFDMA_StartGenericTransfer(&TransferParams, &PCM0TransferId);
                else /* StreamType == PCM_TWO_CHANNEL */
    		        ErrorCode = STFDMA_StartGenericTransfer(&TransferParams, &PCM1TransferId);

    	  	    if(ErrorCode != ST_NO_ERROR)
    	  	    {
    				STTBX_Print(("Error: Failed to start channel\n"));
    				return RETURN_FAIL;
    			}

                if (i == 0)
                    PlayerStart(StreamType);			

                if(TestType!= ALL_PLUS_MTOM)
                {
                    if(StreamType == SPDIF_COMPRESSED)
                    {
#if 1                    
               			STOS_SemaphoreWait(pSPDIFPlaySemap);    /* Wait here until the transfer is complete. Signal received from user callback function */						
#else /*To debug missed calbacks*/               			
						WaitTime1 = (60  * fdmatst_GetClocksPerSec() );									 
						for (count=0; count < 25; count++)
						{						
						 	STOS_TaskDelay(fdmatst_GetClocksPerSec());
							STTBX_Print(("1. Waiting ...%d\n", count));
						}						    
						STOS_SemaphoreWaitTimeOut(pSPDIFPlaySemap, TIMEOUT_INFINITY)	;						    				
					    STTBX_Print(("Waiting done...\n")); 			
#endif							
                    	}				
                    else if(StreamType == PCM_TEN_CHANNEL)                    
                        STOS_SemaphoreWait(pPCM0PlaySemap);												
                    else /* StreamType == PCM_TWO_CHANNEL */
                        STOS_SemaphoreWait(pPCM1PlaySemap);
					
                }
            }

            if(TestType == ALL_PLUS_MTOM)
            {
                if(StreamType == SPDIF_COMPRESSED)
                    STOS_SemaphoreWait(pSPDIFAbortSemap);				
                else if(StreamType == PCM_TEN_CHANNEL)
                    STOS_SemaphoreWait(pPCM0AbortSemap);
                else /* StreamType == PCM_TWO_CHANNEL */
                    STOS_SemaphoreWait(pPCM1AbortSemap);
            }

            ConvertListAddressesToVirtual(StreamType, (void *)Node_p);
            FDMALinkedListDeallocate(StreamType, (void **)&Node_p);

     		PlayerStop(StreamType);
     		PlayerReset(StreamType);

     		ErrorCode = PlayerInterruptUninstall(StreamType);
            fdmatst_ErrorReport("PlayerInterruptUnistall", ErrorCode, ST_NO_ERROR);
        }
        else
            fdmatst_SetSuccessState(FALSE);
    }

    if(StreamType == SPDIF_COMPRESSED)
        STOS_SemaphoreSignal(pSPDIFStopSemap);
    else if(StreamType == PCM_TEN_CHANNEL)
        STOS_SemaphoreSignal(pPCM0StopSemap);
    else /* StreamType == PCM_TWO_CHANNEL */
        STOS_SemaphoreSignal(pPCM1StopSemap);

    STOS_TaskExit(pData);
#if defined (ST_OSLINUX)
    return ST_NO_ERROR;
#endif
}

int InjectStreamIntoMemory(StreamType_t StreamType)
{
    U32 i = 0, j = 0;

    U32 value = 0;
    U32 value_l = 0, value_r = 0;

    unsigned char *temp = NULL;
    U32 bytes_read = 0, offset = 0;
    U32 file_size = 0;

    FILE *stream = NULL;
    U32 total_bytes_read = 0;
    U32 first_read_failed = 1;

  	stream = fopen((StreamType == SPDIF_COMPRESSED)? SPDIF_STREAM: PCM_STREAM, "rb");
 	if (ferror(stream))
  	{
		STTBX_Print(("Error: Unable to open the file %s\n", (StreamType == SPDIF_COMPRESSED)? SPDIF_STREAM: PCM_STREAM));
   		return (0);
  	}

 	fseek(stream, 0, SEEK_END);
 	file_size = ftell(stream);
 	fseek(stream, 0, SEEK_SET);

	temp = (unsigned char *)STOS_MemoryAllocate(system_partition, FILE_BLOCK_SIZE * WORD_SIZE * sizeof(unsigned char));

    /* Loop terminates when bytes_read = 0 (no data read from file) and total data (total_bytes_read) read exceeds the quota of 600*1024 */
	for(total_bytes_read = 0, bytes_read = 1; (bytes_read > 0) && (total_bytes_read <= MAX_FILE_SIZE);)
    {
    	bytes_read = (U32)fread(temp, WORD_SIZE, FILE_BLOCK_SIZE, stream);		
    	if(bytes_read > 0)
    	{
            first_read_failed = 0;

    		if(bytes_read + total_bytes_read > MAX_FILE_SIZE)
    			bytes_read = (MAX_FILE_SIZE - total_bytes_read);

        	for(i = 0; i < bytes_read;)
    		{
    		    if(StreamType == SPDIF_COMPRESSED)
    		    {
                	/* (Left + Right) channel data (compact data mode) */
                    for(j = 4; j > 0; j--)
                    {
                        value <<= 8;
                        value += temp[i+(j-1)];
                    }

                    i += 4;

                    SET_REG_VAL(SPDIF_PLAYER_BUFFER_ADDRESS + offset, value);
                    offset += 4;					   				
                }
                else /* StreamType == PCM_TEN/TWO_CHANNEL */
                {
                    U32 PCM_PLAYER_BUFFER_ADDRESS = (StreamType == PCM_TEN_CHANNEL)? PCM_PLAYER_0_BUFFER_ADDRESS: PCM_PLAYER_1_BUFFER_ADDRESS;

                    value_l = temp[i+1];
                    value_l <<= 8;
                    value_l += temp[i];
                    value_l <<= 16;

                    value_r = temp[i+3];
                    value_r <<= 8;
                    value_r += temp[i+2];
                    value_r <<= 16;

                    i += 4;

                    /* Rear */
                    SET_REG_VAL(PCM_PLAYER_BUFFER_ADDRESS + offset,        value_l);
                    SET_REG_VAL(PCM_PLAYER_BUFFER_ADDRESS + (offset += 4), value_r);

                    /* Center */
                    SET_REG_VAL(PCM_PLAYER_BUFFER_ADDRESS + (offset += 4), value_l);
                    SET_REG_VAL(PCM_PLAYER_BUFFER_ADDRESS + (offset += 4), value_r);

                    /* Front */
                    SET_REG_VAL(PCM_PLAYER_BUFFER_ADDRESS + (offset += 4), value_l);
                    SET_REG_VAL(PCM_PLAYER_BUFFER_ADDRESS + (offset += 4), value_r);

                    /* Surround */
                    SET_REG_VAL(PCM_PLAYER_BUFFER_ADDRESS + (offset += 4), value_l);
                    SET_REG_VAL(PCM_PLAYER_BUFFER_ADDRESS + (offset += 4), value_r);

                    /* Dual */
                    SET_REG_VAL(PCM_PLAYER_BUFFER_ADDRESS + (offset += 4), value_l);
                    SET_REG_VAL(PCM_PLAYER_BUFFER_ADDRESS + (offset += 4), value_r);

                    offset += 4;										
                }
    		}

    		total_bytes_read += bytes_read;
    	}
    	else if(first_read_failed)
    	{
    		STTBX_Print(("\nError: Cannot read from file %s\n", (StreamType == SPDIF_COMPRESSED)? SPDIF_STREAM: PCM_STREAM));
        	STOS_MemoryDeallocate(system_partition, temp);
        	fclose(stream);
    		return(0);
    	}
    	else
    	{
    	    STTBX_Print(("\nEnd of file %s reached\n", (StreamType == SPDIF_COMPRESSED)? SPDIF_STREAM: PCM_STREAM));
    	    break;
    	}

        /* When data extracted from file is a multilple of 1KB, display this information */
    	if(total_bytes_read % FILE_READ == 0)
    		STTBX_Print(("Read %dKB of data from file %s\n", total_bytes_read / FILE_READ, (StreamType == SPDIF_COMPRESSED)? SPDIF_STREAM: PCM_STREAM));
    }

    /* Releasing the buffer area allocated for holding data from the file temporarily before placing it in memory */
	STOS_MemoryDeallocate(system_partition, temp);
	fclose(stream);

	if(file_size >= MAX_FILE_SIZE)
	{
		STTBX_Print(("\nSize of Audio loaded (%s) = %d Bytes\n\n", (StreamType == SPDIF_COMPRESSED)? SPDIF_STREAM: PCM_STREAM, MAX_FILE_SIZE));
		return (MAX_FILE_SIZE);
	}
	else
	{
		STTBX_Print(("\nSize of Audio loaded (%s) = %d Bytes\n\n", (StreamType == SPDIF_COMPRESSED)? SPDIF_STREAM: PCM_STREAM, (StreamType == SPDIF_COMPRESSED)? file_size: (U32)(file_size/4)*4));
		return(file_size);
	}
}

#if defined (ST_OSLINUX)
static int CircularTransferTask(void *pData)
#elif defined (ST_OS20) || defined (ST_OS21)
static void CircularTransferTask(void *pData)
#endif
{
    STFDMA_Node_t *Node_p;
    STFDMA_TransferGenericParams_t TransferParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STOS_TaskEnter(pData);

    ErrorCode = fdmatst_CreateNode(FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                   FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                   FALSE, FALSE,
                                   STFDMA_REQUEST_SIGNAL_NONE,&Node_p);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    ErrorCode = fdmatst_CreateNode(FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                   FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                   FALSE, FALSE,
                                   STFDMA_REQUEST_SIGNAL_NONE,&(Node_p->Next_p));
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    ErrorCode = fdmatst_CreateNode(FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                   FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                   FALSE, FALSE,
                                   STFDMA_REQUEST_SIGNAL_NONE,&(Node_p->Next_p->Next_p));
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    ErrorCode = fdmatst_CreateNode(FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                   FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                   FALSE, FALSE,
                                   STFDMA_REQUEST_SIGNAL_NONE,&(Node_p->Next_p->Next_p->Next_p));
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    Node_p->Next_p->Next_p->Next_p->Next_p = Node_p;

#if defined (ST_7200)
    {
        /* As this has been commented out in fdmatst_CovertNodeAddressDataToPeripheral() for 7200 */
        int i;
        STFDMA_Node_t *TempNode_p = Node_p;

        for(i=0; i<4; i++)
        {
            TempNode_p->DestinationAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(TempNode_p->DestinationAddress_p);
            TempNode_p = TempNode_p->Next_p;
        }
    }
#endif

    TransferParams.Pool = STFDMA_DEFAULT_POOL;
    TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
    fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
    TransferParams.NodeAddress_p = (STFDMA_GenericNode_t *)(TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p));
    TransferParams.CallbackFunction = NULL; /*MTOMUserCallback;*/
    TransferParams.BlockingTimeout = 0;
    TransferParams.ApplicationData_p = NULL;
    TransferParams.FDMABlock = STFDMA_1;

    STOS_SemaphoreSignal(pInitFDMASemap);

    STOS_SemaphoreWait(pMTOMTaskContSemap);

    ErrorCode = STFDMA_StartGenericTransfer(&TransferParams, &MTOMTransferId);
/*    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);*/
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_TRANSFER_ABORTED);

/*    STOS_SemaphoreWait(pMTOMAbortSemap);*/
    STOS_SemaphoreSignal(pMTOMTaskStopSemap);

    STOS_TaskExit(pData);
#if defined (ST_OSLINUX)
    return ST_NO_ERROR;
#endif
}

void PlayStream(void)
{
    ST_ErrorCode_t SPDIFErrorCode = ST_NO_ERROR, PCM0ErrorCode = ST_NO_ERROR, PCM1ErrorCode = ST_NO_ERROR, MTOMErrorCode = ST_NO_ERROR;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t InitParams;

    TestDuration = TEST_DURATION * 3600 * fdmatst_GetClocksPerSec();

    pInitFDMASemap = STOS_SemaphoreCreateFifo(system_partition, 0);

    if(TestType == ALL_PLUS_MTOM)
        pLaunchNextTaskSemap = STOS_SemaphoreCreateFifo(system_partition, 0);

    if(TestType == ALL_PLUS_MTOM || TestType == SPDIF_PLAYER_ONLY)
    {
        pSPDIFFeedSemap      = STOS_SemaphoreCreateFifo(system_partition, 0);
        pSPDIFPlaySemap      = STOS_SemaphoreCreateFifo(system_partition, 0);
		/*To Debug missed callbacks*//* pSPDIFPlaySemap      = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);*/
        pSPDIFAbortSemap     = STOS_SemaphoreCreateFifo(system_partition, 0);
        pSPDIFStopSemap      = STOS_SemaphoreCreateFifo(system_partition, 0);
#if defined (ST_OS20)
        pSPDIFTaskDescriptor = (tdesc_t *)STOS_MemoryAllocate(system_partition, sizeof(tdesc_t));
#endif
    }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_0_ONLY)
    {
        pPCM0FeedSemap       = STOS_SemaphoreCreateFifo(system_partition, 0);
        pPCM0PlaySemap       = STOS_SemaphoreCreateFifo(system_partition, 0);
        pPCM0AbortSemap      = STOS_SemaphoreCreateFifo(system_partition, 0);
        pPCM0StopSemap       = STOS_SemaphoreCreateFifo(system_partition, 0);
#if defined (ST_OS20)
        pPCM0TaskDescriptor  = (tdesc_t *)STOS_MemoryAllocate(system_partition, sizeof(tdesc_t));
#endif
    }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_1_ONLY)
    {
        pPCM1FeedSemap       = STOS_SemaphoreCreateFifo(system_partition, 0);
        pPCM1PlaySemap       = STOS_SemaphoreCreateFifo(system_partition, 0);
        pPCM1AbortSemap      = STOS_SemaphoreCreateFifo(system_partition, 0);
        pPCM1StopSemap       = STOS_SemaphoreCreateFifo(system_partition, 0);
#if defined (ST_OS20)
        pPCM1TaskDescriptor  = (tdesc_t *)STOS_MemoryAllocate(system_partition, sizeof(tdesc_t));
#endif
    }

    if(TestType == ALL_PLUS_MTOM)
    {
        pMTOMTaskContSemap  = STOS_SemaphoreCreateFifo(system_partition, 0);
        pMTOMTaskStopSemap  = STOS_SemaphoreCreateFifo(system_partition, 0);
        pMTOMAbortSemap     = STOS_SemaphoreCreateFifo(system_partition, 0);
#if defined (ST_OS20)
        pMTOMTaskDescriptor = (tdesc_t *)STOS_MemoryAllocate(system_partition, sizeof(tdesc_t));
#endif
        pTestTimeUpSemap    = STOS_SemaphoreCreateFifo(system_partition, 0);
    }
		
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
    FrequencySynthesizerConfigure();
#endif

    if(TestType == ALL_PLUS_MTOM || TestType == SPDIF_PLAYER_ONLY)
    {
        /* SPDIF Player */
        SPDIFErrorCode = STOS_TaskCreate((void (*)(void *))StreamTransferPlaybackTask,
                                         (void *)SPDIF_COMPRESSED,
                                         system_partition,
                                         16384,
                                         (void **)&pSPDIFTaskStack,
                                         system_partition,
                                         &pSPDIFTask,
                                         pSPDIFTaskDescriptor,
                                         MAX_USER_PRIORITY,
                                         "SPDIFStreamTransferPlaybackTask",
                                         (task_flags_t)0);

        if(SPDIFErrorCode != ST_NO_ERROR)
        {
        	STTBX_Print(("\nERROR: SPDIF Player task not created\n"));
            fdmatst_SetSuccessState(FALSE);
        }
        else
            if(TestType == ALL_PLUS_MTOM)
                STOS_SemaphoreWait(pLaunchNextTaskSemap);
            else
                STOS_SemaphoreWait(pInitFDMASemap);
    }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_0_ONLY)
    {
        /* PCM Player #0 */
        PCM0ErrorCode = STOS_TaskCreate((void (*)(void *))StreamTransferPlaybackTask,
                                        (void *)PCM_TEN_CHANNEL,
                                        system_partition,
                                        16384,
                                        (void **)&pPCM0TaskStack,
                                        system_partition,
                                        &pPCM0Task,
                                        pPCM0TaskDescriptor,
                                        MAX_USER_PRIORITY,
                                        "PCM0StreamTransferPlaybackTask",
                                        (task_flags_t)0);

        if(PCM0ErrorCode != ST_NO_ERROR)
        {
        	STTBX_Print(("\nERROR: PCM Player #0 task not created\n"));
            fdmatst_SetSuccessState(FALSE);
        }
        else
            if(TestType == ALL_PLUS_MTOM)
                STOS_SemaphoreWait(pLaunchNextTaskSemap);
            else
                STOS_SemaphoreWait(pInitFDMASemap);
    }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_1_ONLY)
    {
        /* PCM Player #1 */
        PCM1ErrorCode = STOS_TaskCreate((void (*)(void *))StreamTransferPlaybackTask,
                                        (void *)PCM_TWO_CHANNEL,
                                        system_partition,
                                        16384,
                                        (void **)&pPCM1TaskStack,
                                        system_partition,
                                        &pPCM1Task,
                                        pPCM1TaskDescriptor,
                                        MAX_USER_PRIORITY,
                                        "PCM1StreamTransferPlaybackTask",
                                        (task_flags_t)0);

        if(PCM1ErrorCode != ST_NO_ERROR)
        {
        	STTBX_Print(("\nERROR: PCM Player #1 task not created\n"));
            fdmatst_SetSuccessState(FALSE);
        }
        else
            if(TestType == ALL_PLUS_MTOM)
                STOS_SemaphoreWait(pLaunchNextTaskSemap);
            else
                STOS_SemaphoreWait(pInitFDMASemap);
    }

    if(TestType == ALL_PLUS_MTOM)
    {
        /* Free-running */
        MTOMErrorCode = STOS_TaskCreate((void (*)(void *))CircularTransferTask,
                                        (void *)NULL,
                                        system_partition,
                                        16384,
                                        (void **)&pMTOMTaskStack,
                                        system_partition,
                                        &pMTOMTask,
                                        pMTOMTaskDescriptor,
                                        MAX_USER_PRIORITY,
                                        "MTOMCircularTransferTask",
                                        (task_flags_t)0);

        if(MTOMErrorCode != ST_NO_ERROR)
        {
         	STTBX_Print(("\nERROR: MTOM circular transfer task not created\n"));
            fdmatst_SetSuccessState(FALSE);
        }
        else
                STOS_SemaphoreWait(pInitFDMASemap);
    }

    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber = FDMA_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.BaseAddress_p = fdmatst_GetBaseAddress();
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
    InitParams.FDMABlock = STFDMA_1;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);

    if(ST_NO_ERROR != ErrorCode)
    {
        STTBX_Print(("\nError: Failed to initialize FDMA\n"));
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
    }
    else
    {
        if(TestType == ALL_PLUS_MTOM)
            StartTime = STOS_time_now();
        if(TestType == ALL_PLUS_MTOM || TestType == SPDIF_PLAYER_ONLY)
            STOS_SemaphoreSignal(pSPDIFFeedSemap);
        else if(TestType == PCM_PLAYER_0_ONLY)
            STOS_SemaphoreSignal(pPCM0FeedSemap);
        else /* TestType == PCM_PLAYER_1_ONLY */
       		STOS_SemaphoreSignal(pPCM1FeedSemap);
    }

    if(TestType == ALL_PLUS_MTOM)
    {
        STOS_SemaphoreWait(pTestTimeUpSemap);
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, TRUE, STFDMA_1);
        fdmatst_ErrorReport("\nForced terminate failed!\n", ErrorCode, ST_NO_ERROR);
    }

    if(TestType == ALL_PLUS_MTOM || TestType == SPDIF_PLAYER_ONLY)
        /* SPDIF Player */
        if(SPDIFErrorCode == ST_NO_ERROR)
        {
            STOS_SemaphoreWait(pSPDIFStopSemap);

            STOS_TaskWait(&pSPDIFTask, TIMEOUT_INFINITY);
            STOS_TaskDelete(pSPDIFTask,
                            system_partition,
                            pSPDIFTaskStack,
                            system_partition);
        }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_0_ONLY)
        /* PCM Player #0 */
        if(PCM0ErrorCode == ST_NO_ERROR)
        {
            STOS_SemaphoreWait(pPCM0StopSemap);

            STOS_TaskWait(&pPCM0Task, TIMEOUT_INFINITY);
            STOS_TaskDelete(pPCM0Task,
                            system_partition,
                            pPCM0TaskStack,
                            system_partition);
        }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_1_ONLY)
        /* PCM Player #1 */
        if(PCM1ErrorCode == ST_NO_ERROR)
        {
            STOS_SemaphoreWait(pPCM1StopSemap);

            STOS_TaskWait(&pPCM1Task, TIMEOUT_INFINITY);
            STOS_TaskDelete(pPCM1Task,
                            system_partition,
                            pPCM1TaskStack,
                            system_partition);
        }

    if(TestType == ALL_PLUS_MTOM)
        /* Free-running transfer */
        if(MTOMErrorCode == ST_NO_ERROR)
        {
            STOS_SemaphoreWait(pMTOMTaskStopSemap);

            STOS_TaskWait(&pMTOMTask, TIMEOUT_INFINITY);
            STOS_TaskDelete(pMTOMTask,
                            system_partition,
                            pMTOMTaskStack,
                            system_partition);
        }

    ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
    if(TestType != ALL_PLUS_MTOM)
        fdmatst_ErrorReport("STFDMA_Term", ErrorCode, ST_NO_ERROR);
    else
        fdmatst_ErrorReport("STFDMA_Term", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
    FrequencySynthesizerReset();
#endif

    STOS_SemaphoreDelete(system_partition, pInitFDMASemap);

    if(TestType == ALL_PLUS_MTOM)
        STOS_SemaphoreDelete(system_partition, pLaunchNextTaskSemap);

    if(TestType == ALL_PLUS_MTOM || TestType == SPDIF_PLAYER_ONLY)
    {
        STOS_SemaphoreDelete(system_partition, pSPDIFFeedSemap);
        STOS_SemaphoreDelete(system_partition, pSPDIFPlaySemap);
        STOS_SemaphoreDelete(system_partition, pSPDIFAbortSemap);
        STOS_SemaphoreDelete(system_partition, pSPDIFStopSemap);
#if defined (ST_OS20)
        STOS_MemoryDeallocate(system_partition, pSPDIFTaskDescriptor);
#endif
    }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_0_ONLY)
    {
        STOS_SemaphoreDelete(system_partition, pPCM0FeedSemap);
        STOS_SemaphoreDelete(system_partition, pPCM0PlaySemap);
        STOS_SemaphoreDelete(system_partition, pPCM0AbortSemap);
        STOS_SemaphoreDelete(system_partition, pPCM0StopSemap);
#if defined (ST_OS20)
        STOS_MemoryDeallocate(system_partition, pPCM0TaskDescriptor);
#endif
    }

    if(TestType == ALL_PLUS_MTOM || TestType == PCM_PLAYER_1_ONLY)
    {
        STOS_SemaphoreDelete(system_partition, pPCM1FeedSemap);
        STOS_SemaphoreDelete(system_partition, pPCM1PlaySemap);
        STOS_SemaphoreDelete(system_partition, pPCM1AbortSemap);
        STOS_SemaphoreDelete(system_partition, pPCM1StopSemap);
#if defined (ST_OS20)
        STOS_MemoryDeallocate(system_partition, pPCM1TaskDescriptor);
#endif
    }

    if(TestType == ALL_PLUS_MTOM)
    {
        STOS_SemaphoreDelete(system_partition, pMTOMTaskContSemap);
        STOS_SemaphoreDelete(system_partition, pMTOMTaskStopSemap);
        STOS_SemaphoreDelete(system_partition, pMTOMAbortSemap);
#if defined (ST_OS20)
        STOS_MemoryDeallocate(system_partition, pMTOMTaskDescriptor);
#endif
        STOS_SemaphoreDelete(system_partition, pTestTimeUpSemap);
    }

    fmdatst_TestCaseSummarise();
}

void audiopaced_RunAudioPacedTest(void)
{
    STTBX_Print(("\nAudio (Paced) Test Cases:\n"));
    STTBX_Print(("--------------------------------------------------------\n"));

    STTBX_Print(("Executing with SPDIF Player only:\n\n"));
    TestType = SPDIF_PLAYER_ONLY;
    PlayStream();

    STTBX_Print(("Executing with PCM Player #0 only:\n\n"));
    TestType = PCM_PLAYER_0_ONLY;
    PlayStream();

    STTBX_Print(("Executing with PCM Player #1 only:\n\n"));
    TestType = PCM_PLAYER_1_ONLY;
    PlayStream();

    STTBX_Print(("Executing with all three IPs + a free running transfer:\n\n"));
    TestType = ALL_PLUS_MTOM;
    PlayStream();
}
