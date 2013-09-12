#ifndef _ACF_ST20_AUDIOCODEC_H_
#define _ACF_ST20_AUDIOCODEC_H_
#include "acf_st20_defines.h"
#include "stddefs.h"

ACF_ST20_tDecoderInterface * ACF_ST20_MemAllocate(U8 codec_type,ST_Partition_t *CPUPartition);

#ifndef _ST20_C_
enum ACF_ST20_eErrorCode ACF_ST20_AudioCodec(ACF_ST20_tDecoderInterface	*ACF_ST20_DecoderInterface );
#else
extern ACF_ST20_eErrorCode ACF_ST20_AudioCodec(ACF_ST20_tDecoderInterface	*ACF_ST20_DecoderInterface);
#endif
void ACF_ST20_MemFree(ACF_ST20_tDecoderInterface	*ACF_ST20_DecoderInterface,int codec_type,ST_Partition_t *CPUPartition);
//void ACF_ST20_MemFree(ACF_ST20_tDecoderInterface	*ACF_ST20_DecoderInterface,int codec_type);
#endif
