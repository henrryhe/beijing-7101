#ifndef _AIFF_PARSER_H
#define _AIFF_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"

#ifdef __cplusplus
extern "C" {
#endif



/* Constants ---------------------------------------- */



typedef enum
{

    ES_AIFF_F,
    ES_AIFF_O,
    ES_AIFF_R,
    ES_AIFF_M,
    ES_AIFF_A,
    ES_AIFF_I,
    ES_AIFF_F1,
    ES_AIFF_F2,
    ES_AIFF_C,
    ES_AIFF_O1,
    ES_AIFF_M1,
    ES_AIFF_M2,
    ES_AIFF_S1,
    ES_AIFF_S2,
    ES_AIFF_N,
    ES_AIFF_D,
    ES_AIFF_PARSE_COMMON_CHUNK_SIZE_00,
    ES_AIFF_PARSE_COMMON_CHUNK_SIZE_01,
    ES_AIFF_PARSE_COMMON_CHUNK_SIZE_02,
    ES_AIFF_PARSE_COMMON_CHUNK_SIZE_03,
    ES_AIFF_PARSE_CHUNK_SIZE,
    ES_AIFF_PARSE_NUMBER_OF_CHANNELS_N1,
    ES_AIFF_PARSE_NUMBER_OF_CHANNELS_N2,
    ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_00,
    ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_01,
    ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_02,
    ES_AIFF_PARSE_NUMBER_OF_SAMPLE_FRAMES_03,
    ES_AIFF_PARSE_SAMPLE_SIZE_0,
    ES_AIFF_PARSE_SAMPLE_SIZE_1,
    ES_AIFF_PARSE_SAMPLE_RATE,
    ES_AIFF_PARSE_SSND_CHUNK_SIZE_00,
    ES_AIFF_PARSE_SSND_CHUNK_SIZE_01,
    ES_AIFF_PARSE_SSND_CHUNK_SIZE_02,
    ES_AIFF_PARSE_SSND_CHUNK_SIZE_03,
    ES_AIFF_PARSE_SSND_DATA_OFFSET_00,
    ES_AIFF_PARSE_SSND_DATA_OFFSET_01,
    ES_AIFF_PARSE_SSND_DATA_OFFSET_02,
    ES_AIFF_PARSE_SSND_DATA_OFFSET_03,
    ES_AIFF_PARSE_SSND_BLOCK_SIZE_00,
    ES_AIFF_PARSE_SSND_BLOCK_SIZE_01,
    ES_AIFF_PARSE_SSND_BLOCK_SIZE_02,
    ES_AIFF_PARSE_SSND_BLOCK_SIZE_03,
    ES_AIFF_PARSE_DATA,
    ES_AIFF_CALCULATE_SAMPLING_RATE,
    ES_AIFF_PARSE_BLOCK_SIZE_00,
    ES_AIFF_PARSE_BLOCK_SIZE_01,
    ES_AIFF_PARSE_BLOCK_SIZE_02,
    ES_AIFF_PARSE_BLOCK_SIZE_03,
    ES_AIFF_PARSE_COMM_CHUNK_SIZE_S0,
    ES_AIFF_PARSE_COMM_CHUNK_SIZE_S1,
    ES_AIFF_PARSE_COMM_CHUNK_SIZE_S2,
    ES_AIFF_PARSE_COMM_CHUNK_SIZE_S3,
    ES_AIFF_PARSE_BYTES_PER_SEC_B1,
    ES_AIFF_PARSE_BYTES_PER_SEC_B2,
    ES_AIFF_PARSE_BYTES_PER_SEC_B3,
    ES_AIFF_PARSE_BYTES_PER_SEC_B4,
    ES_AIFF_PARSE_CHUNK_SIZE_00,
    ES_AIFF_PARSE_CHUNK_SIZE_01,
    ES_AIFF_PARSE_CHUNK_SIZE_02,
    ES_AIFF_PARSE_CHUNK_SIZE_03,
    ES_AIFF_COMPUTE_FRAME_LENGTH,
    ES_AIFF_FRAME,
    ES_AIFF_REJECT_DATA
} ES_AIFF_ParserState_t;



/* Structures */

typedef struct
{
    U8                              Sample_Rate[10];
    U8                              Sample_Rate_Counter;
    U8                              Get_Block;     /*Used as BOOL*/
    U8                              Put_Block;     /*Used as BOOL*/
    U8                              AppDataFlag;   /*Used as BOOL*/
    U16                             Number_Of_Channels;
    U16                             NoBytesRemainingInFrame;
    U16                             Sample_Size;
    U32                             Bytes_Per_Second;
    U32                             Comm_Chunk_Size;
    U32                             SSND_DataOffset;
    U32                             Number_of_SampleFrames;
    U32                             DataOffset;
    U32                             Chunk_Size;
    U32                             SpaceInBuffer;
    U32                             SpaceIn24Buffer;
    U32                             SamplingRate;
    U32                             FrameSize;
    U32                             Frequency;
    U32                             SSNND_SampleSize;
    U32                             SSND_Offset;
    U32                             SSND_BlockSize;
    U8                              * Current_Write_Ptr1;
    U32                             * AIFFParserFDMANodesBaseAddress;
    U32                             SSND_Chunk_Size;
    ES_AIFF_ParserState_t           ParserState;
    STMEMBLK_Handle_t               OpBufHandle;
    MemoryBlock_t                   * OpBlk_p;
    STAUD_Object_t                  ObjectID;
    STAud_Memory_Address_t          FDMAnodeAddress;
    STFDMA_TransferGenericParams_t  memcpyTransferParam;
    STFDMA_TransferId_t             TransferId;
    STFDMA_GenericNode_t            * node_p;
    STAud_LpcmStreamParams_t        * AppData_p;
    ST_Partition_t                  * CPUPartition_p;
    STAVMEM_PartitionHandle_t       AVMEMPartition;
    STAVMEM_BlockHandle_t           AIFFParserNodeHandle_p;
//    STAVMEM_BlockHandle_t           DummyBufferHandle;
#ifndef STAUD_REMOVE_EMBX
    EMBX_TRANSPORT                  tp;
#endif
    STAud_Memory_Address_t          DummyBufferAddress;
    STAud_Memory_Address_t          ESBufferAddress;
    U32                             DummyBufferSize;
    U32                             DataCopied;
    semaphore_t                     * SemESBuffer_p;

} ES_AIFF_ParserLocalParams_t;


/* *******   Functions ------------------------------- */

ST_ErrorCode_t ES_AIFF_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_AIFF_Parse_Frame(ParserHandle_t const, void *, U32,U32 *);

ST_ErrorCode_t ES_AIFF_Parser_Term(ParserHandle_t *const handle, ST_Partition_t *CPUPartition_p);

ST_ErrorCode_t ES_AIFF_ParserGetParsingFunction(ParsingFunction_t   * ParsingFucntion_p,
                                                    GetFreqFunction_t * GetFreqFunction_p,
                                                    GetInfoFunction_t * GetInfoFunction_p);

ST_ErrorCode_t ES_AIFF_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_AIFF_Parser_Start(ParserHandle_t const Handle);

ST_ErrorCode_t ES_AIFF_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p);

ST_ErrorCode_t ES_AIFF_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo);
ST_ErrorCode_t ES_AIFF_Handle_EOF(ParserHandle_t  Handle);
#ifdef __cplusplus
}
#endif



#endif /* _AIFF_PARSER_H */
