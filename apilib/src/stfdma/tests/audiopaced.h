#ifndef __AUDIOPACED_H
#define __AUDIOPACED_H

/* SPDIF Player Registers */
#define SPDIF_BASE                          (SPDIF_BASE_ADDRESS) /* From chip file */
#define SPDIF_FIFO_DATA_REG       		    (SPDIF_BASE+0x04)
#define SPDIF_ITRP_STATUS_REG               (SPDIF_BASE+0x08)
#define SPDIF_ITRP_STATUS_CLEAR_REG	    	(SPDIF_BASE+0x0C)
#define SPDIF_ITRP_ENABLE_REG			    (SPDIF_BASE+0x10)
#define SPDIF_ITRP_ENABLE_SET_REG		    (SPDIF_BASE+0x14)
#define SPDIF_ITRP_ENABLE_CLEAR_REG		    (SPDIF_BASE+0x18)
#define SPDIF_CONTROL_REG         		    (SPDIF_BASE+0x1C)
#define SPDIF_STATUS_REG  			        (SPDIF_BASE+0x20)
#define SPDIF_PA_PB_REG           		    (SPDIF_BASE+0x24)
#define SPDIF_PC_PD_REG           		    (SPDIF_BASE+0x28)
#define SPDIF_CL1_REG				        (SPDIF_BASE+0x2C)
#define SPDIF_CR1_REG				        (SPDIF_BASE+0x30)
#define SPDIF_CL2_CR2_UV_REG			    (SPDIF_BASE+0x34)
#define SPDIF_PAUSE_LAT_REG			        (SPDIF_BASE+0x38)
#define SPDIF_FRAMELGTH_BURST_REG	       	(SPDIF_BASE+0x3C)

#define FDMA_BASE                           (FDMA_BASE_ADDRESS) /* From chip file */
#define SPDIF_ADD_DATA_REGION_FRAME_COUNT   (FDMA_BASE+0x9380+0x20)
#define SPDIF_ADD_DATA_REGION_STATUS	    (FDMA_BASE+0x9380+0x24)
#define SPDIF_ADD_DATA_REGION_PREC_MASK     (FDMA_BASE+0x9380+0x28)

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
#define INTERRUPT_NO_AUD_SPDIF_PLYR         SPDIF_PLAYER_INTERRUPT /* From chip file */
#elif defined (ST_5162)
#define INTERRUPT_NO_AUD_SPDIF_PLYR         SPDIF_INTERRUPT /* From chip file */
#endif
#define INTERRUPT_LEVEL_AUD_SPDIF_PLYR      14

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
/* PCM Player #0 Registers */
#define PCMPLAYER0_BASE		   	            (PCM_PLAYER_0_BASE_ADDRESS) /* From chip file */
#elif defined (ST_5162)
#define PCMPLAYER0_BASE		   	            (PCM_PLAYER_BASE_ADDRESS) /* From chip file */
#endif
#define PCMPLAYER0_SOFT_RESET_REG           (PCMPLAYER0_BASE+0x00)
#define PCMPLAYER0_FIFO_DATA_REG  		    (PCMPLAYER0_BASE+0x04)
#define PCMPLAYER0_ITRP_STATUS_REG 		    (PCMPLAYER0_BASE+0x08)
#define PCMPLAYER0_ITRP_STATUS_CLEAR_REG 	(PCMPLAYER0_BASE+0x0C)
#define PCMPLAYER0_ITRP_ENABLE_REG	  	    (PCMPLAYER0_BASE+0x10)
#define PCMPLAYER0_ITRP_ENABLE_SET_REG		(PCMPLAYER0_BASE+0x14)
#define PCMPLAYER0_ITRP_ENABLE_CLEAR_REG	(PCMPLAYER0_BASE+0x18)
#define PCMPLAYER0_CONTROL_REG     		    (PCMPLAYER0_BASE+0x1C)
#define PCMPLAYER0_STATUS_REG			    (PCMPLAYER0_BASE+0x20)
#define PCMPLAYER0_FORMAT_REG      		    (PCMPLAYER0_BASE+0x24)

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
#define INTERRUPT_NO_AUD_PCM_PLYR0          PCM_PLAYER_0_INTERRUPT /* From chip file */
#elif defined (ST_5162)
#define INTERRUPT_NO_AUD_PCM_PLYR0          PCM_PLAYER_INTERRUPT /* From chip file */
#endif
#define INTERRUPT_LEVEL_AUD_PCM_PLYR0       14

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
/* PCM Player #1 Registers */
#define PCMPLAYER1_BASE		   	            (PCM_PLAYER_1_BASE_ADDRESS) /* From chip file */
#elif defined (ST_5162)
#define PCMPLAYER1_BASE		   	            (PCM_PLAYER_BASE_ADDRESS) /* From chip file */
#endif
#define PCMPLAYER1_SOFT_RESET_REG           (PCMPLAYER1_BASE+0x00)
#define PCMPLAYER1_FIFO_DATA_REG  		    (PCMPLAYER1_BASE+0x04)
#define PCMPLAYER1_ITRP_STATUS_REG 		    (PCMPLAYER1_BASE+0x08)
#define PCMPLAYER1_ITRP_STATUS_CLEAR_REG 	(PCMPLAYER1_BASE+0x0C)
#define PCMPLAYER1_ITRP_ENABLE_REG	  	    (PCMPLAYER1_BASE+0x10)
#define PCMPLAYER1_ITRP_ENABLE_SET_REG		(PCMPLAYER1_BASE+0x14)
#define PCMPLAYER1_ITRP_ENABLE_CLEAR_REG	(PCMPLAYER1_BASE+0x18)
#define PCMPLAYER1_CONTROL_REG     		    (PCMPLAYER1_BASE+0x1C)
#define PCMPLAYER1_STATUS_REG			    (PCMPLAYER1_BASE+0x20)
#define PCMPLAYER1_FORMAT_REG      		    (PCMPLAYER1_BASE+0x24)

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
#define INTERRUPT_NO_AUD_PCM_PLYR1          PCM_PLAYER_1_INTERRUPT /* From chip file */
#elif defined (ST_5162)
#define INTERRUPT_NO_AUD_PCM_PLYR1          PCM_PLAYER_INTERRUPT /* From chip file */
#endif

#define INTERRUPT_LEVEL_AUD_PCM_PLYR1       14

/* Audio Registers */
#define AUDIO_CONFIG_BASE                   (AUDIO_IF_BASE_ADDRESS) /* From chip file */
#if defined (ST_7200)
#define AUDIO_FSYNA_CFG                     (AUDIO_CONFIG_BASE+0x000)
#define AUDIO_FSYNB_CFG                     (AUDIO_CONFIG_BASE+0x100)
#define AUDIO_FSYN_CFG                      AUDIO_FSYNA_CFG
#else
#define AUDIO_FSYN_CFG                      (AUDIO_CONFIG_BASE+0x000)
#endif

#define AUDIO_FSYN_PCM0_MD                  (AUDIO_CONFIG_BASE+0x10)
#define AUDIO_FSYN_PCM0_PE                  (AUDIO_CONFIG_BASE+0x14)
#define AUDIO_FSYN_PCM0_SDIV                (AUDIO_CONFIG_BASE+0x18)
#define AUDIO_FSYN_PCM0_PROG_EN             (AUDIO_CONFIG_BASE+0x1C)

#define AUDIO_FSYN_PCM1_MD                  (AUDIO_CONFIG_BASE+0x20)
#define AUDIO_FSYN_PCM1_PE                  (AUDIO_CONFIG_BASE+0x24)
#define AUDIO_FSYN_PCM1_SDIV                (AUDIO_CONFIG_BASE+0x28)
#define AUDIO_FSYN_PCM1_PROG_EN             (AUDIO_CONFIG_BASE+0x2C)

#if defined (ST_7200)
#define AUDIO_FSYNB3_MD                     (AUDIO_CONFIG_BASE+0x140)
#define AUDIO_FSYNB3_PE                     (AUDIO_CONFIG_BASE+0x144)
#define AUDIO_FSYNB3_SDIV                   (AUDIO_CONFIG_BASE+0x148)
#define AUDIO_FSYNB3_PROG_EN                (AUDIO_CONFIG_BASE+0x14C)
#define AUDIO_FSYN_SPDIF_MD                 AUDIO_FSYNB3_MD
#define AUDIO_FSYN_SPDIF_PE                 AUDIO_FSYNB3_PE
#define AUDIO_FSYN_SPDIF_SDIV               AUDIO_FSYNB3_SDIV
#define AUDIO_FSYN_SPDIF_PROG_EN            AUDIO_FSYNB3_PROG_EN
#else
#define AUDIO_FSYN2_MD                      (AUDIO_CONFIG_BASE+0x30)
#define AUDIO_FSYN2_PE                      (AUDIO_CONFIG_BASE+0x34)
#define AUDIO_FSYN2_SDIV                    (AUDIO_CONFIG_BASE+0x38)
#define AUDIO_FSYN2_PROG_EN                 (AUDIO_CONFIG_BASE+0x3C)
#define AUDIO_FSYN_SPDIF_MD                 AUDIO_FSYN2_MD
#define AUDIO_FSYN_SPDIF_PE                 AUDIO_FSYN2_PE
#define AUDIO_FSYN_SPDIF_SDIV               AUDIO_FSYN2_SDIV
#define AUDIO_FSYN_SPDIF_PROG_EN            AUDIO_FSYN2_PROG_EN
#endif

#if defined (ST_7200)
#define AUDIO_ADAC0_CTRL                    (AUDIO_CONFIG_BASE+0x400)
#define AUDIO_ADAC1_CTRL                    (AUDIO_CONFIG_BASE+0x500)
#define AUDIO_ADAC_CTRL                     AUDIO_ADAC1_CTRL
#else
#define AUDIO_ADAC_CTRL                     (AUDIO_CONFIG_BASE+0x100)
#endif

#define AUDIO_IO_CONTROL_REG                (AUDIO_CONFIG_BASE+0x200)

#if defined (ST_OSLINUX)
#define SET_REG_VAL(Register, Value) iowrite32((U32)(Value), (void *)((U32)(Register)))
#define GET_REG_VAL(Register) ioread32((void *)((U32)(Register)))
#define RETURN_FAIL -1
#define RETURN_SUCCESS 0
#else
#define SET_REG_VAL(Register, Value) *(volatile U32 *)(Register) = (U32)(Value)
#define GET_REG_VAL(Register) *(volatile U32 *)(Register)
#define RETURN_FAIL
#define RETURN_SUCCESS
#endif

#if !defined (ST_OSLINUX)
#define SPDIF_STREAM                        "../../streams/spdif_48_stream.ac3"
#define PCM_STREAM                          "../../streams/billie_jean.pcm"
#else
#define SPDIF_STREAM                        "spdif_48_stream.ac3"
#define PCM_STREAM                          "billie_jean.pcm"
#endif

#define MAX_FILE_SIZE                       (1024*1024*7)
#define FILE_BLOCK_SIZE                     (500*1024)	 /* Will read 500 KB of data in a go */
#define WORD_SIZE                           1
#define FILE_READ                           1024  	     /* 1024 just to indicate by printing 1Kb of data extracted from file */

#define SPDIF_NODE_COUNT                    600

#if defined (ST_7200)
#define SPDIF_PLAYER_BUFFER_ADDRESS         0xB8300000
#define PCM_PLAYER_0_BUFFER_ADDRESS         0xB8500000
#define PCM_PLAYER_1_BUFFER_ADDRESS         0xB8C00000
#else
#define SPDIF_PLAYER_BUFFER_ADDRESS         0xA7000000   /* LMI SPDIF Player audio buffer base address */
#define PCM_PLAYER_0_BUFFER_ADDRESS         0xA6000000   /* LMI PCM Player #0 audio buffer base address */
                                                         /* 0xA553f8a0 to 0xA563f8a0 lies within the system partition,
                                                            so changed from 0xA5500000 to 0xA6000000 */
#define PCM_PLAYER_1_BUFFER_ADDRESS         0xA6800000   /* LMI PCM Player #1 audio buffer base address */
#endif

#define SPDIF_AC3_REPITION_TIME				1536
#define SPDIF_DTS1_REPITION_TIME		    512
#define SPDIF_MPEG1_2_L1_REPITION_TIME		384
#define SPDIF_MPEG1_2_L2_L3_REPITION_TIME	1152

typedef enum TestType_e
{
    SPDIF_PLAYER_ONLY,
    PCM_PLAYER_0_ONLY,
    PCM_PLAYER_1_ONLY,
    ALL_PLUS_MTOM
}TestType_t;

/* PCM Player format register bits */
typedef enum _pcmplayer_format_bitspersubframe
{
	subframebit_32,
	subframebit_16
}pcmplayer_format_bitspersubframe;

typedef enum _pcmplayer_format_datasize
{
	bit_24,
	bit_20,
	bit_18,
	bit_16
}pcmplayer_format_datasize;

typedef enum _pcmplayer_format_lrclockpolarity
{
	leftword_lrclklow,
	leftword_lrclkhigh
}pcmplayer_format_lrclockpolarity;

typedef enum _pcmplayer_format_sclkedge
{
	risingedge_pcmsclk,
	fallingedge_pcmsclk
}pcmplayer_format_sclkedge;

typedef enum _pcmplayer_format_padding
{
	delay_data_onebit_clk,
	nodelay_data_bit_clk
}pcmplayer_format_padding;

typedef enum _pcmplayer_format_align
{
	data_first_sclkcycle_lrclk,
	data_last_sclkcycle_lrclk
}pcmplayer_format_align;

typedef enum _pcmplayer_format_order
{
	data_lsbfirst,
	data_msbfirst
}pcmplayer_format_order;

typedef struct _pcmplayer_format
{
	pcmplayer_format_bitspersubframe bits_per_subframe;
	pcmplayer_format_datasize data_size;
	pcmplayer_format_lrclockpolarity lr_clock_polarity;
	pcmplayer_format_sclkedge sclk_edge;
	pcmplayer_format_padding padding;
	pcmplayer_format_align align;
	pcmplayer_format_order order;
}pcmplayer_format;

/* PCM Player control register bits */
typedef enum _pcmplayer_control_operation
{
	pcm_off,
	pcm_mute,
	pcm_mode_on_16,
	cd_mode_on
}pcmplayer_control_operation;

typedef enum _pcmplayer_control_memorystorageformat
{
	bits_16_0,
	bits_16_16
}pcmplayer_control_memorystorageformat;

typedef enum _pcmplayer_control_rounding
{
	no_rounding_pcm,
	bit16_rounding_pcm
}pcmplayer_control_rounding;

typedef enum _pcmplayer_control_divider64
{
	pcm_sclk_64=0,
	fs_128_64=1,
	fs_256_64=2,
	fs_384_64=3,
	fs_512_64=4,
	fs_768_64=6
}pcmplayer_control_divider64;

typedef enum _pcmplayer_control_divider32
{
	pcm_sclk_32=0,
	fs_128_32=2,
	fs_192_32=3,
	fs_256_32=4,
	fs_384_32=6,
	fs_512_32=8,
	fs_768_32=12
}pcmplayer_control_divider32;

typedef enum _pcmplayer_control_waitspdiflatency
{
	donot_wait_spdif_latency,
	wait_spdif_latency
}pcmplayer_control_waitspdiflatency;

typedef struct _pcmplayer_control
{
	pcmplayer_control_operation operation;
	pcmplayer_control_memorystorageformat memory_storage_format;
	pcmplayer_control_rounding rounding;
	pcmplayer_control_divider64 divider64;
	pcmplayer_control_divider32 divider32;
	pcmplayer_control_waitspdiflatency wait_spdif_latency;
	U32 samples_read;
}pcmplayer_control;

/* SPDIF Player control register bits */
typedef enum _spdifplayer_control_operation
{
	spdif_off,
	spdif_pcm_null_mute,
	spdif_pause_burst_mute,
	spdif_audio_data,
	spdif_encoded_data
}spdifplayer_control_operation;

typedef enum _spdifplayer_control_idlestate
{
	idle_false,
	idle_true
}spdifplayer_control_idlestate;

typedef enum _spdifplayer_control_rounding
{
	no_rounding_spdif,
	bit16_rounding_spdif
}spdifplayer_control_rounding;

typedef enum _spdifplayer_control_divider64
{
	spdif_sclk_64=0,
	fs1_128_64=1,
	fs1_256_64=2,
	fs1_384_64=3,
	fs1_512_64=4,
	fs1_768_64=6
}spdifplayer_control_divider64;

typedef enum _spdifplayer_control_divider32
{
	spdif_sclk_32=0,
	fs1_128_32=2,
	fs1_192_32=3,
	fs1_256_32=4,
	fs1_384_32=6,
	fs1_512_32=8,
	fs1_768_32=12
}spdifplayer_control_divider32;

typedef enum _spdifplayer_control_byteswap
{
	nobyte_swap_encoded_mode,
	byte_swap_encoded_mode
}spdifplayer_control_byteswap;

typedef enum _spdifplayer_control_stuffing_hard_soft
{
	stuffing_software_encoded_mode,
	stuffing_hardware_encoded_mode
}spdifplayer_control_stuffing_hard_soft;

typedef struct _spdifplayer_control
{
	spdifplayer_control_operation operation;
	spdifplayer_control_idlestate idlestate;
	spdifplayer_control_rounding rounding;
	spdifplayer_control_divider64 divider64;
	spdifplayer_control_divider32 divider32;
	spdifplayer_control_byteswap byteswap;
	spdifplayer_control_stuffing_hard_soft stuffing_hard_soft;
	U32 samples_read;
}spdifplayer_control;

void audiopaced_RunAudioPacedTest(void);

/* Stream type */
typedef enum StreamType_e
{
    SPDIF_COMPRESSED,
    PCM_TEN_CHANNEL,
    PCM_TWO_CHANNEL
}StreamType_t;

/* Frequency Synthesiser data structures */
typedef enum _frequency_synthesizer_frequencies
{   FREQ_256_8MHz,
    FREQ_256_11MHz,
    FREQ_256_12MHz,
    FREQ_256_16MHz,
    FREQ_256_22MHz,
    FREQ_256_24MHz,
    FREQ_256_32MHz,
    FREQ_256_44MHz,
    FREQ_256_48MHz,
    FREQ_256_64MHz,
	FREQ_256_88MHz,
	FREQ_256_96MHz,
	FREQ_256_192MHz,
	TotalFrequencies
}frequency_synthesiser_frequencies;

typedef struct FrequencySynthesizerClockParameters_s
{
    int  SDIV_Val;
	int  PE_Val;
	int  MD_Val;
}FrequencySynthesizerClockParameters_t;

#endif
