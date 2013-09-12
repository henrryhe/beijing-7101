/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : ES_AC3_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_AC3
#include "stos.h"
#include "es_ac3_parser_amphion.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif
#include "aud_evt.h"

/* ----------------------------------------------------------------------------
                               Private Types
---------------------------------------------------------------------------- */



static U32 AC3FrameSizes[2][20] =
	{
		{
	  		 64 * 2,   80 * 2,   96 * 2,  112 * 2,
	 		128 * 2,  160 * 2,  192 * 2,  224 * 2,
	 		256 * 2,  320 * 2,  384 * 2,  448 * 2,
	 		512 * 2,  640 * 2,  768 * 2,  896 * 2,
			1024* 2, 1152 * 2, 1280 * 2,        0
		},
		{
		 	 69 * 2,  87 * 2, 104 * 2, 121 * 2,
	  		139 * 2, 174 * 2, 208 * 2, 243 * 2,
	  		278 * 2, 348 * 2, 417 * 2, 487 * 2,
	  		557 * 2, 696 * 2, 835 * 2, 975 * 2,
	 		1114* 2,1253 * 2, 1393* 2,       0
	 	}
	};






static U32 AC3Frequency[] =
	{
		48000,	44100,	32000,	0
	};

/* Used for testing Purpose */


/* ----------------------------------------------------------------------------
		488 				External Data Types
--------------------------------------------------------------------------- */


/*---------------------------------------------------------------------
		835 			Private Functions
----------------------------------------------------------------------*/


/************************************************************************************
Name         : ES_AC3_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
	       ES_AC3_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_AC3_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle )
{
//	U32 i;
	ST_ErrorCode_t Error = ST_NO_ERROR;
	ES_AC3_ParserLocalParams_t *AC3ParserLocalParams;
	AC3ParserLocalParams=memory_allocate(Init_p->Memory_Partition,sizeof(ES_AC3_ParserLocalParams_t));
	if(AC3ParserLocalParams==NULL)
	{
		return ST_ERROR_NO_MEMORY;
	}
	memset(AC3ParserLocalParams,0,sizeof(ES_AC3_ParserLocalParams_t));
//	AC3ParserLocalParams = (ES_AC3_ParserLocalParams_t *)malloc (sizeof(ES_AC3_ParserLocalParams_t));
	/* Init the Local Parameters */

	AC3ParserLocalParams->AC3ParserState = ES_AC3_SYNC_0;


	AC3ParserLocalParams->FrameComplete = TRUE;
	AC3ParserLocalParams->First_Frame   = TRUE;
	AC3ParserLocalParams->FrameSize = 0;

	AC3ParserLocalParams->Frequency = 0xFFFFFFFF;
	AC3ParserLocalParams->BitRate = 0;
	AC3ParserLocalParams->Get_Block = FALSE;
	AC3ParserLocalParams->Put_Block = TRUE;
	AC3ParserLocalParams->Skip			= 0;
	AC3ParserLocalParams->Pause		= 0;
	AC3ParserLocalParams->Reuse		= FALSE;
	AC3ParserLocalParams->OpBufHandle = Init_p->OpBufHandle;
	AC3ParserLocalParams->EvtHandle 					= Init_p->EvtHandle;
	AC3ParserLocalParams->ObjectID					= Init_p->ObjectID;
	AC3ParserLocalParams->EventIDNewFrame 			= Init_p->EventIDNewFrame;
	AC3ParserLocalParams->EventIDFrequencyChange = Init_p->EventIDFrequencyChange;
	AC3ParserLocalParams->EventIDNewRouting		= Init_p->EventIDNewRouting;

	AC3ParserLocalParams->StartFramePTSValid = FALSE;

	AC3ParserLocalParams->Bit_Rate_Code 	= 0xFF;
	AC3ParserLocalParams->Bit_Stream_Mode  = 0xFF;
	AC3ParserLocalParams->Audio_Coding_Mode = 0xFF;
	*handle=(ParserHandle_t )AC3ParserLocalParams;

	/* to init O/P pointers */
	AC3ParserLocalParams->Blk_Out = 0;


	return (Error);
}

ST_ErrorCode_t ES_AC3_ParserGetParsingFunction(	ParsingFunction_t * ParsingFunc_p,
												GetFreqFunction_t * GetFreqFunction_p,
												GetInfoFunction_t * GetInfoFunction_p)
{

	*ParsingFunc_p 		= (ParsingFunction_t)ES_AC3_Parse_Frame;
	*GetFreqFunction_p	= (GetFreqFunction_t)ES_AC3_Get_Freq;
	*GetInfoFunction_p	= (GetInfoFunction_t)ES_AC3_Get_Info;

	return ST_NO_ERROR;
}




/************************************************************************************
Name         : ES_AC3_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_AC3_Parser_Stop(ParserHandle_t const Handle)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;

	ES_AC3_ParserLocalParams_t *AC3ParserLocalParams;
	AC3ParserLocalParams=(ES_AC3_ParserLocalParams_t *)Handle;

	if(AC3ParserLocalParams->Get_Block == TRUE)
	{
		AC3ParserLocalParams->OpBlk_p->Size = AC3ParserLocalParams->FrameSize ;
		AC3ParserLocalParams->OpBlk_p->Flags  &= ~PTS_VALID;
		Error = MemPool_PutOpBlk(AC3ParserLocalParams->OpBufHandle,(U32 *)&AC3ParserLocalParams->OpBlk_p);
		if(Error != ST_NO_ERROR)
		{
			STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
		}
		else
		{
			AC3ParserLocalParams->Put_Block = TRUE;
			AC3ParserLocalParams->Get_Block = FALSE;
		}
	}



	AC3ParserLocalParams->AC3ParserState = ES_AC3_SYNC_0;


	AC3ParserLocalParams->FrameComplete = TRUE;
	AC3ParserLocalParams->First_Frame   = TRUE;

	AC3ParserLocalParams->Blk_Out = 0;
	return Error;
}

/************************************************************************************
Name         : ES_AC3_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_AC3_Parser_Start(ParserHandle_t const Handle)
{

	ST_ErrorCode_t Error = ST_NO_ERROR;

	ES_AC3_ParserLocalParams_t *AC3ParserLocalParams;
	AC3ParserLocalParams=(ES_AC3_ParserLocalParams_t *)Handle;

	AC3ParserLocalParams->AC3ParserState = ES_AC3_SYNC_0;
	AC3ParserLocalParams->FrameSize = 0;

	AC3ParserLocalParams->Frequency = 0xFFFFFFFF;
	AC3ParserLocalParams->BitRate = 0;
	AC3ParserLocalParams->FrameComplete = TRUE;
	AC3ParserLocalParams->First_Frame   = TRUE;

	AC3ParserLocalParams->Put_Block = TRUE;
	AC3ParserLocalParams->Get_Block = FALSE;
	AC3ParserLocalParams->Skip			= 0;
	AC3ParserLocalParams->Pause		= 0;
	AC3ParserLocalParams->Reuse		= FALSE;

	AC3ParserLocalParams->StartFramePTSValid = FALSE;

	AC3ParserLocalParams->Bit_Rate_Code 	= 0xFF;
	AC3ParserLocalParams->Bit_Stream_Mode  = 0xFF;
	AC3ParserLocalParams->Audio_Coding_Mode = 0xFF;

	AC3ParserLocalParams->Blk_Out = 0;
	return Error;
}

/* Get Sampling Frequency from Parser */

ST_ErrorCode_t ES_AC3_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

	ES_AC3_ParserLocalParams_t *AC3ParserLocalParams;
	AC3ParserLocalParams=(ES_AC3_ParserLocalParams_t *)Handle;

	*SamplingFrequency_p=AC3ParserLocalParams->Frequency;

	return ST_NO_ERROR;
}

/* Get GetStreamInfo from Parser */

ST_ErrorCode_t ES_AC3_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
	ES_AC3_ParserLocalParams_t *AC3ParserLocalParams;
	AC3ParserLocalParams=(ES_AC3_ParserLocalParams_t *)Handle;

	StreamInfo->SamplingFrequency	= AC3ParserLocalParams->Frequency;
	StreamInfo->Bit_Rate_Code		= AC3ParserLocalParams->Bit_Rate_Code ;
	StreamInfo->Bit_Stream_Mode		= AC3ParserLocalParams->Bit_Stream_Mode ;
	StreamInfo->Audio_Coding_Mode	= AC3ParserLocalParams->Audio_Coding_Mode ;
	StreamInfo->CopyRight = 0; /*Currently not supported*/
	return ST_NO_ERROR;
}

/* handle EOF from Parser */
ST_ErrorCode_t ES_AC3_Handle_EOF(ParserHandle_t  Handle)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
	ES_AC3_ParserLocalParams_t *AC3ParserLocalParams = (ES_AC3_ParserLocalParams_t *)Handle;

	if(AC3ParserLocalParams->Get_Block == TRUE)
	{
		memset(AC3ParserLocalParams->Current_Write_Ptr1,0x00,AC3ParserLocalParams->NoBytesRemainingInFrame);

		AC3ParserLocalParams->OpBlk_p->Size = AC3ParserLocalParams->FrameSize;
		AC3ParserLocalParams->OpBlk_p->Flags  |= EOF_VALID;
		AC3ParserLocalParams->OpBlk_p->Flags  &= ~PTS_VALID;
		AC3ParserLocalParams->OpBlk_p->Flags  |= EOF_VALID;
		Error = MemPool_PutOpBlk(AC3ParserLocalParams->OpBufHandle,(U32 *)&AC3ParserLocalParams->OpBlk_p);
		if(Error != ST_NO_ERROR)
		{
			STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
		}
		else
		{
			AC3ParserLocalParams->Put_Block = TRUE;
			AC3ParserLocalParams->Get_Block = FALSE;
		}
	}
	else
	{
		Error = MemPool_GetOpBlk(AC3ParserLocalParams->OpBufHandle, (U32 *)&AC3ParserLocalParams->OpBlk_p);
		if(Error != ST_NO_ERROR)
		{
			return STAUD_MEMORY_GET_ERROR;
		}
		else
		{
			AC3ParserLocalParams->Put_Block = FALSE;
			AC3ParserLocalParams->Get_Block = TRUE;
			AC3ParserLocalParams->OpBlk_p->Size = 1536;
			AC3ParserLocalParams->OpBlk_p->Flags  |= EOF_VALID;
			memset((void*)AC3ParserLocalParams->OpBlk_p->MemoryStart,0x00,AC3ParserLocalParams->OpBlk_p->MaxSize);
			Error = MemPool_PutOpBlk(AC3ParserLocalParams->OpBufHandle,(U32 *)&AC3ParserLocalParams->OpBlk_p);
			if(Error != ST_NO_ERROR)
			{
				STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
			}
			else
			{
				AC3ParserLocalParams->Put_Block = TRUE;
				AC3ParserLocalParams->Get_Block = FALSE;
			}
		}
	}

	return ST_NO_ERROR;
}

/************************************************************************************
Name         : Parse_AC3_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
				U32 Size  Size of Memory Block
				U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
	ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
	ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t ES_AC3_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used,PTS_t * CurrentPTS_p, Private_Info_t * Private_Info_p)
{

	U8 *pos;
	U32 offset;
	U8 value;
	ST_ErrorCode_t Error;
	ES_AC3_ParserLocalParams_t *AC3ParserLocalParams;
	STAUD_PTS_t	NotifyPTS;

	offset=*No_Bytes_Used;

	pos=(U8 *)MemoryStart;

//	STTBX_Print((" ES_AC3 Parser Enrty \n"));

	AC3ParserLocalParams=(ES_AC3_ParserLocalParams_t *)Handle;

	while(offset<Size)
	{
		if(AC3ParserLocalParams->FrameComplete == TRUE)
		{

				if(AC3ParserLocalParams->Put_Block == TRUE)
				{
					if(AC3ParserLocalParams->Reuse == FALSE)
					{
						Error = MemPool_GetOpBlk(AC3ParserLocalParams->OpBufHandle, (U32 *)&AC3ParserLocalParams->OpBlk_p);
						if(Error != ST_NO_ERROR)
						{
							*No_Bytes_Used = offset;
							return STAUD_MEMORY_GET_ERROR;
						}
						else
						{
							if(AC3ParserLocalParams->Pause)
							{
								AUD_TaskDelayMs(AC3ParserLocalParams->Pause);
								AC3ParserLocalParams->Pause = 0;
							}
						}
					}

					AC3ParserLocalParams->Put_Block = FALSE;
					AC3ParserLocalParams->Get_Block = TRUE;
					AC3ParserLocalParams->Current_Write_Ptr1 = (U8 *)AC3ParserLocalParams->OpBlk_p->MemoryStart;
		//			memset(AC3ParserLocalParams->Current_Write_Ptr1,0x00,AC3ParserLocalParams->OpBlk_p->MaxSize);
					if(Private_Info_p->Private_Data[2] == 1) // FADE PAN VALID FLAG
					{
						AC3ParserLocalParams->OpBlk_p->Flags   |= FADE_VALID;
						AC3ParserLocalParams->OpBlk_p->Flags   |= PAN_VALID;
						AC3ParserLocalParams->OpBlk_p->Data[FADE_OFFSET]	= (Private_Info_p->Private_Data[5]<<8)|(Private_Info_p->Private_Data[0]); // FADE Value
						AC3ParserLocalParams->OpBlk_p->Data[PAN_OFFSET]	= (Private_Info_p->Private_Data[6]<<8)|(Private_Info_p->Private_Data[1]); // PAN Value
					}

					//mark the alignment tag
					AC3ParserLocalParams->OpBlk_p->Flags   |= DATAFORMAT_VALID;
					AC3ParserLocalParams->OpBlk_p->Data[DATAFORMAT_OFFSET]	= BE;
			}
		}

		value=pos[offset];
		switch(AC3ParserLocalParams->AC3ParserState)
		{
		case ES_AC3_SYNC_0:
			if (value == 0x0b)
			{

				AC3ParserLocalParams->AC3ParserState = ES_AC3_SYNC_1;
				AC3ParserLocalParams->NoBytesCopied = 1;

				if(CurrentPTS_p->PTSValidFlag)
				{
					AC3ParserLocalParams->StartFramePTS = CurrentPTS_p->PTS;
					AC3ParserLocalParams->StartFramePTS33 = CurrentPTS_p->PTS33Bit;
					AC3ParserLocalParams->StartFramePTSValid = TRUE;
					CurrentPTS_p->PTSValidFlag = FALSE;
				}

			}
			else
			{
				AC3ParserLocalParams->NoBytesCopied = 0;
			}

			offset++;
			break;

		case ES_AC3_SYNC_1:
			if (value == 0x77)
			{
				AC3ParserLocalParams->AC3ParserState = ES_AC3_HDR_0;
				AC3ParserLocalParams->NoBytesCopied++;
				offset++;
				*AC3ParserLocalParams->Current_Write_Ptr1++ = 0x0b;
				*AC3ParserLocalParams->Current_Write_Ptr1++ = 0x77;
			}
			else
			{
				AC3ParserLocalParams->AC3ParserState = ES_AC3_SYNC_0;

			}
			break;

		case ES_AC3_HDR_0:
			AC3ParserLocalParams->HDR[0]=value;
			AC3ParserLocalParams->AC3ParserState = ES_AC3_HDR_1;
			offset++;
			AC3ParserLocalParams->NoBytesCopied++;
			*AC3ParserLocalParams->Current_Write_Ptr1++ = value;
			break;

		case ES_AC3_HDR_1:
			AC3ParserLocalParams->HDR[1]=value;
			AC3ParserLocalParams->AC3ParserState = ES_AC3_HDR_2;
			offset++;
			AC3ParserLocalParams->NoBytesCopied++;
			*AC3ParserLocalParams->Current_Write_Ptr1++ = value;
			break;

		case ES_AC3_HDR_2:
			AC3ParserLocalParams->HDR[2]=value;
			AC3ParserLocalParams->AC3ParserState = ES_AC3_HDR_3;
			offset++;
			AC3ParserLocalParams->NoBytesCopied++;
			*AC3ParserLocalParams->Current_Write_Ptr1++ = value;
			break;

		case ES_AC3_HDR_3:
			AC3ParserLocalParams->HDR[3]=value;
			AC3ParserLocalParams->AC3ParserState = ES_AC3_FSCOD;
			offset++;
			AC3ParserLocalParams->NoBytesCopied++;
			*AC3ParserLocalParams->Current_Write_Ptr1++ = value;
			break;
		case ES_AC3_FSCOD:
			if((AC3ParserLocalParams->HDR[2] & 0x3f) > 37 || (AC3ParserLocalParams->HDR[2] & 0xc0) == 0xc0 ) // MAX fscode 100101
			{
				AC3ParserLocalParams->AC3ParserState = ES_AC3_SYNC_0;
				AC3ParserLocalParams->Current_Write_Ptr1 = (U8 *)AC3ParserLocalParams->OpBlk_p->MemoryStart;
			}
			else
			{
				U8 samp_freq_code = (AC3ParserLocalParams->HDR[2] & 0xc0) >> 6;
				U8 frame_size_scode = (AC3ParserLocalParams->HDR[2] & 0x3f)>>1;
				U8 index1 = samp_freq_code & 0x1;
				if(samp_freq_code==3)
				{
					AC3ParserLocalParams->AC3ParserState = ES_AC3_SYNC_0;
					AC3ParserLocalParams->Current_Write_Ptr1-=AC3ParserLocalParams->NoBytesCopied;
					STTBX_Print((" ES_AC3 ERROR in Sampling frequency code \n"));
					break;
				}
				AC3ParserLocalParams->Bit_Rate_Code 	= (((AC3ParserLocalParams->HDR[2] & 0x01)<<5)|frame_size_scode);
				AC3ParserLocalParams->Bit_Stream_Mode 	= (AC3ParserLocalParams->HDR[3] & 0x07);
				/*report change in audio coding mode*/
				{
					U8 ACMode = ((value >> 5) & 0x07);

					if(AC3ParserLocalParams->Audio_Coding_Mode != ACMode)
					{
						AC3ParserLocalParams->Audio_Coding_Mode = ACMode;
						/*Notify routing event*/
						Error=STAudEVT_Notify(AC3ParserLocalParams->EvtHandle,AC3ParserLocalParams->EventIDNewRouting, &AC3ParserLocalParams->Audio_Coding_Mode, sizeof(AC3ParserLocalParams->Audio_Coding_Mode), AC3ParserLocalParams->ObjectID);
					}
				}

				if(AC3ParserLocalParams->Frequency != AC3Frequency[samp_freq_code])
				{
					AC3ParserLocalParams->OpBlk_p->Flags   |= FREQ_VALID;
					AC3ParserLocalParams->Frequency = AC3Frequency[samp_freq_code];
					AC3ParserLocalParams->OpBlk_p->Data[FREQ_OFFSET] = AC3ParserLocalParams->Frequency;

					Error=STAudEVT_Notify(AC3ParserLocalParams->EvtHandle,AC3ParserLocalParams->EventIDFrequencyChange, &AC3ParserLocalParams->Frequency, sizeof(AC3ParserLocalParams->Frequency), AC3ParserLocalParams->ObjectID);

				}

				if(AC3ParserLocalParams->StartFramePTSValid)
				{
					AC3ParserLocalParams->OpBlk_p->Flags   |= PTS_VALID;
					AC3ParserLocalParams->OpBlk_p->Data[PTS_OFFSET]      = AC3ParserLocalParams->StartFramePTS;
					AC3ParserLocalParams->OpBlk_p->Data[PTS33_OFFSET]    = AC3ParserLocalParams->StartFramePTS33;
					AC3ParserLocalParams->StartFramePTSValid = FALSE;
				}

				AC3ParserLocalParams->FrameSize = AC3FrameSizes[index1][frame_size_scode];

				if(samp_freq_code == 1)
				{
					AC3ParserLocalParams->FrameSize = AC3ParserLocalParams->FrameSize + 2*(value & 0x1);
				}
				else if(samp_freq_code == 2)
				{
					AC3ParserLocalParams->FrameSize = (AC3ParserLocalParams->FrameSize + (AC3ParserLocalParams->FrameSize >> 1));
				}


				AC3ParserLocalParams->BitRate = ((AC3ParserLocalParams->FrameSize * AC3ParserLocalParams->Frequency)/(1536 * 1000));



				*AC3ParserLocalParams->Current_Write_Ptr1++ = value;
				AC3ParserLocalParams->NoBytesCopied++;
				offset++;

				AC3ParserLocalParams->NoBytesRemainingInFrame = AC3ParserLocalParams->FrameSize - AC3ParserLocalParams->NoBytesCopied;
				AC3ParserLocalParams->AC3ParserState =ES_AC3_FRAME;

			}
			break;
		case ES_AC3_FRAME:
			if((Size-offset)>=AC3ParserLocalParams->NoBytesRemainingInFrame)
			{
				/* 1 or more frame */
				AC3ParserLocalParams->FrameComplete = TRUE;

				memcpy(AC3ParserLocalParams->Current_Write_Ptr1,((char *)MemoryStart+offset),AC3ParserLocalParams->NoBytesRemainingInFrame);


				AC3ParserLocalParams->OpBlk_p->Size = AC3ParserLocalParams->FrameSize;
				if(AC3ParserLocalParams->Get_Block == TRUE)
				{
					if(AC3ParserLocalParams->OpBlk_p->Flags & PTS_VALID)
					{
						/* Get the PTS in the correct structure */
						NotifyPTS.Low  = AC3ParserLocalParams->OpBlk_p->Data[PTS_OFFSET];
						NotifyPTS.High = AC3ParserLocalParams->OpBlk_p->Data[PTS33_OFFSET];
					}
					else
					{
						NotifyPTS.Low  = 0;
						NotifyPTS.High = 0;
					}

					NotifyPTS.Interpolated	= FALSE;

					if(AC3ParserLocalParams->Skip == 0)
					{
						Error = MemPool_PutOpBlk(AC3ParserLocalParams->OpBufHandle,(U32 *)&AC3ParserLocalParams->OpBlk_p);
						if(Error != ST_NO_ERROR)
						{
							STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
							return Error;
						}
						else
						{
							Error = STAudEVT_Notify(AC3ParserLocalParams->EvtHandle,AC3ParserLocalParams->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), AC3ParserLocalParams->ObjectID);
							STTBX_Print((" AC3 Deli:%d\n",AC3ParserLocalParams->Blk_Out));
							AC3ParserLocalParams->Blk_Out++;
							AC3ParserLocalParams->Reuse = FALSE;
							AC3ParserLocalParams->Put_Block = TRUE;
							AC3ParserLocalParams->Get_Block = FALSE;
						}
					}
					else
					{
						AC3ParserLocalParams->Skip --;
						AC3ParserLocalParams->Reuse = TRUE;
						AC3ParserLocalParams->Put_Block = TRUE;
						AC3ParserLocalParams->Get_Block = FALSE;
					}
				}

				AC3ParserLocalParams->Current_Write_Ptr1 += AC3ParserLocalParams->NoBytesRemainingInFrame;

				offset +=  AC3ParserLocalParams->NoBytesRemainingInFrame;
				AC3ParserLocalParams->NoBytesRemainingInFrame = 0;
				AC3ParserLocalParams->NoBytesCopied = 0;
				AC3ParserLocalParams->AC3ParserState = ES_AC3_SYNC_0;
			}
			else
			{
				/* less than a frame */
				memcpy(AC3ParserLocalParams->Current_Write_Ptr1,((char *)MemoryStart+offset),(Size - offset));
				AC3ParserLocalParams->Current_Write_Ptr1 += (Size - offset);
				AC3ParserLocalParams->NoBytesRemainingInFrame -= (Size-offset);
				AC3ParserLocalParams->FrameComplete = FALSE;

				offset =  Size;
				//AC3ParserLocalParams->NoBytesCopied = 0;
				/*be in the same state*/
				//AC3ParserLocalParams->AC3ParserState = ES_AC3_PARTIAL_FRAME;
			}
			break;

			break;
		default:
		    break;
		}
	}

	*No_Bytes_Used=offset;
	return ST_NO_ERROR;
}

ST_ErrorCode_t ES_AC3_Parser_Term(ParserHandle_t *const handle, ST_Partition_t	*CPUPartition_p)
{

	ES_AC3_ParserLocalParams_t *AC3ParserLocalParams = (ES_AC3_ParserLocalParams_t *)handle;

	if(handle)
	{
		memory_deallocate(CPUPartition_p, AC3ParserLocalParams);
	}

	return ST_NO_ERROR;

}

ST_ErrorCode_t  ES_AC3_GetSynchroUnit(ParserHandle_t  Handle,STAUD_SynchroUnit_t *SynchroUnit_p)
{
	ES_AC3_ParserLocalParams_t *AC3ParserLocalParams_p = (ES_AC3_ParserLocalParams_t *)Handle;
	ST_ErrorCode_t Error = ST_NO_ERROR;

	if(AC3ParserLocalParams_p->Frequency)
	{
		SynchroUnit_p->SkipPrecision = ((1536 * 1000)/AC3ParserLocalParams_p->Frequency);
		SynchroUnit_p->PausePrecision = 1;
	}
	else
	{
		Error = STAUD_ERROR_DECODER_PREPARING;
	}

	return Error;
}

U32 ES_AC3_GetRemainTime(ParserHandle_t  Handle)
{
	ES_AC3_ParserLocalParams_t *AC3ParserLocalParams_p = (ES_AC3_ParserLocalParams_t *)Handle;
	U32 Duration = 0;

	if(AC3ParserLocalParams_p)
	{
		switch (AC3ParserLocalParams_p->AC3ParserState)
		{
			case ES_AC3_FRAME:
				if(AC3ParserLocalParams_p->BitRate)
				{
					Duration = AC3ParserLocalParams_p->NoBytesRemainingInFrame/AC3ParserLocalParams_p->BitRate;
				}
				break;
			default:
				break;
		}
	}

	return Duration;
}

#endif //#ifdef STAUD_INSTALL_AC3
