/*****************************************************************************
 *
 *  Module      : staudlx_core_proc.c
 *  Date        : 02-07-2005
 *  Author      : WALKERM
 *  Description : procfs implementation
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/proc_fs.h>
#include <linux/errno.h>   /* Defines standard error codes */

#include "stos.h"
#include "staudlx_core_types.h"       /* Modules specific defines */
#include "staudlx_core_proc.h"
#include "blockmanager.h"
#include "aud_mmeaudiostreamdecoder.h"
#include "aud_mmepp.h"
#include "aud_pes_es_parser.h"
#include "aud_pcmplayer.h"
#include "aud_spdifplayer.h"
#include "aud_utils.h"

#ifdef ENABLE_LX_DEBUGGING
//#include "audio_decodertypes.h"
#define ARRAY_SIZE 4000
#endif
#define DUMP_DATA_MAX       100
#define DUMP_LINE_LENGTH    55

/* All proc files for this module will appear in here. */
static struct proc_dir_entry* staudlx_core_proc_root_dir = NULL;

/* see aud_pes_es_parser.c */
#define USE_TRACE_BUFFER_FOR_DEBUG 0
#if USE_TRACE_BUFFER_FOR_DEBUG
extern U32 STC_array_aud [5][10000];
extern U32 sampleCount_aud;
#endif

#ifdef ENABLE_LX_DEBUGGING
extern char* lx_debug_ptr;
#endif


#if defined (ENABLE_SPDIF_INTERRUPT)
extern U32 Spdif_underflow_count ;
extern U32 Spdif_InterruptValue ;
extern U32 Spdif_InterruptEnable;
#endif

#define LOG_AVSYNC_LINUX1 0
#if LOG_AVSYNC_LINUX1
#define NUM_OF_ELEMENTS 200
extern U32 Log_To_Array;
extern U32 Aud_Array[NUM_OF_ELEMENTS][2];
#endif
//extern U32 Parser_PTS_Count;

//extern U32 Received_PTS_Count;
//static int countdump = 0;

extern U32 PCR_array_aud[6][500];
extern U32 PCRSampleCount_aud;

extern DecoderControlBlockList_t	*DecoderQueueHead_p;
extern DataProcesserControlBlock_t *dataProcesserControlBlock;
extern PPControlBlockList_t	*PPQueueHead_p;
extern PESES_ParserControl_t *PESESHead_p;
extern PCMPlayerControlBlock_t *pcmPlayerControlBlock;
extern SPDIFPlayerControlBlock_t *spdifPlayerControlBlock;
extern MemoryPoolControl_t *MemPoolHead_p;
//extern U32 wrap_count;

/* static int countPcrDump = 0; */


/*****************************************************************************/

int PrintParserInfo(PESES_ParserControl_t	*ParserControlBlock,char * buf)
{
	int len = 0;

	len += sprintf(buf+len,"Name    = %s\n", ParserControlBlock->Name);
	len += sprintf(buf+len,"Block Recvd    = %d\n", ParserControlBlock->Blk_in);
	len += sprintf(buf+len,"Block Sentl    = %d\n", ParserControlBlock->Blk_out);
	len += sprintf(buf+len,"Cur State    = %d\n", ParserControlBlock->PESESCurrentState.State);
	len += sprintf(buf+len,"Next State    = %d\n", ParserControlBlock->PESESNextState.State);
	len += sprintf(buf+len,"Pes Filled    = %d\n", ParserControlBlock->PESFilled);
	len += sprintf(buf+len,"Sent For Parsing    = %d\n", ParserControlBlock->sentForParsing);
	len += sprintf(buf+len,"Bytes Used   = %d\n", ParserControlBlock->BytesUsed);
	len += sprintf(buf+len,"Write Pointer  = %x\n", (U32)ParserControlBlock->CurrentPTIWritePointer_p);
	len += sprintf(buf+len,"Read Pointer   = %x\n", (U32)ParserControlBlock->PTIReadPointer_p);
	len += sprintf(buf+len,"Bit Buffer Level(pointer diff)  = %d\n", ParserControlBlock->PointerDifference);

	return len;

}

int staudlx_core_Parser0State
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
 	PESES_ParserControl_t	*ParserControlBlock=NULL;
	int                    len  = 0;

	ParserControlBlock = PESESHead_p;
	while(ParserControlBlock != NULL)
	{
		if(!strcmp(ParserControlBlock->Name,"PESES0"))
			break;
		ParserControlBlock = ParserControlBlock->Next_p;
	}

	if(ParserControlBlock)
	{
		len += PrintParserInfo(ParserControlBlock,buf);
	}
	else
	{
		len += sprintf(buf+len,"Component not in chain  = %s\n", "PESES0");
	}

	*eof = 1;
	return len;
}

int staudlx_core_Parser1State
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
 	PESES_ParserControl_t	*ParserControlBlock=NULL;
	int                    len  = 0;

	ParserControlBlock = PESESHead_p;
	while(ParserControlBlock != NULL)
	{
		if(!strcmp(ParserControlBlock->Name,"PESES1"))
			break;
		ParserControlBlock = ParserControlBlock->Next_p;
	}
	if(ParserControlBlock)
	{
		len += PrintParserInfo(ParserControlBlock,buf);
	}
	else
	{
		len += sprintf(buf+len,"Component not in chain  = %s\n", "PESES1");
	}

	*eof = 1;
	return len;
}

int staudlx_core_SpdifP0State
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
 	SPDIFPlayerControl_t		spdifPlayerControl;
	SPDIFPlayerControlBlock_t	*PlayerControlBlock;
	MemoryPoolControl_t 		*TempBlkMgr_p;
	int                    len  = 0,j=0;

//	spdifPlayerControl = spdifPlayerControlBlock->spdifPlayerControl;
	PlayerControlBlock = spdifPlayerControlBlock;
	TempBlkMgr_p = MemPoolHead_p;

	while(PlayerControlBlock != NULL)
	{
		if(!strcmp(PlayerControlBlock->spdifPlayerControl.Name,"SPDIFPLAYER"))
			break;
		PlayerControlBlock = PlayerControlBlock->next;
	}
	if(PlayerControlBlock)
	{
		spdifPlayerControl = PlayerControlBlock->spdifPlayerControl;
		len += sprintf(buf+len,"Name    = %s\n", spdifPlayerControl.Name);
		len += sprintf(buf+len,"Player ID   = %d\n", spdifPlayerControl.spdifPlayerIdentifier);
		len += sprintf(buf+len,"Cur Statel    = %d\n", spdifPlayerControl.spdifPlayerCurrentState);
		len += sprintf(buf+len,"Next State    = %d\n", spdifPlayerControl.spdifPlayerNextState);
		len += sprintf(buf+len,"spdifPlayerNumNode    = %d\n", spdifPlayerControl.spdifPlayerNumNode);
		len += sprintf(buf+len,"spdifPlayerPlayed    = %d\n", spdifPlayerControl.spdifPlayerPlayed);
		len += sprintf(buf+len,"spdifPlayerToProgram   = %d\n", spdifPlayerControl.spdifPlayerToProgram);
		len += sprintf(buf+len,"Abort Sent   = %d\n", spdifPlayerControl.FDMAAbortSent);
		len += sprintf(buf+len,"Abort callback Received   = %d\n", spdifPlayerControl.FDMAAbortCBRecvd);
//		len += sprintf(buf+len,"Current Block Count  = %d\n", spdifPlayerControl.CurrentNoBlock);
		len += sprintf(buf+len,"Spdif_Pause_Performed   = %d\n", spdifPlayerControl.Spdif_Pause_Performed);
		len += sprintf(buf+len,"Spdif_Skip_Performed   = %d\n", spdifPlayerControl.Spdif_Skip_Performed);
		len += sprintf(buf+len,"Spdif_Huge_Diff   = %d\n", spdifPlayerControl.Spdif_Huge_Diff);
#if LINUX_SPDIF_INTERRUPT
		len += sprintf(buf+len,"Spdif_underflow_count   = %d\n", Spdif_underflow_count);
		len += sprintf(buf+len,"Spdif_InterruptValue   = %d\n", Spdif_InterruptValue);
		len += sprintf(buf+len,"Spdif_InterruptEnable   = %d\n", Spdif_InterruptEnable);
#endif
		while(TempBlkMgr_p != NULL)
		{
			if(TempBlkMgr_p == (MemoryPoolControl_t*)spdifPlayerControl.memoryBlockManagerHandle)
				break;
			TempBlkMgr_p = TempBlkMgr_p->Next_p;
		}
		if(TempBlkMgr_p)
		{
			len += sprintf(buf+len,"Block Manager Name   = %s\n", TempBlkMgr_p->PoolName);
			for(j=1;j<TempBlkMgr_p->LoggedEntries;j++)
			{
				if(TempBlkMgr_p->Logged[j].Handle == (U32)PlayerControlBlock)
					break;
			}
			if( j < TempBlkMgr_p->LoggedEntries)
			{
				len += sprintf(buf+len,"BMgr Next Free   = %d\n", TempBlkMgr_p->NextFree);
				len += sprintf(buf+len,"BMgr Next Filled  = %d\n", TempBlkMgr_p->NextFilled);
				len += sprintf(buf+len,"BMgr Next  Sent  = %d, j =%d \n", TempBlkMgr_p->NextSent[j],j);
				len += sprintf(buf+len,"BMgr Next  Release = %d, j = %d \n", TempBlkMgr_p->NextRelease[j], j);
				len += sprintf(buf+len,"BMgr Request   = %d\n", TempBlkMgr_p->Request[j]);
			}
		}
	}
	else
	{
		len += sprintf(buf+len,"Component not in chain  = %s\n", "SPDIF");
	}
	*eof = 1;
	return len;
}

int PrintPlayerInfo(PCMPlayerControlBlock_t *PlayerControlBlock, MemoryPoolControl_t  *TempBlkMgr_p, char * buf)
{
	PCMPlayerControl_t	*PcmPlayerControl;
	int len = 0,j=0;

	PcmPlayerControl = &PlayerControlBlock->pcmPlayerControl;

	len += sprintf(buf+len,"Name    = %s\n", PcmPlayerControl->Name);
	len += sprintf(buf+len,"Player ID   = %d\n", PcmPlayerControl->pcmPlayerIdentifier);
	len += sprintf(buf+len,"Cur Statel    = %d\n", PcmPlayerControl->pcmPlayerCurrentState);
	len += sprintf(buf+len,"Next State    = %d\n", PcmPlayerControl->pcmPlayerNextState);
	len += sprintf(buf+len,"pcmPlayerNumNode    = %d\n", PcmPlayerControl->pcmPlayerNumNode);
	len += sprintf(buf+len,"pcmPlayerPlayed    = %d\n", PcmPlayerControl->pcmPlayerPlayed);
	len += sprintf(buf+len,"pcmPlayerToProgram   = %d\n", PcmPlayerControl->pcmPlayerToProgram);
	len += sprintf(buf+len,"Bad Count   = %d\n", PcmPlayerControl->audioSTCSync.BadClockCount);
	len += sprintf(buf+len,"Abort Sent   = %d\n", PcmPlayerControl->FDMAAbortSent);
	len += sprintf(buf+len,"Abort callback Received   = %d\n", PcmPlayerControl->FDMAAbortCBRecvd);
//		len += sprintf(buf+len,"Current Block Count   = %d\n", PcmPlayerControl->CurrentNoBlock);
	len += sprintf(buf+len,"Pause_Performed = %d\n", PcmPlayerControl->Pause_Performed);
	len += sprintf(buf+len,"Skip_Performed = %d\n", PcmPlayerControl->Skip_Performed);
	len += sprintf(buf+len,"Huge Diff Count   = %d\n", PcmPlayerControl->Huge_Diff);
#if ENABLE_PCM_INTERRUPT
	len += sprintf(buf+len,"underflow_count   = %d\n", PcmPlayerControl->Underflow_Count);
#endif
	while(TempBlkMgr_p != NULL)
	{
		if(TempBlkMgr_p == (MemoryPoolControl_t*)PcmPlayerControl->memoryBlockManagerHandle)
			break;
		TempBlkMgr_p = TempBlkMgr_p->Next_p;
	}
	if(TempBlkMgr_p)
	{
		len += sprintf(buf+len,"Block Manager Name   = %s\n", TempBlkMgr_p->PoolName);
		for(j=1;j<TempBlkMgr_p->LoggedEntries;j++)
		{
			if(TempBlkMgr_p->Logged[j].Handle == (U32)PlayerControlBlock)
				break;
		}
		if( j < TempBlkMgr_p->LoggedEntries)
		{
			len += sprintf(buf+len,"BMgr Next Free   = %d\n", TempBlkMgr_p->NextFree);
			len += sprintf(buf+len,"BMgr Next Filled  = %d\n", TempBlkMgr_p->NextFilled);
			len += sprintf(buf+len,"BMgr Next  Sent  = %d, j =%d \n", TempBlkMgr_p->NextSent[j],j);
			len += sprintf(buf+len,"BMgr Next  Release = %d, j = %d \n", TempBlkMgr_p->NextRelease[j], j);
			len += sprintf(buf+len,"BMgr Request   = %d\n", TempBlkMgr_p->Request[j]);
		}
	}

	return len;

}
int staudlx_core_PCMP1State
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
	PCMPlayerControlBlock_t	*PlayerControlBlock;
	MemoryPoolControl_t 		*TempBlkMgr_p;
	int                    len  = 0;

	PlayerControlBlock = pcmPlayerControlBlock;
	TempBlkMgr_p = MemPoolHead_p;
	while(PlayerControlBlock != NULL)
	{
		if(!strcmp(PlayerControlBlock->pcmPlayerControl.Name,"PCMPLAYER1"))
			break;
		PlayerControlBlock = PlayerControlBlock->next;
	}

	if(PlayerControlBlock)
	{
		len = PrintPlayerInfo(PlayerControlBlock,TempBlkMgr_p,buf);
	}
	else
	{
		len += sprintf(buf+len,"Component not in chain  = %s\n", "PCMPLAYER1");
	}

	*eof = 1;
	return len;
}
int staudlx_core_PCMP0State
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
	PCMPlayerControlBlock_t	*PlayerControlBlock;
	MemoryPoolControl_t 		*TempBlkMgr_p;
	int                    len  = 0;

	PlayerControlBlock = pcmPlayerControlBlock;
	TempBlkMgr_p = MemPoolHead_p;

	while(PlayerControlBlock != NULL)
	{
		if(!strcmp(PlayerControlBlock->pcmPlayerControl.Name,"PCMPLAYER0"))
			break;
		PlayerControlBlock = PlayerControlBlock->next;
	}

	if(PlayerControlBlock)
	{
		len = PrintPlayerInfo(PlayerControlBlock,TempBlkMgr_p,buf);
	}
	else
	{
		len += sprintf(buf+len,"Component not in chain  = %s\n", "PCMPLAYER0");
	}
	*eof = 1;
	return len;
}

int PrintDecoderInfo(DecoderControlBlock_t	*DecoderControlBlock,char * buf)
{
	int len =0;

	len += sprintf(buf+len,"Device Name    = %s\n", DecoderControlBlock->DeviceName);
	len += sprintf(buf+len,"Cur State    = %d\n", DecoderControlBlock->CurDecoderState);
	len += sprintf(buf+len,"Next State    = %d\n", DecoderControlBlock->NextDecoderState);
	len += sprintf(buf+len,"DecInState    = %d\n", DecoderControlBlock->DecInState);
	len += sprintf(buf+len,"DecoderInputQueueTail    = %d\n", DecoderControlBlock->DecoderInputQueueTail);
	len += sprintf(buf+len,"DecoderInputQueueFront    = %d\n", DecoderControlBlock->DecoderInputQueueFront);
	len += sprintf(buf+len,"DecoderOutputQueueTail    = %d\n", DecoderControlBlock->DecoderOutputQueueTail);
	len += sprintf(buf+len,"DecoderOutputQueueFront    = %d\n", DecoderControlBlock->DecoderOutputQueueFront);
	len += sprintf(buf+len,"Mute    = %d\n", DecoderControlBlock->PcmProcess.Mute);
	return len;
}
int staudlx_core_Dec0State
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
	DecoderControlBlockList_t	*DecoderQueueHead;
	int                    len  = 0;

	DecoderQueueHead = DecoderQueueHead_p;

	while(DecoderQueueHead != NULL)
	{
		if(!strcmp(DecoderQueueHead->DecoderControlBlock.DeviceName,"DEC0"))
			break;
		DecoderQueueHead = DecoderQueueHead->Next_p;
	}
	if(DecoderQueueHead)
	{
		len += PrintDecoderInfo(&DecoderQueueHead->DecoderControlBlock, buf);
	}
	else
	{
		len += sprintf(buf+len,"Component not in chain  = %s\n", "DEC0");
	}

	*eof = 1;
	return len;
}

int staudlx_core_Dec1State
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
	DecoderControlBlockList_t	*DecoderQueueHead;
	int                    len  = 0;

	DecoderQueueHead = DecoderQueueHead_p;

	while(DecoderQueueHead != NULL)
	{
		if(!strcmp(DecoderQueueHead->DecoderControlBlock.DeviceName,"DEC1"))
			break;
		DecoderQueueHead = DecoderQueueHead->Next_p;
	}
	if(DecoderQueueHead)
	{
		len += PrintDecoderInfo(&DecoderQueueHead->DecoderControlBlock, buf);
	}
	else
	{
		len += sprintf(buf+len,"Component not in chain  = %s\n", "DEC1");
	}

	*eof = 1;
	return len;
}

int staudlx_core_PP0State
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
	int                    len  = 0;
	PPControlBlockList_t	*PPQueueHead;

	PPQueueHead= PPQueueHead_p;

	while(PPQueueHead != NULL)
	{
		if(!strcmp(PPQueueHead->PPControlBlock.DeviceName,"PP0"))
    		break;
		PPQueueHead=PPQueueHead->Next_p;
	}


	if(PPQueueHead)
	{
		len += sprintf(buf+len,"Device Name    = %s\n", PPQueueHead->PPControlBlock.DeviceName);
		len += sprintf(buf+len,"PP State    = %d\n", PPQueueHead->PPControlBlock.CurPPState);
		len += sprintf(buf+len,"PPInState    = %d\n", PPQueueHead->PPControlBlock.PPInState);
		len += sprintf(buf+len,"GetDataState    = %d\n", PPQueueHead->PPControlBlock.GetDataState);
		len += sprintf(buf+len,"PPQueueTail    = %d\n", PPQueueHead->PPControlBlock.PPQueueTail);
		len += sprintf(buf+len,"PPQueueFront    = %d\n", PPQueueHead->PPControlBlock.PPQueueFront);

		if(PPQueueHead->PPControlBlock.CurrentBufferToSend != NULL)
		{
			len += sprintf(buf+len,"**** Cur PP Buf To Send \n");
			len += sprintf(buf+len,"PP Input Start Address    = %x\n", (U32)PPQueueHead->PPControlBlock.CurrentBufferToSend->PPBufIn.BufferIn_p[0]->ScatterPages_p->Page_p);
			len += sprintf(buf+len,"PP Input Size     = %d\n", PPQueueHead->PPControlBlock.CurrentBufferToSend->PPBufIn.BufferIn_p[0]->ScatterPages_p->Size);
			len += sprintf(buf+len,"PP Output Start Address    = %x\n", (U32)PPQueueHead->PPControlBlock.CurrentBufferToSend->PPBufOut.BufferOut_p[0]->ScatterPages_p->Page_p);
			len += sprintf(buf+len,"PP Output Size     = %d\n", PPQueueHead->PPControlBlock.CurrentBufferToSend->PPBufOut.BufferOut_p[0]->ScatterPages_p->Size);
			len += sprintf(buf+len,"PP Input Num of scatter pages    = %d\n", PPQueueHead->PPControlBlock.CurrentBufferToSend->BufferPtrs[0]->NumberOfScatterPages);
		}
	}
	else
	{
		len += sprintf(buf+len,"Component not in chain  = %s\n", "PP0");
	}

	*eof = 1;
	return len;
}


int staudlx_core_DataProcState
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
	int                    len  = 0;
	DataProcesserControl_t	 * dataProcesserControl		= &(dataProcesserControlBlock->dataProcesserControl);

	len += sprintf(buf+len,"Device Name    = %d\n", dataProcesserControl->dataProcesserIdentifier);
	len += sprintf(buf+len,"dataProcesserCurrentState    = %d\n", dataProcesserControl->dataProcesserCurrentState);
	len += sprintf(buf+len,"dataProcesserNextState    = %d\n", dataProcesserControl->dataProcesserNextState);

	*eof = 1;
	return len;
}
#ifdef ENABLE_LX_DEBUGGING
int staudlx_core_LxState
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
 		int                    len  = 0;
		if(lx_debug_ptr){
			*(lx_debug_ptr+ARRAY_SIZE)='\0';
			len += sprintf(buf+len,"%s\n", lx_debug_ptr);
		}
		else {
			len += sprintf(buf+len,"Debug Buffer Pointer is NULL : No debug data available! \n");
		}

	*eof = 1;
	return len;
}
#endif

int staudlx_core_DriverState
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
	int                    len  = 0;
	int Dec_Present=FALSE, Parser_Present=FALSE;
	DecoderControlBlockList_t	*DecoderQueueHead;
	PESES_ParserControl_t	*ParserControlBlock=NULL;
	U32 fmduration;

	/* Decoder Related Details */
	DecoderQueueHead = DecoderQueueHead_p;
	while(DecoderQueueHead != NULL)
	{
		if(!strncmp(DecoderQueueHead->DecoderControlBlock.DeviceName,"DEC",3))
		{
			/* Decoder Name */
			len += sprintf(buf+len,"\nComponent = %s\n", DecoderQueueHead->DecoderControlBlock.DeviceName);
			/* Sampling Frequency */
			len += sprintf(buf+len,"Sampling Frequency    = %d\n",
				DecoderQueueHead->DecoderControlBlock.StreamParams.SamplingFrequency);
			/* Frames Decoded */
			len += sprintf(buf+len,"Frames Decoded    = %d\n",
				DecoderQueueHead->DecoderControlBlock.DecoderOutputQueueFront);
			/* LISTENING MODE*/
			len += sprintf(buf+len,"No. Of Channels = %d\n",
				DecoderQueueHead->DecoderControlBlock.NumChannels);
			/*FRAME DURATION */
			fmduration=(1000*(DecoderQueueHead->DecoderControlBlock.DefaultDecodedSize))/(DecoderQueueHead->DecoderControlBlock.StreamParams.SamplingFrequency);
			fmduration=fmduration/(4*(DecoderQueueHead->DecoderControlBlock.NumChannels));
			len += sprintf(buf+len,"Frame Duration = %d ms\n",fmduration);
            Dec_Present=TRUE;
		}
		DecoderQueueHead = DecoderQueueHead->Next_p;
	}
    if(!Dec_Present)
	{
		len += sprintf(buf+len,"Component not in chain (Data unavailable) = %s\n", "Decoder");
	}


	/* Parser Related Details */
	ParserControlBlock = PESESHead_p;
	while(ParserControlBlock != NULL)
	{
		if(!strncmp(ParserControlBlock->Name,"PESES",5))
		{
			/* Parser Name */
			Parser_Present=TRUE;
			len += sprintf(buf+len,"\nComponent = %s\n", ParserControlBlock->Name);

			/*STREAM BIT RATE */
			//len += sprintf(buf+len,"Stream Bit Rate    = %d \n", ParserControlBlock->StreamInfo.BitRate);

			/* STREAM CONTENT*/
			len += sprintf(buf+len,"Stream Content   = %d\n", ParserControlBlock->StreamInfo.StreamContent);
			/* STREAM TYPE*/
			switch (ParserControlBlock->StreamType)
			{
				case STAUD_STREAM_TYPE_ES :
					len += sprintf(buf+len,"Stream Type = ES \n");
					break;
        		case STAUD_STREAM_TYPE_PES:
					len += sprintf(buf+len,"Stream Type = PES \n");
					break;
		 		case STAUD_STREAM_TYPE_PES_AD:
					len += sprintf(buf+len,"Stream Type = PES_AD \n");
					break;
         		case STAUD_STREAM_TYPE_MPEG1_PACKET:
					len += sprintf(buf+len,"Stream Type = MPEG1 \n");
			 		break;
         		case STAUD_STREAM_TYPE_PES_DVD:
					len += sprintf(buf+len,"Stream Type = PES_DVD \n");
			 		break;
         		case STAUD_STREAM_TYPE_PES_DVD_AUDIO:
					len += sprintf(buf+len,"Stream Type = PES_DVD_AUDIO \n");
			 		break;
         		case STAUD_STREAM_TYPE_SPDIF:
					len += sprintf(buf+len,"Stream Type = SPDIF \n");
			 		break;
        		case STAUD_STREAM_TYPE_PES_ST:
					len += sprintf(buf+len,"Stream Type = PES_ST \n");
					break;
				default:
					len += sprintf(buf+len,"Stream Type = Unknown \n");
					break;
			}

			/* Free PES buffer Size */
			if(((ParserControlBlock->CurrentPTIWritePointer_p)-(ParserControlBlock->PTIReadPointer_p))>= 0)
			{
				len += sprintf(buf+len,"Free PES Buffer Size  = %d\n",
					(ParserControlBlock->BitBufferSize)-((ParserControlBlock->CurrentPTIWritePointer_p)-(ParserControlBlock->PTIReadPointer_p)));
			}
			else
			{
				/*read below write so write has wrapped around*/
				len += sprintf(buf+len,"Free PES Buffer Size  = %d\n",
					(ParserControlBlock->PTIReadPointer_p) - (ParserControlBlock->CurrentPTIWritePointer_p));
			}
		}
		ParserControlBlock = ParserControlBlock->Next_p;
	}
    if(!Parser_Present)
	{
		len += sprintf(buf+len,"Component not in chain (Data unavailable) = %s\n", "Parser");
	}



	/*PES BUFFER SIZE */
	len += sprintf(buf+len,"PES Buffer Size (bytes) = %d\n", PES_BUFFER_SIZE);


	*eof = 1;
	return len;
}

int staudlx_core_pcm0_registers
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
    volatile unsigned int* pReg = (unsigned int*)0xB8101000;
    int                    len  = 0;

    len += sprintf(buf+len,"Reset            = 0x%08X\n", pReg[0]);
    len += sprintf(buf+len,"Fifo             = 0x%08X\n", pReg[1]);
    len += sprintf(buf+len,"Interrupt status = 0x%08X\n", pReg[2]);
    len += sprintf(buf+len,"Interrupt enable = 0x%08X\n", pReg[4]);
    len += sprintf(buf+len,"Ctrl             = 0x%08X\n", pReg[7]);
    len += sprintf(buf+len,"Status           = 0x%08X\n", pReg[8]);
    len += sprintf(buf+len,"Format           = 0x%08X\n", pReg[9]);

    *eof = 1;
    return len;
}

/*****************************************************************************/

int staudlx_core_pcm1_registers
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
    volatile unsigned int* pReg = (unsigned int*)0xB8101800;
    int                    len  = 0;

    len += sprintf(buf+len,"Reset            = 0x%08X\n", pReg[0]);
    len += sprintf(buf+len,"Fifo             = 0x%08X\n", pReg[1]);
    len += sprintf(buf+len,"Interrupt status = 0x%08X\n", pReg[2]);
    len += sprintf(buf+len,"Interrupt enable = 0x%08X\n", pReg[4]);
    len += sprintf(buf+len,"Ctrl             = 0x%08X\n", pReg[7]);
    len += sprintf(buf+len,"Status           = 0x%08X\n", pReg[8]);
    len += sprintf(buf+len,"Format           = 0x%08X\n", pReg[9]);

    *eof = 1;
    return len;
}

/*****************************************************************************/

int staudlx_core_spdif_registers
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
    volatile unsigned int* pReg = (unsigned int*)0xB8103000;
    int                    len  = 0;

    len += sprintf(buf+len,"Reset               = 0x%08X\n", pReg[0]);
    len += sprintf(buf+len,"Fifo                = 0x%08X\n", pReg[1]);
    len += sprintf(buf+len,"Interrupt enable    = 0x%08X\n", pReg[4]);
    len += sprintf(buf+len,"Ctrl                = 0x%08X\n", pReg[7]);
    len += sprintf(buf+len,"Status              = 0x%08X\n", pReg[8]);
    len += sprintf(buf+len,"PA_PB               = 0x%08X\n", pReg[9]);
    len += sprintf(buf+len,"PC_PD               = 0x%08X\n", pReg[10]);
    len += sprintf(buf+len,"Channel status L    = 0x%08X\n", pReg[11]);
    len += sprintf(buf+len,"Channel status R    = 0x%08X\n", pReg[12]);
    len += sprintf(buf+len,"Channel status UV   = 0x%08X\n", pReg[13]);
    len += sprintf(buf+len,"Pause/Latency       = 0x%08X\n", pReg[14]);
    len += sprintf(buf+len,"Burst length        = 0x%08X\n", pReg[15]);

    *eof = 1;
    return len;
}

/*****************************************************************************/
/* ~RY Debug only */
unsigned char* PESBufferData;
unsigned int PESBufferSize;
int staudlx_core_pes_buffer
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
    int len = 0;

    if (NULL != PESBufferData)
    {
        if (PESBufferSize)
        {
            if (count > PESBufferSize)
                count = PESBufferSize;
            memcpy(buf, PESBufferData, count);
            len = count;
            PESBufferSize -= count;

            *start = buf;
            *eof = 0;
        }
        else
        {
            *eof = 1;
        }
    }
    else
    {
        len += sprintf(buf+len,"No destination data.\n");
        *eof = 1;
    }
    return len;
}

#if 0
/*****************************************************************************/
int staudlx_core_aud_sync_dump
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
    int len  = 0;
    int i;

    if((countdump + 10) >= 500)
    {
        *eof = 1;
    return len;
    }

    for(i = countdump; i < (countdump+10); i++)
    {
#if USE_TRACE_BUFFER_FOR_DEBUG
        len += sprintf(buf+len,"******************************\n");
        len += sprintf(buf+len,"NewPCR     = %d\n", STC_array_aud [0][i]);
        len += sprintf(buf+len,"CurrentPTS = %d\n", STC_array_aud [1][i]);
        len += sprintf(buf+len,"%s = %d\n", (STC_array_aud [3][i]?"PauseFrame":"SkiFrame"),STC_array_aud [2][i] );
        len += sprintf(buf+len,"Dummy = %d\n", STC_array_aud [4][i]);
        len += sprintf(buf+len,"******************************");
#endif
    }

    countdump += 10;

    *eof = 1;
    return len;
}

int staudlx_core_aud_sync_dump
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
    int len  = 0;

#if LOG_AVSYNC_LINUX1
    int i;
    for(i = 0; i < NUM_OF_ELEMENTS ; i++)
    {
        len += sprintf(buf+len,"%u %15u \n", Aud_Array[i][0],Aud_Array [i][1]);
    }

    Log_To_Array = 1;
#endif
    *eof = 1;
    return len;
}


/*****************************************************************************/
int staudlx_core_aud_pcr_dump
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
    int len  = 0;
    int i;

    if((countPcrDump+ 10) >= 500)
    {
        *eof = 1;
    return len;
    }

    for(i = countPcrDump; i < (countPcrDump+10); i++)
    {
        len += sprintf(buf+len,"******************************\n");
        len += sprintf(buf+len,"PCRBaseBit32     = %d\n", PCR_array_aud [0][i]);
        len += sprintf(buf+len,"PCRBaseValue = %d\n", PCR_array_aud [1][i]);
        len += sprintf(buf+len,"PCRExtension= %d\n", PCR_array_aud [2][i]);
        len += sprintf(buf+len,"STCBaseBit32 = %d\n", PCR_array_aud [3][i]);
        len += sprintf(buf+len,"STCBaseValue = %d\n", PCR_array_aud [4][i]);
        len += sprintf(buf+len,"STCExtension = %d\n", PCR_array_aud [5][i]);
        len += sprintf(buf+len,"******************************\n");
    }

    countPcrDump += 10;

    *eof = 1;
    return len;
}
#endif

#if 0
/*****************************************************************************/
extern U32  StptiArrivalTime[6][1000];
extern U32 StptiArrivalTimeCount;
static U32 StptiArrivalTimeCountLocal = 0;
int staudlx_core_aud_clkrv_dump
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
    int len  = 0;
    int i;

    if((StptiArrivalTimeCountLocal + 10) >= 1000 )
    {
        *eof = 1;
    return len;
    }


    for(i = StptiArrivalTimeCountLocal; i < (StptiArrivalTimeCountLocal+10); i++)
    {
       len += sprintf(buf+len,"******************************\n");
        len += sprintf(buf+len,"ArrivalTime = %d\n", StptiArrivalTime[0] [i]);
        len += sprintf(buf+len,"ActvRefPCRTime = %d\n", StptiArrivalTime[1] [i]);
        len += sprintf(buf+len,"STC = %d\n", StptiArrivalTime[2] [i]);
        len += sprintf(buf+len,"ElapsedTime = %d\n", StptiArrivalTime[3] [i]);
        len += sprintf(buf+len,"STCOffset = %d\n", StptiArrivalTime[4] [i]);
        len += sprintf(buf+len,"ActvRefPCRBaseValue = %d\n", StptiArrivalTime[5] [i]);
       len += sprintf(buf+len,"******************************\n");
    }


    StptiArrivalTimeCountLocal += 10;

    *eof = 1;
    return len;
}
#endif

/*****************************************************************************/
extern U32 PTS_STC_array[2][1000];
extern U32 PTS_STC_arrayCount;
static U32 PTS_STC_arrayCountLocal = 0;
int staudlx_core_aud_ptsstc_dump
(
    char*   buf,
    char**  start,
    off_t   offset,
    int     count,
    int*    eof,
    void*   data
)
{
    int len  = 0;
    int i;

    if((PTS_STC_arrayCountLocal + 10) >= 1000)
    {
        *eof = 1;
    return len;
    }

    for(i = PTS_STC_arrayCountLocal; i < (PTS_STC_arrayCountLocal+10); i++)
    {
#if USE_TRACE_BUFFER_FOR_DEBUG
        len += sprintf(buf+len,"******************************\n");
        len += sprintf(buf+len,"STC     = %d\n", STC_array_aud [0][i]);
        len += sprintf(buf+len,"PTS = %d\n", STC_array_aud [1][i]);
        len += sprintf(buf+len,"Diff = %d\n", (STC_array_aud [0][i] - STC_array_aud [1][i]));
        len += sprintf(buf+len,"ms = %d\n", ((STC_array_aud [0][i] - STC_array_aud [1][i])/90));
        len += sprintf(buf+len,"******************************\n");
#endif
    }

    PTS_STC_arrayCountLocal += 10;

    *eof = 1;
    return len;
}


/*****************************************************************************/

int staudlx_core_init_proc_fs(void)
{
	staudlx_core_proc_root_dir = proc_mkdir("staudlx_core", NULL);
	if (staudlx_core_proc_root_dir == NULL)
	{
	    goto fault_no_dir;
	}

	create_proc_read_entry("Dec0State",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_Dec0State,
        NULL);

	create_proc_read_entry("Dec1State",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_Dec1State,
        NULL);

	 create_proc_read_entry("PcmP0State",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_PCMP0State,
        NULL);

       create_proc_read_entry("PcmP1State",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_PCMP1State,
        NULL);

	create_proc_read_entry("SpdifP0State",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_SpdifP0State,
        NULL);

	create_proc_read_entry("Parser0State",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_Parser0State,
        NULL);

	create_proc_read_entry("Parser1State",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_Parser1State,
        NULL);
	 create_proc_read_entry("PP0State",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_PP0State,
        NULL);
	 create_proc_read_entry("DriverState",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_DriverState,
        NULL);
#ifdef ENABLE_LX_DEBUGGING
	create_proc_read_entry("LxState",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_LxState,
        NULL);
#endif
#if 0
	create_proc_read_entry("DataProcState",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_DataProcState,
        NULL);

	create_proc_read_entry("PCM0registers",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_pcm0_registers,
        NULL);

	create_proc_read_entry("PCM1registers",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_pcm1_registers,
        NULL);

	create_proc_read_entry("SPDIFregisters",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_spdif_registers,
        NULL);

	create_proc_read_entry("AudPESBuffer",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_pes_buffer,
        NULL);

	create_proc_read_entry("AudSyncDump",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_aud_sync_dump,
        NULL);


    create_proc_read_entry("AudPCRDump",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_aud_pcr_dump,
        NULL);
    create_proc_read_entry("AudClkRvDump",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_aud_clkrv_dump,
        NULL);

    create_proc_read_entry("PTSSTCDump",
        0444,
        staudlx_core_proc_root_dir,
        staudlx_core_aud_ptsstc_dump,
        NULL);
#endif

    return 0;  /* If we get here then we have succeeded */

    /*** Clean up on error ***/

fault_no_dir:
    return (-1);
}

/*****************************************************************************/

int staudlx_core_cleanup_proc_fs(void)
{
	int err = 0;

//	remove_proc_entry("SPDIFregisters",staudlx_core_proc_root_dir);
//	remove_proc_entry("PCM1registers",staudlx_core_proc_root_dir);
//	remove_proc_entry("PCM0registers",staudlx_core_proc_root_dir);
	remove_proc_entry("Dec0State",staudlx_core_proc_root_dir);
	remove_proc_entry("Dec1State",staudlx_core_proc_root_dir);
	remove_proc_entry("PcmP0State",staudlx_core_proc_root_dir);
	remove_proc_entry("PcmP1State",staudlx_core_proc_root_dir);
	remove_proc_entry("SpdifP0State",staudlx_core_proc_root_dir);
	remove_proc_entry("Parser0State",staudlx_core_proc_root_dir);
	remove_proc_entry("Parser1State",staudlx_core_proc_root_dir);
	remove_proc_entry("PP0State",staudlx_core_proc_root_dir);
#ifdef ENABLE_LX_DEBUGGING
	remove_proc_entry("LxState",staudlx_core_proc_root_dir);
#endif
//	remove_proc_entry("DataProcState",staudlx_core_proc_root_dir);
//	remove_proc_entry("PTSSTCDump",staudlx_core_proc_root_dir);
	remove_proc_entry("staudlx_core",NULL);

	return (err);
}
