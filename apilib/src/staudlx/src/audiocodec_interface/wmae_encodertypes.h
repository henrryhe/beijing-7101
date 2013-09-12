#ifndef _WMAE_ENCODER_TYPES_H_
#define _WMAE_ENCODER_TYPES_H_

#include "acc_mmedefines.h"
typedef struct
{
	U32 Config; //Reserved for future use.
} tMMEWmaeConfig;

typedef struct
{
	int  SamplingFreq;
	long PacketSize;
	long SizeOf_s_header_obj;
	long SizeOf_s_asf_header_type;
	long SizeOf_s_file_obj;
	long SizeOf_s_stream_obj;
	long SizeOf_s_ext_obj;
	long SizeOf_s_data_obj;
#ifdef ST_OSWINCE
	__int64 nAudioDelayBuffer;	
	__int64 SamplesPerPack;
#else
	long long nAudioDelayBuffer;	
	long long SamplesPerPack;
#endif
	

}MME_WmaeAsfPostProcessing_t;

typedef struct
{
#ifdef ST_OSWINCE
	__int64 ErrorCode;
#else
	long long ErrorCode;
#endif	
	MME_WmaeAsfPostProcessing_t AsfPostProcessing;

}MME_WmaeStatus_t;


#endif //_WMAE_ENCODER_TYPES_H_
