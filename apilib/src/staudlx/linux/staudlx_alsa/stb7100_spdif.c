
/*
 *  ALSA Driver on the top of STAUDLX
 *  Copyright (c)   (c) 2006 STMicroelectronics Limited
 *
 *  *  Authors:  Udit Kumar <udit-dlh.kumar@st.com>
 *
 *   This program is interface between STAUDLX and ALSA application. 
 * For ALSA application, this will serve as ALSA driver but at the lower layer, it will map ALSA lib calls to STAULDX calls
 */


#include <asm/dma.h>




/*
 * IEC60958 channel status and format handling for SPDIF and I2S->SPDIF
 * protocol converters
 */
void iec60958_default_channel_status(pcm_hw_t *chip)
{
	chip->default_spdif_control.channel.status[0]  = (IEC958_AES0_CON_NOT_COPYRIGHT |
							  IEC958_AES0_CON_EMPHASIS_NONE);

	chip->default_spdif_control.channel.status[1] |= (IEC958_AES1_CON_NON_IEC908_DVD |
							  IEC958_AES1_CON_ORIGINAL) ;


	chip->default_spdif_control.channel.status[2] |= (IEC958_AES2_CON_SOURCE_UNSPEC |
							  IEC958_AES2_CON_CHANNEL_UNSPEC);

	chip->default_spdif_control.channel.status[3] |= (IEC958_AES3_CON_FS_44100 |
							  IEC958_AES3_CON_CLOCK_VARIABLE);

	chip->default_spdif_control.channel.status[4]  = (IEC958_AES4_CON_WORDLEN_MAX_24 |
							  IEC958_AES4_CON_WORDLEN_24_20);

	memset(chip->default_spdif_control.user,      0x0,sizeof(u8)*48);
	memset(chip->default_spdif_control.validity_l,0x0,sizeof(u8)*24);
	memset(chip->default_spdif_control.validity_r,0x0,sizeof(u8)*24);
}



static int snd_iec60958_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_IEC958;
	uinfo->count = 1;
	return 0;
}


static int snd_iec60958_default_get(snd_kcontrol_t * kcontrol,
				    snd_ctl_elem_value_t * ucontrol)
{
	pcm_hw_t *chip = snd_kcontrol_chip(kcontrol);

	ucontrol->value.iec958.status[0] = chip->default_spdif_control.channel.status[0];
	ucontrol->value.iec958.status[1] = chip->default_spdif_control.channel.status[1];
	ucontrol->value.iec958.status[2] = chip->default_spdif_control.channel.status[2];
	ucontrol->value.iec958.status[3] = chip->default_spdif_control.channel.status[3];

	return 0;
}


static int snd_iec60958_default_put(snd_kcontrol_t * kcontrol,
				    snd_ctl_elem_value_t * ucontrol)
{
	pcm_hw_t *chip = snd_kcontrol_chip(kcontrol);
	u32 val, old;
/*Allow to set SPDIF Interface only in STOP/IDLE Mode*/	
	if(chip->State ==STAUD_ALSA_START)
		return 0;
	
	val =  ucontrol->value.iec958.status[0]        |
	      (ucontrol->value.iec958.status[1] << 8)  |
	      (ucontrol->value.iec958.status[2] << 16) |
	      (ucontrol->value.iec958.status[3] << 24);

	old =  chip->default_spdif_control.channel.status[0] 	    |
	      (chip->default_spdif_control.channel.status[1] << 8)  |
	      (chip->default_spdif_control.channel.status[2] << 16) |
	      (chip->default_spdif_control.channel.status[3] << 24);

	if(val == old)
		return 0;


	chip->default_spdif_control.channel = ucontrol->value.iec958;

	return (val != old);
}


static snd_kcontrol_new_t snd_iec60958_default __devinitdata =
{
	.iface =	SNDRV_CTL_ELEM_IFACE_PCM,
	.name =         SNDRV_CTL_NAME_IEC958("",PLAYBACK,DEFAULT),
	.info =		snd_iec60958_info,
	.get =		snd_iec60958_default_get,
	.put =		snd_iec60958_default_put
};


static snd_kcontrol_new_t snd_iec60958_stream __devinitdata =
{
	.iface =	SNDRV_CTL_ELEM_IFACE_PCM,
	.name =         SNDRV_CTL_NAME_IEC958("",PLAYBACK,PCM_STREAM),
	.info =		snd_iec60958_info,
	.get =		snd_iec60958_default_get,
	.put =		snd_iec60958_default_put
};

static int snd_iec60958_maskc_get(snd_kcontrol_t * kcontrol,
				  snd_ctl_elem_value_t * ucontrol)
{
	ucontrol->value.iec958.status[0] = IEC958_AES0_NONAUDIO          |
					   IEC958_AES0_PROFESSIONAL      |
					   IEC958_AES0_CON_NOT_COPYRIGHT |
					   IEC958_AES0_CON_EMPHASIS;

	ucontrol->value.iec958.status[1] = IEC958_AES1_CON_ORIGINAL |
					   IEC958_AES1_CON_CATEGORY;

	ucontrol->value.iec958.status[2] = 0;

	ucontrol->value.iec958.status[3] = 0;
	return 0;
}


static int snd_iec60958_maskp_get(snd_kcontrol_t * kcontrol,
				       snd_ctl_elem_value_t * ucontrol)
{
	ucontrol->value.iec958.status[0] = IEC958_AES0_NONAUDIO     |
					   IEC958_AES0_PROFESSIONAL |
					   IEC958_AES0_PRO_EMPHASIS;

	ucontrol->value.iec958.status[1] = IEC958_AES1_PRO_MODE |
					   IEC958_AES1_PRO_USERBITS;

	return 0;
}


static snd_kcontrol_new_t snd_iec60958_maskc __devinitdata =
{
	.access =	SNDRV_CTL_ELEM_ACCESS_READ,
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		SNDRV_CTL_NAME_IEC958("",PLAYBACK,CON_MASK),
	.info =		snd_iec60958_info,
	.get =		snd_iec60958_maskc_get,
};


static snd_kcontrol_new_t snd_iec60958_mask __devinitdata =
{
	.access =	SNDRV_CTL_ELEM_ACCESS_READ,
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		SNDRV_CTL_NAME_IEC958("",PLAYBACK,MASK),
	.info =		snd_iec60958_info,
	.get =		snd_iec60958_maskc_get,
};


static snd_kcontrol_new_t snd_iec60958_maskp __devinitdata =
{
	.access =	SNDRV_CTL_ELEM_ACCESS_READ,
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =         SNDRV_CTL_NAME_IEC958("",PLAYBACK,PRO_MASK),
	.info =		snd_iec60958_info,
	.get =		snd_iec60958_maskp_get,
};


static int snd_iec60958_raw_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}


static int snd_iec60958_raw_get(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	pcm_hw_t *chip = snd_kcontrol_chip(kcontrol);
	ucontrol->value.integer.value[0] = chip->iec60958_rawmode;
	return 0;
}


static int snd_iec60958_raw_put(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	pcm_hw_t *chip = snd_kcontrol_chip(kcontrol);
	unsigned char old, val;

	/*Allow to set SPDIF Interface only in STOP/IDLE Mode*/	
	if(chip->State ==STAUD_ALSA_START)
		return 0;
	
	//spin_lock_irq(&chip->lock);
	old = chip->iec60958_rawmode;
	val = ucontrol->value.integer.value[0];
	chip->iec60958_rawmode = val;
	//spin_unlock_irq(&chip->lock);
	return old != val;
}


static snd_kcontrol_new_t snd_iec60958_raw __devinitdata = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		SNDRV_CTL_NAME_IEC958("",PLAYBACK,NONE) "RAW",
	.info =		snd_iec60958_raw_info,
	.get =		snd_iec60958_raw_get,
	.put =		snd_iec60958_raw_put
};


static int snd_iec60958_sync_get(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	pcm_hw_t *chip = snd_kcontrol_chip(kcontrol);	
	ucontrol->value.integer.value[0] = chip->SPDIFLatency;
	return 0;
}


static int snd_iec60958_sync_put(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{

	unsigned char old, val;
	pcm_hw_t *chip = snd_kcontrol_chip(kcontrol);
	/*Allow to set SPDIF Interface only in STOP/IDLE Mode*/	

	if(chip->State ==STAUD_ALSA_START)
		return 0;
	
	old = chip->SPDIFLatency;
	val=chip->SPDIFLatency = ucontrol->value.integer.value[0];	
	return old != val;
}


static snd_kcontrol_new_t snd_iec60958_sync __devinitdata = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		SNDRV_CTL_NAME_IEC958("",PLAYBACK,NONE) "PCM Sync",
	.info =		snd_iec60958_raw_info, /* Reuse from the RAW switch */
	.get =		snd_iec60958_sync_get,
	.put =		snd_iec60958_sync_put
};


/*IEC61937 encoding mode status -  when transmitting a surround encoded data
 * stream the repitition period of iec61937 pause bursts and external decode latency
 *  are dependant on stream type*/


typedef struct iec_encoding_mode_tbl {
	char name[30];
	iec_encodings_t id_flag;
}iec_encoding_mode_tbl_t;

static iec_encoding_mode_tbl_t iec_xfer_modes[12]= 
	{
		{"IEC60958"	,ENCODING_IEC60958},
		{"IEC61937_AC3"	,ENCODING_IEC61937_AC3},
		{"IEC61937_DTS1",ENCODING_IEC61937_DTS_1},
		{"IEC61937_DTS2",ENCODING_IEC61937_DTS_2},
		{"IEC61937_DTS3",ENCODING_IEC61937_DTS_3},
		{"IEC61937_MPEG_384",ENCODING_IEC61937_MPEG_384_FRAME},
		{"IEC61937_MPEG_1152",ENCODING_IEC61937_MPEG_1152_FRAME},
		{"IEC61937_MPEG_1024",ENCODING_IEC61937_MPEG_1024_FRAME},
		{"IEC61937_MPEG_2304",ENCODING_IEC61937_MPEG_2304_FRAME},
		{"IEC61937_MPEG_768",ENCODING_IEC61937_MPEG_768_FRAME},
		{"IEC61937_MPEG_2304_LSF",ENCODING_IEC61937_MPEG_2304_FRAME_LSF},
		{"IEC61937_MPEG_768_LSF",ENCODING_IEC61937_MPEG_768_FRAME_LSF},
	};



static int snd_iec_encoding_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = ENCODED_STREAM_TYPES;
	if (uinfo->value.enumerated.item > (ENCODED_STREAM_TYPES-1))
		uinfo->value.enumerated.item = (ENCODED_STREAM_TYPES);
	strcpy(uinfo->value.enumerated.name,iec_xfer_modes[uinfo->value.enumerated.item].name);
	
	return 0;
}

static int snd_iec_encoding_get(snd_kcontrol_t* kcontrol,snd_ctl_elem_value_t* ucontrol)
{
	int i;
	
	for(i=0; i< ENCODED_STREAM_TYPES; i++)
		ucontrol->value.integer.value[i] = iec_xfer_modes[i].id_flag;
		
	return 0;
}

static int snd_iec_encoding_put(	 snd_kcontrol_t * kcontrol,
					 snd_ctl_elem_value_t * ucontrol)
{
	pcm_hw_t *chip = snd_kcontrol_chip(kcontrol);	
	//spin_lock_irq(&chip->lock);
	// Do not Update in case of Started Driver //
	if(chip->State ==STAUD_ALSA_START)
		return 0;
		
	chip->iec_encoding_mode = ucontrol->value.integer.value[0];
	chip->iec60958_rawmode = FALSE;
	//spin_unlock_irq(&chip->lock);
	
	return 0;
}

static snd_kcontrol_new_t snd_iec_encoding __devinitdata = {
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		SNDRV_CTL_NAME_IEC958("",PLAYBACK,NONE)"Encoding",
	.info =		snd_iec_encoding_info, 
	.get =		snd_iec_encoding_get,
	.put =		snd_iec_encoding_put,
};



static int __devinit snd_iec60958_create_controls(pcm_hw_t *chip)
{
	int err;
	snd_kcontrol_t *kctl;

	if(chip->card_data->input_type == STM_DATA_TYPE_IEC60958)
	{
		err = snd_ctl_add(chip->card, snd_ctl_new1(&snd_iec60958_raw, chip));
		if (err < 0)
			return err;

		err = snd_ctl_add(chip->card, snd_ctl_new1(&snd_iec60958_sync, chip));
		if (err < 0)
			return err;
	}

	err = snd_ctl_add(chip->card, kctl = snd_ctl_new1(&snd_iec60958_default, chip));
	if (err < 0)
		return err;

	/*
	 * stream is a copy of default for the moment for application
	 * compatibility, more investigation required
	 */
	err = snd_ctl_add(chip->card, kctl = snd_ctl_new1(&snd_iec60958_stream, chip));
	if (err < 0)
		return err;

	err = snd_ctl_add(chip->card, kctl = snd_ctl_new1(&snd_iec60958_maskc, chip));
	if (err < 0)
		return err;

	/*
	 * Mask is a copy of the consumer mask.
	 */
	err = snd_ctl_add(chip->card, kctl = snd_ctl_new1(&snd_iec60958_mask, chip));
	if (err < 0)
		return err;

	err = snd_ctl_add(chip->card, kctl = snd_ctl_new1(&snd_iec60958_maskp, chip));
	if (err < 0)
		return err;
	
	err = snd_ctl_add(chip->card, kctl = snd_ctl_new1(&snd_iec_encoding,chip));
	if(err < 0)
		return err;
		
	return 0;
}



