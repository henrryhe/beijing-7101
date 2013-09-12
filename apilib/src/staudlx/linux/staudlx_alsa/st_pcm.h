/*
 *  ALSA Driver on the top of STAUDLX
 *  Copyright (c)   (c) 2006 STMicroelectronics Limited
 *
 *  *  Authors:  Udit Kumar <udit-dlh.kumar@st.com>
 *
 *   This program is interface between STAUDLX and ALSA application. 
 * For ALSA application, this will serve as ALSA driver but at the lower layer, it will map ALSA lib calls to STAULDX calls
 */

#ifndef _PCM_PLAYER_HW_H
#define _PCM_PLAYER_HW_H

#include <sound/asound.h>
#include "staudlx.h"
#ifdef __cplusplus
extern "C" {
#endif


/*make some named definitions to refer to each output device so we dont need to rely on the card number*/
/*card majors*/
#define PCM0_DEVICE			0
#define PCM1_DEVICE			1
#define SPDIF_DEVICE			2
#define CAPTURE_DEVICE		3
#define PROTOCOL_CONVERTER_DEVICE	3
/*card minors*/
#define MAIN_DEVICE			0

struct pcm_hw_t;


typedef enum {
	STM_DATA_TYPE_LPCM,
	STM_DATA_TYPE_IEC60958
} stm_snd_data_type_t;


typedef struct {
        int                  major;
        int                  minor;
        stm_snd_data_type_t  input_type;
        stm_snd_data_type_t  output_type;
        snd_card_t          *device;
        int                  in_use;
} stm_snd_output_device_t;

// Need to Check #define PCMP_MAX_SAMPLES		(104000)

#define PCMP_MAX_SAMPLES		(104000) 
#define PCM_SAMPLE_SIZE_BYTES		4


#define PCM_MAX_FRAMES			4096 // for Audio Driver 48000 /* 1s @ 48KHz */
#define CAPTURE_MAX_FRAMES		3072
/* Note: we cannot preallocate a buffer from ALSA if we want to
 * use bigphysmem for large buffers and the standard page
 * alocation for small buffers. The preallocation is spotted by
 * the generic ALSA driver layer and the size is used to limit
 * the buffer size requests before they even get to this driver.
 * This overrides the buffer_bytes_max value in the hardware
 * capabilities structure we set up.
 */
#define PCM_PREALLOC_SIZE		0
#define PCM_PREALLOC_MAX		0


/*
 * Buffers larger than 128k should come from bigphysmem to avoid
 * page fragmentation and random resource starvation in the rest of
 * the system. Buffers <=128k come from the ALSA dma memory allocation
 * system, which uses get_free_pages directly, not a slab memory cache. 
 */
#define PCM_BIGALLOC_SIZE		(128*1024)
					   
#define FRAMES_TO_BYTES(x,channels) (( (x) * (channels) ) * PCM_SAMPLE_SIZE_BYTES)
#define BYTES_TO_FRAMES(x,channels) (( (x) / (channels) ) / PCM_SAMPLE_SIZE_BYTES)




#define IEC958_AES1_CON_NON_IEC908_DVD (IEC958_AES1_CON_LASEROPT_ID|0x018)
/*
 * Extension to the ALSA channel status definitions for consumer mode 24bit
 * wordlength.
 */
#define IEC958_AES4_CON_WORDLEN_MAX_24 (1<<0)
#define IEC958_AES4_CON_WORDLEN_24_20  (5<<1)

typedef enum iec_encodings {
	ENCODING_IEC60958 = 0,
	ENCODING_IEC61937_AC3,
	ENCODING_IEC61937_DTS_1,
	ENCODING_IEC61937_DTS_2,
	ENCODING_IEC61937_DTS_3,
	ENCODING_IEC61937_MPEG_384_FRAME,
	ENCODING_IEC61937_MPEG_1152_FRAME,
	ENCODING_IEC61937_MPEG_1024_FRAME,
	ENCODING_IEC61937_MPEG_2304_FRAME,
	ENCODING_IEC61937_MPEG_768_FRAME,
	ENCODING_IEC61937_MPEG_2304_FRAME_LSF,
	ENCODING_IEC61937_MPEG_768_FRAME_LSF,
}iec_encodings_t;


typedef struct IEC60958 {
	/* Channel status bits are the same for L/R subframes */
       	snd_aes_iec958_t  channel;

        /* Validity bits can be different on L and R e.g. in
         * professional applications
         */
        char                validity_l[24];
        char                validity_r[24];
        /* User bits are considered contiguous across L/R subframes */
        char                 user[48];
}IEC60958_t;

#define ENCODED_STREAM_TYPES  12
typedef struct {
	int			(*free_device)     (struct pcm_hw_t *card);
	int			(*open_device)     (snd_pcm_substream_t *substream);
	int			(*program_hw)      (snd_pcm_substream_t *substream);
	snd_pcm_uframes_t	(*playback_pointer)(snd_pcm_substream_t *substream);

	void			(*start_playback)  (snd_pcm_substream_t *substream);
	void			(*stop_playback)   (snd_pcm_substream_t *substream);
	void			(*pause_playback)  (snd_pcm_substream_t *substream);
	void			(*unpause_playback)(snd_pcm_substream_t *substream);
} stm_playback_ops_t;

typedef enum
{
 	STAUD_ALSA_IDLE,
	 STAUD_ALSA_START,
	 STAUD_ALSA_STOP
 	
} STAUDALSA_State_t; 
typedef enum
{
	LEFTCHANNEL,
	RIGHTCHANNEL, 
	MAXCHANNEL
}STAUDLX_ALSA_Channel_t;

typedef struct pcm_hw_t {
	snd_card_t		*card;

//	spinlock_t		lock;
	int			irq;
        stm_snd_output_device_t *card_data;
	unsigned long		buffer_start_addr;
	unsigned long		pcmplayer_control;
	unsigned long		irq_mask;
	snd_pcm_hardware_t      hw;
	snd_pcm_t *		pcm;
	BOOL			Copy; 
	snd_pcm_uframes_t    	hwbuf_current_addr;
	snd_pcm_substream_t 	*current_substream;
	char		   	*out_pipe;
	char		    	*pcm_clock_reg;
	char 			*pcm_player;
	char                    *pcm_converter;
	int		     	are_clocks_active;
	int                     oversampling_frequency;

	stm_playback_ops_t	*playback_ops;
	STAUDALSA_State_t	State;

	int                     iec60958_output_count;
	char			iec60958_rawmode;
	char 			iec_encoding_mode;
	char				SPDIFLatency; 	
	 IEC60958_t              current_spdif_control;
        IEC60958_t              pending_spdif_control;
        IEC60958_t              default_spdif_control;
	
	/*Needed for STAULDX*/
	STAUD_Handle_t		AudioHandle;
	U8*					WritePointer;
	U8*					ReadPointer;
	U8*					BaseAddress;
	U8*					TopAddress;
	U8*					PrevReadPointer;
	unsigned int			SizeOfBuffer;
	ST_DeviceName_t 			EvtDeviceName;
	ST_DeviceName_t 			ClockDeviceName;
	unsigned int 				NumberOfBytesCopied;
	unsigned int 				NumberUsed;
	unsigned char				Volume[MAXCHANNEL];
/*New Variables for LDDE 2.2*/
	U32 min_ch;
	U32 max_ch;

	

} pcm_hw_t;


#define PCM0_SYNC_ID		2
#define PCM1_SYNC_ID		4
#define SPDIF_SYNC_MODE_ON	1

#define chip_t pcm_hw_t

static int snd_pcm_dev_free(snd_device_t *dev);

static int __devinit snd_card_pcm_allocate(pcm_hw_t *chip, int device,char* name);
static int __devinit snd_card_capture_allocate(pcm_hw_t *chip, int device,char* name);
static int __devinit snd_generic_create_controls(pcm_hw_t *chip);
void iec60958_default_channel_status(pcm_hw_t * chip);

static int __devinit  snd_iec60958_create_controls(pcm_hw_t *chip);




#define DEBUG_PRINT(_x)  
#define DEBUG_CAPTURE_CALLS(_x)
#define DEBUG_PRINT_FN_CALL(_x) 

#ifdef __cplusplus
}
#endif

#endif  /* _ST_PCM_H */
