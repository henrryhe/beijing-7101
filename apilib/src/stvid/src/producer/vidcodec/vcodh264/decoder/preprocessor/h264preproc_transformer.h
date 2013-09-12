/*
///
/// @file     : .\h264preproc_transformer.h
///
/// @brief    :
///
/// @par OWNER: Jerome Audu
///
/// @author   : Jerome Audu
///
/// @par SCOPE:
///
/// @date     : 2003-10-01
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.
///
*/

#ifndef H264_PREPROC_TRANSFORMER_INC
#define H264_PREPROC_TRANSFORMER_INC

#include "stddefs.h"
#include "stavmem.h"
#include "preprocessor.h"

typedef struct H264Preprocessor_Data_s
{
    ST_Partition_t             *CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;
    void*                       CallbackUserData;
    H264PreprocCallback_t       Callback;
    void                       *H264PreprocessorHardwareHandle;
} H264Preprocessor_Data_t;

#endif /* H264_PREPROC_TRANSFORMER_INC */
