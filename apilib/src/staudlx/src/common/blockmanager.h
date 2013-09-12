/*******************************************************

 File Name: blockmanager.h

Description : Private datastructures for BlockManager.

Copyright (C) 2005 STMicroelectronics

History     :

    08/09/2005    Module created.

Reference   :

*******************************************************/
#ifndef _BLOCK_MANAGER_H
    #define _BLOCK_MANAGER_H

    /* Includes ----------------------------------------- */
    #include "staudlx.h"
    #include "stlite.h"
    #include "stddefs.h"
    #ifndef STAUD_REMOVE_EMBX
        #include "embx.h"
    #endif
    #include "stos.h"

    #ifdef __cplusplus
        extern "C" {
    #endif
    #ifdef  ST_51XX
        #define MEMORY_SPLIT_SUPPORT            0
    #else
        #define MEMORY_SPLIT_SUPPORT            1
    #endif
    /* Constants */
    /* --------- */
    #ifdef ST_51XX
        #define MEMBLK_MAX_NUMBER               8 /*Can be increased*/
        #define MAX_NUM_BLOCK                   8
    #else
        #define MEMBLK_MAX_NUMBER               32 /*Can be increased*/
        #define MAX_NUM_BLOCK                   32
    #endif

    #define MAX_LOGGING_ENTRIES                 5 /*Can be increased*/
    #define PTS_OFFSET                          0
    #define PTS_VALID                           (1 << PTS_OFFSET)
    #define FREQ_OFFSET                         1
    #define FREQ_VALID                          (1 << FREQ_OFFSET)
    #define LAYER_OFFSET                        2
    #define LAYER_VALID                         (1 << LAYER_OFFSET)
    #define FADE_OFFSET                         3
    #define FADE_VALID                          (1 << FADE_OFFSET)
    #define PAN_OFFSET                          4
    #define PAN_VALID                           (1 << PAN_OFFSET)
    #define CODING_OFFSET                       5
    #define MODE_VALID                          (1 << CODING_OFFSET)
    #define EOF_OFFSET                          6
    #define EOF_VALID                           (1 << EOF_OFFSET)
    #define DUMMYBLOCK_OFFSET                   7
    #define DUMMYBLOCK_VALID                    (1<< DUMMYBLOCK_OFFSET)
    #define NCH_OFFSET                          8
    #define NCH_VALID                           (1 << NCH_OFFSET)
    #define SAMPLESIZE_OFFSET                   9
    #define SAMPLESIZE_VALID                    (1 << SAMPLESIZE_OFFSET)
    #define PTS33_OFFSET                        10
    #define PTS33_VALID                         (1 << PTS33_OFFSET)
    #define DATAFORMAT_OFFSET                   11
    #define DATAFORMAT_VALID                    (1 << DATAFORMAT_OFFSET)
    #define WMAE_ASF_HEADER_OFFSET              12
    #define WMAE_ASF_HEADER_VALID               (1 << WMAE_ASF_HEADER_OFFSET)
    #define CONSUMER_HANDLE                     13
    #define BUFFER_DIRTY                        (1 << CONSUMER_HANDLE)
    #define WMA_FORMAT_OFFSET                   16
    #define WMA_FORMAT_VALID                    (1 << 16)
    #define WMA_NUMBER_CHANNELS_OFFSET          17
    #define WMA_NUMBER_CHANNELS_VALID           (1 << 17)
    #define WMA_SAMPLES_P_SEC_OFFSET            18
    #define WMA_SAMPLES_P_SEC_VALID             (1 << 18)
    #define WMA_AVG_BYTES_P_SEC_OFFSET          19
    #define WMA_AVG_BYTES_P_SEC_VALID           (1 << 19)
    #define WMA_BLK_ALIGN_OFFSET                20
    #define WMA_BLK_ALIGN_VALID                 (1 << 20)
    #define WMA_BITS_P_SAMPLE_OFFSET            21
    #define WMA_BITS_P_SAMPLE_VALID             (1 << 21)
    #define WMA_LAST_DATA_BLOCK_OFFSET          22
    #define WMA_LAST_DATA_BLOCK_VALID           (1 << 22)
    #define WMA_SAMPLES_P_BLK_OFFSET            23
    #define WMA_SAMPLES_P_BLK_VALID             (1 << 23)
    #define WMA_ENCODE_OPT_OFFSET               24
    #define WMA_ENCODE_OPT_VALID                (1 << 24)
    //#define   WMA_SUPER_BLK_OFFSET            25
    //#define   WMA_SUPER_BLK_VALID             (1 << 25)
    #define WMA_VERSION_OFFSET                  26
    #define WMA_VERSION_VALID                   (1 << 26)
    #define WMA_CHANNEL_MASK_OFFSET             27
    #define WMA_CHANNEL_MASK_VALID              (1 << 27)
    #define WMA_VALID_BITS_P_SAMPLE_OFFSET      28
    #define WMA_VALID_BITS_P_SAMPLE_VALID       (1 << 28)
    #define WMA_STREAM_ID_OFFSET                29
    #define WMA_STREAM_ID_VALID                 (1 << 29)
    #define NEW_DMIX_TABLE_OFFSET               30
    #define NEW_DMIX_TABLE_VALID                (1 << 30)





    #define WMA_FLAG_MASK   (WMA_FORMAT_VALID|WMA_NUMBER_CHANNELS_VALID|WMA_SAMPLES_P_SEC_VALID|WMA_AVG_BYTES_P_SEC_VALID| \
                            WMA_BLK_ALIGN_VALID|WMA_BITS_P_SAMPLE_VALID|WMA_SAMPLES_P_BLK_VALID|WMA_ENCODE_OPT_VALID|WMA_VERSION_VALID| \
                            WMA_CHANNEL_MASK_VALID|WMA_VALID_BITS_P_SAMPLE_VALID|WMA_STREAM_ID_VALID)


    /* Memory Pool Handle */
    typedef U32 STMEMBLK_Handle_t;

    /* Error Types Specific to the MPEG Parser */
    typedef enum    DataAlignment_e
    {
        LE,
        BE
    }   DataAlignment_t;

    /* Structures */
    typedef struct  MemoryBlock_s
    {
        U32     MemoryStart;
        U32     MaxSize; /* Max allocated size of a block */
        U32     Size; /* Size of valid data. Required as we can't do a run time allocation*/
        U32     Flags;
        void    *AppData_p;
        U32     Data[32];
        //can be expanded
    }MemoryBlock_t;

    typedef struct  MemoryParams_s
    {
        U32     MemoryStart;
        U32     Size;
        U32     Flags;
    }MemoryParams_t;

    typedef struct  MemPoolInitParams_s
    {
        U8                          NumBlocks;
        U32                         BlockSize;
        STAVMEM_PartitionHandle_t   AVMEMPartition;
        ST_Partition_t              *CPUPartition_p;
        BOOL                        AllocateFromEMBX;
        BOOL                        NoProducer; /*Is this Pool a infinite producer*/
        BOOL                        NoConsumer; /*Is this Pool and infinite consumer*/
    }   MemPoolInitParams_t;

    typedef struct  Block_s
    {
        MemoryBlock_t   Block;
        U8              Free; /*free for producer, used as BOOL*/
        U8              Filled; /*filled by producer, used as BOOL*/
        U8              Fill[MAX_LOGGING_ENTRIES]; /*this block can be used by the consumers, used as BOOL*/
        U8              InUse[MAX_LOGGING_ENTRIES]; /*shows that the block is being used by consumer, used as BOOL*/
        #if MEMORY_SPLIT_SUPPORT
            U8          SplitCount[MAX_LOGGING_ENTRIES]; /*splits per consumer*/
            U8          SplitSent[MAX_LOGGING_ENTRIES]; /*splits per consumer*/
        #endif
        U8              ConsumerCount; /*how many consumers will use this block*/
        struct  Block_s *Next_p;
    }Block_t;

    typedef struct  BlockList_s
    {
        Block_t             * Blk_p;
        struct  BlockList_s * Next_p;
    }   BlockList_t;


    typedef struct  LoggedEntries_s
    {
        U32             Handle;
        semaphore_t     * sem;
    }   LoggedEntries_t;

    typedef LoggedEntries_t     ProducerParams_t ;
    typedef LoggedEntries_t     ConsumerParams_t ;

    typedef struct  MemoryPoolControl_s
    {
        ST_DeviceName_t             PoolName;
        STMEMBLK_Handle_t           Handle;
        STAVMEM_PartitionHandle_t   AVMEMPartition;
        ST_Partition_t              * CPUPartition_p;
        #ifndef STAUD_REMOVE_EMBX
            EMBX_TRANSPORT          tp;
        #endif
        Block_t                     * Block_p;
        BlockList_t                 * BlkList_p;    /*the full list*/
        BlockList_t                 * BlkForPro_p; /*blocks for the producer*/
        BlockList_t                 * BlkForCon_p; /*Blocks for consumer*/
        U32                         NextFree; /*producer will be given buffer from here*/
        U32                         NextFilled; /*producer returns filled data from here*/
        U32                         NextSent[MAX_LOGGING_ENTRIES];  /*consumer received data from here*/
        U32                         NextRelease[MAX_LOGGING_ENTRIES];/*consumer frees data from here*/
        LoggedEntries_t             Logged[MAX_LOGGING_ENTRIES];    /*Array of the logged entries*/
        STAVMEM_BlockHandle_t       BufferHandle;
        U32                         * Buffer_p;
        U32                         TotalSize;
        U32                         BlockSize;
        U8                          NumBlocks;
        U8                          AllocateFromEMBX;
        U8                          MaxLoggingEntries; /*max 1 producer and (MaxLoggingEntries-1) consumer*/
        U8                          LoggedEntries; /*max 1 producer and (LoggedEntries -1) consumer*/
        U8                          ProdQSize;
        U8                          ConQSize;
        U8                          NoProducer; /*Is this Pool a infinite producer, used as BOOL*/
        U8                          NoConsumer; /*Is this Pool and infinite consumer, used as BOOL*/
        U8                          Request[MAX_LOGGING_ENTRIES]; /*Log any request, used as BOOL*/
        U8                          Start; /*used as BOOL*/
        U8                          NumConsumer;
        U8                          NumProducer;
        #if MEMORY_SPLIT_SUPPORT
            U32                     SplitSize;
        #endif
        STOS_Mutex_t                * Lock_p;
        struct  MemoryPoolControl_s * Next_p;
        }   MemoryPoolControl_t;

    /* External declarations */
    /* --------------------- */

/* Prototypes */
/* ---------- */
    ST_ErrorCode_t  MemPool_Init            (ST_DeviceName_t  Name, MemPoolInitParams_t *InitParams, STMEMBLK_Handle_t *Handle);
    BOOL            MemPool_IsInit          (const ST_DeviceName_t PoolName);
    ST_ErrorCode_t  MemPool_RegProducer     (STMEMBLK_Handle_t Handle, ProducerParams_t Producer);
    ST_ErrorCode_t  MemPool_RegConsumer     (STMEMBLK_Handle_t Handle, ConsumerParams_t Consumer);
    ST_ErrorCode_t  MemPool_GetOpBlk        (STMEMBLK_Handle_t Handle, U32 *AddressToBlkPtr_p);
    ST_ErrorCode_t  MemPool_PutOpBlk        (STMEMBLK_Handle_t Handle, U32 *AddressToBlkPtr_p);
    ST_ErrorCode_t  MemPool_ReturnOpBlk     (STMEMBLK_Handle_t Handle, U32 *AddressToBlkPtr_p);
    ST_ErrorCode_t  MemPool_GetIpBlk        (STMEMBLK_Handle_t Handle, U32 *AddressToBlkPtr_p, U32 ConHandle);
    ST_ErrorCode_t  MemPool_FreeIpBlk       (STMEMBLK_Handle_t Handle, U32 *AddressToBlkPtr_p, U32 ConHandle);
    ST_ErrorCode_t  MemPool_ReturnIpBlk     (STMEMBLK_Handle_t Handle, U32 *AddressToBlkPtr_p, U32 ConHandle);
    ST_ErrorCode_t  MemPool_Flush           (STMEMBLK_Handle_t Handle);
    ST_ErrorCode_t  MemPool_Term            (STMEMBLK_Handle_t Handle);


    ST_ErrorCode_t  MemPool_UnRegProducer   (STMEMBLK_Handle_t Handle, U32 ProdHandle);
    ST_ErrorCode_t  MemPool_UnRegConsumer   (STMEMBLK_Handle_t Handle, U32 ConHandle);
    ST_ErrorCode_t  MemPool_Start           (STMEMBLK_Handle_t Handle);
    ST_ErrorCode_t  MemPool_GetBufferParams (STMEMBLK_Handle_t Handle,STAUD_BufferParams_t* DataParams_p);
    #if MEMORY_SPLIT_SUPPORT
        ST_ErrorCode_t  MemPool_SetSplit      (STMEMBLK_Handle_t Handle, U32 ProdHandle, U32 SplitSize);
        ST_ErrorCode_t  MemPool_GetSplitIpBlk ( STMEMBLK_Handle_t Handle, U32  *AddressToBlkPtr_p, U32 ConHandle, MemoryParams_t *Offset_p);
    #endif

    #ifdef __cplusplus
        }
    #endif

#endif /* _BLOCK_MANAGER_H */

