#ifndef _Wave_PARSER_H
#define _Wave_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"
#include "staud.h"
#include "stos.h"

#ifdef __cplusplus
extern "C" {
#endif



/* Constants ---------------------------------------- */



typedef enum
{

    ES_WAVE_R,
    ES_WAVE_I,
    ES_WAVE_F1,
    ES_WAVE_F2,
    ES_WAVE_Chunk_Data_Size_0,
    ES_WAVE_Chunk_Data_Size_1,
    ES_WAVE_Chunk_Data_Size_2,
    ES_WAVE_Chunk_Data_Size_3,
    ES_WAVE_RIFF_TYPE_W,
    ES_WAVE_RIFF_TYPE_A,
    ES_WAVE_RIFF_TYPE_V,
    ES_WAVE_RIFF_TYPE_E,
    ES_WAVE_F,
    ES_WAVE_M,
    ES_WAVE_T1,
    ES_WAVE_T2,
    ES_WAVE_Parse_Chunk_Data_Size_S1,
    ES_WAVE_Parse_Chunk_Data_Size_S2,
    ES_WAVE_Parse_Chunk_Data_Size_S3,
    ES_WAVE_Parse_Chunk_Data_Size_S4,
    ES_WAVE_Parse_Compression_Code_C1,
    ES_WAVE_Parse_Compression_Code_C2,
    ES_WAVE_Parse_Number_of_Channels_N1,
    ES_WAVE_Parse_Number_of_Channels_N2,
    ES_WAVE_Parse_Sample_Rate_R1,
    ES_WAVE_Parse_Sample_Rate_R2,
    ES_WAVE_Parse_Sample_Rate_R3,
    ES_WAVE_Parse_Sample_Rate_R4,
    ES_WAVE_Parse_Bytes_per_sec_B1,
    ES_WAVE_Parse_Bytes_per_sec_B2,
    ES_WAVE_Parse_Bytes_per_sec_B3,
    ES_WAVE_Parse_Bytes_per_sec_B4,
    ES_WAVE_Parse_Block_Align_BA1,
    ES_WAVE_Parse_Block_Align_BA2,
    ES_WAVE_Parse_Significant_Bits_Per_Sample_1,
    ES_WAVE_Parse_Significant_Bits_Per_Sample_2,
    ES_WAVE_CHECK_COMPRESSED_CODE,
    ES_WAVE_Parse_COMP_Data_D,
    ES_WAVE_BYTES_EXTRA1,
    ES_WAVE_CHECK_SIZE,
    ES_WAVE_COM_DATA_EXT,
    ES_WAVE_COM_SAMPLES_BLOCK,
    ES_WAVE_COM_CHECK_BLOCK,
    ES_WAVE_COM_COEFF_EXT1,
    ES_WAVE_COM_COEFF_EXT2,
    ES_WAVE_COM_SKIP_COEFF,
    ES_WAVE_COM_SKIP_BYTES,
    ES_WAVE_Parse_Data_D,
    ES_WAVE_Parse_Data_A0,
    ES_WAVE_Parse_Data_A1,
    ES_WAVE_Parse_Data_T,
    ES_WAVE_Parse_Chunk_Size_00,
    ES_WAVE_Parse_Chunk_Size_01,
    ES_WAVE_Parse_Chunk_Size_02,
    ES_WAVE_Parse_Chunk_Size_03,
    ES_WAVE_COMPUTE_FRAME_LENGTH,
    ES_WAVE_FRAME,
    ES_WAVE_FRAME_20_24_0,
    ES_WAVE_FRAME_20_24_1,
    ES_WAVE_FRAME_20_24_2,
    ES_WAVE_FRAME_20_24_3,
    ES_REJECT_DATA
} ES_WAVE_ParserState_t;



/* Structures */

typedef struct
{
    U8                              Nch;
    U8                              AppDataFlag;    /*Used as BOOL*/
    U8                              UpdatePcm;      /*Used as BOOL*/
    U8                              Get_Block;      /*Used as BOOL*/
    U8                              Put_Block;      /*Used as BOOL*/
    U16                             Compression_Code;
    U16                             Number_Of_Channels;
    U16                             ExtraSize;
    U16                             NumberOfCoeffs;
    U16                             Block_Align;
    U16                             Significant_Bits_Per_Sample;
    U16                             NoBytesRemainingInFrame;
    U32                             Sample_Rate;
    U32                             Bytes_Per_Second;
    U32                             Chunk_Size;
    U32                             SpaceInBuffer;
    U32                             FrameSize;
    U32                             Frequency;
    U32                             SampleSize;
    U32                             SamplesPerBlock;
    U32                             BytesToSkip;
    U32                             DummyBufferSize;
    U32                             * WaveParserFDMANodesBaseAddress;
    U32                             Blk_Out;
    U8                              * Current_Write_Ptr1;
    ES_WAVE_ParserState_t           ParserState;
    STMEMBLK_Handle_t               OpBufHandle;
    MemoryBlock_t                   *OpBlk_p;
    STAUD_Object_t                  ObjectID;
    STFDMA_GenericNode_t            * node_p;
    STAud_Memory_Address_t          FDMAnodeAddress;
    STFDMA_TransferGenericParams_t  memcpyTransferParam;
    STFDMA_TransferId_t             TransferId;
    STAud_LpcmStreamParams_t        *AppData_p;
    ST_Partition_t                  *CPUPartition_p;
    ST_Partition_t                  * AudioParserMemory_p;
    STAVMEM_PartitionHandle_t       AVMEMPartition;
    STAVMEM_BlockHandle_t           WaveParserNodeHandle_p;
    //    STAVMEM_BlockHandle_t           DummyBufferHandle;
#ifndef STAUD_REMOVE_EMBX
    EMBX_TRANSPORT                  tp;
#endif
    STAud_Memory_Address_t          DummyBufferAddress;
    STAud_Memory_Address_t          ESBufferAddress;

} ES_Wave_ParserLocalParams_t;


/* *******   Functions ------------------------------- */

ST_ErrorCode_t ES_Wave_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_Wave_Parse_Frame(ParserHandle_t const, void *, U32,U32 *);

ST_ErrorCode_t ES_Wave_Parser_Deallocate(ParserHandle_t const );

ST_ErrorCode_t ES_Wave_Parser_Term(ParserHandle_t *const handle, ST_Partition_t *CPUPartition_p);

ST_ErrorCode_t ES_Wave_ParserGetParsingFunction(ParsingFunction_t   * ParsingFucntion_p,
                                                    GetFreqFunction_t * GetFreqFunction_p,
                                                    GetInfoFunction_t * GetInfoFunction_p);

ST_ErrorCode_t ES_Wave_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_Wave_Parser_Start(ParserHandle_t const Handle);

ST_ErrorCode_t ES_Wave_Set_Pcm(ParserHandle_t const Handle,U32 Frequency, U32 SampleSize, U32 Nch);

ST_ErrorCode_t ES_Wave_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p);

ST_ErrorCode_t ES_Wave_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo);
ST_ErrorCode_t ES_Wave_Handle_EOF(ParserHandle_t  Handle);
#ifdef __cplusplus
}
#endif



#endif /* _Wave_PARSER_H */
