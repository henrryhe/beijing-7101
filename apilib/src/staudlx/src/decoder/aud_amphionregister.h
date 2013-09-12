/********************************************************************************
 *   Filename   	: aud_amphionregister.h										*
 *   Start       	: 15.06.2007                                                *
 *   By          	: Rahul Bansal											    *
 *   Contact     	: rahul.bansal@st.com										*
 *   Description  	: The file contains register address and macros for amphion *
 *                    ip.                                                       *
 *				                    											*
 ********************************************************************************
 */

#ifndef __AMPHIONREG_H__
#define __AMPHIONREG_H__

/*
=============================================================================
Audio decoder registers AudioBaseAddress 0x20700000.
=============================================================================
*/

#define AUDIO_BASE_ADDRESS		0x20700000

#define AUD_DEC_CONFIG		0x000

#define AUD_DEC_STATUS		0x004
// Decoder status bits
#define	AUD_SW_RESET 		0x02
#define AUD_FRAME_LOCKED	0x04
#define AUD_CRC_ERROR		0x08
#define AUD_ORIGINAL		0x10
#define AUD_EMPHASIS		0x60
#define AUD_VERSION_ID		0x80
#define AUD_LAYER_ID		0x300
#define AUD_BITRATE_INDEX	0x3C00
#define AUD_SAMPFREQ_INDEX	0xC000
#define AUD_AUDIO_MODE		0x30000
#define AUD_MODE_EXTN		0xC0000
#define AUD_COPY_RIGHT		0x100000
#define AUD_CDFIFO_STATUS	0x7100000
#define AUD_PCMFIFO_STATUS	0xF0000000

#define AUD_DEC_INT_ENABLE		0x008
// Interrupt enable bits
enum
{
    AUD_GLOBAL_ENB_INT         =0x01,
    AUD_SW_RESET_ENB_INT       =0x02, /* Interrut After the Soft Reset */
    AUD_FRAME_LOCKED_ENB_INT   =0x04,
    AUD_CRC_ERROR_ENB_INT      =0x08,
    AUD_ORIGINAL_ENB_INT       =0x10,
    AUD_EMPHASIS_ENB_INT       =0x20,
    AUD_CDFIFO_EMPTY_ENB_INT   =0x40,
    AUD_PCMFIFO_FULL_ENB_INT   =0x80,
    AUD_FS_CHANGE_ENB_INT      =0x100
};

#define AUD_DEC_INT_STATUS       	0x00C
// Interrupt enable bits
enum
{
    AUD_INT_PENDING        =0x01,
    AUD_SW_RESET_INT       =0x02,
    AUD_FRAME_LOCKED_INT   =0x04,
    AUD_CRC_ERROR_INT      =0x08,
    AUD_ORIGINAL_INT       =0x10,
    AUD_EMPHASIS_INT       =0x20,
    AUD_CDFIFO_EMPTY_INT   =0x40,
    AUD_PCMFIFO_FULL_INT   =0x80,
    AUD_FS_CHANGE_INT      =0x100
};




#define AUD_DEC_INT_CLEAR		0x010
// Interrupt clear bits
enum
{
    CLR_SW_RESET_INT        =0x02,
    CLR_FRAME_LOCKED_INT    =0x04,
    CLR_CRC_ERROR_INT       =0x08,
    CLR_ORIGINAL_INT        =0x10,
    CLR_EMPHASIS_INT        =0x20,
    CLR_CDFIFO_EMPTY_INT    =0x40,
    CLR_PCMFIFO_FULL_INT    =0x80,
    CLR_FS_CHANGE_INT       =0x100
};

#define AUD_DEC_VOLUME_CONTROL	0x014
#define AUD_DEC_CD_IN			0x040
#define AUD_DEC_PCM_OUT			0x060

#endif /* __AMPHIONREG_H__ */



