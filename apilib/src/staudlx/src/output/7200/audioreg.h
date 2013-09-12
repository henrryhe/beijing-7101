/*****************************************/
/* AUDIO CONFIGURATION REGISTER ADDRESS  */
/*           STB 7100                    */
/*****************************************/

#include "stdevice.h"


// PCM PLAYER #0 CONFIGURATION REGISTERS
# define PCMPLAYER0_BASE		   	 		    PCM_PLAYER_0_BASE_ADDRESS
# define PCMPLAYER0_SOFT_RESET_REG                 PCMPLAYER0_BASE+0x00
# define PCMPLAYER0_FIFO_DATA_REG  		    PCMPLAYER0_BASE+0x04
# define PCMPLAYER0_ITRP_STATUS_REG 	           PCMPLAYER0_BASE+0x08
# define PCMPLAYER0_ITRP_STATUS_CLEAR_REG   PCMPLAYER0_BASE+0x0C
# define PCMPLAYER0_ITRP_ENABLE_REG	  	    PCMPLAYER0_BASE+0x10
# define PCMPLAYER0_ITRP_ENABLE_SET_REG	    PCMPLAYER0_BASE+0x14
# define PCMPLAYER0_ITRP_ENABLE_CLEAR_REG   PCMPLAYER0_BASE+0x18
# define PCMPLAYER0_CONTROL_REG     		    PCMPLAYER0_BASE+0x1C
# define PCMPLAYER0_STATUS_REG			    PCMPLAYER0_BASE+0x20
# define PCMPLAYER0_FORMAT_REG      		           PCMPLAYER0_BASE+0x24
# define PCMPLAYER0_REGION_SIZE              	    (4*1024)

// PCM PLAYER #1 CONFIGURATION REGISTERS
# define PCMPLAYER1_BASE		   	 		    PCM_PLAYER_1_BASE_ADDRESS
# define PCMPLAYER1_SOFT_RESET_REG                 PCMPLAYER1_BASE+0x00
# define PCMPLAYER1_FIFO_DATA_REG  		    PCMPLAYER1_BASE+0x04
# define PCMPLAYER1_ITRP_STATUS_REG 		    PCMPLAYER1_BASE+0x08
# define PCMPLAYER1_ITRP_STATUS_CLEAR_REG   PCMPLAYER1_BASE+0x0C
# define PCMPLAYER1_ITRP_ENABLE_REG	  	    PCMPLAYER1_BASE+0x10
# define PCMPLAYER1_ITRP_ENABLE_SET_REG	    PCMPLAYER1_BASE+0x14
# define PCMPLAYER1_ITRP_ENABLE_CLEAR_REG   PCMPLAYER1_BASE+0x18
# define PCMPLAYER1_CONTROL_REG     		    PCMPLAYER1_BASE+0x1C
# define PCMPLAYER1_STATUS_REG			    PCMPLAYER1_BASE+0x20
# define PCMPLAYER1_FORMAT_REG      	                  PCMPLAYER1_BASE+0x24
# define PCMPLAYER1_REGION_SIZE              	    (4*1024)

// PCM PLAYER #2 CONFIGURATION REGISTERS
# define PCMPLAYER2_BASE		   	 		       PCM_PLAYER_2_BASE_ADDRESS
# define PCMPLAYER2_SOFT_RESET_REG                    PCMPLAYER2_BASE+0x00
# define PCMPLAYER2_FIFO_DATA_REG  		 	PCMPLAYER2_BASE+0x04
# define PCMPLAYER2_ITRP_STATUS_REG 		       PCMPLAYER2_BASE+0x08
# define PCMPLAYER2_ITRP_STATUS_CLEAR_REG 	PCMPLAYER2_BASE+0x0C
# define PCMPLAYER2_ITRP_ENABLE_REG	  	 	PCMPLAYER2_BASE+0x10
# define PCMPLAYER2_ITRP_ENABLE_SET_REG		PCMPLAYER2_BASE+0x14
# define PCMPLAYER2_ITRP_ENABLE_CLEAR_REG	PCMPLAYER2_BASE+0x18
# define PCMPLAYER2_CONTROL_REG     		       PCMPLAYER2_BASE+0x1C
# define PCMPLAYER2_STATUS_REG			 	PCMPLAYER2_BASE+0x20
# define PCMPLAYER2_FORMAT_REG      		              PCMPLAYER2_BASE+0x24
# define PCMPLAYER2_REGION_SIZE              	       (4*1024)

// PCM PLAYER #3 CONFIGURATION REGISTERS
# define PCMPLAYER3_BASE		   	 		       PCM_PLAYER_3_BASE_ADDRESS
# define PCMPLAYER3_SOFT_RESET_REG                    PCMPLAYER3_BASE+0x00
# define PCMPLAYER3_FIFO_DATA_REG  		 	PCMPLAYER3_BASE+0x04
# define PCMPLAYER3_ITRP_STATUS_REG 		       PCMPLAYER3_BASE+0x08
# define PCMPLAYER3_ITRP_STATUS_CLEAR_REG 	PCMPLAYER3_BASE+0x0C
# define PCMPLAYER3_ITRP_ENABLE_REG	  	 	PCMPLAYER3_BASE+0x10
# define PCMPLAYER3_ITRP_ENABLE_SET_REG		PCMPLAYER3_BASE+0x14
# define PCMPLAYER3_ITRP_ENABLE_CLEAR_REG	PCMPLAYER3_BASE+0x18
# define PCMPLAYER3_CONTROL_REG     		       PCMPLAYER3_BASE+0x1C
# define PCMPLAYER3_STATUS_REG			 	PCMPLAYER3_BASE+0x20
# define PCMPLAYER3_FORMAT_REG      		              PCMPLAYER3_BASE+0x24
# define PCMPLAYER3_REGION_SIZE              	       (4*1024)

// HDMI PCM PLAYER CONFIGURATION REGISTERS
# define HDMI_BASE_TVOUT		   	                            ST7200_HDMI_BASE_ADDRESS
# define HDMI_PCMPLAYER_BASE		   	 		        HDMI_BASE_TVOUT+0x0D00
# define HDMI_PCMPLAYER_SOFT_RESET_REG                   HDMI_PCMPLAYER_BASE+0x00
# define HDMI_PCMPLAYER_FIFO_DATA_REG  		 	 HDMI_PCMPLAYER_BASE+0x04
# define HDMI_PCMPLAYER_ITRP_STATUS_REG 		        HDMI_PCMPLAYER_BASE+0x08
# define HDMI_PCMPLAYER_ITRP_STATUS_CLEAR_REG 	 HDMI_PCMPLAYER_BASE+0x0C
# define HDMI_PCMPLAYER_ITRP_ENABLE_REG	  	 	 HDMI_PCMPLAYER_BASE+0x10
# define HDMI_PCMPLAYER_ITRP_ENABLE_SET_REG		 HDMI_PCMPLAYER_BASE+0x14
# define HDMI_PCMPLAYER_ITRP_ENABLE_CLEAR_REG	 HDMI_PCMPLAYER_BASE+0x18
# define HDMI_PCMPLAYER_CONTROL_REG     		        HDMI_PCMPLAYER_BASE+0x1C
# define HDMI_PCMPLAYER_STATUS_REG			 	 HDMI_PCMPLAYER_BASE+0x20
# define HDMI_PCMPLAYER_FORMAT_REG      		        HDMI_PCMPLAYER_BASE+0x24
# define HDMI_PCMPLAYER_REGION_SIZE              	       (40)

// HDMI SPDIF PLAYER CONFIGURATION REGISTERS
# define HDMI_SPDIFPLAYER_BASE		   	 		       HDMI_BASE_TVOUT+0x0C00
# define HDMI_SPDIF_FIFO_DATA_REG       		 	       HDMI_SPDIFPLAYER_BASE+0x04
# define HDMI_SPDIF_ITRP_STATUS_REG                         HDMI_SPDIFPLAYER_BASE+0x08
# define HDMI_SPDIF_ITRP_STATUS_CLEAR_REG		HDMI_SPDIFPLAYER_BASE+0x0C
# define HDMI_SPDIF_ITRP_ENABLE_REG			 	HDMI_SPDIFPLAYER_BASE+0x10
# define HDMI_SPDIF_ITRP_ENABLE_SET_REG		 	HDMI_SPDIFPLAYER_BASE+0x14
# define HDMI_SPDIF_ITRP_ENABLE_CLEAR_REG		HDMI_SPDIFPLAYER_BASE+0x18
# define HDMI_SPDIF_CONTROL_REG         		 	       HDMI_SPDIFPLAYER_BASE+0x1C
# define HDMI_SPDIF_STATUS_REG  			 	    	HDMI_SPDIFPLAYER_BASE+0x20
# define HDMI_SPDIF_PA_PB_REG           		 	       HDMI_SPDIFPLAYER_BASE+0x24
# define HDMI_SPDIF_PC_PD_REG           		 	       HDMI_SPDIFPLAYER_BASE+0x28
# define HDMI_SPDIF_CL1_REG				 		HDMI_SPDIFPLAYER_BASE+0x2C
# define HDMI_SPDIF_CR1_REG				 		HDMI_SPDIFPLAYER_BASE+0x30
# define HDMI_SPDIF_CL2_CR2_UV_REG				HDMI_SPDIFPLAYER_BASE+0x34
# define HDMI_SPDIF_PAUSE_LAT_REG			 	       HDMI_SPDIFPLAYER_BASE+0x38
# define HDMI_SPDIF_FRAMELGTH_BURST_REG	              HDMI_SPDIFPLAYER_BASE+0x3C
# define HDMI_SPDIF_REGION_SIZE              	       	(64)

//PCM READER CONFIGURATION REGISTERS
# define PCMREADER_BASE		   	 			PCM_READER_BASE_ADDRESS
# define PCMREADER_SOFT_RESET_REG          		PCMREADER_BASE+0x00
# define PCMREADER_FIFO_DATA_REG  		 		PCMREADER_BASE+0x04
# define PCMREADER_ITRP_STATUS_REG 		 	PCMREADER_BASE+0x08
# define PCMREADER_ITRP_STATUS_CLEAR_REG 	PCMREADER_BASE+0x0C
# define PCMREADER_ITRP_ENABLE_REG	  	 	PCMREADER_BASE+0x10
# define PCMREADER_ITRP_ENABLE_SET_REG		PCMREADER_BASE+0x14
# define PCMREADER_ITRP_ENABLE_CLEAR_REG		PCMREADER_BASE+0x18
# define PCMREADER_CONTROL_REG     		 		PCMREADER_BASE+0x1C
# define PCMREADER_STATUS_REG			 		PCMREADER_BASE+0x20
# define PCMREADER_FORMAT_REG      		 		PCMREADER_BASE+0x24
# define PCMREADER_REGION_SIZE              	       	(4*1024)

//SPDIF PLAYER CONFIGURATION REGISTERS
# define SPDIFPLAYER_BASE                			SPDIF_PLAYER_BASE_ADDRESS
# define SPDIF_FIFO_DATA_REG       		 	SPDIFPLAYER_BASE+0x04
# define SPDIF_ITRP_STATUS_REG              		SPDIFPLAYER_BASE+0x08
# define SPDIF_ITRP_STATUS_CLEAR_REG		SPDIFPLAYER_BASE+0x0C
# define SPDIF_ITRP_ENABLE_REG			 	SPDIFPLAYER_BASE+0x10
# define SPDIF_ITRP_ENABLE_SET_REG		 	SPDIFPLAYER_BASE+0x14
# define SPDIF_ITRP_ENABLE_CLEAR_REG		SPDIFPLAYER_BASE+0x18
# define SPDIF_CONTROL_REG         		 	SPDIFPLAYER_BASE+0x1C
# define SPDIF_STATUS_REG  			 		SPDIFPLAYER_BASE+0x20
# define SPDIF_PA_PB_REG           		 		SPDIFPLAYER_BASE+0x24
# define SPDIF_PC_PD_REG           		 		SPDIFPLAYER_BASE+0x28
# define SPDIF_CL1_REG				 		SPDIFPLAYER_BASE+0x2C
# define SPDIF_CR1_REG				 		SPDIFPLAYER_BASE+0x30
# define SPDIF_CL2_CR2_UV_REG				SPDIFPLAYER_BASE+0x34
# define SPDIF_PAUSE_LAT_REG			 	SPDIFPLAYER_BASE+0x38
# define SPDIF_FRAMELGTH_BURST_REG	       SPDIFPLAYER_BASE+0x3C
# define SPDIFPLAYER_REGION_SIZE              	(4*1024)

/******************************************************************************************/
/*    Definition of register addresses corresponding to clockGenC frequency synth A/B     */
/******************************************************************************************/

//AUDIO CONFIG BASE REGISTERS
# define AUDIO_CONFIG_BASE				    	AUDIO_IF_BASE_ADDRESS
# define AUDIO_FSYNA_CFG					AUDIO_CONFIG_BASE+0x00
# define AUDIO_FSYNB_CFG	                            AUDIO_CONFIG_BASE+0x100
# define AUDIO_FSYNA0_MD					AUDIO_CONFIG_BASE+0x10		// For PCM player 0
# define AUDIO_FSYNA0_PE					AUDIO_CONFIG_BASE+0x14
# define AUDIO_FSYNA0_SDIV					AUDIO_CONFIG_BASE+0x18
# define AUDIO_FSYNA0_PROG_EN				AUDIO_CONFIG_BASE+0x1C

# define AUDIO_FSYNA1_MD					AUDIO_CONFIG_BASE+0x20		// For PCM player 1
# define AUDIO_FSYNA1_PE					AUDIO_CONFIG_BASE+0x24
# define AUDIO_FSYNA1_SDIV					AUDIO_CONFIG_BASE+0x28
# define AUDIO_FSYNA1_PROG_EN				AUDIO_CONFIG_BASE+0x2C

# define AUDIO_FSYNA2_MD					AUDIO_CONFIG_BASE+0x30		// For PCM player 2
# define AUDIO_FSYNA2_PE					AUDIO_CONFIG_BASE+0x34
# define AUDIO_FSYNA2_SDIV					AUDIO_CONFIG_BASE+0x38
# define AUDIO_FSYNA2_PROG_EN				AUDIO_CONFIG_BASE+0x3C

# define AUDIO_FSYNA3_MD					AUDIO_CONFIG_BASE+0x40		// For PCM player 3
# define AUDIO_FSYNA3_PE					AUDIO_CONFIG_BASE+0x44
# define AUDIO_FSYNA3_SDIV					AUDIO_CONFIG_BASE+0x48
# define AUDIO_FSYNA3_PROG_EN				AUDIO_CONFIG_BASE+0x4C
# define AUDIO_CONFIG_REGION_SIZE           	(4*1024)

# define AUDIO_FSYNB2_MD					AUDIO_CONFIG_BASE+0x130		// For HDMI player (Both PCM and SPDIF player)
# define AUDIO_FSYNB2_PE					AUDIO_CONFIG_BASE+0x134
# define AUDIO_FSYNB2_SDIV					AUDIO_CONFIG_BASE+0x138
# define AUDIO_FSYNB2_PROG_EN				AUDIO_CONFIG_BASE+0x13C

# define AUDIO_FSYNB3_MD					AUDIO_CONFIG_BASE+0x140		// For SPDIF player 0
# define AUDIO_FSYNB3_PE					AUDIO_CONFIG_BASE+0x144
# define AUDIO_FSYNB3_SDIV					AUDIO_CONFIG_BASE+0x148
# define AUDIO_FSYNB3_PROG_EN				AUDIO_CONFIG_BASE+0x14C

/* I/O control of audio_peripheral     */


/*Audio outputs control and Digital Muxing*/
# define AUDIO_IOCTL						AUDIO_CONFIG_BASE+0x200
# define AUDIO_HDMI							AUDIO_CONFIG_BASE+0x204
# define AUD_RECOVERY_CTRL_ADD	              AUDIO_CONFIG_BASE+0x208


/* Audio DAC Control specific registers     */

#define AUDIO_DAC_PCMP1                     		AUDIO_CONFIG_BASE+0x500     //Internal DAC 0 (PCM PLAYER1)
#define AUDIO_DAC_PCMP0                    		AUDIO_CONFIG_BASE+0x400     //Internal DAC 1 (PCM PLAYER0)

#if defined (ST_7200)
	#define SPDIF_ADDITIONAL_DATA_REGION 0x9380 /*check*/
#endif

/*EOF*/
