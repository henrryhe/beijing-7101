/*******************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

File name   : aud_test.c
Description : Minimal set of audio commands
Note        : All functions return TRUE if error for CLILIB compatibility
 *
Date          Modification                                      Initials
----          ------------                                    --------
25 Mar 2003   Creation                                        CL

*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* For us: stpti4 = stpti -> we use stpti */
#ifndef ST_7710
#ifdef DVD_TRANSPORT_STPTI4
#if !defined DVD_TRANSPORT_STPTI
#define DVD_TRANSPORT_STPTI
#endif
#endif
#endif



/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/
#include <stdio.h>
#include <string.h>
#if (!defined (ST_7100)) && (!defined (ST_7109)) && (!defined(ST_7200))
#include <debug.h>
#endif

#include "stddefs.h"
#include "stlite.h"

#ifdef ST_OSLINUX
#include "compat.h"
/*#else*/
#endif

#include "sttbx.h"
#if defined(mb290)
#define USE_7020_GENERIC_ADDRESSES
#endif

#include "stdevice.h"
#include "stsys.h"

#include "stcommon.h"

#ifdef DVD_TRANSPORT_STPTI
#include "stpti.h"
#else
#include "pti.h"
#endif

#if defined STAUD_FIX_PTI_LINK
#include "ptilink.h"
#endif

#if defined (ST_5514)
#include "stgpdma.h"
#endif

#if defined (ST_5517)
#include "stfdma.h"
#endif

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
#if defined ST_OSLINUX || defined ST_OSWINCE
#include "staudlt.h"
#else
#include "../../dvdbr-prj-staudio/staudio.h"
#endif
#include "stfdma.h"

#else
#include "staud.h"
#endif

#include "stevt.h"

#include "testtool.h"

#include "api_cmd.h"
#include "clevt.h"
#include "clclkrv.h"
#if defined(ST_7020) || defined (ST_4629)
#include "clintmr.h"
#endif

#include "aud_cmd.h"




/*#######################################################################*/
/*############################ CONSTANTS ################################*/
/*#######################################################################*/

#define MAX_PCMBUFFERS 5

#if defined(ST_7015) || defined(ST_7020)
#ifndef AUD_BASE_ADDRESS_4600
#define AUD_BASE_ADDRESS_4600   0x70600000
#endif
#define AUD1_BASE_ADDRESS_4600  0x00000000
#ifndef EXTERNAL_1_INTERRUPT
#define EXTERNAL_1_INTERRUPT    ST5514_EXTERNAL1_INTERRUPT
#endif
#endif

#ifndef AUDIO_IF_BASE_ADDRESS
#if defined (mb290)
#define AUDIO_IF_BASE_ADDRESS           ST5514_AUDIO_IF_BASE_ADDRESS
#else
#define AUDIO_IF_BASE_ADDRESS NULL
#endif
#endif
#if defined (ST_7100) || defined (ST_7109) || defined(ST_7200)
#define STEVT_HANDLER                       STEVT_DEVICE_NAME
#define STCLKRV_HANDLER_NAME                "STCLKRV0"
#define MAX_STRING_LENGTH                   120

#ifdef ST_OSWINCE
# define player_buff_address                0xA7A00000 //LMI(PLAYER) audio buffer base address
#else
#define player_buff_address                 0xA5000000      /*LMI(PLAYER) audio buffer base address*/
#endif

# define MAX_FILE_SIZE                      1024*1024*4     /*1 samples is of 16bit data (file to read),
                                                             hence 131072 samples = 131072*16 = 2097152bit = 262144Bytes = 256KBytes*/
# define FILE_BLOCK_SIZE                    500*1024        /*Will read 50 KB of data in a go*/
# define FILE_READ                          1024            /*1024 just to indicate by printing 1Kb of data extracted from file*/
# define BYTES                              1
# define FILE_BLOCK_SIZE_SPDIF              192*4           /*192 IEC BLOCKS and Each Blocks = 192 Frames, 1 Frame = 4Bytes of actual data*/
# define pcmreader_numbits                  2*1024*1024

# if defined(ST_7200)
# define PCMPLAYER0_BASE                    ST7200_PCM_PLAYER_BASE_ADDRESS
# elif defined(ST_7109)
# define PCMPLAYER0_BASE                    ST7109_PCM_PLAYER_BASE_ADDRESS /*Fazio: 0xB8101000*/
# elif defined(ST_7100)
# define PCMPLAYER0_BASE                    ST7100_PCM_PLAYER_BASE_ADDRESS /*Fazio: 0xB8101000*/
# endif /* ST_7200 */
# define PCMPLAYER0_SOFT_RESET_REG          PCMPLAYER0_BASE+0x00
# define PCMPLAYER0_FIFO_DATA_REG  		 	PCMPLAYER0_BASE+0x04
# define PCMPLAYER0_ITRP_STATUS_REG 		PCMPLAYER0_BASE+0x08
# define PCMPLAYER0_ITRP_STATUS_CLEAR_REG 	PCMPLAYER0_BASE+0x0C
# define PCMPLAYER0_ITRP_ENABLE_REG	  	 	PCMPLAYER0_BASE+0x10
# define PCMPLAYER0_ITRP_ENABLE_SET_REG		PCMPLAYER0_BASE+0x14
# define PCMPLAYER0_ITRP_ENABLE_CLEAR_REG	PCMPLAYER0_BASE+0x18
# define PCMPLAYER0_CONTROL_REG     		PCMPLAYER0_BASE+0x1C
# define PCMPLAYER0_STATUS_REG			 	PCMPLAYER0_BASE+0x20
# define PCMPLAYER0_FORMAT_REG              PCMPLAYER0_BASE+0x24

# if defined(ST_7200)
# define AUDIO_CONFIG_CLKG_C                ST7200_AUDIO_IF_BASE_ADDRESS   /* Base address for Audio configuration + generating clocks for AUDIO IP's*/
# elif defined(ST_7109)
# define AUDIO_CONFIG_CLKG_C                ST7109_AUDIO_IF_BASE_ADDRESS   /*Fazio: 0xB9210000*//* Base address for Audio configuration + generating clocks for AUDIO IP's*/
# elif defined(ST_7100)
# define AUDIO_CONFIG_CLKG_C                ST7100_AUDIO_IF_BASE_ADDRESS   /*Fazio: 0xB9210000*//* Base address for Audio configuration + generating clocks for AUDIO IP's*/
# endif /* ST_7200 */
# define AUDIO_DAC_PCMP1                    AUDIO_CONFIG_CLKG_C+0x100      /* Address to configure the Audio DAC PCMP#1*/
# define PCMCLK_IOCTL                       AUDIO_CONFIG_CLKG_C+0x200      /* Address to Enable PCMCLK,PCMPlayer#0,PCMPlayer#1 Data*/
# define AUDIO_FSYN_CFG                     AUDIO_CONFIG_CLKG_C+0x00       /* Address to Configure the Frequency Synthesizer of Audio IP's*/
# define AUDIO_SPDIF_HDMI                   AUDIO_CONFIG_CLKG_C+0x204      /* Address to configure the spdif output on HDMI block */

# define AUDIO_FSYN0_MD                     AUDIO_CONFIG_CLKG_C+0x10
# define AUDIO_FSYN0_PE                     AUDIO_CONFIG_CLKG_C+0x14
# define AUDIO_FSYN0_SDIV                   AUDIO_CONFIG_CLKG_C+0x18
# define AUDIO_FSYN0_PROG_EN                AUDIO_CONFIG_CLKG_C+0x1C

# define AUDIO_FSYN1_MD                     AUDIO_CONFIG_CLKG_C+0x20
# define AUDIO_FSYN1_PE                     AUDIO_CONFIG_CLKG_C+0x24
# define AUDIO_FSYN1_SDIV                   AUDIO_CONFIG_CLKG_C+0x28
# define AUDIO_FSYN1_PROG_EN                AUDIO_CONFIG_CLKG_C+0x2C

# define AUDIO_FSYN2_MD                     AUDIO_CONFIG_CLKG_C+0x30
# define AUDIO_FSYN2_PE                     AUDIO_CONFIG_CLKG_C+0x34
# define AUDIO_FSYN2_SDIV                   AUDIO_CONFIG_CLKG_C+0x38
# define AUDIO_FSYN2_PROG_EN                AUDIO_CONFIG_CLKG_C+0x3C

//SPDIF PLAYER CONFIGURATION REGISTERS
# if defined(ST_7200)
# define SPDIF_BASE                         ST7200_SPDIF_PLAYER_BASE_ADDRESS
# elif defined(ST_7109)
# define SPDIF_BASE                         ST7109_SPDIF_PLAYER_BASE_ADDRESS /* Fazio: 0xB8103000*/
# elif defined(ST_7100)
# define SPDIF_BASE                         ST7100_SPDIF_PLAYER_BASE_ADDRESS /* Fazio: 0xB8103000*/
# endif /* ST_7200 */
# define SPDIF_FIFO_DATA_REG       		 	SPDIF_BASE+0x04
# define SPDIF_ITRP_STATUS_REG              SPDIF_BASE+0x08
# define SPDIF_ITRP_STATUS_CLEAR_REG		SPDIF_BASE+0x0C
# define SPDIF_ITRP_ENABLE_REG			 	SPDIF_BASE+0x10
# define SPDIF_ITRP_ENABLE_SET_REG		 	SPDIF_BASE+0x14
# define SPDIF_ITRP_ENABLE_CLEAR_REG		SPDIF_BASE+0x18
# define SPDIF_CONTROL_REG         		 	SPDIF_BASE+0x1C
# define SPDIF_STATUS_REG  			 		SPDIF_BASE+0x20
# define SPDIF_PA_PB_REG           		 	SPDIF_BASE+0x24
# define SPDIF_PC_PD_REG           		 	SPDIF_BASE+0x28
# define SPDIF_CL1_REG				 		SPDIF_BASE+0x2C
# define SPDIF_CR1_REG				 		SPDIF_BASE+0x30
# define SPDIF_CL2_CR2_UV_REG				SPDIF_BASE+0x34
# define SPDIF_PAUSE_LAT_REG			 	SPDIF_BASE+0x38
# define SPDIF_FRAMELGTH_BURST_REG          SPDIF_BASE+0x3C

#define SPDIF_AC3_REPITION_TIME             1536
#define SPDIF_DTS1_REPITION_TIME			512
#define SPDIF_DTS2_REPITION_TIME			1024
#define SPDIF_DTS3_REPITION_TIME			2048
#define SPDIF_MPEG1_2_L1_REPITION_TIME		384
#define SPDIF_MPEG1_2_L2_L3_REPITION_TIME   1152

#ifdef ST_OSWINCE
# define player_buff_address_spdif   0xA7A00000 //LMI(PLAYER) audio buffer base address
#else
# define player_buff_address_spdif   0xA6000000   //LMI(PLAYER) audio buffer base address
#endif

#endif

#define K_BYTE                  1024
#define M_BYTE                  (K_BYTE * K_BYTE)
#if defined (mb391)   /* TBD */
#ifdef STAUD_EMULATOR
#define SYSTEM_MEMORY_SIZE      (  4 * M_BYTE)  /* for audio streams */
#define NCACHE_MEMORY_SIZE      (  1 * M_BYTE)
#define DVD_PLATFORM_STRING     "mb391"
#else
#define SYSTEM_MEMORY_SIZE      (  7 * M_BYTE)  /* for audio streams */
#define NCACHE_MEMORY_SIZE      (  5 * M_BYTE)
#define DVD_PLATFORM_STRING     "mb391"
#endif /* STAUD_EMULATOR */
#endif

#if defined (mb391)   /* TBD */
#ifdef STAUD_EMULATOR
#define SYSTEM_MEMORY_SIZE      (  4 * M_BYTE)  /* for audio streams */
#define NCACHE_MEMORY_SIZE      (  1 * M_BYTE)
#define DVD_PLATFORM_STRING     "mb391"
#else
#define SYSTEM_MEMORY_SIZE      (  7 * M_BYTE)  /* for audio streams */
#define NCACHE_MEMORY_SIZE      (  5 * M_BYTE)
#define DVD_PLATFORM_STRING     "mb391"
#endif /* STAUD_EMULATOR */
#endif

#if defined(ST_OS21)
#define AUDIO_MEMORY_SIZE       ( 128 * K_BYTE)   /* audio driver partition */
#else
#define AUDIO_MEMORY_SIZE       ( 64 * K_BYTE)   /* audio driver partition */
#endif

#define PTI_BUFFER_SIZE       (8 * 1024)
#define PTICMD_AUDIO_SLOT      1
#define PTICMD_PCR_SLOT        2
#define PTICMD_MAX_DEVICES     1
#define PTICMD_MAX_SLOTS       4
#define PTICMD_BASE_AUDIO      2




/*#######################################################################*/
/*######################## IMPORTED VARIABLES ###########################*/
/*#######################################################################*/

/*#if !defined (ST_OS21) && !defined(ST_5301)*/
/* Allow room for OS20 segments in internal memory */
/*static unsigned char    InternalBlock [ST20_INTERNAL_MEMORY_SIZE-1200];*/
/*#endif*/

/*static unsigned char    ExternalBlock[SYSTEM_MEMORY_SIZE-AUDIO_MEMORY_SIZE]; */
/*static unsigned char    NCacheMemory [NCACHE_MEMORY_SIZE];
static unsigned char    ExternalBlock[SYSTEM_MEMORY_SIZE-AUDIO_MEMORY_SIZE];
static unsigned char    DriverBlock[AUDIO_MEMORY_SIZE];                 */


extern ST_Partition_t   *DriverPartition_p;
extern ST_Partition_t   *NCachePartition_p;
extern ST_Partition_t   *AudioPartition_p;
extern ST_Partition_t   *SystemPartition_p;
extern STAVMEM_PartitionHandle_t AvmemPartitionHandle;

/* This is to avoid a linker warning */
/* static unsigned char    internal_block_noinit[1];
 #pragma ST_section      ( internal_block_noinit, "internal_section_noinit")     */

 /*#if defined(ST_5528) || defined(ST_5100) || defined(ST_5301) || defined(ST_7710)
  static unsigned char    data_section[1];
 #pragma ST_section      ( data_section, "data_section")
 #endif      */

#if defined (ST_7015) || defined (ST_7020) || defined(ST_5528) || defined(ST_5100)\
    || defined (ST_7710) || defined(ST_5301)
    #define SHIFT(val) ((val) << 2)     /* shift by two for these chips */
#else
    #define SHIFT(val) (val)            /* leave unshifted for others */
#endif

/*#if defined(ST_7015) || defined(ST_7020)
#define AUD_BBG         0x10
#define AUD_BBL         0x13
#define AUD_BBS         0x11
#define AUD_BBT         0x12
#endif                                  */




/*#######################################################################*/
/*######################## TYPES DEFINITION #############################*/
/*#######################################################################*/
#if defined (ST_7100) || defined (ST_7109) || defined(ST_7200)
typedef enum{ FREQ_256_8MHz, FREQ_256_11MHz, FREQ_256_12MHz, FREQ_256_16MHz,    FREQ_256_22MHz,
	                  FREQ_256_24MHz, FREQ_256_32MHz, FREQ_256_44MHz, FREQ_256_48MHz, FREQ_256_64MHz,
                      FREQ_256_88MHz, FREQ_256_96MHz, FREQ_256_192MHz,TotalFrequencies} PCMCLK_FREQ;

typedef struct {
				int  SDIV_Val;
				int  PE_Val;
				int  MD_Val;
                }SETCLKDSP;

SETCLKDSP CLK_VALUES[]=
						{ /*SDIV    PE      MD */
                            0x6,  0x5A00,  0xFD,  /*Freq =  2.048MHz :      256*8kHz       */
							0x6,  0x5E86,  0xF5,  /*Freq =  2.8224MHz :		256*11.025kHz  */
                            0x6,  0x3C00,  0xF3,  /*Freq =  3.072MHz :      256*12kHz      */
                            0x5,  0x5A00,  0xFD,  /*Freq =  4.096MHz :      256*16kHz      */
							0x5,  0x5E86,  0xF5,  /*Freq =  5.6448MHz :		256*22.05kHz   */
                            0x5,  0x3C00,  0xF3,  /*Freq =  6.144MHz :      256*24kHz      */
                            0x4,  0x5A00,  0xFD,  /*Freq =  8.192MHz :      256*32kHz      */
                            0x4,  0x5EC4,  0xF5,  /*Freq = 11.2896MHz :     256*44.1kHz    */
                            0x4,  0x5A00,  0xF3,  /*Freq = 12.288MHz :      256*48kHz      */
                            0x3,  0x3C00,  0xFD,  /*Freq = 16.384MHz :      256*64kHz      */
                            0x3,  0x58BA,  0xF5,  /*Freq = 22.528MHz :      256*88kHz      */
                            0x3,  0x3C00,  0xF3,  /*Freq = 24.576MHz :      256*96kHz      */
                            0x2,  0x3C00,  0xF3,  /*Freq = 49.152MHz :      256*192kHz     */

                        };
#endif  /* 7100 */




/*#######################################################################*/
/*######################### LOCAL VARIABLES #############################*/
/*#######################################################################*/
static char AUD_Msg[200];  /* text for trace */
static STAUD_Handle_t   AudDecHndl0;    /* handle for audio */
/* used in AUD_Verbose to make functions print details or not */
static BOOL Verbose;

ST_DeviceName_t AUD_DeviceName="AUDIO";

extern STPTI_Handle_t STPTIHandle[PTICMD_MAX_DEVICES];
extern STPTI_Buffer_t PTI_AudioBufferHandle;
extern STPTI_Slot_t SlotHandle[PTICMD_MAX_DEVICES][PTICMD_MAX_SLOTS];
#ifdef ST_OSLINUX
/* And what about OS21*/
#endif  /* LINUX */

#if defined (ST_7100) || defined (ST_7109) || defined(ST_7200)
/*-----------------9/20/2005 11:03AM----------------
 * Reserved for PCM player#0 config
 * --------------------------------------------------*/
AudioStream_Player AudioStream_PCMP0;
static semaphore_t *PCMP0_Feed_p;
static semaphore_t *PCMP0_Play_p;
static task_t *PCMPlayer_0_p;
static unsigned int sample=0;
static int playpcm0_first_time = 1;                   //flag to be use in configuration to install the interrupt if running for the first time
static unsigned int j_loop0     = 1;                  // Identifier 'j' is used 'for loop' used to play the stream specified number of times

/*-----------------9/20/2005 11:04AM----------------
 * Reserved for Spdif player config
 * --------------------------------------------------*/

audiostream_player audiostream_spdif;

static semaphore_t *spdif_feed;
static semaphore_t *spdif_play;
int flag_loop_spdif      = 1;
int playspdif_first_time = 1;                      //flag to be use in configuration to install the interrupt if running for the first time
unsigned int j_spdif     = 1;
BOOL mode_encoded;                               //This flag decides the MODE of DATA transfer i.e. SPDIF_ENCODED(when TRUE) or SPDIF_PCM(when_FALSE)
BOOL node_first_time = TRUE;
unsigned int underflow_interrupt_spdif=0, eodb_interrupt_spdif=0, eob_interrupt_spdif=0;
unsigned int eol_interrupt_spdif=0, eopd_interrupt_spdif=0, memblock_interrupt_spdif=0;
unsigned long frameSize;
int  CD_mode = 0;
int  AC3_Stream=0, DTS_StreamType1=0, MPEG1_Layer1=0, MPEG1_Layer2=0, MPEG1_Layer3=0;

#if !defined ST_OSLINUX
interrupt_name_t OS21_INTERRUPT_AUD_SPDIF_PLYR; //as per defination in cpu_stb7100.c
#endif

#endif  /* 7100 */




/*#######################################################################*/
/*#########################  LOCAL FUNCTIONS  ###########################*/
/*#######################################################################*/
static boolean SetDeviceSpecificParams( STAUD_InitParams_t * InitParams );
static void SetNonSpecificParams( STAUD_InitParams_t * InitParams );

#if defined (ST_7100) || defined (ST_7109) || defined(ST_7200)
/*-----------------9/20/2005 11:08AM----------------
 * Functions Reserved for PCM player configuration
 * --------------------------------------------------*/
static void set_reg(unsigned int address, unsigned int value);
static unsigned int read_reg(unsigned int address);
static int Load_Stream(char *FileName, char FileIsByteReversed);
static void PlayPCM0_Stream(unsigned int loop,unsigned int sample);
static void PlayAudio_PCM0();
static void PlayPCM0_Config();
static void Create_PCMP0_Node();
static void PCM_ClockConfiguration();


/*-----------------9/20/2005 11:08AM----------------
 * Functions Reserved for spdif player configuration
 * --------------------------------------------------*/
static boolean STAUD_Spdifplayer_pcm_encoded(parse_t *pars_p, char *result);    //command function to play spdifplayer both encoded/pcm
static boolean STAUD_freq_synth_32(parse_t *pars_p, char *result);
void playspdif_stream(unsigned int loop,unsigned int audiosize);
static int load_aud_spdif(const char *file_spdif , char FileIsByteReversed);
static void playaudio_spdif();
void playspdif_interrupt_init();
void playspdif_pcm_config();
void playspdif_encoded_config();
void create_spdif_pcm_node();
void create_spdif_encode_node();
int handler_audio_spdif();
void   StreamType(void);
void   FrameSize_All(void);
#endif  /* 7100 */


/*#ifdef ST_7710 */
#if defined(ST_OS21)
#define AUDIO_WORKSPACE (4096 + 16 * 1024)
#else
#define AUDIO_WORKSPACE 4096
#endif

#if defined (ST_5528) && defined(ST_5528SPDIF)
#define MAX_INJECT_TASK                 4
#elif !defined(ST_5528SPDIF)
#define MAX_INJECT_TASK                 3
#endif

typedef struct {
    unsigned char *FileBuffer;      /* address of stream buffer */
    int           NbBytes;          /* size of stream buffer */
    task_t        *TaskID;
#if !defined(ST_OS21)
    tdesc_t       TaskDesc;
    int           Stack[AUDIO_WORKSPACE];
#endif
    int           Loop;
    void          (*Injector_p)(void *Loop);
    char          *AllocatedBuffer;
    U32           BBLAddr;
    U32           BBTAddr;
    U32           CDFIFOAddr;
    U32           CDPlayAddr;
#if defined (ST_5514) && defined(USE_STGPDMA_INJECT) && !defined (mb290)
    STGPDMA_DmaTransferId_t DMATransferId;
#endif
} AUD_InjectTask_t;

typedef struct {
    BOOL Initialized;
    BOOL InternalPLL;
    U32  DACClockToFsRatio;
    STAUD_PCMDataConf_t DACConfiguration;
} LocalDACConfiguration_t;

enum {
    UNKNOWN_CELL,
    MPEG1_CELL,
    MMDSP_CELL
};

typedef struct structure1{
    STAUD_Handle_t DecoderHandle;
    struct structure1 *next;
} Open_t;

typedef Open_t *Open_p;

typedef struct structure2{
    char DeviceName[40];
    Open_p Open_head;
    struct structure2 *next;
} Init_t;

typedef Init_t *Init_p;
Init_p Init_head = NULL;
typedef char ObjectString_t[20];
#ifdef ST_7710
static void DeviceToText( STAUD_Device_t Device, ObjectString_t * Text );
static void ObjectToText( STAUD_Object_t Object, ObjectString_t *Text );
static void DoAudioInject( void *Loop_p );
static void DoAudioInject2( void *Loop_p );
#define DEFAULT_AUD_NAME                    "LONGAUDDEVNAME"   /* device name */
#define STEVT_HANDLER                   STEVT_DEVICE_NAME
#define STCLKRV_HANDLER_NAME            "CLKRV"
#define AUD1_BASE_ADDRESS    (AUDIO_IF_BASE_ADDRESS+0x00)
#endif
static AUD_InjectTask_t InjectTaskTable[MAX_INJECT_TASK];       /* Injection task(s) data */

/*#endif*/
#if !defined(ST_OS21)
semaphore_t SwitchTaskOKSemaphore;
#endif
semaphore_t *SwitchTaskOKSemaphore_p;

/*** Interactive DAC configuration structure  ***/
static LocalDACConfiguration_t LocalDACConfiguration;
volatile U32 cd1InjectCnt = 0;
volatile U32 ply1Value = 0;
volatile U32 Loop1Cnt = 0;
volatile U32 Loop1Cur = 0;
volatile BOOL SwitchTaskInject = FALSE;
#if defined (ST_7710)
#define AUDIO_CD_FIFO1   AUDIO_CD_FIFO
#endif
/*#define AUD_BBT         0x48 */               /* Audio bit buffer Threshold */
/*#define AUD_BBL         0x4c   */             /* Audio bit buffer Level */
/*#define AUD_PLY         0x5c */               /* Audio Play */
#define AUD_BBT   0x08
#define AUD_BBL   0x0C
#define AUD_PLY   0x24


volatile U32 cd2InjectCnt = 0;
volatile U32 cd3InjectCnt = 0;
volatile U32 ply2Value = 0;
volatile U32 ply3Value = 0;
volatile U32 Loop2Cnt = 0;
volatile U32 Loop2Cur = 0;
volatile U32 Loop3Cnt = 0;
volatile U32 Loop3Cur = 0;

#if defined (ST_OS21)
static BOOL AUD_SyncEnable( parse_t *pars_p, char *result_sym_p );
static BOOL AUD_SyncDisable( parse_t *pars_p, char *result_sym_p );
static BOOL AUD_Init(parse_t *pars_p, char *result_sym_p);
static BOOL AUD_DRStart( parse_t *pars_p, char *result_sym_p );
static BOOL AUD_DRStop( parse_t *pars_p, char *result_sym_p );
static boolean STAUD_load_audio_memory_spdif(parse_t *pars_p, char *result);
static boolean AUD_Stop_PCMPlayer_0(parse_t *pars_p, char *result);
#endif

/*-------------------------------------------------------------------------
 * Function : GetPartitionFreeSpace
 *            Displays the amount of free memory in a particular partition
 * Input    : Msg         Text string to display
 *            Partition_p Partition to report on
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
#if !defined(ST_OS21)
#if defined STTBX_REPORT
static void GetPartitionFreeSpace(char *Msg,  ST_Partition_t *Partition_p)
{
    U32 FreeSpace = 0;
    struct partition_heap_block_s *P;

    P = Partition_p->u.partition_heap.partition_heap_free_head;
    while( P !=  Partition_p->u.partition_heap.partition_heap_free_tail )
    {
        FreeSpace += P->partition_heap_block_blocksize;
        P = P->partition_heap_block_next;
    }
    FreeSpace += P->partition_heap_block_blocksize;

    STTBX_Report(( STTBX_REPORT_LEVEL_INFO, "%s = 0x%08X", Msg, FreeSpace ));
}
#else
/* NULL macro if TBX not in use */
#define GetPartitionFreeSpace( a, b )
#endif
#else
#define GetPartitionFreeSpace( a, b )
#endif

/*-------------------------------------------------------------------------
 * Function : AUDTEST_ErrorReport
 *            Converts an error code to a text string
 * Input    : ErrCode
 * Output   :
 * Return   : FALSE if ErrCode = ST_NO_ERROR, else TRUE
 * ----------------------------------------------------------------------*/
BOOL AUDTEST_ErrorReport( ST_ErrorCode_t ErrCode, char *AUD_Msg )
{
    BOOL RetErr = TRUE;

    switch ( ErrCode )
    {
      case ST_NO_ERROR:
        RetErr = FALSE;
        strcat( AUD_Msg, "ok\n" );
        break;
      case ST_ERROR_INVALID_HANDLE:
        API_ErrorCount++;
        strcat( AUD_Msg, "the handle is invalid !\n");
        break;
      case ST_ERROR_ALREADY_INITIALIZED:
        API_ErrorCount++;
        strcat( AUD_Msg, "the decoder is already initialized !\n" );
        break;
      case ST_ERROR_OPEN_HANDLE:
        API_ErrorCount++;
        strcat( AUD_Msg, "at least one handle is still open !\n" );
        break;
      case ST_ERROR_NO_MEMORY:
        API_ErrorCount++;
        strcat( AUD_Msg, "not enough memory !\n" );
        break;
      case ST_ERROR_BAD_PARAMETER:
        API_ErrorCount++;
        strcat( AUD_Msg, "one parameter is invalid !\n" );
        break;
      case ST_ERROR_FEATURE_NOT_SUPPORTED:
        API_ErrorCount++;
        strcat( AUD_Msg, "feature or parameter not supported !\n" );
        break;
      case ST_ERROR_UNKNOWN_DEVICE:
        API_ErrorCount++;
        strcat( AUD_Msg, "Unknown device (not initialized) !\n" );
        break;
      case ST_ERROR_NO_FREE_HANDLES:
        API_ErrorCount++;
        strcat( AUD_Msg, "too many connections !\n" );
        break;
      case STAUD_ERROR_DECODER_RUNNING:
        API_ErrorCount++;
        strcat( AUD_Msg, "decoder already decoding !\n" );
        break;
      case STAUD_ERROR_DECODER_PAUSING:
        API_ErrorCount++;
        strcat( AUD_Msg, "decoder currently pausing !\n" );
        break;
      case STAUD_ERROR_DECODER_STOPPED:
        API_ErrorCount++;
        strcat( AUD_Msg, "decoder stopped !\n" );
        break;
      case STAUD_ERROR_DECODER_PREPARING:
        API_ErrorCount++;
        strcat( AUD_Msg, "decoder already preparing !\n" );
        break;
      case STAUD_ERROR_DECODER_PREPARED:
        API_ErrorCount++;
        strcat( AUD_Msg, "decoder already prepared !\n" );
        break;
      case STAUD_ERROR_DECODER_NOT_PAUSING:
        API_ErrorCount++;
        strcat( AUD_Msg, "decoder is not pausing !\n" );
        break;
      case STAUD_ERROR_INVALID_STREAM_ID:
        API_ErrorCount++;
        strcat( AUD_Msg, "StreamId is invalid !\n" );
        break;
      case STAUD_ERROR_INVALID_STREAM_TYPE:
        API_ErrorCount++;
        strcat( AUD_Msg, "StreamType is invalid !\n" );
        break;
      case STAUD_ERROR_INVALID_DEVICE:
        API_ErrorCount++;
        strcat( AUD_Msg, "STAUD_ERROR_INVALID_DEVICE\n" );
        break;
      case ST_ERROR_INTERRUPT_INSTALL:
        API_ErrorCount++;
        strcat( AUD_Msg, "Interrupt install failed !\n" );
        break;
      case ST_ERROR_INTERRUPT_UNINSTALL:
        API_ErrorCount++;
        strcat( AUD_Msg, "Interrupt unistall failed !\n" );
        break;
      default:
        API_ErrorCount++;
        sprintf( AUD_Msg, "%s unexpected error [%X] !\n", AUD_Msg, ErrCode );
        break;
    }

    return( RetErr ); /* Returns TRUE if and error was detected */
}
#ifdef ST_7710
static void DeviceToText( STAUD_Device_t Device, ObjectString_t * Text )
{
    switch (Device)
    {
        case STAUD_DEVICE_STI7710:
            strcpy( *Text, "STi7710" );
            break;

        case STAUD_DEVICE_STI5510:
            strcpy( *Text, "STi5510" );
            break;

        case STAUD_DEVICE_STI5512:
            strcpy( *Text, "STi5512" );
            break;

        case STAUD_DEVICE_STI4600:
            strcpy( *Text, "STi4600" );
            break;

        case STAUD_DEVICE_STI5508:
            strcpy( *Text, "STi5508" );
            break;

        case STAUD_DEVICE_STI5518:
            strcpy( *Text, "STi5518" );
            break;

        case STAUD_DEVICE_STI7015:
            strcpy( *Text, "STi7015" );
            break;

        case STAUD_DEVICE_STI5514:
            strcpy( *Text, "STi5514" );
            break;

        case STAUD_DEVICE_STI5516:
            strcpy( *Text, "STi5516" );
            break;

        case STAUD_DEVICE_STI5517:
            strcpy( *Text, "STi5517" );
            break;

        case STAUD_DEVICE_STI5580:
            strcpy( *Text, "STi5580" );
            break;

        case STAUD_DEVICE_SAVP:
            strcpy( *Text, "SAVP" );
            break;

        default:
            sprintf( *Text, "INVALID_DEVICE %x", (U32)Device );
            break;
    }
}

static void ObjectToText( STAUD_Object_t Object, ObjectString_t *Text )
{
    switch (Object)
    {
        case STAUD_OBJECT_INPUT_CD1:
            strcpy( *Text, "IP_CD1" );
            break;
        case STAUD_OBJECT_INPUT_CD2:
            strcpy( *Text, "IP_CD2" );
            break;

        case STAUD_OBJECT_INPUT_CD3:
            strcpy( *Text, "IP_CD3" );
            break;

        case STAUD_OBJECT_INPUT_PCM1:
            strcpy( *Text, "IP_PCM1" );
            break;

        case STAUD_OBJECT_DECODER_COMPRESSED1:
            strcpy( *Text, "DR_COMPRESSED" );
            break;
        case STAUD_OBJECT_DECODER_PCMPLAYER1:
            strcpy( *Text, "DR_PCMPLAYER" );
            break;
        case STAUD_OBJECT_DECODER_EXTERNAL1:
            strcpy( *Text, "DR_EXTERNAL" );
            break;

        case STAUD_OBJECT_DECODER_SPDIF:
            strcpy( *Text, "DR_SPDIF" );
            break;

        case STAUD_OBJECT_PPROC_GENERIC1:
            strcpy( *Text, "PP_GENERIC" );
            break;
        case STAUD_OBJECT_PPROC_TWOCHAN1:
            strcpy( *Text, "PP_TWOCHAN" );
            break;

        case STAUD_OBJECT_MIXER_2INPUT1:
            strcpy( *Text, "MX_TWOINPUT" );
            break;

        case STAUD_OBJECT_OUTPUT_MULTIPCM1:
            strcpy( *Text, "OP_MULTI" );
            break;
        case STAUD_OBJECT_OUTPUT_STEREOPCM1:
            strcpy( *Text, "OP_STEREO" );
            break;
        case STAUD_OBJECT_OUTPUT_SPDIF1:
            strcpy( *Text, "OP_SPDIF" );
            break;
        case STAUD_OBJECT_OUTPUT_DAC:
            strcpy( *Text, "OP_DAC");
            break;
        default:
            strcpy( *Text, "INVALID_OBJECT" );
            break;
    }
}
#endif

#if defined (ST_7100) || defined (ST_7109) || defined(ST_7200)
/*-------------------------------------------------------------------------
 * Function : set_reg
              writes the data to the specified address
 * Input    : address, value
 * Output   :
 * Return   : none
 * ----------------------------------------------------------------------*/
static void set_reg(unsigned int address, unsigned int value)
{
      *((volatile unsigned int *)(address))=value;
}
/*-------------------------------------------------------------------------
 * Function : read_reg
              writes the data to the specified address
 * Input    : address
 * Output   :
 * Return   : Value of register
 * ----------------------------------------------------------------------*/
static unsigned int read_reg(unsigned int address)
{
     unsigned int value = 0;
     value=  *((volatile unsigned int *)(address));
     return(value);
}
#endif  /* 7100 */





/*#######################################################################*/
/*############################# Not 7100 ################################*/
/*#######################################################################*/

#if !defined (ST_7100) && !defined (ST_7109)
/*-------------------------------------------------------------------------
 * Function : DoAudioInject
 *            Function is called as subtask
 * Input    : Loop_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static void DoAudioInject( void *Loop_p )
{

#if defined(USE_STGPDMA_INJECT)

    STGPDMA_DmaTransfer_t DMATransfer;
    STGPDMA_DmaTransferId_t DMATransferId;
    STGPDMA_DmaTransferStatus_t DMAStatus;
    ST_ErrorCode_t ErrorCode;
    AUD_InjectTask_t *InjectTask_p;

    InjectTask_p = &InjectTaskTable[0];

    if(InjectTask_p->Loop == 0)
    {
       InjectTask_p->Loop = 1;
    }

    DMATransfer.TimingModel = STGPDMA_TIMING_PACED_DST;
    DMATransfer.Count = InjectTask_p->NbBytes;
    DMATransfer.Next = NULL;

    DMATransfer.Source.TransferType = STGPDMA_TRANSFER_1D_INCREMENTING;
    DMATransfer.Source.Address = (U32)InjectTask_p->FileBuffer;
    DMATransfer.Source.UnitSize = 4;    /* 32 bits per transfer */
    DMATransfer.Source.Length = 0;      /* Not required for 1D */
    DMATransfer.Source.Stride = 0;      /* Not required for 1D */

    DMATransfer.Destination.TransferType = STGPDMA_TRANSFER_0D;
    DMATransfer.Destination.Address = AUDIO_CD_FIFO;
    DMATransfer.Destination.UnitSize = 1;    /* 1 byte per transfer */
    DMATransfer.Destination.Length = 0;      /* Not required for 0D */
    DMATransfer.Destination.Stride = 0;      /* Not required for 0D */


    /* Inject each iteration 1 by 1 to allow
       aud_kill to stop injection when an iteration completes */
    STTBX_Print(("STGPDMA_DmaStartChannel(%u bytes)\n",
                 (U32)InjectTaskTable[0].NbBytes));
    while( InjectTask_p->Loop > 0 )
    {
#if defined(ST_5514)
        ErrorCode = STGPDMA_DmaStartChannel( GPDMAHandle,
                                             STGPDMA_MODE_BLOCK,
                                             &DMATransfer,
                                             1, /* 1 DMATransfer in linked list */
                                             ST5514_GPDMA_AUDIO_CDREQ,
                                             0, /* no timeout */
                                             &DMATransferId,
                                             &DMAStatus );
#else
       ErrorCode = STGPDMA_DmaStartChannel( GPDMAHandle,
                                             STGPDMA_MODE_BLOCK,
                                             &DMATransfer,
                                             1, /* 1 DMATransfer in linked list */
                                             ST5528_GPDMA_AUDIO_CDREQ1,
                                             0, /* no timeout */
                                             &DMATransferId,
                                             &DMAStatus );
#endif
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Print(("STGPDMA_DmaStartChannel() returned %x\n", ErrorCode));
            break;
        }

        InjectTask_p->Loop--;
    }

#else /* not USE_STGPDMA_INJECT */

#if defined (ST_7015) || defined (ST_7020) || defined (ST_5528) \
    || defined (ST_5100) || defined (ST_5301) || defined (ST_7710)
    /* PRIVATE_INJECT is forced for 70xx for now */

    int LocalDeviceType;

    evaluate_integer("AUDDEVICETYPE", &LocalDeviceType, 10);
    if ((LocalDeviceType == 7015)|| (LocalDeviceType == 7020) ||(LocalDeviceType == 5301) || \
       (LocalDeviceType == 5528) || (LocalDeviceType == 5100) || (LocalDeviceType == 7710))
    {
        U32 *Ptr;
        U32 i;
        U32 BBL, BBT, PLY;
 /*       U32 ErrorCode = ST_NO_ERROR;
        ErrorCode = STAUD_IPSetLowDataLevelEvent ( AudDecHndl0,
                                                   STAUD_OBJECT_INPUT_CD1,
                                                   20 );
        if ( ErrorCode != ST_NO_ERROR )
        {
            STTBX_Print(( "STAUD_IPSetLowDataLevelEvent(%d) returned %x\n",
                                           20, (U32)ErrorCode ));
        }
 */
        if(InjectTaskTable[0].Loop == 0)
        {
           InjectTaskTable[0].Loop = 1;
        }

         Loop1Cnt = InjectTaskTable[0].Loop;

        STTBX_Print(("AUD_PrivateInject(%u bytes)\n",
                     (U32)InjectTaskTable[0].NbBytes));


        for (; (InjectTaskTable[0].Loop > 0); InjectTaskTable[0].Loop-- )
        {
            Ptr = (U32 *)InjectTaskTable[0].FileBuffer;
            Loop1Cur = InjectTaskTable[0].Loop;

            for ( i=0; i<InjectTaskTable[0].NbBytes/4; i++ )
            {
                BBL = STSYS_ReadRegDev32LE((void*)(InjectTaskTable[0].BBLAddr)) & 0x03FFFF00;
                BBT = STSYS_ReadRegDev32LE((void*)(InjectTaskTable[0].BBTAddr)) & 0x03FFFF00;
                PLY = STSYS_ReadRegDev32LE((void*)(InjectTaskTable[0].CDPlayAddr));

                ply1Value = PLY;

                if((BBL < BBT) && (PLY & 0x04))
                {
                    cd1InjectCnt++;
                    STSYS_WriteRegDev32LE(InjectTaskTable[0].CDFIFOAddr,*Ptr);
                    Ptr++;

                }else
                {
                    /*STTBX_Print(("#### Wait ######\n")); */
                    i--;
                }

                if(SwitchTaskInject == TRUE)
                {
                    SwitchTaskInject = FALSE;
                    semaphore_signal(SwitchTaskOKSemaphore_p);
                    return;
                }
            }
        }

    }

#endif /* 70xx */
#endif
} /* end DoAudioInject */
/*-------------------------------------------------------------------------
 * Function : DoAudioInject2
 *            Function is called as subtask. Injects data directly to the
2nd compress
              data interface. It polls the bitbuffer level
 * Input    : Loop_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static void DoAudioInject2( void *Loop_p )
{
#if defined (ST_7015) || defined (ST_7020) || defined (ST_5528) \
    || defined (ST_5100) || defined (ST_7710)   || defined (ST_5301)

    U32 *Ptr;
    U32 i;
    U32 BBL,BBT, PLY;

    if(InjectTaskTable[1].Loop == 0)
    {
       InjectTaskTable[1].Loop = 1;
    }

    Loop2Cnt = InjectTaskTable[1].Loop;

    for (; (InjectTaskTable[1].Loop > 0); InjectTaskTable[1].Loop -- )
    {
        Ptr = (U32 *)InjectTaskTable[1].FileBuffer;
        Loop2Cur = InjectTaskTable[1].Loop;

        for ( i=0; i<InjectTaskTable[1].NbBytes/4; i++ )
        {
             BBL = STSYS_ReadRegDev32LE((void*)(InjectTaskTable[1].BBLAddr)) & 0x03FFFF00;
             BBT = STSYS_ReadRegDev32LE((void*)(InjectTaskTable[1].BBTAddr)) & 0x03FFFF00;
             PLY = STSYS_ReadRegDev32LE((void*)(InjectTaskTable[1].CDPlayAddr));

           ply2Value = PLY;

         if((BBL < BBT) && ( PLY & 0x04))
         {
            cd2InjectCnt++;
            STSYS_WriteRegDev32LE(InjectTaskTable[1].CDFIFOAddr,*Ptr);
            Ptr++;
         }
         else
         {
            /*STTBX_Print(("#### Wait ######\n")); */
            /*if(Loop2 == 0) break;*/
                i--;
         }
       }

    }

#elif defined(ST_5514) && defined(USE_STGPDMA_INJECT)

    /* GPDMA must be used for secondary inject on 5514 */

    STGPDMA_DmaTransfer_t DMATransfer;
    STGPDMA_DmaTransferStatus_t DMAStatus;
    ST_ErrorCode_t ErrorCode;
    AUD_InjectTask_t *InjectTask_p;
#ifdef AUDTST_DMA2MEM
    U32 Temp[3]; /* will have cache coherency problems but that doesn't matter for quick test; [3] to fix alignment */
#endif

    InjectTask_p = &InjectTaskTable[1];

    if(InjectTask_p->Loop == 0)
    {
       InjectTask_p->Loop = 1;
    }

#ifdef AUDTST_DMA2MEM
    DMATransfer.TimingModel = STGPDMA_TIMING_FREE_RUNNING;
#else
    DMATransfer.TimingModel = STGPDMA_TIMING_PACED_DST; /* FREE_RUNNING avoids hang but hogs bus */
#endif
    DMATransfer.Count = InjectTask_p->NbBytes;
    DMATransfer.Next = &DMATransfer; /* Circular */

    DMATransfer.Source.TransferType = STGPDMA_TRANSFER_1D_INCREMENTING;
    DMATransfer.Source.Address = (U32)InjectTask_p->FileBuffer;
    DMATransfer.Source.UnitSize = 4;    /* 32 bits per transfer */
    DMATransfer.Source.Length = 0;      /* Not required for 1D */
    DMATransfer.Source.Stride = 0;      /* Not required for 1D */

    DMATransfer.Destination.TransferType = STGPDMA_TRANSFER_0D;
#ifdef AUDTST_DMA2MEM
    DMATransfer.Destination.Address = ((((U32)Temp)+4)/8)*8;
#else
    DMATransfer.Destination.Address = (AUDIO_IF_BASE_ADDRESS + AUDIF_PCMO);
#endif
    DMATransfer.Destination.UnitSize = 8;    /* 2*32 bits per transfer */
    DMATransfer.Destination.Length = 0;      /* Not required for 1D */
    DMATransfer.Destination.Stride = 0;      /* Not required for 1D */

    /*
    ** Number of bytes must be an exact multiple of largest UnitSize
    */
    while((DMATransfer.Count/8)*8 != DMATransfer.Count)
    {
        DMATransfer.Count--;
    }

    /* Inject each iteration 1 by 1 to allow
       aud_kill to stop injection when an iteration completes */
    STTBX_Print(("STGPDMA_DmaStartChannel(%u bytes)\n",
                 (U32)DMATransfer.Count));
    while( InjectTask_p->Loop > 0 )
    {
        ErrorCode = STGPDMA_DmaStartChannel( GPDMAHandle2,
                                             STGPDMA_MODE_BLOCK,
                                             &DMATransfer,
                                             1, /* 1 DMATransfer in linked list */
                                             ST5514_GPDMA_PCMOUT,
                                             0, /* no timeout */
                                             &InjectTask_p->DMATransferId,
                                             &DMAStatus );

        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Print(("STGPDMA_DmaStartChannel() returned %x\n", ErrorCode));
            break;
        }
        else
        {
            /*STTBX_Print(("."));*/
        }

        InjectTask_p->Loop--;
    }

#elif defined (STAUD_FDMA_SECONDARY_INJECT) && defined(ST_5517)
    {
        /* FDMA secondary inject on 5517 */
        STFDMA_Node_t           *NodeAlloc_p, *Node_p;
        STFDMA_TransferParams_t TfParams;
        STFDMA_TransferId_t     TfId;
        ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
        AUD_InjectTask_t        *InjectTask_p;

        InjectTask_p = &InjectTaskTable[1];

        if(InjectTask_p->Loop == 0)
        {
            InjectTask_p->Loop = 1;
        }

        /* FDMA nodes must be 32-byte aligned, in non-cached memory */
        NodeAlloc_p = memory_allocate(NCachePartition, 2*sizeof(STFDMA_Node_t));
        if (NodeAlloc_p == NULL)
        {
            STTBX_Print(("FDMA Node allocation in NCachePartition failed\n"));
            return;
        }
        Node_p = (STFDMA_Node_t*)
            (((U32)NodeAlloc_p + sizeof(STFDMA_Node_t)) & 0xffffffe0);

        Node_p->Next_p                           = NULL; /* single transfer */
        Node_p->NodeControl.PaceSignal           = STFDMA_REQUEST_SIGNAL_PCMO_HI;
        Node_p->NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
        Node_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_STATIC;
        Node_p->NodeControl.Reserved             = 0;
        Node_p->NodeControl.NodeCompletePause    = FALSE;
        Node_p->NodeControl.NodeCompleteNotify   = FALSE;
        Node_p->NumberBytes                      = (U32) InjectTask_p->NbBytes;
        Node_p->SourceAddress_p                  = (U32*) InjectTask_p->FileBuffer;
        Node_p->DestinationAddress_p             = (U32*)(AUDIO_IF_BASE_ADDRESS + AUDIF_PCMO);
        Node_p->Length                           = Node_p->NumberBytes; /* 1D transfer */
        Node_p->SourceStride                     = Node_p->Length; /* (ignored) */
        Node_p->DestinationStride                = 0; /* (ignored) */

        TfParams.ChannelId          = STFDMA_USE_ANY_CHANNEL;
        TfParams.NodeAddress_p      = Node_p;
        TfParams.BlockingTimeout    = 0;    /* no timeout */
        TfParams.CallbackFunction   = NULL; /* blocking transfer */
        TfParams.ApplicationData_p  = NULL;

        TfParams.FDMABlock          = STFDMA_1;

        for (; (InjectTask_p->Loop > 0) && (ErrorCode==ST_NO_ERROR); InjectTask_p->Loop-- )
        {
            /* setup real data transfer */
            ErrorCode = STFDMA_StartTransfer(&TfParams, &TfId);
            if(ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STFDMA_StartTransfer() returned %x\n", ErrorCode));
                break;
            }
        }
        InjectTask_p->Loop = 0;
        memory_deallocate(NCachePartition, NodeAlloc_p);
    }

#elif defined (DVD_TRANSPORT_STPTI) && (defined(ST_5516) || defined(ST_5517))
    /* STPTI must be used for secondary inject on STi5516 and may be used on 5517 */
    {
        STPTI_DMAParams_t DMA1Params, DMA2Params;
        ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
        AUD_InjectTask_t  *InjectTask_p;

        InjectTask_p = &InjectTaskTable[1];

        if(InjectTask_p->Loop == 0)
        {
            InjectTask_p->Loop = 1;
        }

        DMA1Params.Destination = (AUDIO_IF_BASE_ADDRESS + AUDIF_PCMO_REMAP1);
        DMA1Params.Holdoff = 0;
        DMA1Params.WriteLength = 4;
        DMA1Params.CDReqLine = STPTI_CDREQ_PCMO;
        STTBX_Print(("STPTI_UserDataWrite(%u bytes)\n",
                     (U32)InjectTask_p->NbBytes));
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "Buffer Start %x Buffer Stop %x",
                      (U32)InjectTask_p->FileBuffer,
                      (U32)InjectTask_p->FileBuffer+InjectTask_p->NbBytes));


        for (; (InjectTask_p->Loop > 0) && (ErrorCode==ST_NO_ERROR); InjectTask_p->Loop-- )
        {
            /* setup real data transfer */
            ErrorCode = STPTI_UserDataWrite((U8 *)InjectTask_p->FileBuffer,
                                            (U32)InjectTask_p->NbBytes,
                                            &DMA1Params);
            if(ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STPTI_UserDataWrite() returned %x\n", ErrorCode));
                break;
            }

            /* setup dummy transfer that clocks data into the PCMO fifo */
            DMA2Params.Destination = (AUDIO_IF_BASE_ADDRESS + AUDIF_PCMO_REMAP2);
            DMA2Params.Holdoff = 0;
            DMA2Params.WriteLength = 4;
            DMA2Params.CDReqLine = STPTI_CDREQ_PCMO;
            ErrorCode = STPTI_UserDataWrite((U8 *)InjectTask_p->FileBuffer,
                                            (U32)InjectTask_p->NbBytes,
                                            &DMA2Params);
            if(ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STPTI_UserDataWrite() returned %x\n", ErrorCode));
                break;
            }

            /* wait for transfer to complete before reusing dma */
            ErrorCode = STPTI_UserDataSynchronize(&DMA1Params);
            if(ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STPTI_UserDataSynchronize() returned %x\n", ErrorCode));
                break;
            }
            ErrorCode = STPTI_UserDataSynchronize(&DMA2Params);
            if(ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STPTI_UserDataSynchronize() returned %x\n", ErrorCode));
                break;
            }
            /*STTBX_Print(("STPTI_UserDataWrite() complete\n"));*/
            /*STTBX_Print(("."));*/
        }
        InjectTask_p->Loop = 0;
    }
#else

    STTBX_Print(("ERROR: 2nd audio inject is only supported for GPDMA (5514), PRIVATE_INJECT (7015) or STPTI (5516/17)\n"));

#endif
} /* end DoAudioInject2 */

/*-------------------------------------------------------------------------
 * Function : AUD_Close
 *            Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_Close( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    long LVar;
    STAUD_Handle_t LocalHandle;

    ErrCode = ST_NO_ERROR;
    /* get handle from user */
    RetErr = cget_integer( pars_p, AudDecHndl0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected handle" );
    }
    else
    {
        LocalHandle = (STAUD_Handle_t)LVar;
    }

    sprintf( AUD_Msg, "STAUD_Close(%u) : ", (U32)LocalHandle );

    ErrCode = STAUD_Close( LocalHandle );

    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTST_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);

    return ( API_EnableError ? RetErr : FALSE );
} /* end AUD_Close */

/*-------------------------------------------------------------------------
 * Function : GetDefaultDACConfig
 *            Returns default DAC params according to board type
 * Input    : Pointer to DAC config structure
 * Output   :
 * Return   : none
 * ----------------------------------------------------------------------*/
static void GetDefaultDACConfig(LocalDACConfiguration_t *LocalDACConfig_p)
{
#if defined(mb295)
    /* --- Dacs CONFIG --- */
    /* CS4330 in Sony format (DIF=0, FOR=1, INV=1, SCL=0) */
    LocalDACConfig_p->InternalPLL = TRUE; /* internal PLL */
    LocalDACConfig_p->DACClockToFsRatio = 256;  /* PCMCLK = K*Fs => aud_div=0x2*/
    LocalDACConfig_p->DACConfiguration.InvertWordClock = TRUE;   /* LRCLK inverted */
    LocalDACConfig_p->DACConfiguration.InvertBitClock = FALSE;   /* SCLK not inverted */
    LocalDACConfig_p->DACConfiguration.Format = STAUD_DAC_DATA_FORMAT_STANDARD; /* Standard format */
    LocalDACConfig_p->DACConfiguration.Precision = STAUD_DAC_DATA_PRECISION_18BITS;  /* 18 bit data */
    LocalDACConfig_p->DACConfiguration.Alignment = STAUD_DAC_DATA_ALIGNMENT_ZERO_EXT;   /* PCM data is right aligned */
    LocalDACConfig_p->DACConfiguration.MSBFirst = TRUE;          /* Not relevant for > 16 bits */
#elif defined(ST_7020)
    /* mb290: AK4356 in I2S mode (DIF=1, FOR=0, INV=0, SCL=0) */
    /* 7020 STEM board: AK4356/94 in I2S 256 Fs mode (combination of settings from Board_Init
      and default SW2/1 settings 1=2=OFF (high); 3=4=ON (low) */
    LocalDACConfig_p->InternalPLL = TRUE; /* internal PLL */
    LocalDACConfig_p->DACClockToFsRatio = 256;  /* PCMCLK = K*Fs => aud_div=0x2*/
    LocalDACConfig_p->DACConfiguration.InvertWordClock = FALSE;   /* LRCLK not inverted */
    LocalDACConfig_p->DACConfiguration.InvertBitClock = FALSE;   /* SCLK not inverted */
    LocalDACConfig_p->DACConfiguration.Format = STAUD_DAC_DATA_FORMAT_I2S; /* I2S format */
    LocalDACConfig_p->DACConfiguration.Precision = STAUD_DAC_DATA_PRECISION_24BITS;  /* 24 bit data */
    LocalDACConfig_p->DACConfiguration.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT_SIGNED;   /* PCM data is left aligned */
    LocalDACConfig_p->DACConfiguration.MSBFirst = TRUE;          /* Not relevant for > 16 bits */
#elif defined(mb275) || defined(mb275_64)  || defined(mb314) || \
      defined(mb361) || defined(mb382) || defined(mb376) || defined(espresso)|| defined(mb391)||defined(mb411)
    /* --- Dacs CONFIG --- */
    /* AK4356/94 in I2S mode (DIF=1, FOR=0, INV=0, SCL=0) */
    LocalDACConfig_p->InternalPLL = TRUE; /* internal PLL */
    LocalDACConfig_p->DACClockToFsRatio = 256;  /* PCMCLK = K*Fs => aud_div=0x2*/
    LocalDACConfig_p->DACConfiguration.InvertWordClock = FALSE;   /* FALSE = LRCLK not inverted */
    LocalDACConfig_p->DACConfiguration.InvertBitClock = FALSE;   /* SCLK not inverted */
    LocalDACConfig_p->DACConfiguration.Format = STAUD_DAC_DATA_FORMAT_I2S; /* I2S format */
    LocalDACConfig_p->DACConfiguration.Precision = STAUD_DAC_DATA_PRECISION_18BITS;  /* 18 bit data */
    LocalDACConfig_p->DACConfiguration.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT_SIGNED;   /* PCM data is left aligned */
    LocalDACConfig_p->DACConfiguration.MSBFirst = FALSE;          /* Not relevant for > 16 bits */
#elif defined(mb5518)
    /* AK4356 256 Fs Mode 1 (20 bit LSB justified) */
    LocalDACConfig_p->InternalPLL = TRUE; /* internal PLL */
    LocalDACConfig_p->DACClockToFsRatio = 256;  /* PCMCLK = K*Fs => aud_div=0x2*/
    LocalDACConfig_p->DACConfiguration.InvertWordClock = FALSE;   /* FALSE = LRCLK not inverted */
    LocalDACConfig_p->DACConfiguration.InvertBitClock = FALSE;   /* SCLK not inverted */
    LocalDACConfig_p->DACConfiguration.Format = STAUD_DAC_DATA_FORMAT_STANDARD; /* Standard format */
    LocalDACConfig_p->DACConfiguration.Precision = STAUD_DAC_DATA_PRECISION_20BITS;  /* 20 bit data */
    LocalDACConfig_p->DACConfiguration.Alignment = STAUD_DAC_DATA_ALIGNMENT_ZERO_EXT;   /* PCM data is right aligned */
    LocalDACConfig_p->DACConfiguration.MSBFirst = TRUE;          /* Not relevant for > 16 bits */
#elif defined(mb231) || defined(mb282b)
    /* --- Dacs CONFIG --- */
    LocalDACConfig_p->InternalPLL = TRUE; /* internal PLL */
    LocalDACConfig_p->DACClockToFsRatio = 512; /* PCMCLK = K*Fs => aud_div=0x2*/
    LocalDACConfig_p->DACConfiguration.InvertWordClock = FALSE; /* FALSE = LRCLK not inverted */
#if defined(mb231)
    LocalDACConfig_p->DACConfiguration.InvertWordClock = TRUE;  /* FALSE = LRCLK inverted to correct L/R xover on board */
#endif
    LocalDACConfig_p->DACConfiguration.InvertBitClock = FALSE;  /* SCLK not inverted */
    LocalDACConfig_p->DACConfiguration.Format = STAUD_DAC_DATA_FORMAT_STANDARD; /* standard format (not I2S)*/
    LocalDACConfig_p->DACConfiguration.Precision = STAUD_DAC_DATA_PRECISION_18BITS;  /* 18 bit data */
    LocalDACConfig_p->DACConfiguration.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT_SIGNED;   /* PCM data is left aligned */
    LocalDACConfig_p->DACConfiguration.MSBFirst = TRUE;      /* PCM data order MSB first */
#else
#error ERROR: Unsupported board for DAC settings
#endif
}

/*-------------------------------------------------------------------------
 * Function : AUD_ConfigDAC
 *            Set the DAC configuration used in the AUD_Init function
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_ConfigDAC(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    char Msg[80];
    long LVar;
    LocalDACConfiguration_t TmpConfig;

    /* Set default DAC config */
    GetDefaultDACConfig(&TmpConfig);
    TmpConfig.Initialized = FALSE;

    /* use internal PLL or not */
    RetErr = cget_integer( pars_p, TmpConfig.InternalPLL, &LVar);
    if ( (RetErr == TRUE ) || ((LVar != 0) && (LVar != 1)))
    {
        sprintf( Msg, "internalPLL : 0 or 1 expected ");
        tag_current_line( pars_p, Msg );
    }
    else
    {
        TmpConfig.InternalPLL = (BOOL) LVar;
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACClockToFsRatio, &LVar);
        if ( RetErr == TRUE )
        {
            sprintf( Msg, "DACClockToFsRatio : unsigned integer expected ");
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACClockToFsRatio = (U32) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.InvertWordClock, &LVar);
        if ( (RetErr == TRUE ) || ((LVar != 0) && (LVar != 1)))
        {
            sprintf( Msg, "InvertWordClock : 0 or 1 expected ");
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACConfiguration.InvertWordClock = (BOOL) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.InvertBitClock, &LVar);
        if ( (RetErr == TRUE ) || ((LVar != 0) && (LVar != 1)))
        {
            sprintf( Msg, "InvertBitClock : 0 or 1 expected ");
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACConfiguration.InvertBitClock = (BOOL) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.Format, &LVar);
        if ( (RetErr == TRUE ) ||
             ((LVar != STAUD_DAC_DATA_FORMAT_I2S) && (LVar != STAUD_DAC_DATA_FORMAT_STANDARD)))
        {
            sprintf( Msg, "Format : %d (I2S) or %d (STANDARD) expected ",
                     STAUD_DAC_DATA_FORMAT_I2S, STAUD_DAC_DATA_FORMAT_STANDARD);
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACConfiguration.Format = (STAUD_DACDataFormat_t) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.Precision, &LVar);
        if ( (RetErr == TRUE ) ||
             ((LVar < STAUD_DAC_DATA_PRECISION_16BITS) || (LVar > STAUD_DAC_DATA_PRECISION_24BITS)))
        {
            sprintf( Msg, "Precision : %d (16Bits) or %d (18bits) or %d (20Bits) or %d (24bits) expected ",
                     STAUD_DAC_DATA_PRECISION_16BITS, STAUD_DAC_DATA_PRECISION_18BITS,
                     STAUD_DAC_DATA_PRECISION_20BITS, STAUD_DAC_DATA_PRECISION_24BITS);
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACConfiguration.Precision = (STAUD_DACDataPrecision_t) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.Alignment,
                               &LVar);
        if ( (RetErr == TRUE ) || ((LVar != 0) && (LVar != 1)))
        {
            sprintf( Msg, "RightPadded : 0 or 1 expected ");
            tag_current_line( pars_p, Msg );
        }
        else
        {
            if( (BOOL)LVar )
            {
                TmpConfig.DACConfiguration.Alignment = STAUD_DAC_DATA_ALIGNMENT_ZERO_EXT;
            }
            else
            {
                TmpConfig.DACConfiguration.Alignment = STAUD_DAC_DATA_ALIGNMENT_LEFT_SIGNED;
            }
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, TmpConfig.DACConfiguration.MSBFirst, &LVar);
        if ( (RetErr == TRUE ) || ((LVar != 0) && (LVar != 1)))
        {
            sprintf( Msg, "MSBFirst : 0 or 1 expected ");
            tag_current_line( pars_p, Msg );
        }
        else
        {
            TmpConfig.DACConfiguration.MSBFirst = (BOOL) LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        TmpConfig.Initialized = TRUE;
        LocalDACConfiguration = TmpConfig;
        STTST_Print(( "AUD_ConfigDAC(%d, %d, %d, %d, %d, %d, %d, %d) ok\n",
                      LocalDACConfiguration.InternalPLL,
                      LocalDACConfiguration.DACClockToFsRatio,
                      LocalDACConfiguration.DACConfiguration.InvertWordClock,
                      LocalDACConfiguration.DACConfiguration.InvertBitClock,
                      LocalDACConfiguration.DACConfiguration.Format,
                      LocalDACConfiguration.DACConfiguration.Precision,
                      LocalDACConfiguration.DACConfiguration.Alignment,
                      LocalDACConfiguration.DACConfiguration.MSBFirst));
    }
    else
    {
        STTST_Print(( "AUD_ConfigDAC() error : %s\n", Msg));
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of AUD_ConfigDAC() */

void SetNonSpecificParams( STAUD_InitParams_t * InitParams )
{
#if defined (DVD)
    InitParams->Configuration = STAUD_CONFIG_DVD_COMPACT;
#elif defined (DVD_SERVICE_DIRECTV)
    InitParams->Configuration = STAUD_CONFIG_DIRECTV_COMPACT;
#else /* default to STB */
    InitParams->Configuration = STAUD_CONFIG_DVB_COMPACT;
#endif

    InitParams->MaxOpen         = 1;
    InitParams->CPUPartition_p  = DriverPartition_p;

    /* Configure interface to AUDIO registers */
    InitParams->InterfaceType = STAUD_INTERFACE_EMI;

    /* Configure CD data interface */
    InitParams->CDInterfaceType = STAUD_INTERFACE_EMI;
    InitParams->AVMEMPartition = AvmemPartitionHandle;
    InitParams->CD1BufferAddress_p = NULL;
    InitParams->CD1BufferSize = 0;

    strcpy(InitParams->EvtHandlerName, STEVT_HANDLER);


#if defined (DVD) || defined(STAUD_REMOVE_CLKRV_SUPPORT)
    strcpy(InitParams->ClockRecoveryName, "\0");
#else
    strcpy(InitParams->ClockRecoveryName, STCLKRV_HANDLER_NAME);
#endif

}

/*-------------------------------------------------------------------------*/
boolean SetDeviceSpecificParams ( STAUD_InitParams_t * InitParams )
{
    int LocalDeviceType;
    LocalDeviceType=0;

    /* Initialise device requested in DEVICETYPE variable */

    evaluate_integer("AUDDEVICETYPE", &LocalDeviceType, 10);
    switch (LocalDeviceType) /* basic conversion and validation */
    {
#if defined (STAUD_OPEN_PLATFORM)
    case DEVTYPE_OPEN:
        InitParams->DeviceType = STAUD_OPEN_PLATFORM; /* value is STAUD_Device_t being simulated  */
        break;
#endif

#if defined(ST_7015)
    case 7015:
        InitParams->DeviceType = STAUD_DEVICE_STI7015;
        break;
#endif

#if defined(ST_7020)
    case 7020:
        InitParams->DeviceType = STAUD_DEVICE_STI7020;
        break;
#endif

#if defined(ST_7015) || defined(ST_7020)
    case 4600: /* allow 4600 connected as external decoder */
        InitParams->DeviceType = STAUD_DEVICE_STI4600;
        break;
#endif

#if defined(ST_5528)
    case 5528:
        InitParams->DeviceType = STAUD_DEVICE_STI5528;
        break;
#endif

#if defined (ST_4629)
    case 4629: /* external decoder to 5528 */
        InitParams->DeviceType = STAUD_DEVICE_STI4629;
        break;
#endif

#if defined (ST_5528SPDIF)
    case 55282: /* external SPDIF formatter to 5528 */
        InitParams->DeviceType = STAUD_DEVICE_STISPDIF;
        break;
#endif

#if defined(ST_5517)
    case 5517:
        InitParams->DeviceType = STAUD_DEVICE_STI5517;
        break;
#endif

#if defined(ST_5516)
    case 5516:
        InitParams->DeviceType = STAUD_DEVICE_STI5516;
        break;
#endif

#if defined(ST_5514)
    case 5514:
        InitParams->DeviceType = STAUD_DEVICE_STI5514;
        break;
#endif

#if defined(ST_5512)
    case 5512:
        InitParams->DeviceType = STAUD_DEVICE_STI5512;
        break;
#endif

#if defined(ST_5510)
    case 5510:
        InitParams->DeviceType = STAUD_DEVICE_STI5510;
        break;
#endif

#if defined(ST_5580)
    case 5580:
        InitParams->DeviceType = STAUD_DEVICE_STI5580;
        break;
#endif

#if defined(ST_5518)
    case 5518:
        InitParams->DeviceType = STAUD_DEVICE_STI5518;
        break;
#endif

#if defined(ST_5508)
    case 5508:
        InitParams->DeviceType = STAUD_DEVICE_STI5508;
        break;
#endif

#if defined(ST_5100)
    case 5100:
        InitParams->DeviceType = STAUD_DEVICE_STI5100;
        break;
#endif

#if defined(ST_5301)
    case 5301:
        InitParams->DeviceType = STAUD_DEVICE_STI5301;
        break;
#endif

#if defined(ST_7710)
    case 7710:
        InitParams->DeviceType = STAUD_DEVICE_STI7710;
        break;
#endif
    default:
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unrecognised DeviceType %i (check build options)", LocalDeviceType));
        return TRUE; /* error; unable to continue with Init */
    };

#if defined (STAUD_OPEN_PLATFORM)
    if (LocalDeviceType == DEVTYPE_OPEN)
    {
        ObjectString_t DeviceName;

        InitParams->DeviceType = STAUD_OPEN_PLATFORM; /* value is STAUD_Device_t being simulated  */
        DeviceToText(InitParams->DeviceType, &DeviceName);
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "Initializing for MMDSP+ OpenPlatform as %s",
                      DeviceName ));
    }
    else
#endif
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "Initializing for STi%i DeviceType", LocalDeviceType));
    }

    /* DAC configuration is board specific
       Default values can be overridden using AUD_CONFDAC */
    if (!LocalDACConfiguration.Initialized)
    {
        /* DAC configuration has NOT been set by user
           so obtain default values */
        GetDefaultDACConfig(&LocalDACConfiguration);
    }
    InitParams->InternalPLL  = LocalDACConfiguration.InternalPLL;
    InitParams->DACClockToFsRatio = LocalDACConfiguration.DACClockToFsRatio;
    InitParams->PCMOutParams = LocalDACConfiguration.DACConfiguration;

#if defined(ST_7015) || defined(ST_7020)
    if( (LocalDeviceType == 7015) || (LocalDeviceType == 7020) ||
        (LocalDeviceType == 4600) )
    {
        /* external decoders */

        InitParams->InterruptNumber = 0; /* Not relevant since the interrupts are */
        InitParams->InterruptLevel  = 0; /*   mapped through the STINTMR */

        /* Interrupt manager */
        strcpy(InitParams->RegistrantDeviceName, INTMR_REGISTRANT);

        InitParams->ClockGeneratorBaseAddress_p = (void*)(STI70XX_CKG_BASE_ADDRESS);

        if( LocalDeviceType == 4600 )
        {
            InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)AUD_BASE_ADDRESS_4600;
            InitParams->InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_16;

            InitParams->CDInterfaceParams.EMIParams.BaseAddress_p = (void *)AUD1_BASE_ADDRESS_4600;
            InitParams->CDInterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_16;

            InitParams->IntEventNumber = EXT_AUDIO_EVT_NUMBER;
        }
        else
        {
            /* 7015/7020 */
            InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)MMDSP_BASE_ADDRESS;
            InitParams->InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;

            InitParams->CDInterfaceParams.EMIParams.BaseAddress_p = (void *)AUD1_BASE_ADDRESS;
            InitParams->CDInterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;

            InitParams->CD2InterfaceType = STAUD_INTERFACE_EMI;
            InitParams->CD2InterfaceParams.EMIParams.BaseAddress_p = (void *)AUD2_BASE_ADDRESS;
            InitParams->CD2InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;
            InitParams->CD2BufferAddress_p = NULL;
            InitParams->CD2BufferSize = 0;
            InitParams->PCM1QueueSize = MAX_PCMBUFFERS;

            InitParams->IntEventNumber = AUDIO_EVT_NUMBER;
        }
    }
    else
#endif

#if defined(ST_5528)
    if (LocalDeviceType == 5528)
    {
        /* 5528 is an internal decoder with more interrupts, a unified audio
          interface, and registers spread out like 70xx */

        InitParams->InterruptNumber    = AUDIO_INTERRUPT;
        InitParams->CD1InterruptNumber = AUD_CD1_INTERRUPT;
        InitParams->CD2InterruptNumber = AUD_CD2_INTERRUPT;
        InitParams->CD3InterruptNumber = AUD_CD3_INTERRUPT;
        InitParams->PCMInterruptNumber = AUD_PCM_INTERRUPT;

        InitParams->InterruptLevel     = AUDIO_INTERRUPT_LEVEL;
        InitParams->CD1InterruptLevel  = AUDIO_INTERRUPT_LEVEL;
        InitParams->CD2InterruptLevel  = AUDIO_INTERRUPT_LEVEL;
        InitParams->CD3InterruptLevel  = AUDIO_INTERRUPT_LEVEL;
        InitParams->PCMInterruptLevel  = AUDIO_INTERRUPT_LEVEL;

        /* Configure interface to AUDIO registers */
        InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)AUDIO_BASE_ADDRESS;
        InitParams->InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;

        /* Configure interface to ADSC registers */
        InitParams->CDInterfaceType = STAUD_INTERFACE_EMI;
        InitParams->CDInterfaceParams.EMIParams.BaseAddress_p = (void *)AUDIO_IF_BASE_ADDRESS;
        InitParams->CDInterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;

        /* CD2InterfaceParams and ClockGeneratorBaseAddress_p not used */

        InitParams->CD2BufferAddress_p = NULL;
        InitParams->CD2BufferSize = 0;
        InitParams->CD3BufferAddress_p = NULL;
        InitParams->CD3BufferSize = 0;

        InitParams->PCM1QueueSize = MAX_PCMBUFFERS;
    }
    else
#endif

#if defined (ST_4629)
    if (LocalDeviceType == 4629)
    {
        /* Two modes to handle interrupts. Writing -1 to interrupt number chooses the
       intmr route. Therefore event number should be assigned */
#if defined (espresso) || defined(ST_OS21)
        InitParams->InterruptNumber    = EXTERNAL_1_INTERRUPT_NUMBER;
        InitParams->InterruptLevel     = EXTERNAL_1_INTERRUPT_LEVEL;
#else
        InitParams->InterruptNumber    = -1 ;
#endif

    InitParams->IntEventNumber = AUDIO_EVT_NUMBER;
    strcpy(InitParams->RegistrantDeviceName, INTMR_REGISTRANT);

        /* Configure interface to AUDIO registers */
#if defined(ST_OS21)
        InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)(STI4629_ST40_BASE_ADDRESS + ST4629_AUDIO_OFFSET);
#else
        InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)(STI4629_BASE_ADDRESS + ST4629_AUDIO_OFFSET);
#endif
        InitParams->InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_8;

    }
    else
#endif

#if defined (ST_5528SPDIF)
    if (LocalDeviceType == 55282)
    {
        InitParams->InterruptNumber    = SPDIF_INTERRUPT;
        InitParams->InterruptLevel     = AUDIO_INTERRUPT_LEVEL;
        InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)SPDIF_BASE_ADDRESS;
        InitParams->InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;
    }
    else
#endif

#if defined (ST_5100)
    if (LocalDeviceType == 5100)
    {
        /* 5100  is an internal decoder with more interrupts, a unified audio
          interface, and registers spread out like 70xx */

        InitParams->InterruptNumber    = AUDIO_INTERRUPT;
        InitParams->CD1InterruptNumber = ST5100_AUD_CD_INTERRUPT;
        InitParams->PCMInterruptNumber = ST5100_AUD_PCM_INTERRUPT;

        InitParams->InterruptLevel     = AUDIO_INTERRUPT_LEVEL;
        InitParams->CD1InterruptLevel  = AUDIO_INTERRUPT_LEVEL;
        InitParams->PCMInterruptLevel  = AUDIO_INTERRUPT_LEVEL;

        /* Configure interface to AUDIO registers */
        InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)ST5100_AUDIO_BASE_ADDRESS;
        InitParams->InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;

        /* Configure interface to ADSC registers */
        InitParams->CDInterfaceType = STAUD_INTERFACE_EMI;
        InitParams->CDInterfaceParams.EMIParams.BaseAddress_p = (void *)ST5100_AUDIO_IF_BASE_ADDRESS;
        InitParams->CDInterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;

        /* CD2InterfaceParams and ClockGeneratorBaseAddress_p not used */

        InitParams->PCM1QueueSize = MAX_PCMBUFFERS;

    }
    else
#endif

#if defined (ST_5301)
   if (LocalDeviceType == 5301)
    {
        /* 5301 is an internal decoder with more interrupts, a unified audio
          interface, and registers spread out like 70xx */

        InitParams->InterruptNumber    = AUDIO_INTERRUPT;
        InitParams->CD1InterruptNumber = ST5301_AUD_CD_INTERRUPT;
        InitParams->PCMInterruptNumber = ST5301_AUD_PCM_INTERRUPT;

        InitParams->InterruptLevel     = AUDIO_INTERRUPT_LEVEL;
        InitParams->CD1InterruptLevel  = AUDIO_INTERRUPT_LEVEL;
        InitParams->PCMInterruptLevel  = AUDIO_INTERRUPT_LEVEL;

        /* Configure interface to AUDIO registers */
        InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)ST5301_AUDIO_BASE_ADDRESS;
        InitParams->InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;

        /* Configure interface to ADSC registers */
        InitParams->CDInterfaceType = STAUD_INTERFACE_EMI;
        InitParams->CDInterfaceParams.EMIParams.BaseAddress_p = (void *)ST5301_AUDIO_IF_BASE_ADDRESS;
        InitParams->CDInterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;

        /* CD2InterfaceParams and ClockGeneratorBaseAddress_p not used */

        InitParams->PCM1QueueSize = MAX_PCMBUFFERS;

    }
    else
#endif

#if defined(ST_7710)
    if (LocalDeviceType == 7710)
    {
        /* 7710 is an internal decoder with more interrupts, a unified audio
          interface, and registers spread out like 70xx */

        InitParams->InterruptNumber    = AUDIO_INTERRUPT;
        InitParams->CD1InterruptNumber = AUD_CD_INTERRUPT;
        InitParams->PCMInterruptNumber = AUD_PCM_INTERRUPT;

        InitParams->InterruptLevel     = AUDIO_INTERRUPT_LEVEL;
        InitParams->CD1InterruptLevel  = AUDIO_INTERRUPT_LEVEL;
        InitParams->PCMInterruptLevel  = AUDIO_INTERRUPT_LEVEL;

        /* Configure interface to AUDIO registers */
        InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)AUDIO_BASE_ADDRESS;
        InitParams->InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;

        /* Configure interface to ADSC registers */
        InitParams->CDInterfaceType = STAUD_INTERFACE_EMI;
        InitParams->CDInterfaceParams.EMIParams.BaseAddress_p = (void *)AUDIO_IF_BASE_ADDRESS;
        InitParams->CDInterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;

        /* CD2InterfaceParams and ClockGeneratorBaseAddress_p not used */

        InitParams->PCM1QueueSize = MAX_PCMBUFFERS;

    }
    else
#endif
    {
        /* open platform / internal decoders */

        InitParams->InterruptNumber = AUDIO_INTERRUPT;
        InitParams->InterruptLevel  = AUDIO_INTERRUPT_LEVEL;

        /* Configure interface to AUDIO registers */
        InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)AUDIO_BASE_ADDRESS;
        InitParams->InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_8;

        /* Configure CD data interface */
        InitParams->CDInterfaceType = STAUD_INTERFACE_EMI;
        InitParams->CDInterfaceParams.EMIParams.BaseAddress_p = (void *)VIDEO_BASE_ADDRESS;
        InitParams->CDInterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_8;

        InitParams->CD2InterfaceType = STAUD_INTERFACE_EMI;
        InitParams->CD2InterfaceParams.EMIParams.BaseAddress_p = (void *)AUDIO_IF_BASE_ADDRESS;
        InitParams->CD2InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_8;
        InitParams->CD2BufferAddress_p = NULL;
        InitParams->CD2BufferSize = 0;

        InitParams->ClockGeneratorBaseAddress_p = (void*)CKG_BASE_ADDRESS;

#if defined (STAUD_OPEN_PLATFORM) /* not available with MPEG1 cell */
        if( LocalDeviceType == DEVTYPE_OPEN )
        {
            /* override a few aspects of the above configuration */
            InitParams->InterruptNumber = EXTERNAL_0_INTERRUPT;
            InitParams->InterfaceParams.EMIParams.BaseAddress_p = (void *)AUD_BASE_ADDRESS_OPEN;
            InitParams->CD2InterfaceParams.EMIParams.BaseAddress_p = NULL;
            InitParams->PCMOutParams.Precision =  STAUD_DAC_DATA_PRECISION_24BITS;  /* 24 bit data */
        }
#endif
    }

    return FALSE; /* okay */
}
/*-------------------------------------------------------------------------
 * Function : AUD_Init
 *              Initialisation
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_Init( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    char LocalDeviceName[40];
    char DefaultDeviceName[40] = DEFAULT_AUD_NAME;
    char EvtHandlerName[40];
    char ClkRecoveryName[40];
    STAUD_InitParams_t InitParams;
    long LVar;
    boolean RetErr = FALSE;
    Init_p new=NULL;
    Init_p WorkPointer=NULL;

#if 0
    /* TEMPORARY: try Dan's workaround from VTG interrupt */
    #pragma ST_translate(interrupt_trigger_mode_level, \
            "interrupt_trigger_mode_level%os")
    extern interrupt_trigger_mode_t interrupt_trigger_mode_level(
            int Level, interrupt_trigger_mode_t trigger_mode);
#endif


/*------Setup (hardwired) InitParams based on testtool switch variable-------*/

    if( RetErr == FALSE )
    {
        SetNonSpecificParams( &InitParams );
        RetErr = SetDeviceSpecificParams (&InitParams );
    }


/*-----------------Setup (user configurable) InitParams-----------------------*/

    /* get DeviceName */
    if ( RetErr == FALSE )
    {
        RetErr = cget_string( pars_p, DefaultDeviceName, LocalDeviceName, sizeof(LocalDeviceName) );
        if( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected DeviceName" );
        }
    }

    /* get BaseAddress */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p,
                               InitParams.InterfaceParams.EMIParams.BaseAddress_p,
                               (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected BaseAddress" );
        }
        else
        {
          InitParams.InterfaceParams.EMIParams.BaseAddress_p = (U8*)LVar;
        }
    }

    /* get MaxOpen */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, InitParams.MaxOpen, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected integer (MaxOpen)" );
        }
        else
        {
            InitParams.MaxOpen = (U32)LVar;
        }
    }

    /* get EvtHandlerName */
    if ( RetErr == FALSE )
    {
        RetErr = cget_string( pars_p, InitParams.EvtHandlerName, EvtHandlerName, sizeof(EvtHandlerName) );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected EvtHandlerName" );
        }
        else
        {
            strcpy(InitParams.EvtHandlerName, EvtHandlerName);
        }
    }

    /* get ClockRecoveryName */
    if ( RetErr == FALSE )
    {
        RetErr = cget_string( pars_p, InitParams.ClockRecoveryName, ClkRecoveryName, sizeof(ClkRecoveryName) );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected ClockRecoveryName" );
        }
        else
        {
            strcpy(InitParams.ClockRecoveryName, ClkRecoveryName);
        }
    }

/*-----------------------------call STAUD_Init-------------------------------*/

    if ( RetErr == FALSE )
    {
        GetPartitionFreeSpace( "Free space before Init", InitParams.CPUPartition_p );
        ErrCode = STAUD_Init( LocalDeviceName, &InitParams );
        GetPartitionFreeSpace( "Free space after Init", InitParams.CPUPartition_p );
    }


#if 0
    /* TEMPORARY: try Dan's workaround from VTG interrupt */
    interrupt_trigger_mode_level(3, interrupt_trigger_mode_high_level);
#endif

/*--------------add structure to the global linked list------------------*/

    if( (RetErr == FALSE) && (ErrCode == ST_NO_ERROR) )    /* create new Init structure */
    {
        new = (Init_p)memory_allocate( AudioPartition_p, sizeof(Init_t));
        new -> next = NULL;
        new -> Open_head = NULL;
        strcpy( new -> DeviceName, LocalDeviceName );

        if(Init_head == NULL)
        {
            Init_head = new;
        }
        else
        {
            WorkPointer = Init_head;
            while( WorkPointer->next != NULL )
            {
                WorkPointer = WorkPointer->next;
            }
            WorkPointer->next = new;
        }
    }

/*-----------------------------report errors----------------------------*/

    if ( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "STAUD_Init(%s,%d) Driver version %s:", LocalDeviceName, (int)
                InitParams.InterfaceParams.EMIParams.BaseAddress_p,
                 STAUD_GetRevision());
        /*RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));      */
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTST_Print(( AUD_Msg ));

        if ( (ErrCode == ST_NO_ERROR) && (Verbose == TRUE))
        {
            {
                ObjectString_t DeviceName;

                DeviceToText(InitParams.DeviceType, &DeviceName);
                sprintf(AUD_Msg, "Device Type        %s\n",DeviceName);
                STTBX_Print(( AUD_Msg ));
            }
            sprintf(AUD_Msg, "Base Address       %X\n",
                    (int)InitParams.InterfaceParams.EMIParams.BaseAddress_p);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "Bit buffer Address %X\n", (int)InitParams.CD1BufferAddress_p);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "Bit buffer size    %X\n", InitParams.CD1BufferSize);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "InterruptNumber    %X\n", InitParams.InterruptNumber);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "InterruptLevel     %X\n", InitParams.InterruptLevel);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "Max Open           %X\n", InitParams.MaxOpen);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "EvtHandlerName     %s\n", InitParams.EvtHandlerName);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "ClkRecoveryName    %s\n", InitParams.ClockRecoveryName);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "Internal PLL       %X\n", InitParams.InternalPLL);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "DACClockToFsRatio  %d\n", InitParams.DACClockToFsRatio);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "InvertWordClock    %d\n", InitParams.PCMOutParams.InvertWordClock);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "InvertBitClock     %d\n", InitParams.PCMOutParams.InvertBitClock);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "DAC Format         %d\n", InitParams.PCMOutParams.Format);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "DAC Precision      %d\n", InitParams.PCMOutParams.Precision);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "Alignment          %d\n", InitParams.PCMOutParams.Alignment);
            STTBX_Print(( AUD_Msg ));
            sprintf(AUD_Msg, "MSBFirst           %d\n", InitParams.PCMOutParams.MSBFirst);
            STTBX_Print(( AUD_Msg ));
        }
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : AUD_Mute

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_Mute( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL AnalogMuted;
    BOOL DigitalMuted;
    boolean RetErr;
    long LVar;

    AnalogMuted = TRUE;
    DigitalMuted = TRUE;
    ErrCode = ST_NO_ERROR;

     /* get Channels */
    RetErr = cget_integer( pars_p, TRUE, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "Mute/Unmute analogue channel. 0|1 expected (1:muted, 0:unmuted)");
        tag_current_line( pars_p, AUD_Msg );
    }
    else
    {
        AnalogMuted = (BOOL)LVar;
        RetErr = cget_integer( pars_p, TRUE, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            sprintf( AUD_Msg, "Mute/Unmute digital channel. 0|1 expected (1:muted, 0:unmuted)");
            tag_current_line( pars_p, AUD_Msg );
        }
        else
        {
            DigitalMuted = (BOOL)LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        ErrCode = STAUD_Mute( AudDecHndl0, AnalogMuted, DigitalMuted );
        sprintf( AUD_Msg, "STAUD_Mute(%u,%d,%d) : ", (U32)AudDecHndl0, AnalogMuted, DigitalMuted);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTST_Print(( AUD_Msg ));
#if !defined (ST_7020) && (defined (ST_5514) || defined (ST_5516) || defined (ST_5517) || defined (ST_5528) || defined(ST_4629))
        /* Wrapper on board DAC mute/unmute in this call */
        if(AnalogMuted)
        {
            ErrCode = STAUD_OPMute( AudDecHndl0, STAUD_OBJECT_OUTPUT_STEREOPCM1);
        }
        else
        {
            ErrCode = STAUD_OPUnMute( AudDecHndl0, STAUD_OBJECT_OUTPUT_STEREOPCM1);
        }
        sprintf( AUD_Msg, "STAUD_Mute(%u,%d,%d) : ", (U32)AudDecHndl0, AnalogMuted, DigitalMuted);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTST_Print(( AUD_Msg ));
#endif
    }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_Mute */
/*-------------------------------------------------------------------------
 * Function : AUD_OPUnMute

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/

static BOOL AUD_OPUnMute( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    long LVar = 0;

    STAUD_Object_t OutputObject;    ObjectString_t OPObject;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_OUTPUT_MULTIPCM1, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Object expected: OBJ_OPMULTI, OBJ_OPSPDIF\n");
    }
    else
    {
        OutputObject = (STAUD_Object_t)LVar;
        ObjectToText( OutputObject, &OPObject);
    }

    if ( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "STAUD_OPUnMute(%u, %s) : ", (U32)AudDecHndl0, OPObject);
        ErrCode = STAUD_OPUnMute( AudDecHndl0, OutputObject );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );
}
/*-------------------------------------------------------------------------
 * Function : LoadStream
 *            Load a mpeg file in memory
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean LoadStream(char *FileName, BOOL FileIsByteReversed)
{
#define FILE_BLOCK_SIZE 102400
    boolean RetErr;
    static long int FileSize, BytesRead, tmp, index;
    static char *BlockTampon;
#if defined(ST_OS21)
    FILE *FileDescriptor_p;
#else
    static long int FileDescriptor;
#endif

    int InjectDest;
    AUD_InjectTask_t *InjectTask_p;

    RetErr = FALSE;

    /* switch according to INJECT_DEST testtool variable */
    evaluate_integer( "INJECT_DEST", &InjectDest, 10 );
    switch (InjectDest)
    {
        case 1:
        case 2:
        case 3:
        case 4:
            InjectTask_p = &InjectTaskTable[InjectDest-1];
            break;
        default:
            RetErr = TRUE;
            break;
    }

    if( RetErr )
    {
        STTBX_Print(("Error: Unsupported INJECT_DEST=%d\n", InjectDest));
        return RetErr;
    }

    if (InjectTask_p->AllocatedBuffer != NULL)
    {
        memory_deallocate(AudioPartition_p, InjectTask_p->AllocatedBuffer);
        InjectTask_p->AllocatedBuffer = NULL;
    }
    /* Get memory to store file data */
#if defined(ST_OS21)
    FileDescriptor_p = fopen(FileName, "rb");
    if (FileDescriptor_p == NULL )
    {
        sprintf(AUD_Msg, "unable to open %s ...\n", FileName);
        STTBX_Print(( AUD_Msg));
        RetErr = TRUE;
    }
#else
    FileDescriptor = debugopen(FileName, "rb" );
    if (FileDescriptor< 0 )
    {
        sprintf(AUD_Msg, "unable to open %s ...\n", FileName);
        STTBX_Print(( AUD_Msg));
        RetErr = TRUE;
    }
#endif
    else
    {
        STTBX_Print(("loading file for CD%d...\n", InjectDest));
#if defined(ST_OS21)
         fseek(FileDescriptor_p,0,SEEK_END);
         FileSize = (U32) ftell(FileDescriptor_p);
         rewind(FileDescriptor_p);
#else
        FileSize = debugfilesize(FileDescriptor);
#endif

        while ( FileSize > 0 && InjectTask_p->AllocatedBuffer == NULL )
        {
            /* we want to align to a 16-byte boundary, which means we could
              start up to 15 bytes into the allocated block, and still write
              FileSize bytes. We also pad the end to a 16-byte boundary */

            InjectTask_p->AllocatedBuffer = (char *)
            memory_allocate(AudioPartition_p, (U32)((FileSize + 31) & (~15)) );
            if (InjectTask_p->AllocatedBuffer == NULL)
            {
                FileSize-=200000; /* file too big : truncate the size */
                STTBX_Print(( "Not enough memory for file loading : try to truncate to %ld \n",FileSize));
                if ( FileSize < 0 )
                {
                    STTBX_Print(( "Not enough memory, file %s not loaded \n",FileName));
                    RetErr = TRUE;
                }
            }
        }

        STTBX_Print(( "Before copying the file\n" ));
        if ( RetErr == FALSE )
        {
            /* --- Setup the alligned buffer, load file and return --- */
            InjectTask_p->FileBuffer = (unsigned char *)(((unsigned int)InjectTask_p->AllocatedBuffer + 15) & (~15));
            InjectTask_p->NbBytes = (int)FileSize;

            if ( !FileIsByteReversed )
                /* file is not byte reversed. The file is read in one big block */
            {
#if defined(ST_OS21)

               BytesRead = fread(InjectTask_p->FileBuffer,sizeof(U32),FileSize,FileDescriptor_p);
#else
                BytesRead = debugread(FileDescriptor, InjectTask_p->FileBuffer, (size_t) FileSize);
                STTBX_Print(( "After coping the file\n" ));
#endif

            }
            else
                /* file is byte reversed. The file is read in small blocks and reversed before copying */
            {
                BlockTampon = (char *)memory_allocate(AudioPartition_p, (U32) FILE_BLOCK_SIZE);
                BytesRead = 0;
                do
                {
#if defined(ST_OS21)
                    tmp = fread(BlockTampon,sizeof(U32), FILE_BLOCK_SIZE,FileDescriptor_p);
#else
                    tmp = debugread(FileDescriptor, BlockTampon, FILE_BLOCK_SIZE );
#endif
                    for ( index = 0; index <= tmp; index+=2 )
                    {
                        *(InjectTask_p->FileBuffer+BytesRead+index) = *(&BlockTampon[0]+index+1);
                        *(InjectTask_p->FileBuffer+BytesRead+index+1) = *(&BlockTampon[0]+index);
                    }
                    if( tmp > 0 )
                    {
                        BytesRead += tmp;
                    }
                } while (BytesRead<FileSize);
                memory_deallocate(AudioPartition_p, BlockTampon);
            }

            sprintf(AUD_Msg, "file [%s] loaded : size %ld at address 0x%08x \n",
                    FileName, FileSize, (int)InjectTask_p->FileBuffer);

            STTBX_Print(( AUD_Msg));
        }
#if defined(ST_OS21)
        fclose(FileDescriptor_p);
#else
        debugclose(FileDescriptor);
#endif

    }

#if defined (STAUD_DEBUG) && 0
#define TMP_SIZE 2048
    /* Paranoid read again and check for errors mode */
    {
        int i = 0, j = 0;
        long ReadSize;
        static U8 TmpBuffer[TMP_SIZE] = {0};
#if defined(ST_OS21)
        FileDescriptor_p = fopen(FileName, "rb");
        fseek(FileDescriptor_p,0,SEEK_END);
        {
            U32 Tmp;
            Tmp = (U32) ftell(FileDescriptor_p);
            rewind(FileDescriptor_p);
            if(FileSize != Tmp )
            {
                STTBX_Print(("ERROR: File reload failed, sizes differ\n"));
            }
        }
#else
        FileDescriptor = debugopen(FileName, "rb" );
        if(FileSize != debugfilesize(FileDescriptor) )
        {
            STTBX_Print(("ERROR: File reload failed, sizes differ\n"));
        }
#endif

        else
        {
            while(j<FileSize)
            {
#if defined(ST_OS21)
                ReadSize = fread(TmpBuffer,sizeof(U32), TMP_SIZE,FileDescriptor_p);
#else
                ReadSize = debugread(FileDescriptor, TmpBuffer, TMP_SIZE);
#endif
                if(ReadSize > 0)
                {
                    for(i=0; i<ReadSize; i++, j++)
                    {
                        if(InjectTask_p->FileBuffer[j] != TmpBuffer[i])
                        {
                            STTBX_Print(("ERROR: File reload failed %u, a:%02x b:%02x\n",
                                         j, InjectTask_p->FileBuffer[j], TmpBuffer[i]));
                            break;
                        }
                    }

                    if(i != ReadSize)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        if(j == FileSize)
        {
            STTBX_Print(("File reread is okay\n"));
        }
#if defined(ST_OS21)
        fclose(FileDescriptor_p)
#else
        debugclose(FileDescriptor);
#endif
    }
#endif
    return(RetErr);

} /* end LoadStream */

/*-------------------------------------------------------------------------
 * Function : AUD_Load
 *            Load a mpeg file in memory
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static boolean AUD_Load(parse_t *pars_p, char *result_sym_p )
{
  boolean RetErr;
  char FileName[256];
  BOOL bMode;

    RetErr = FALSE;
    /* get file name */
    memset( FileName, 0, sizeof(FileName));
    RetErr = cget_string( pars_p, "", FileName, sizeof(FileName) );
    if ( RetErr == TRUE )
    {
      tag_current_line( pars_p, "expected FileName" );
    }

    RetErr = cget_integer( pars_p, 0, (long *)&bMode );
    if ( RetErr == TRUE )
    {
      tag_current_line( pars_p, "expected 1 for reversed, 0 for normal" );
    }

    if (RetErr == FALSE)
    {
        RetErr = LoadStream(FileName, bMode);
    }

    return(RetErr);

} /* end AUD_Load */

/*-------------------------------------------------------------------------
 * Function : AUD_MemInject
 *            Inject Audio in Memory to DMA
 * Input    : *pars_p, *Result_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_MemInject( parse_t *pars_p, char *Result )
{
    boolean RetErr = FALSE;
    BOOL NotSubTask;
    int Ret;
    int InjectDest;
    AUD_InjectTask_t *InjectTask_p;


    /* switch according to INJECT_DEST testtool variable */
    evaluate_integer( "INJECT_DEST", &InjectDest, 10 );
    switch (InjectDest)
    {
        case 1:
        case 2:
        case 3:
        case 4:
            InjectTask_p = &InjectTaskTable[InjectDest-1];
            break;
        default:
            RetErr = TRUE;
            break;
    }

    if( RetErr )
    {
        STTBX_Print(("Error: Unsupported INJECT_DEST=%d\n", InjectDest));
        return RetErr;
    }

    if ( InjectTask_p->NbBytes<=0 )
    {
        STTBX_Print(("No Audio in memory for CD%d\n", InjectDest));
        return TRUE;
    }

    cget_integer( pars_p, 1, (long *)&InjectTask_p->Loop );
    cget_integer( pars_p, FALSE, (long *)&NotSubTask );

    if ( InjectTask_p->TaskID != NULL )
    {
        STTBX_Print(("Audio Task for CD%d already running\n", InjectDest));
        return TRUE;
    }

    if ( NotSubTask == FALSE )
    {
        if(InjectTask_p->Injector_p == NULL)
        {
            STTBX_Print(("InjectTask_p->Injector_p is NULL\n"));
            return TRUE;
        }

#if !defined(ST_OS21)
        /*allocate memory for task*/
        InjectTask_p->TaskID = (task_t*)memory_allocate( AudioPartition_p, sizeof(task_t));

        Ret = task_init(InjectTask_p->Injector_p,
                        &InjectTask_p->Loop,
                        InjectTask_p->Stack,
                        AUDIO_WORKSPACE,
                        InjectTask_p->TaskID,
                        &InjectTask_p->TaskDesc,
                        0,
                        "AudioInject",
                        0);

        if ( Ret != 0 )
        {
            /* deallocate memory */
            memory_deallocate( AudioPartition_p, InjectTask_p->TaskID );
            InjectTask_p->TaskID = NULL;
        }
#else
        InjectTask_p->TaskID = task_create(InjectTask_p->Injector_p,
                                           &InjectTask_p->Loop,
                                           AUDIO_WORKSPACE,
                                           0,
                                           "AudioInject",
                                           task_flags_no_min_stack_size);
#endif

        if ( InjectTask_p->TaskID == NULL )
        {
            STTBX_Print(("Unable to Create Audio task for CD%d\n",
                         InjectDest));
        }
        else
        {
            STTBX_Print(("Playing Audio %d times into CD%d\n",
                         InjectTask_p->Loop, InjectDest ));
        }
    }
    else
    {
        InjectTask_p->Injector_p( &InjectTask_p->Loop );
    }


    return FALSE;
}

/*-------------------------------------------------------------------------
 * Function : AUD_DRStart
 *            Inject Audio in Memory to DMA
 * Input    : *pars_p, *Result_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/

static BOOL AUD_DRStart( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STAUD_StreamParams_t StreamParams;
    STAUD_Object_t DecoderObject;
    char Buff[160];
    boolean RetErr = FALSE;
    long LVar;

    /* get Stream Content (default: STAUD_STREAM_CONTENT_MPEG1) */
    RetErr = cget_integer( pars_p, STAUD_STREAM_CONTENT_MPEG1, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( Buff,"expected Decoder (%d=AC3 %d=DTS %d=MPG1 "
                 "%d=MPG2 %d=CDDA %d=PCM %d=LPCM %d=PINK %d=MP3 %d=NULL)",
                 STAUD_STREAM_CONTENT_AC3,
                 STAUD_STREAM_CONTENT_DTS,
                 STAUD_STREAM_CONTENT_MPEG1,
                 STAUD_STREAM_CONTENT_MPEG2,
                 STAUD_STREAM_CONTENT_CDDA,
                 STAUD_STREAM_CONTENT_PCM,
                 STAUD_STREAM_CONTENT_LPCM,
                 STAUD_STREAM_CONTENT_PINK_NOISE,
                 STAUD_STREAM_CONTENT_MP3,
                 STAUD_STREAM_CONTENT_NULL);
        tag_current_line( pars_p, Buff );
    }
    else
    {
        StreamParams.StreamContent = (STAUD_StreamContent_t)LVar;

        /* get Stream Type (default: STAUD_STREAM_TYPE_PES) */
        RetErr = cget_integer( pars_p, STAUD_STREAM_TYPE_ES, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( Buff,"expected Decoder (%d=ES %d=PES "
                "%d=MPEG1_PACKET %d=PES_DVD)",
                STAUD_STREAM_TYPE_ES,
                STAUD_STREAM_TYPE_PES,
                STAUD_STREAM_TYPE_MPEG1_PACKET,
                STAUD_STREAM_TYPE_PES_DVD);
            tag_current_line( pars_p, Buff );
        }
        else
        {
            StreamParams.StreamType = (STAUD_StreamType_t)LVar;

            /* get SamplingFrequency (default: 44100) */
            RetErr = cget_integer( pars_p, 44100, &LVar );
            if ( RetErr == TRUE )
            {
                tag_current_line( pars_p, "expected SamplingFrequency "
                    "(32000, 44100, 48000)" );
            }
            else
            {
                StreamParams.SamplingFrequency = (U32)LVar;

                /* get object */
                RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED1, &LVar );
                if ( RetErr == TRUE )
                {
                    tag_current_line( pars_p, "expected Object: OBJ_DRCOMP or OBJ_DRPCM" );
                }
                else
                {
                    DecoderObject = (STAUD_Object_t)LVar;

                    /* get Stream ID (default: STAUD_IGNORE_ID) */
                    RetErr = cget_integer( pars_p, STAUD_IGNORE_ID, &LVar );
                    if ( RetErr == TRUE )
                    {
                        tag_current_line( pars_p, "expected id (from 0 to 31)" );
                    }
                    else
                    {
                        StreamParams.StreamID = (U8)LVar;
                    }
                }
            }
        }
    }

    if ( RetErr == FALSE ) /* only start if all parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_DRStart(%u,(%d, %d, %d, %02.2x)) : ",
                 (U32)AudDecHndl0, StreamParams.StreamContent,
                 StreamParams.StreamType, StreamParams.SamplingFrequency,
                 StreamParams.StreamID );
        /* set parameters and start */
        ErrCode = STAUD_DRStart(AudDecHndl0, DecoderObject, &StreamParams );

        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );


        STTBX_Print(( AUD_Msg ));

#if defined(ST_5100) || defined(ST_5301)
    if(ErrCode == ST_NO_ERROR)
    {
        STAUD_OutputParams_t spdifOutParams;

        ErrCode = STAUD_OPGetParams(AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1,
                                                         &spdifOutParams);

        if(ErrCode == ST_NO_ERROR)
        {
            spdifOutParams.SPDIFOutParams.ConnectSPDIFOut = TRUE;
            ErrCode = STAUD_OPSetParams(AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1,
                                                         &spdifOutParams);
        }
    }
#endif
    } /* end if */

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DRStart */

/*-------------------------------------------------------------------------
 * Function : AUD_OPSetDigitalOutput

 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_OPSetDigitalOutput( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    long LVar1, LVar2, LVar3;
    boolean RetErr;
    STAUD_OutputParams_t spdifOutParams;

    /* Get SPDIF is DISABLE or ENABLE */
    RetErr = cget_integer( pars_p, TRUE, (long *)&LVar1 );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected TRUE | FALSE" );
    }

    /* Get The source of SPDIF Output */
    RetErr = cget_integer( pars_p, STAUD_OBJECT_INPUT_CD1, (long *)&LVar2 );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected OBJ_IPCD1 | OBJ_IPCD2 | OBJ_IPCD3 | OBJ_OPMULTI | OBJ_OPSTEREO" );
    }

    if(LVar2 == STAUD_OBJECT_INPUT_CD1)
    {
        sprintf( AUD_Msg, "STAUD_OPConnectSource(%u,Input objectPut:%d)) : ",
            (U32)AudDecHndl0, STAUD_OBJECT_INPUT_CD1);
        ErrorCode = STAUD_OPConnectSource (AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1,
                                                   STAUD_OBJECT_INPUT_CD1);
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );


    }
    else if(LVar2 == STAUD_OBJECT_INPUT_CD2)
    {
        sprintf( AUD_Msg, "STAUD_OPConnectSource(%u,Input objectPut:%d)) : ",
            (U32)AudDecHndl0, STAUD_OBJECT_INPUT_CD2);
        ErrorCode = STAUD_OPConnectSource (AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1,
                                                   STAUD_OBJECT_INPUT_CD2);
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
    }
    else if(LVar2 == STAUD_OBJECT_INPUT_CD3)
    {
        sprintf( AUD_Msg, "STAUD_OPConnectSource(%u,Input objectPut:%d)) : ",
            (U32)AudDecHndl0, STAUD_OBJECT_INPUT_CD3);
        ErrorCode = STAUD_OPConnectSource (AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1,
                                                   STAUD_OBJECT_INPUT_CD3);
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
    }
    else if(LVar2 == STAUD_OBJECT_OUTPUT_MULTIPCM1)
    {
        sprintf( AUD_Msg, "STAUD_OPConnectSource(%u,Input objectPut:%d)) : ",
            (U32)AudDecHndl0, STAUD_OBJECT_OUTPUT_MULTIPCM1);
        ErrorCode = STAUD_OPConnectSource (AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1,
                                                   STAUD_OBJECT_OUTPUT_MULTIPCM1);
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
    }
    else if(LVar2 == STAUD_OBJECT_OUTPUT_STEREOPCM1)
    {
        sprintf( AUD_Msg, "STAUD_OPConnectSource(%u,Input objectPut:%d)) : ",
            (U32)AudDecHndl0, STAUD_OBJECT_OUTPUT_STEREOPCM1);
        ErrorCode = STAUD_OPConnectSource (AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1,
                                                   STAUD_OBJECT_OUTPUT_STEREOPCM1);
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
    }

    STTBX_Print(( AUD_Msg ));

     /* get copyright parameter */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, STAUD_COPYRIGHT_MODE_NO_RESTRICTION, (long *)&LVar3 );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected COPYRIGHT_FREE | COPYRIGHT_ONE_COPY | COPYRIGHT_NO_COPY" );
        }
        spdifOutParams.SPDIFOutParams.CopyPermitted = (STAUD_CopyRightMode_t)LVar3;
    }

     /* get latency parameter */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar3 );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected latency parameter" );
        }
        spdifOutParams.SPDIFOutParams.Latency = (U32)LVar3;
        if(0 == spdifOutParams.SPDIFOutParams.Latency)
        {
            spdifOutParams.SPDIFOutParams.AutoLatency = TRUE;
        }
    }

    /* Configure SPDIF output */
    spdifOutParams.SPDIFOutParams.AutoCategoryCode = TRUE;
    spdifOutParams.SPDIFOutParams.CategoryCode = 0;
    spdifOutParams.SPDIFOutParams.AutoDTDI = TRUE;
    spdifOutParams.SPDIFOutParams.DTDI = 0;

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_OPSetParams(%u,Output:%d)) : ",
            (U32)AudDecHndl0,STAUD_OBJECT_OUTPUT_SPDIF1);

        /* set digital output parameters */
        ErrorCode = STAUD_OPSetParams(AudDecHndl0,STAUD_OBJECT_OUTPUT_SPDIF1, &spdifOutParams);
        RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));

        if(FALSE == LVar1)
        {
            sprintf( AUD_Msg, "STAUD_OPDisable(%u,OutPut:%d)) : ",
                (U32)AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1);
            ErrorCode = STAUD_OPDisable (AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1 );
            RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
            STTBX_Print(( AUD_Msg ));
            return ( API_EnableError ? RetErr : FALSE );
        }
        else if (TRUE == LVar1)
        {
            sprintf( AUD_Msg, "STAUD_OPEnable(%u,OutPut:%d)) : ",
                (U32)AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1);
            ErrorCode = STAUD_OPEnable (AudDecHndl0, STAUD_OBJECT_OUTPUT_SPDIF1 );
            RetErr = AUDTEST_ErrorReport( ErrorCode, AUD_Msg );
        }

        STTBX_Print(( AUD_Msg ));
    } /* Params ok */

    assign_integer(result_sym_p, (long)ErrorCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_OPSetDigitalOutput */

/*-------------------------------------------------------------------------
 * Function : AUD_DisableSynchronisation
 *            Disable synchronisation with PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_DisableSynchronisation( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;

    sprintf( AUD_Msg, "STAUD_DisableSynchronisation(%u) : ", (U32)AudDecHndl0 );
    ErrCode = STAUD_DisableSynchronisation( AudDecHndl0 );
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTST_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_DisableSynchronisation */

/*-------------------------------------------------------------------------
 * Function : AUD_EnableSynchronisation
 *            Enable synchronisation with PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_EnableSynchronisation( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;

    sprintf( AUD_Msg, "STAUD_EnableSynchronisation(%u) : ", (U32)AudDecHndl0 );
    ErrCode = STAUD_EnableSynchronisation( AudDecHndl0 );
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    STTST_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_EnableSynchronisation */
/*-------------------------------------------------------------------------
 * Function : AUD_Open
 *            Share physical decoder usage
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean AUD_Open( parse_t *pars_p, char *result_sym_p )
{
   ST_ErrorCode_t ErrCode;
    STAUD_OpenParams_t OpenParams;
    STAUD_DynamicRange_t DynamicRange;
    char LocalDeviceName[40];
    char DefaultDeviceName[40] = DEFAULT_AUD_NAME;
    long LVar;
    boolean RetErr;
    Init_p SearchPointer=NULL;
    Init_p ResultPointer=NULL;
    Open_p new=NULL;
    Open_p WorkPointer=NULL;

    ErrCode = ST_NO_ERROR;

    RetErr = cget_string( pars_p, DefaultDeviceName, LocalDeviceName, sizeof(LocalDeviceName) );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected DeviceName" );
    }

    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected SyncDelay" );
        }
        else
        {
            OpenParams.SyncDelay=(U32)LVar;
        }
    }

    if ( RetErr == FALSE )
    {
        ErrCode = STAUD_Open( LocalDeviceName, &OpenParams, &AudDecHndl0 );
    }

/*--------------add structure to the linked list------------------*/
    if( ErrCode == ST_NO_ERROR )
    {
        /*get appropriate Init - maybe this should be in a separate function*/
        SearchPointer = Init_head;             /*initialise pointers*/
        ResultPointer = NULL;

        while( SearchPointer != NULL )
        {
            if( strcmp(SearchPointer->DeviceName, LocalDeviceName) == 0 )
            {
                ResultPointer = SearchPointer;
                STTBX_Report(( STTBX_REPORT_LEVEL_INFO,
                               "Found correct devicename, storing handle\n" ));
                break;
            }
            else
            {
                SearchPointer = SearchPointer->next;
            }
        }

        if( ResultPointer == NULL )
        {
            STTBX_Report(( STTBX_REPORT_LEVEL_ERROR,
                           "This device was not initialised in the test harness\n" ));
        }
        else                                /* create new Open structure */
        {
            new = (Open_p)memory_allocate( AudioPartition_p, sizeof(Open_t));
            new -> next = NULL;
            new -> DecoderHandle = AudDecHndl0;

            if(ResultPointer -> Open_head == NULL)                /* list empty */
            {
                ResultPointer -> Open_head = new;
            }
            else
            {
                WorkPointer = ResultPointer -> Open_head;
                while( WorkPointer->next != NULL )
                {
                    WorkPointer = WorkPointer->next;
                }
                WorkPointer->next = new;
            }
        }
    }

    if( RetErr == FALSE )
    {
        sprintf( AUD_Msg, "STAUD_Open(%s,%d,%u) : ", LocalDeviceName, OpenParams.SyncDelay, (U32)AudDecHndl0 );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);

#ifdef DVD_SERVICE_DIRECTV
    if( RetErr == FALSE )
    {
        /* need to set BroadcastProfile to get correct CLKRV conversion factor */
        ErrCode = STAUD_DRSetBroadcastProfile(AudDecHndl0, STAUD_OBJECT_DECODER_COMPRESSED1,
                                              STAUD_BROADCAST_DIRECTV);
        sprintf( AUD_Msg, "STAUD_DRSetBroadcastProfile(STAUD_BROADCAST_DIRECTV) : ");
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }
#endif

    /* override MPEG2/MP3 Dynamic Range Control, which defaults to on, but
      requires ancillary data that is not present in most of our test streams.
      Running the algorithm with other data leads to volume jumps on 7020 */

    if( RetErr == FALSE )
    {

        ErrCode = STAUD_DRGetDynamicRangeControl(AudDecHndl0,
                                                 STAUD_OBJECT_DECODER_COMPRESSED1,
                                                 &DynamicRange);
        sprintf( AUD_Msg, "STAUD_DRGetDynamicRangeControl() : ");
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }

    if( RetErr == FALSE )
    {
        DynamicRange.Enable = FALSE;

        ErrCode = STAUD_DRSetDynamicRangeControl(AudDecHndl0,
                                                 STAUD_OBJECT_DECODER_COMPRESSED1,
                                                 &DynamicRange);
        sprintf( AUD_Msg, "STAUD_DRSetDynamicRangeControl(DISABLE) : ");
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTBX_Print(( AUD_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end AUD_Open */


/*-------------------------------------------------------------------------
 * Function : AUD_SetSpeaker
 *            Defines the state of the speakers
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_SetSpeaker( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    long LVar;
    STAUD_Speaker_t SpeakerOn;

    SpeakerOn.Front = TRUE;
    SpeakerOn.LeftSurround = FALSE;
    SpeakerOn.RightSurround = FALSE;
    SpeakerOn.Center = FALSE;
    SpeakerOn.Subwoofer = FALSE;
    RetErr = cget_integer( pars_p, TRUE, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Front speakers active expected 0 or 1");
    }
    if (RetErr == FALSE)
    {
        SpeakerOn.Front = (BOOL) LVar;
    }

    /* Only LR outputs present on 5516/17 */
    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, FALSE, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Left Surround speaker active expected 0 or 1");
        }
        if (RetErr == FALSE)
        {
            SpeakerOn.LeftSurround = (BOOL) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, FALSE, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Right Surround speaker active expected 0 or 1");
        }
        if (RetErr == FALSE)
        {
            SpeakerOn.RightSurround = (BOOL) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, FALSE, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Center speaker active expected 0 or 1");
        }
        if (RetErr == FALSE)
        {
            SpeakerOn.Center = (BOOL) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = cget_integer( pars_p, FALSE, (long *)&LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "Subwoofer speaker active expected 0 or 1");
        }
        if (RetErr == FALSE)
        {
            SpeakerOn.Subwoofer = (BOOL) LVar;
        }
    }

    if (RetErr == FALSE)
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_SetSpeaker(%u,%u,%u,%u,%u,%u) : ", (U32)AudDecHndl0,
            SpeakerOn.Front, SpeakerOn.LeftSurround, SpeakerOn.RightSurround,
            SpeakerOn.Center, SpeakerOn.Subwoofer);
        ErrCode = STAUD_SetSpeaker(AudDecHndl0, SpeakerOn);
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTST_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_SetSpeaker */

/*-------------------------------------------------------------------------
 * Function : AUD_GetSpeaker
 *            Show the state of the speakers
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_GetSpeaker( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    boolean RetErr;
    STAUD_Speaker_t SpeakerOn;

    SpeakerOn.Front = FALSE;
    SpeakerOn.LeftSurround = FALSE;
    SpeakerOn.RightSurround = FALSE;
    SpeakerOn.Center = FALSE;
    SpeakerOn.Subwoofer = FALSE;

    RetErr = TRUE;
    sprintf( AUD_Msg, "STAUD_GetSpeaker(%u) : ", (U32)AudDecHndl0);
    ErrCode = STAUD_GetSpeaker(AudDecHndl0, &SpeakerOn);
    RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
    if( RetErr == FALSE )
    {
        if (SpeakerOn.Front         == TRUE)
        {
            strcat( AUD_Msg, " Front" );
        }
        if (SpeakerOn.LeftSurround  == TRUE)
        {
            strcat( AUD_Msg, " LeftSurround" );
        }
        if (SpeakerOn.RightSurround == TRUE)
        {
            strcat( AUD_Msg, " RightSurround" );
        }
        if (SpeakerOn.Center        == TRUE)
        {
            strcat( AUD_Msg, " Center" );
        }
        if (SpeakerOn.Subwoofer     == TRUE)
        {
            strcat( AUD_Msg, " Subwoofer" );
        }
        strcat( AUD_Msg, "\n" );
    }
    STTST_Print(( AUD_Msg ));
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_GetSpeaker */


static STAUD_StartParams_t StartParams =
    {STAUD_STREAM_CONTENT_MPEG1, STAUD_STREAM_TYPE_ES, 44100, STAUD_IGNORE_ID};
/*-------------------------------------------------------------------------
 * Function : AUD_Start
 *            Start decoding of input stream
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_Start( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    char Buff[160];
    boolean RetErr;
    long LVar;

    RetErr = FALSE;
    ErrCode = ST_NO_ERROR;

    /* get Stream Content (default: STAUD_STREAM_CONTENT_MPEG1) */
    RetErr = cget_integer( pars_p, STAUD_STREAM_CONTENT_MPEG1, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( Buff,"expected Decoder (%d=AC3 %d=DTS %d=MPG1 "
                 "%d=MPG2 %d=CDDA %d=PCM %d=LPCM %d=PINK %d=MP3)",
                 STAUD_STREAM_CONTENT_AC3,
                 STAUD_STREAM_CONTENT_DTS,
                 STAUD_STREAM_CONTENT_MPEG1,
                 STAUD_STREAM_CONTENT_MPEG2,
                 STAUD_STREAM_CONTENT_CDDA,
                 STAUD_STREAM_CONTENT_PCM,
                 STAUD_STREAM_CONTENT_LPCM,
                 STAUD_STREAM_CONTENT_PINK_NOISE,
                 STAUD_STREAM_CONTENT_MP3 );
        tag_current_line( pars_p, Buff );
    }
    else
    {
	StartParams.StreamContent = (STAUD_StreamContent_t)LVar;

	/* get Stream Type (default: STAUD_STREAM_TYPE_PES) */
	RetErr = cget_integer( pars_p, STAUD_STREAM_TYPE_ES, &LVar );
	if ( RetErr == TRUE )
	{
	    sprintf( Buff,"expected Decoder (%d=ES %d=PES "
		     "%d=MPEG1_PACKET %d=PES_DVD)",
		     STAUD_STREAM_TYPE_ES,
		     STAUD_STREAM_TYPE_PES,
		     STAUD_STREAM_TYPE_MPEG1_PACKET,
		     STAUD_STREAM_TYPE_PES_DVD);
	    tag_current_line( pars_p, Buff );
	}
	else
	{
	    StartParams.StreamType = (STAUD_StreamType_t)LVar;

	    /* get SamplingFrequency (default: 44100) */
	    RetErr = cget_integer( pars_p, 44100, &LVar );
	    if ( RetErr == TRUE )
	    {
		tag_current_line( pars_p, "expected SamplingFrequency "
				  "(32000, 44100, 48000)" );
	    }
	    else
	    {
		StartParams.SamplingFrequency = (U32)LVar;

		/* get Stream ID (default: STAUD_IGNORE_ID) */
		RetErr = cget_integer( pars_p, STAUD_IGNORE_ID, &LVar );
		if ( RetErr == TRUE )
		{
		    tag_current_line( pars_p, "expected id (from 0 to 31)" );
		}
		else
		{
		    StartParams.StreamID = (U8)LVar;
		}
	    }
	}
    }

    if ( RetErr == FALSE ) /* only start if all parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_Start(%u,(%d, %d, %d, %02.2x)) : ",
                 (U32)AudDecHndl0, StartParams.StreamContent,
                 StartParams.StreamType, StartParams.SamplingFrequency,
                 StartParams.StreamID );
        /* set parameters and start */
        ErrCode = STAUD_Start(AudDecHndl0, &StartParams );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTST_Print(( AUD_Msg ));
    } /* end if */
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_Start */

/*-------------------------------------------------------------------------
 * Function : AUD_Stop
 *            Stop decoding of input stream
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_Stop( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Stop_t StopMode;
    STAUD_Fade_t Fade;
    long LVar;
    boolean RetErr;

    ErrCode = ST_NO_ERROR;
    /* Stop mode */
    RetErr = cget_integer( pars_p, STAUD_STOP_NOW, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( AUD_Msg, "expected StopMode (%d=end of data,%d=now)",
            STAUD_STOP_WHEN_END_OF_DATA, STAUD_STOP_NOW);
        tag_current_line( pars_p, AUD_Msg );
    }
    StopMode = (STAUD_Stop_t)LVar;

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        sprintf( AUD_Msg, "STAUD_Stop(%u,%d)) :",
                 (U32)AudDecHndl0, StopMode);
        ErrCode = STAUD_Stop( AudDecHndl0, StopMode, &Fade );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );

        STTST_Print(( AUD_Msg ));
    }
    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_Stop */

/*-------------------------------------------------------------------------
 * Function : AUD_Term
 *            Terminate the audio decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_Term( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    char LocalDeviceName[40];
    char DefaultName[40] = STAUD_DEVICE_NAME1;   /*make into a #define for modularity*/
    STAUD_TermParams_t TermParams;
    long LVar;
    boolean RetErr;

    ErrCode = ST_NO_ERROR;

    /* get device name */
    RetErr = cget_string( pars_p, DefaultName, LocalDeviceName, sizeof(LocalDeviceName) );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected DeviceName" );
    }
    /* get term param */
    if ( RetErr == FALSE )
    {
        RetErr = cget_integer( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            tag_current_line( pars_p, "expected ForceTerminate" );
        }
        TermParams.ForceTerminate = (U32)LVar;
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        LocalDACConfiguration.Initialized = FALSE;
        sprintf( AUD_Msg, "STAUD_Term(%s,%d) : ", LocalDeviceName, TermParams.ForceTerminate );

        ErrCode = STAUD_Term( LocalDeviceName, &TermParams );
        RetErr = AUDTEST_ErrorReport( ErrCode, AUD_Msg );
        STTST_Print(( AUD_Msg ));
        assign_integer(result_sym_p, (long)ErrCode, false);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_Term */

/*-------------------------------------------------------------------------
 * Function : AUD_Verbose
 *            set the Verbose flag used in functions to print or not details
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static boolean AUD_Verbose( parse_t *pars_p, char *result_sym_p )
{
    long LVar;
    boolean RetErr;

    /* get Verbose param */
    RetErr = cget_integer( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected TRUE (1) or FALSE (0)" );
    }
    else
    {
        Verbose = (BOOL)LVar;
    }
    STTST_Print(( "AUD_Verbose(%s)\n", ( Verbose ) ? "TRUE" : "FALSE" ));
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_Verbose */
#endif  /* !7100 */






/*#######################################################################*/
/*######################### 7100 but Not LINUX ##########################*/
/*#######################################################################*/

#if defined (ST_7100) || defined (ST_7109)
/*-------------------------------------------------------------------------
 * Function : AUD_LoadAudioMemory
 *            Load the audio file into the designated audio memory location
 *            of LMI
 * Input    :
 * Ouput    :
 * Return   : TRUE if ok
 * ------------------------------------------------------------------------*/
 static boolean AUD_LoadAudioMemory( parse_t *pars_p, char *result_sym_p )
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return FALSE;

#else

    char FileName[MAX_STRING_LENGTH*2];
	static char Argument[MAX_STRING_LENGTH];
    boolean FileIsByteReversed = false;
    BOOL Error;

   	Error = cget_string(pars_p, "toto", FileName, MAX_STRING_LENGTH );
	if ( Error == FALSE )
 	{
        Error = cget_string(pars_p, "n", Argument, MAX_STRING_LENGTH );  /* optional */
        if (!Error)
        {
            printf("Came to the Load Audio to Memory section.......\n");
            /* This function Load the sudio file into the designated audio memory location of LMI */
            sample= Load_Stream(FileName, FileIsByteReversed );
		}
	}
	else
		printf( "The path given too long to handle, extracted ");

    return(FALSE);
#endif  /* LINUX */
}
/*-------------------------------------------------------------------------
 * Function : Load_Stream
 *            load_stream in memory for Pcm player 0 or 1
 * Input    :
 * Ouput    :
 * Return   : File size.
 * ------------------------------------------------------------------------*/

static int Load_Stream(char *FileName, char FileIsByteReversed )
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return 0;

#else

    const char ch_opt;               /*Array to hold the file name and the choice for byte reversible option*/
    FILE *stream1;                            /*File pointer to hold the address of the file to open to load*/
    unsigned char *tmp;                       /* temporary pointer (buffer) where the readed value of the file will be placed*/
    unsigned int value0=0,value1=0,NoBytes=0; /*value0, for lower byte and value1 for higher byte,NoBytes:-Total no of byutes readed.*/
	unsigned int value_l=0,value_r=0;
    unsigned int offset=0;                    /*offset:- increment for memory location*/
    unsigned int *pcmp_add,*pcmp_add1;                   /*memory location address where the readed value from the file to be loaded*/
    int numread=0,i=0;                        /*number of bytes readed from the file in a go;i is incremental to buffer address to fetch the data placed in it*/
    int flag=0;                               /*flag:- condition to check wether the file is opened once, sets when opened once*/
    unsigned int FileSize=0;                  /*Total number of FileSize the audio file consists of, it will be used to configure PCM CONTROL register 13:31 Bit*/


    printf(" \nReading from the file and loading into Buffer.......................\n");
    pcmp_add = (unsigned int *)(player_buff_address);    /*Memory (LMI) base address where the stream is to be loaded*/


	/**************************Calculating the File Size******************************/
	stream1=fopen(FileName,"rb");
 	if (stream1==NULL)
  	{
   		printf("--> Unable to open the file %s \n",FileName);

  	}
 	fseek(stream1,0,SEEK_END);
 	FileSize=ftell(stream1);
 	fclose(stream1);
	/*******************************************************************************/

    /*File to open*/
    stream1=fopen(FileName,"rb");
	if(stream1==NULL)
		printf(" Unable To Open %s \n",FileName);
	else
	{
		if(FileIsByteReversed)
		{
			printf("%s to memory in Reverse mode\n",FileName);
		}
		else
		{
			printf("%s to memory in No Reverse mode\n",FileName);
		}

		tmp=(unsigned char *)malloc(FILE_BLOCK_SIZE);

		if(tmp==NULL)
			printf("\n Memory Heap allocation cannot be done");
		else
		{
            for(NoBytes=0,numread=1;(numread>0)&&(NoBytes<=MAX_FILE_SIZE);) /*Loop terminate when Numread=0
                                                                             * (no data read from file) and total data (bytes)
                                                                             * read exceed the quota of 600*1024*/

			{
				numread=(int)fread(tmp,BYTES,FILE_BLOCK_SIZE,stream1);

				if(numread>0)
				{
					flag=1;
					if(numread+NoBytes>MAX_FILE_SIZE)

					numread=(MAX_FILE_SIZE-NoBytes);

					for(i=0;i<=(numread-1);)
					{
						value0=0;
						value1=0;
						if(FileIsByteReversed)
						{
							/***********************************************************************/
							/*               PCM MODE  MEMORY WRITING                              */
							/***********************************************************************/

                            value_l=(((tmp[i+1])<<24)+((tmp[i]<<16))); /*making 16Bit MSB LEFT ALLIGN Left channel*/
							i=i+2;

                            value_r=(((tmp[i+1])<<24)+((tmp[i]<<16))); /*making 16Bit MSB LEFT ALLIGN Right channel*/
							i=i+2;


                            *((pcmp_add)+(offset)) = value_l;  /*Left Rear...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_r;  /*Right Rear...in one memory location*/
							offset=offset+1;


                            *((pcmp_add)+(offset)) = value_l;  /*Left Center...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_r;  /*Right Center...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_l;  /*Left Front...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_r;  /*Right Front...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_l;  /*Left Surround...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_r;  /*Right Surround...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_l;  /*Left Dual...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_r;  /*Right Dual...in one memory location*/
							offset=offset+1;

							pcmp_add1=(pcmp_add+offset);
						}
						else
						{
							/***********************************************************************/
							/*               PCM MODE  MEMORY WRITING                              */
							/***********************************************************************/
                            value_l=(((tmp[i])<<24)+((tmp[i+1]<<16))); /*making 16Bit MSB LEFT ALLIGN Left channel*/
							i=i+2;

                            value_r=(((tmp[i])<<24)+((tmp[i+1]<<16))); /*making 16Bit MSB LEFT ALLIGN Right channel*/
							i=i+2;


                            *((pcmp_add)+(offset)) = value_l;  /*Left Rear...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_r;  /*Right Rear...in one memory location*/

							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_l;  /*Left Center...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_r;  /*Right Center...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_l;  /*Left Front...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_r;  /*Right Front...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_l;  /*Left Surround...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_r;  /*Right Surround...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_l;  /*Left Dual...in one memory location*/
							offset=offset+1;

                            *((pcmp_add)+(offset)) = value_r;  /*Right Dual...in one memory location*/
							offset=offset+1;

							pcmp_add1=(pcmp_add+offset);
						}
					}

                    NoBytes=(NoBytes+numread);      /*adding 1 because subtracted in for loop*/
				}
				else
				{
					if(flag==0)
						printf("\n READING from the file faled");
					else
						printf("\n\n ALL DATA READED SUCCESSFULLY");
				}
                if(NoBytes%FILE_READ==0)   /*when the data extracted from file multilple of 1Kb, display this information.*/

				{
					printf("\n GOT %dKB of data from the file",(NoBytes/FILE_READ));

				}
			}
		}
    free(tmp);         /*releasing the buffer area allocated for temporarly holding data from the file before placing it to memory*/
	fclose(stream1);
	}

	printf("\n\n The last two sample (L+R) is placed at address in LMI = 0x%x",pcmp_add = pcmp_add+(offset-1));


	printf("\n FileSize of Audio loaded = %d\n",FileSize);

	printf("\n Memory Occupied  = %d\n",FileSize);

	return(FileSize);
#endif  /* LINUX */
} /* end of Load_Stream() */
/* ----------------------------------------------------------------------------
 *  function:   AUD_CommandSetStatusI2s
 *              Read all registers off IS2 Spdif
 *  input:      testtool parameters
 *  output   : none
 * Return   : TRUE if ok
 * ---------------------------------------------------------------------------- */
boolean AUD_CommandWriteStatusI2s( parse_t *parse_p, char *result_sym_p )
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return FALSE;

#else

  printf("\t  SET all I2S Registers I2s Spdif \n");

  printf(" Set Configuration_REG    = 0x%x\n",0x25);
  STSYS_WriteRegDev32LE(0xB8103800,0x25);

  printf(" Set Spdif_control_REG    = 0x%x\n",0x4053);
  STSYS_WriteRegDev32LE(0xB8103a00,0x4053);
  return(FALSE);
#endif  /* LINUX */
}
/* ----------------------------------------------------------------------------
 *  function : AUD_PCMPlayer_Digital_0
 *             Play the pcmplayer#0 Digital
 *  input    : Testtool parameters
 *  output   : none
 * Return    : TRUE if ok
 * ---------------------------------------------------------------------------- */
static boolean AUD_PCMPlayer_Digital_0(parse_t *pars_p, char *result)
{
#ifdef CLEAN_AUD_CMD
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return FALSE;

#else

	unsigned int temp;
    BOOL Error;

	Error = cget_integer(pars_p,1, &temp);
	if ( Error == FALSE )
	{
		printf( "The argument extracted = %d\n", temp);
        PlayPCM0_Stream(temp,sample);
	}
	return(FALSE);
#endif  /* LINUX */
#endif
}
/* ----------------------------------------------------------------------------
 *  function :   PlayPCM0_Config
 *              configure the PCM player configuration register,
 *              intialize the interrupts, maps the interrupts, enable the interrupt.
 *  input    : Testtool parameters
 *  output   : none
 * Return    : none
 * ---------------------------------------------------------------------------- */
static void PlayPCM0_Config()
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return;

#else

    int result_pcmp0;
    PCMPlayer_Control PCMPControl0;
    PCMPlayer_Format  PCMPFormat0;
    unsigned int Format0=0;
    unsigned int Control0=0;


    /***************************************************/
    /*        PCM Clock Configuration                  */
    /***************************************************/
    PCM_ClockConfiguration();

    /***************************************************/
	/*       SETTING PCMPlayer#0 INTERRUPT             */
	/***************************************************/
	*((unsigned int *)(PCMPLAYER0_ITRP_ENABLE_REG)) = 0x2;
    *((unsigned int *)(PCMPLAYER0_ITRP_ENABLE_SET_REG)) = 0x3; /*setting the 0th and 1st bit to enable
                                                                Data Underflow and Memory Block fully read interrupt*/



	/*****      CONFIGURATION OF PCMP FORMAT REGISTER       ****/
    PCMPFormat0.Bit_SubFrame = subframebit_32;
    PCMPFormat0.DataLength = bit_24;
    PCMPFormat0.LRlevel    = leftword_lrclkhigh;
    PCMPFormat0.SClkedge   = risingedge_pcmsclk;
    PCMPFormat0.Padding    = delay_data_onebit_clk;
    PCMPFormat0.Align      = data_first_sclkcycle_lrclk;
    PCMPFormat0.Order      = data_msbfirst;

    Format0 = (PCMPFormat0.Bit_SubFrame);
    Format0 |= (PCMPFormat0.DataLength<<1);
    Format0 |= (PCMPFormat0.LRlevel<<3);
    Format0 |= (PCMPFormat0.SClkedge<<4);
    Format0 |= (PCMPFormat0.Padding<<5);
    Format0 |= (PCMPFormat0.Align<<6);
    Format0 |= (PCMPFormat0.Order<<7);

    set_reg(PCMPLAYER0_FORMAT_REG,Format0);
    printf("\n PCMPLAYER FORMAT REGISTER = %x\n",Format0);
	/**********************************************************/

	/*****  CONFIGURATION OF PCMP CONTROL REGISTER         ****/
    PCMPControl0.Operation             = pcm_off;
    PCMPControl0.Memory_Storage_Format = bits_16_0;
    //pcmpcontrol0.Memory_Storage_Format = bits_16_16;
    PCMPControl0.Rounding              = no_rounding_pcm;
    PCMPControl0.Divider64             = fs_256_64;
    PCMPControl0.Wait_Eolatency        = donot_wait_spdif_latency;
    PCMPControl0.Samples_Read          = AudioStream_PCMP0.Samples;

    Control0 = (PCMPControl0.Operation);
    Control0 |= (PCMPControl0.Memory_Storage_Format<<2);
    Control0 |= (PCMPControl0.Rounding<<3);
    Control0 |= (PCMPControl0.Divider64<<4);
    Control0 |= (PCMPControl0.Wait_Eolatency<<12);
    Control0 |= ((0x100)<<13);

    set_reg(PCMPLAYER0_CONTROL_REG,Control0);
    printf("\n PCMPLAYER CONTROL REGISTER =%x\n",Control0);

	/***********************************************************/
    playpcm0_first_time = 0;                                          /*This playpcm0_config() will be done once*/
#endif /* LINUX */
}


/* ----------------------------------------------------------------------------
 *  function :   Create_PCMP0_Node
 *              create and configure the nodes
 *              Link List creation for the FDMA,return the first node pointer.
 *  input    : Testtool parameters
 *  output   : none
 *  Return   : none
 * ---------------------------------------------------------------------------- */
static void Create_PCMP0_Node()
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return;

#else

    unsigned int control_start0,temporary1_pcm0;
    PCMPlayer_Control PCMPControl0;
	ST_ErrorCode_t ErrorCode;
	STFDMA_Node_t *nodepcmp0,*temp_nodepcmp0;
	STFDMA_TransferParams_t pcmp0_transfer_param;
	STFDMA_TransferId_t FDMATransferId = 0;

    //nodepcmp0 = (STFDMA_Node_t *)calloc(5,sizeof(generic_node));  /*alocating the memory 5 times the size of the node (32Bytes), which need to free after use.*/
	//temp_nodepcmp0   = memory_allocate(NcachePartition,(sizeof(STFDMA_Node_t))+31);
	//temp_nodepcmp0	   = (STFDMA_Node_t *)malloc((sizeof(STFDMA_Node_t))+31);
	temp_nodepcmp0  =  (STFDMA_Node_t *)0xA5100000;

	if(temp_nodepcmp0 == NULL)
		printf("Memory allocation to node structure of size %d failed .....\n",sizeof(STFDMA_Node_t));
	else
		printf("Memory allocation to node structure of size %d successfull .....\n",sizeof(STFDMA_Node_t));


	//nodepcmp0_free = nodepcmp0;                                   //Node structure pointer need to kept to free the allocated memory after it has been used

	/*******************************************************/
	/*       making the nodepcmp 32Byte allign             */
	/*******************************************************/
	//nodepcmp0 = (STFDMA_Node_t *)(((unsigned int)(temp_nodepcmp0+31))&~31);

	nodepcmp0 =(STFDMA_Node_t *)0xA5100000;

	/*******************************************************/
	/*     configuring the FIRST node                      */
	/*******************************************************/
	(*nodepcmp0).Next_p	= NULL;
	(*nodepcmp0).NodeControl.PaceSignal				= STFDMA_REQUEST_SIGNAL_PCM0;
	(*nodepcmp0).NodeControl.SourceDirection		= STFDMA_DIRECTION_INCREMENTING;
	(*nodepcmp0).NodeControl.DestinationDirection	= STFDMA_DIRECTION_STATIC;
	(*nodepcmp0).NodeControl.Reserved				= 0;
	(*nodepcmp0).NodeControl.NodeCompletePause		= FALSE;
	(*nodepcmp0).NodeControl.NodeCompleteNotify		= FALSE;


    (*nodepcmp0).NumberBytes                        = (10*(AudioStream_PCMP0.Audio_Size));     //For 10 channels
    temporary1_pcm0                                 = (unsigned int)(AudioStream_PCMP0.P_Audio_pointer);
	(*nodepcmp0).SourceAddress_p					= (unsigned int *)((temporary1_pcm0) & (0x0FFFFFFF));
	(*nodepcmp0).DestinationAddress_p				= (unsigned int *)((PCMPLAYER0_FIFO_DATA_REG)&(0x1FFFFFFF));
    (*nodepcmp0).Length                             = (10*(AudioStream_PCMP0.Audio_Size));
    (*nodepcmp0).SourceStride                       = (10*(AudioStream_PCMP0.Audio_Size));/*(*nodepcmp0).Length;*/
	(*nodepcmp0).DestinationStride					= 0;


	/*******************************************************/
	/*     STFDMA_TransferParams_s                      */
	/*******************************************************/
	 pcmp0_transfer_param.ChannelId					= STFDMA_USE_ANY_CHANNEL;
     //pcmp0_transfer_param.NodeAddress_p             = (unsigned int *)((unsigned int)(nodepcmp0)&0x0FFFFFFF);
     pcmp0_transfer_param.NodeAddress_p             = (STFDMA_Node_t *)((unsigned int)(nodepcmp0)&0x0FFFFFFF);
	 pcmp0_transfer_param.BlockingTimeout			= 0;
	 pcmp0_transfer_param.CallbackFunction			= NULL;/*pcmp0_FDMAcallback;*/
	 pcmp0_transfer_param.ApplicationData_p			= NULL;

     pcmp0_transfer_param.FDMABlock                 = STFDMA_1;

	/*******************************************************/
	/*     Start the Channel					            */
	/*******************************************************/

	control_start0 	   = *((volatile unsigned int *)(PCMPLAYER0_CONTROL_REG));
    control_start0    &= (0xFFFFFFFC);                     /*bitwise and to make last to bit 00 and then OR'ing   ...case for MUTE --> START*/
    PCMPControl0.Operation             = pcm_mode_on_16;
	//pcmpcontrol0.operation             = cd_mode_on;
    control_start0 |= (PCMPControl0.Operation);

	set_reg(PCMPLAYER0_CONTROL_REG,control_start0);    //Starting the PCMP0 Player

	ErrorCode = STFDMA_StartTransfer(&pcmp0_transfer_param,&FDMATransferId);
	if(ErrorCode != ST_NO_ERROR)
		printf(" Failded to start the channel................\n");

		printf("\nafter transfer t  INTERRUPT_STATUS_REG = %x\n",read_reg(PCMPLAYER0_ITRP_STATUS_REG));

	// free(temp_nodepcmp0);

        /********************************************************************************/
#endif  /* LINUX */
}
/* ----------------------------------------------------------------------------
 *  function :   PlayPCM0_Stream
 *              This function will be called when audio play command is given,
 *  input    : number of loop, sample
 *  output   : none
 *  Return   : none
 * ---------------------------------------------------------------------------- */
static void PlayPCM0_Stream(unsigned int loop,unsigned int sample)
{
#ifdef CLEAN_AUD_CMD
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return;

#else

    int t;
	task_t *audio_task_list[5];

    AudioStream_PCMP0.P_Audio_pointer = ((unsigned int *)(player_buff_address));  //address from where the audio player will pick the first sample (not the correct one for 7100,change for simulator)
    AudioStream_PCMP0.Audio_Size = sample;
    AudioStream_PCMP0.Loop = loop;                      /*No of times playing has to be done*/

	if( playpcm0_first_time==1)
	{
        PCMP0_Feed_p = semaphore_create_fifo(0);
        PCMP0_Play_p = semaphore_create_fifo(0);
        /* create task.....will call function.....playaudio_pcm();*/

             PCMPlayer_0_p = task_create(
            (void (*)(void *))PlayAudio_PCM0,        //Function to call
			 NULL,                                   //No Arguement
			 16384,                                  //Minimum Define stack size (16KB)
             MAX_USER_PRIORITY,                      //Priority 254, main runs at 255 ( MAX_USER_PRIORITY)
			 "PCM_Player_0",
			 NULL								     //No flag
			 );

        if(PCMPlayer_0_p == NULL)
			printf("\n PCM Player 0 Task not created \n");
		else
            printf("\n PCMPlayer 0  task is running at priority = %d\n",task_priority(PCMPlayer_0_p));

	}

    audio_task_list[0] = PCMPlayer_0_p;

	//semaphore will signal to feed the data and configure to pcmp_0
	j_loop0=1;
	//audio_finish =0;
    semaphore_signal(PCMP0_Feed_p);
    task_delay(500000);
#endif  /* LINUX */
#endif
}

/* ----------------------------------------------------------------------------
 *  function :   PlayAudio_PCM0
 *              This function will be called when audio play command is given,
 *  input    : number of loop, sample
 *  output   : none
 *  Return   : none
 * ---------------------------------------------------------------------------- */
static void PlayAudio_PCM0()
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return;

#else

	if(playpcm0_first_time == 1)
        PlayPCM0_Config();                    //Configures the PCMPLAYER#0 FORMAT and CONTROL registers (start pcm)

    while(1)
    {
        semaphore_wait(PCMP0_Feed_p);
        {
            for(;j_loop0<=AudioStream_PCMP0.Loop;j_loop0++)
			{
                Create_PCMP0_Node();          //Host creates linked list of nodes describing the transfer in main memory

       		}//end of for loop
        } //end of if condition
    }//end of while loop
#endif  /* LINUX */
}
/* ----------------------------------------------------------------------------
 *  function :   PCM_ClockConfiguration
 *              Enable the PCM clock,
 *              Set the Configuration the Frequency Synthesizer for Audio IP's
 *              and Set the Configuration of the PCMPlayer#0, PCMPlayer#1 Clock
 *  input    : Testtool parameters
 *  output   : none
 *  Return   : none
 * ---------------------------------------------------------------------------- */
static void PCM_ClockConfiguration()
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return;

#else

    unsigned int cnfgval1 = 0x00007FFE;                      /*  Generic setting   for 256*48Khz         */
    unsigned int cnfgval2 = 0x00007FFF;                      /*  Generic setting   for 256*48Khz         */

    int val_md = 0;                                          /*  md_ config            for 256*48Khz     */
    int val_pe = 0;                                          /*  pe_ config            for 256*48Khz     */
    int val_sdiv = 0;                                        /*  sdiv config           for 256*48Khz     */
    int val_prg_en = 0x1;                                    /*  prg_en_ config        for 256*48Khz     */

    int clock_pcmp0 = ((int)(FREQ_256_48MHz));
    int clock_pcmp1 = ((int)(FREQ_256_48MHz));


    /* Configuration of the frequency synthesizer */
    set_reg(AUDIO_FSYN_CFG,cnfgval2);
    set_reg(AUDIO_FSYN_CFG,cnfgval1);

    /**************************************************************************/
    /*                          Configuring the PCMPlayer#0 Clock             */
    /**************************************************************************/
    set_reg(AUDIO_FSYN0_PROG_EN,0x0);

    /*AUD_FSYN0_MD*/
    val_md = (CLK_VALUES[clock_pcmp0].MD_Val);
    set_reg(AUDIO_FSYN0_MD,val_md);

    /*AUD_FSYN0_PE*/
    val_pe = (CLK_VALUES[clock_pcmp0].PE_Val);
    set_reg(AUDIO_FSYN0_PE,val_pe);

    /*AUD_FSYN0_SDIV*/
    val_sdiv = (CLK_VALUES[clock_pcmp0].SDIV_Val);
    set_reg(AUDIO_FSYN0_SDIV,val_sdiv);
    /* Enable the AUD_FSYN0 */
    set_reg(AUDIO_FSYN0_PROG_EN,val_prg_en);

    /**************************************************************************/
    /*                          Configuring the PCMPlayer#1 Clock             */
    /**************************************************************************/

     set_reg(AUDIO_FSYN1_PROG_EN,0x0);

     /*AUD_FSYN1_MD*/
     val_md = (CLK_VALUES[clock_pcmp1].MD_Val);
     set_reg(AUDIO_FSYN1_MD,val_md);

     /*AUD_FSYN1_PE*/
     val_pe = (CLK_VALUES[clock_pcmp1].PE_Val);
     set_reg(AUDIO_FSYN1_PE,val_pe);

     /*AUD_FSYN1_SDIV*/
     val_sdiv = (CLK_VALUES[clock_pcmp1].SDIV_Val);
     set_reg(AUDIO_FSYN1_SDIV,val_sdiv);

     /* Enable the AUD_FSYN1 */
     set_reg(AUDIO_FSYN1_PROG_EN,val_prg_en);

     /**************************************************************************/
    /*                          Configuring the PCMPlayer#1 Clock             */
    /**************************************************************************/

     /*Enabling the internal DAC*/
     set_reg(AUDIO_DAC_PCMP1,0x69);
     /* Soft reset of the stereo dacs */
     set_reg(AUDIO_DAC_PCMP1,0x68);
     task_delay(1000);
     set_reg(AUDIO_DAC_PCMP1,0x69);

     /* Enabling the PCM CLK, PCM0 and PCM1 out  */
     set_reg(PCMCLK_IOCTL,0x8);
#endif
}
/* ----------------------------------------------------------------------------
 *  function : AUD_Stop_PCMPlayer_0
               Stop the PCM player 0
 *  input    : Testtool parameters
 *  output   : none
 *  Return   : none
 * ---------------------------------------------------------------------------- */

static boolean AUD_Stop_PCMPlayer_0(parse_t *pars_p, char *result)
{
#ifdef ST_OSLINUX
        printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
        return(TRUE);
#else
		unsigned int control_stop;

		control_stop = *((volatile unsigned int *)(PCMPLAYER0_CONTROL_REG));
		control_stop    &= ((0xFFFFFFFC));
		*((volatile unsigned int *)(PCMPLAYER0_CONTROL_REG)) = control_stop;


        semaphore_delete(PCMP0_Feed_p);
        semaphore_delete(PCMP0_Play_p);
        task_wait(&PCMPlayer_0_p, 1, TIMEOUT_INFINITY);
        task_delete(PCMPlayer_0_p);
        return(TRUE);
#endif
}
/*********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/********************************************************************************/
/*             Name	     :-  load_audio_memory                                  */
/*             Description :-  Play the SPDIF Digital                           */
/********************************************************************************/
static boolean STAUD_load_audio_memory_spdif(parse_t *pars_p, char *result)
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return FALSE;

#else

    char FileName[MAX_STRING_LENGTH*2];
	static char Argument[MAX_STRING_LENGTH];
    boolean FileIsByteReversed = false;
    BOOL Error;


	Error = cget_string(pars_p, "toto", FileName, MAX_STRING_LENGTH );
	if ( Error == FALSE )
 	{
        Error = cget_string(pars_p, "n", Argument, MAX_STRING_LENGTH );  /* optional */
        if (!Error)
        {
            /*if ( is_matched (Argument, "r", 2) || is_matched (Argument, "R", MAX_STRING_LENGTH) )
            {
                FileIsByteReversed = true;
            }           */
			printf("Came to the Load Audio to Memory section.......\n");
			sample = load_aud_spdif(FileName, FileIsByteReversed );               //This function Load the sudio file into the designated audio memory location of LMI
		}
	}
	else
		printf( "The path given too long to handle, extracted ");

	return(FALSE);
#endif  /* LINUX */
}
/***************************************************************/
/* load_stream in memory for SPDIF player    				   */
/***************************************************************/
static int load_aud_spdif(const char *file_spdif , char FileIsByteReversed)
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return 0;

#else

	unsigned char ch_opt,ch_opt1;			  //Array to hold the file name and the choice for byte reversible option
	int ByteReverse;                          //Flag which will be casted as 1 for true and 0 for fasle (byte reversible option)
	FILE *stream1;                            //File pointer to hold the address of the file to open to load
	unsigned char *tmp;                       // temporary pointer (buffer) where the readed value of the file will be placed
	unsigned int value0=0,value1=0,NoBytes=0; //value0, for lower byte and value1 for higher byte,NoBytes:-Total no of byutes readed.
	unsigned int value_l=0,value_r=0;
	unsigned int offset=0;                    //offset:- increment for memory location
	unsigned int *pcmp_add,*pcmp_add1;                   //memory location address where the readed value from the file to be loaded
	int numread=0,i=0;                        //number of bytes readed from the file in a go;i is incremental to buffer address to fetch the data placed in it
	int flag=0;                               //flag:- condition to check wether the file is opened once, sets when opened once
	unsigned int FileSize=0;                  //Total number of FileSize the audio file consists of, it will be used to configure PCM CONTROL register 13:31 Bit


    printf(" \nReading from the file and loading into Buffer.......................\n");
	pcmp_add = (unsigned int *)(player_buff_address_spdif);    //Memory (LMI) base address where the stream is to be loaded

	/**************************Calculating the File Size******************************/
	stream1=fopen(file_spdif,"rb");
 	if (stream1==NULL)
  	{
   		printf("--> Unable to open the file %s \n\n",file_spdif);
  	}
 	fseek(stream1,0,SEEK_END);
 	FileSize=ftell(stream1);
 	fclose(stream1);
	/*******************************************************************************/


	stream1=fopen(file_spdif,"rb");          //File to open
	if(stream1==NULL)
		printf(" Unable To Open %s \n",file_spdif);
	else
	{
		if(FileIsByteReversed)
		{
			printf("%s to memory in Reverse mode\n",file_spdif);
		}
		else
		{
			printf("%s to memory in No Reverse mode\n",file_spdif);
		}

		tmp=(unsigned char *)malloc(FILE_BLOCK_SIZE);

		if(tmp==NULL)
			printf("\n Memory Heap allocation cannot be done");
		else
		{
			for(NoBytes=0,numread=1;(numread>0)&&(NoBytes<=MAX_FILE_SIZE);) //Loop terminate when Numread=0 (no data read from file) and total data (bytes) read exceed the quota of 600*1024
			//for(NoBytes=0,numread=1;(numread>0)&&(NoBytes<=FileSize);) //Loop terminate when Numread=0 (no data read from file) and total data (bytes) read exceed the quota of 600*1024

			{
				numread=(int)fread(tmp,BYTES,FILE_BLOCK_SIZE,stream1);

				if(numread>0)
				{
					flag=1;
					if(numread+NoBytes>MAX_FILE_SIZE)
					//if(numread+NoBytes>FileSize)

						numread=(MAX_FILE_SIZE-NoBytes);
						//numread=(FileSize-NoBytes);

				for(i=0;i<=(numread-1);)
					{
						value0=0;
						value1=0;
						if(FileIsByteReversed)
						{
							//LEFT + RIGHT CHANNEL DATA  (Compact Data MODE)
							value0=(((tmp[i+1])<<8)+((tmp[i]<<0))); //making 16Bit MSB LEFT ALLIGN
							i=i+2;
							value1=(((tmp[i+1])<<24)+((tmp[i]<<16))); //making 16Bit MSB LEFT ALLIGN
							i=i+2;
							value_l = value0+value1;

							*((pcmp_add)+(offset)) = value_l;  //Left Right Dual
							offset=offset+1;
						}
						else
						{
							//LEFT + RIGHT CHANNEL DATA (Compact Data MODE)
							value0=(((tmp[i])<<8)+((tmp[i+1]<<0))); //making 16Bit MSB LEFT ALLIGN
							i=i+2;
							value1=(((tmp[i])<<24)+((tmp[i+1]<<16))); //making 16Bit MSB LEFT ALLIGN
							i=i+2;
							value_l = value0+value1;

							*((pcmp_add)+(offset)) = value_l;  //Left Right Dual
							offset=offset+1;
						}
					}
                    NoBytes=(NoBytes+numread);      //adding 1 bcoz subtracted in for loop
				}
				else
				{
					if(flag==0)
						printf("\n READING from the file faled");
					else
						printf("\n\n ALL DATA READED SUCCESSFULLY");
				}
				if(NoBytes%FILE_READ==0)   //when the data extracted from file multilple of 1Kb, display this information.

				{
					printf("\n GOT %dKB of data from the file",(NoBytes/FILE_READ));

				}
			}
		}
	free(tmp);         //releasing the buffer area allocated for temporarly holding data from the file before placing it to memory
	fclose(stream1);
	}

	printf("\n\n The last two sample (L+R) is placed at address in LMI = 0x%x",pcmp_add = pcmp_add+(offset-1));


	if(FileSize>=MAX_FILE_SIZE)
		{
			printf("\n FileSize1 of Audio loaded = %d\n",MAX_FILE_SIZE);
			return(MAX_FILE_SIZE);
		}
	else
		{
			printf("\n FileSize2 of Audio loaded = %d\n",FileSize);
			return(FileSize);
		}
#endif  /* LINUX */
}

/***********************************************************/
/*     void playspdif_pcm_config()                         */
/***********************************************************/
/*  this function configure the SPDIF player configuration */
/*  register (For PCM mode)                                */
/***********************************************************/
void playspdif_pcm_config()
	{
#ifdef ST_OSLINUX
        printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
        return;
#else
		spdifplayer_control spdifcontrol;                //SPDIF control regiter....configuration
		unsigned int spdifcontrol_temp = 0;

		/*****  CONFIGURATION OF SPDIF CONTROL REGISTER      ****/
		spdifcontrol.operation             = spdif_off;
		spdifcontrol.idlestate			   = idle_false;
		spdifcontrol.rounding		       = no_rounding_spdif;
       /* spdifcontrol.divider64             = fs1_256_64;    */
        spdifcontrol.divider64             = fs1_128_64;
        spdifcontrol.byteswap              = nobyte_swap_encoded_mode;
		spdifcontrol.stuffing_hard_soft    = stuffing_software_encoded_mode;
		spdifcontrol.samples_read          = audiostream_spdif.samples;

		spdifcontrol_temp  = (spdifcontrol.operation);
		spdifcontrol_temp |= (spdifcontrol.idlestate<<3);
		spdifcontrol_temp |= (spdifcontrol.rounding<<4);
		spdifcontrol_temp |= (spdifcontrol.divider64<<5);
		spdifcontrol_temp |= (spdifcontrol.byteswap<<13);
		spdifcontrol_temp |= (spdifcontrol.stuffing_hard_soft<<14);
		//spdifcontrol_temp |= (spdifcontrol.samples_read<<15);

		spdifcontrol_temp |= (0x200<<15);
//		spdifcontrol_temp |= (0x180<<15);

		set_reg(SPDIF_CONTROL_REG,spdifcontrol_temp);
		printf("\n SPDIFLAYER CONTROL REGISTER =%x\n",spdifcontrol_temp);
		/***********************************************************/

//		set_reg(SPDIF_BASE,0x1);      //soft Reset
//		set_reg(SPDIF_BASE,0x0);
#endif  /* LINUX */
    }

/***********************************************************/
/*     void playspdif_encoded_config()                     */
/***********************************************************/
/*  this function configure the SPDIF player configuration */
/*  register (For ENCODED mode)                            */
/***********************************************************/
void playspdif_encoded_config()
	{
#ifdef ST_OSLINUX
        printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
        return;
#else

		spdifplayer_control spdifcontrol;                //SPDIF control regiter....configuration
		unsigned int spdifcontrol_temp = 0;


		/*****  CONFIGURATION OF SPDIF CONTROL REGISTER         ****/
		spdifcontrol.operation             = spdif_off;
		spdifcontrol.idlestate			   = idle_false;
		spdifcontrol.rounding			   = no_rounding_spdif;
		spdifcontrol.divider64             = fs1_256_64;
		spdifcontrol.byteswap	           = nobyte_swap_encoded_mode;
		spdifcontrol.stuffing_hard_soft    = stuffing_software_encoded_mode;
		spdifcontrol.samples_read          = audiostream_spdif.samples;

		spdifcontrol_temp  = (spdifcontrol.operation);
		spdifcontrol_temp |= (spdifcontrol.idlestate<<3);
		spdifcontrol_temp |= (spdifcontrol.rounding<<4);
		spdifcontrol_temp |= (spdifcontrol.divider64<<5);
		spdifcontrol_temp |= (spdifcontrol.byteswap<<13);
		spdifcontrol_temp |= (spdifcontrol.stuffing_hard_soft<<14);
		//spdifcontrol_temp |= (spdifcontrol.samples_read<<15);

		spdifcontrol_temp |= (0x200<<15);

		set_reg(SPDIF_CONTROL_REG,spdifcontrol_temp);
		printf("\n SPDIFLAYER CONTROL REGISTER =%x\n",spdifcontrol_temp);
		/***********************************************************/

#endif  /* LINUX */
    }

/*******************************************************************/
/*          void handler_audio_spdif()	 			   */
/*******************************************************************/
/*   This is the Interrupt handler for the SPDIF Player Audio      */
/*		- Channel SPDIF PLAYER				   */
/*******************************************************************/
int handler_audio_spdif()
	{
#ifdef ST_OSLINUX
        /* feature NOT supported under LINUX */
        return -1;
#else
		unsigned int control_mute	      = 0;
		unsigned int spdif_itrp_status  = 0;
		unsigned int spdif_itrp_enable = 0;


		spdif_itrp_enable   = read_reg(SPDIF_ITRP_ENABLE_REG);


		/*****************SPDIF UNDERFLOW***************************/
		spdif_itrp_status = read_reg(SPDIF_ITRP_STATUS_REG);
		if((spdif_itrp_enable&0x0001)==0x0001)
		{
		if((spdif_itrp_status&0x0001) == 0x0001)

			{
				underflow_interrupt_spdif ++;

				control_mute  = *((volatile unsigned int *)(SPDIF_CONTROL_REG));
				control_mute &= ((0xFFFFFFF8));
				if(mode_encoded == TRUE)
					control_mute |= (0x2);                 //Mute for the Encoded Mode
				else
					control_mute |= (0x1); 			//Nute for the PCM Mode

				*((volatile unsigned int *)(SPDIF_CONTROL_REG)) = control_mute;
				set_reg(SPDIF_ITRP_STATUS_CLEAR_REG,0x1);
			}
		}

		/*****************SPDIF EoDB***************************/
		spdif_itrp_status = read_reg(SPDIF_ITRP_STATUS_REG);
		if((spdif_itrp_enable& 0x0002)==0x0002)
		{
		if((spdif_itrp_status&0x0002) == 0x0002)
			{
				eodb_interrupt_spdif++;

				set_reg(SPDIF_ITRP_STATUS_CLEAR_REG,0x2);
			}

		}

		/*****************SPDIF EoB***************************/
		spdif_itrp_status = read_reg(SPDIF_ITRP_STATUS_REG);
		if((spdif_itrp_enable& 0x0004)==0x0004)
		{
		if((spdif_itrp_status&0x0004) == 0x0004)
			{
				eob_interrupt_spdif++;

				set_reg(SPDIF_ITRP_STATUS_CLEAR_REG,0x4);
			}

		}


		/*****************SPDIF Eol***************************/
		spdif_itrp_status = read_reg(SPDIF_ITRP_STATUS_REG);
		if((spdif_itrp_enable& 0x0008)==0x0008)
		{
		if((spdif_itrp_status&0x0008) == 0x0008)
			{
				eol_interrupt_spdif++;

				set_reg(SPDIF_ITRP_STATUS_CLEAR_REG,0x8);
			}

		}


		/*****************SPDIF EoPd***************************/
		spdif_itrp_status = read_reg(SPDIF_ITRP_STATUS_REG);
		if((spdif_itrp_enable& 0x0010)==0x0010)
		{
		if((spdif_itrp_status&0x0010) == 0x0010)
			{
				eopd_interrupt_spdif++;

				set_reg(SPDIF_ITRP_STATUS_CLEAR_REG,0x10);
			}

		}


		/*****************SPDIF Memory Block Fully Read************/
		spdif_itrp_status = read_reg(SPDIF_ITRP_STATUS_REG);
		if((spdif_itrp_enable& 0x0020)==0x0020)
		{
		if((spdif_itrp_status&0x0020) == 0x0020)
			{
				memblock_interrupt_spdif++;

				set_reg(SPDIF_ITRP_STATUS_CLEAR_REG,0x20);
			}

		}


		return(OS21_SUCCESS);
#endif  /* LINUX */
    }

 /***********************************************************/
/*     void playspdif_interrupt_init()                     */
/***********************************************************/
/*  Intialize the interrupts, maps the interrupts enable   */
/*  the interrupt.                                         */
/***********************************************************/
void playspdif_interrupt_init()
	{
#ifdef ST_OSLINUX
        printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
        return;
#else
		int result_spdif;
        interrupt_t *spdif_interrupt_audio;      	 //SPDIF AUDIO INTERRUPT handle pointer


		/***************************************************/
		/*       SETTING SPDIF Player INTERRUPT            */
		/***************************************************/
		*((unsigned int *)(SPDIF_ITRP_ENABLE_SET_REG)) = 0x1F;  //setting the 0th and 1st bit to enable 1F
																  //Data Underflow and Memory Block fully read interrupt
#if 1
		{
			/**********  Obtaining the Handle pointer for the SPDIF interrupt*************/
			spdif_interrupt_audio = interrupt_handle(OS21_INTERRUPT_AUD_SPDIF_PLYR);
			if(spdif_interrupt_audio ==NULL)
				printf("\n ERROR : Failed to get .....Handle for AUDIO SPDIF INTERRUPT\n");
			else
				printf("\n Handle for AUDIO SPDIF INTERRUPT.....Successfull\n");

			/**********  Installing the Interrupt handler (SPDIF) *********/
			result_spdif = -1;
			result_spdif = interrupt_install(OS21_INTERRUPT_AUD_SPDIF_PLYR,NULL,handler_audio_spdif,NULL);
			if(result_spdif != OS21_SUCCESS)
				printf("\n ERROR : Failed to attach Handler for handler_audio_spdif\n");
			else
				printf("\n Handler for handler_audio_spdif attached SUCCESSFULLY\n");

			/**********  ENABLE THE INTERRUPT (AUDIO SPDIF) AND THEN WRITE ITS HANDLER ***********/
			result_spdif = -1;
			result_spdif = interrupt_enable(spdif_interrupt_audio);
			if(result_spdif != OS21_SUCCESS)
				printf("\n  ERROR : Failed to enable spdif_interrupt_audio\n");
			else
				printf("\n spdif_interrupt_audio enabled SUCCESSFULL\n");
		}
#endif
		playspdif_first_time = 0;                                          //This playpcm_interrupt_init() will be done once

#endif  /* LINUX */
    }
/*******************************************************************/
/*                            void StreamType(void)	 	        			 	     */
/*******************************************************************/
/*   This function identifies the Type of Encoded stream and Sampling Frequency */
/*   										   						     */
/*******************************************************************/
void StreamType(void)
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return;

#else

	unsigned long int sync_value = 0 , sync_value1 = 0 , sync_value2 = 0;

	CD_mode = 0;
	AC3_Stream = 0;
	DTS_StreamType1=0;
	MPEG1_Layer1=0; MPEG1_Layer2=0;
	MPEG1_Layer3=0;

    sync_value = read_reg((int)audiostream_spdif.p_audio_pointer);
	sync_value = (sync_value << 16) |( sync_value >> 16);
	sync_value1 = sync_value;
	sync_value = (sync_value) & 0xFFFF0000;

    /* For AC3: syncword 0x0B77, crc1 16 bit,   Sample Rate Codes 48 kHz  44.1 kHz 32 kHz reserved   ... */

	if(sync_value==0x0B770000)
		{
			AC3_Stream = 1;
			printf("\nAC3Sync  :0x%x\n",sync_value);

            sync_value2 = read_reg((int)audiostream_spdif.p_audio_pointer + 1);
			sync_value2 = (sync_value2 << 16) |( sync_value2 >> 16);

			if((sync_value2 & 0xc0000000) == 0x00000000)
				{
//	 				ST_Ckg_SetFSClock( FS2_out2, FREQ_256_48kHz);
					printf("\nIt's AC3 Stream of 48 KHz\n");
				}

			if((sync_value2 & 0xc0000000) == 0x40000000)
				{
//	 				ST_Ckg_SetFSClock( FS2_out2, FREQ_256_44kHz);
					printf("\nIt's AC3 Stream of 44.1 KHz\n");
				}

			if((sync_value2 & 0xc0000000) == 0x80000000)
				{
//	 				ST_Ckg_SetFSClock( FS2_out2, FREQ_256_32kHz);
					printf("\nIt's AC3 Stream of 32 KHz\n");
				}
			CD_mode = 1;
		}


	/*For DTSType1: Sampling frequency 48 kHz only*/

	if(sync_value==0x7FFE0000)
		{
			DTS_StreamType1 = 1;
			printf("\nDTSSync  :0x%x\n",sync_value);

			printf("\nIt's DTS_StreamType1 Stream of 48 KHz\n");
			CD_mode = 1;
		}

	/*For MPEG 32 bit Header : syncword[1111 1111 1111], ID[1],	Layer "11" Layer I	"10" Layer II "01" Layer III "00" reserved,	protection_bit[1]
	    bit_rate_index[0000],	sampling_frequency '00' 44.1 kHz '01' 48 kHz '10' 32 kHz '11' reserved,	padding_bit[1], private_bit[1], mode[00],
	    mode_extension[00], copyright[1], original/home[1], emphasis[00]	*/

	if(((sync_value >> 20)==0x00000FFF) && ((sync_value & 0x00060000)==0x00060000))
		{
			MPEG1_Layer1 = 1;
			printf("\nMPEG1_Layer1_Sync  :0x%x\n",sync_value1);

            sync_value2 = read_reg((int)audiostream_spdif.p_audio_pointer);
			sync_value2 = (sync_value2 << 16) |( sync_value2 >> 16);

			if((sync_value2 & 0x00000c00) == 0x00000400)
				{
					printf("\nIt's MPEG1_Layer1 Stream of 48 KHz\n");
				}

			if((sync_value2 & 0x00000c00) == 0x00000000)
				{
					printf("\nIt's MPEG1_Layer1 Stream of 44.1 KHz\n");
				}

			if((sync_value2 & 0x00000c00) == 0x00000800)
				{
					printf("\nIt's MPEG1_Layer1 Stream of 32 KHz\n");
				}

			CD_mode = 1;
		}


	if(((sync_value >> 20)==0x00000FFF) && ((sync_value & 0x00060000)==0x00040000))
		{
			MPEG1_Layer2 = 1;
			printf("\nMPEG1_Layer2_Sync  :0x%x\n",sync_value1);

            sync_value2 = read_reg((int)audiostream_spdif.p_audio_pointer);
			sync_value2 = (sync_value2 << 16) |( sync_value2 >> 16);

			if((sync_value2 & 0x00000c00) == 0x00000400)
				{
					printf("\nIt's MPEG1_Layer2 Stream of 48 KHz\n");
				}

			if((sync_value2 & 0x00000c00) == 0x00000000)
				{
					printf("\nIt's MPEG1_Layer2 Stream of 44.1 KHz\n");
				}

			if((sync_value2 & 0x00000c00) == 0x00000800)
				{
					printf("\nIt's MPEG1_Layer2 Stream of 32 KHz\n");
				}

			CD_mode = 1;
		}


	if(((sync_value >> 20)==0x00000FFF) && ((sync_value & 0x00060000)==0x00020000))
		{
			MPEG1_Layer3 = 1;
			printf("\nMPEG1_Layer3_Sync  :0x%x\n",sync_value1);

            sync_value2 = read_reg((int)audiostream_spdif.p_audio_pointer);
			sync_value2 = (sync_value2 << 16) |( sync_value2 >> 16);

			if((sync_value2 & 0x00000c00) == 0x00000400)
				{
					printf("\nIt's MPEG1_Layer3 Stream of 48 KHz\n");
				}

			if((sync_value2 & 0x00000c00) == 0x00000000)
				{
					printf("\nIt's MPEG1_Layer3 Stream of 44.1 KHz\n");
				}

			if((sync_value2 & 0x00000c00) == 0x00000800)
				{
					printf("\nIt's MPEG1_Layer3 Stream of 32 KHz\n");
				}

			CD_mode = 1;
		}

#endif  /* LINUX */
}
/*******************************************************************/
/*             void  FrameSize_All(void)	 		   */
/*******************************************************************/
/*   This function calculates the Burst Payload length (Pc) for    */
/*   different STREAM type					   */
/*******************************************************************/
void FrameSize_All(void)
{
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return;
#else
	/* sync_word = ABCDXXXX ( First Two byte ABCD)
	cases 1. ABCDXXXX; 2. XXABCDXX; 3. XXXXABCD; 4 XXXXXXAB and CDXXXXXX.
	*/
	unsigned long int sync_value = 0, value =0 , value1 = 0 , sync_address = 0;
	int loop = 1;
    sync_address = (int)audiostream_spdif.p_audio_pointer;
    sync_value = read_reg((int)audiostream_spdif.p_audio_pointer);
	sync_value = (sync_value << 16) |( sync_value >> 16);	    //due to 5700 FirmWare Issue ...
	sync_value = (sync_value) & 0xFFFF0000;
	while(loop)
	{
		sync_address = sync_address + 4;
		value = (read_reg(sync_address));
		value = (value << 16) | (value >> 16);            //due to 5700 FirmWare Issue ...

		// case 1.
		if(sync_value == (value & 0xFFFF0000))
		{
			frameSize = frameSize + 4;
			loop = 0;
		}
		//case 2.
		if(sync_value == ((value >> 8) << 16))
		{
			frameSize = frameSize + 5;
			loop = 0;
		}

		//case 3.
		if(sync_value == (value << 16))
		{
			frameSize = frameSize + 6;
			loop = 0;
		}

		value1 = (read_reg(sync_address + 4));
		value1 = (value1 << 16) | (value1 >> 16);
		value1 = ((value << 24) | ((value1 >> 8) & 0x00FF0000));

		// case 4.
		if(sync_value == value1)
		{
			frameSize = frameSize + 7;
			loop = 0;
		}

		frameSize = frameSize + 4;
	}
	frameSize = frameSize - 4;
#endif  /* LINUX */
}

 /*******************************************************************/
/*          void create_spdif_encode_node()               */
/*******************************************************************/
/*   this function create and configure the nodes 		   */
/*   Link List creation for the FDMA,return the first node pointer */
/*******************************************************************/
void create_spdif_encode_node()
	{
#ifdef ST_OSLINUX
        printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
        return;
#else
		unsigned int temporary_spdif_encode,temporary1_spdif_encoded;
		unsigned int ChannelId_encode, value_encode;
		unsigned int node_spdif_counter =1;                            //Identifier need to place in the for loop for creating the nodes
		unsigned int control_start = 0;
		spdifplayer_control spdifcontrol;

		ST_ErrorCode_t ErrorCode_spdif_encode;
		STFDMA_SPDIFNode_t *nodespdif_encode,*temp_nodespdif_encode;
		STFDMA_TransferParams_t spdif_transfer_param_encode;
		STFDMA_TransferId_t FDMATransferId_encode = 0;


		temp_nodespdif_encode  =  (STFDMA_SPDIFNode_t *)0xA5300000;
		nodespdif_encode	   =  (STFDMA_SPDIFNode_t *)0xA5300000;

		StreamType();

		FrameSize_All();

		#if 1
		/***********************RESETING ADDITIONAL DATA REGION OF SPDIF..STATE OF CHANNEL**************/
		set_reg(0xB92295A4,0x0);    //ADR 0
		set_reg(0xB9229624,0x0);    //ADR 1
		set_reg(0xB92296A4,0x0);   //ADR 2
		set_reg(0xB9229724,0x0);   //ADR 3
		#endif


		/*******************************************************/
		/*     configuring the Nodes                           */
		/*******************************************************/
		if(node_first_time == TRUE)
		{
		for(node_spdif_counter = 1;node_spdif_counter <600;node_spdif_counter++)
			{
				temporary_spdif_encode                  = ((0x0FFFFFFF) & (unsigned int)(temp_nodespdif_encode+node_spdif_counter));
				(*nodespdif_encode).Next_p				= (STFDMA_SPDIFNode_t *)(temporary_spdif_encode);//NULL;

				(*nodespdif_encode).Extended			= STFDMA_EXTENDED_NODE;
				(*nodespdif_encode).Type				= STFDMA_EXT_NODE_SPDIF;                                      //SPDIF Node
				(*nodespdif_encode).DReq				= STFDMA_REQUEST_SIGNAL_SPDIF_AUDIO;
				(*nodespdif_encode).Pad					= 0;//((unsigned int)(SPDIF_AC3_REPITION_TIME*4)- 0x8 -frameSize) ;	 //Repetion Period - 8B(Pa,Pb,Pc,Pd)-FrameSize	(Burst PayLoad)
				(*nodespdif_encode).BurstEnd			= TRUE;										//Burst Start or Continuation
				(*nodespdif_encode).Valid				= TRUE;										//Node is Valid
				(*nodespdif_encode).NodeCompletePause	= 0x0;										//Continue processing next node
				(*nodespdif_encode).NodeCompleteNotify	= 0x0;										//No interrupt at end of node
				(*nodespdif_encode).NumberBytes			= frameSize;

				temporary1_spdif_encoded				= (unsigned int)((audiostream_spdif.p_audio_pointer)+((node_spdif_counter-1)*(frameSize/4)));
				(*nodespdif_encode).SourceAddress_p		= (unsigned int *)((temporary1_spdif_encoded) & (0x0FFFFFFF));
				(*nodespdif_encode).DestinationAddress_p	= (unsigned int *)((SPDIF_FIFO_DATA_REG)&(0x1FFFFFFF));
#if defined (ST_7100) || defined (ST_7109)
                (*nodespdif_encode).Data.CompressedMode.PreambleB           = 0x4E1F;   //  0;                          //Sync Word
                (*nodespdif_encode).Data.CompressedMode.PreambleA           = 0xF872;// 0;                              //Sync Word
                (*nodespdif_encode).Data.CompressedMode.PreambleD           = (frameSize*8);                                //Making Busrt period to transfer

				if(AC3_Stream == 1)
					{
                        (*nodespdif_encode).Data.CompressedMode.PreambleC           = 0x01;
                        (*nodespdif_encode).Data.CompressedMode.BurstPeriod         = SPDIF_AC3_REPITION_TIME;
					}
				if(DTS_StreamType1 == 1)
					{
                        (*nodespdif_encode).Data.CompressedMode.PreambleC           = 0x0b;
                        (*nodespdif_encode).Data.CompressedMode.BurstPeriod         = SPDIF_DTS1_REPITION_TIME;
					}

				if( MPEG1_Layer1 == 1)
					{
                        (*nodespdif_encode).Data.CompressedMode.PreambleC           = 0x04;
                        (*nodespdif_encode).Data.CompressedMode.BurstPeriod         = SPDIF_MPEG1_2_L1_REPITION_TIME;
					}

				if(MPEG1_Layer2 == 1)
					{
                        (*nodespdif_encode).Data.CompressedMode.PreambleC           = 0x05;
                        (*nodespdif_encode).Data.CompressedMode.BurstPeriod         = SPDIF_MPEG1_2_L1_REPITION_TIME;
					}

				if(MPEG1_Layer3 == 1)
					{
                        (*nodespdif_encode).Data.CompressedMode.PreambleC           = 0x05;
                        (*nodespdif_encode).Data.CompressedMode.BurstPeriod         = SPDIF_MPEG1_2_L2_L3_REPITION_TIME;
					}
#else
                (*nodespdif_encode).PreambleB           = 0x4E1F;   //  0;                          //Sync Word
				(*nodespdif_encode).PreambleA			= 0xF872;//	0;								//Sync Word
				(*nodespdif_encode).PreambleD			= (frameSize*8);								//Making Busrt period to transfer

				if(AC3_Stream == 1)
					{
						(*nodespdif_encode).PreambleC			= 0x01;
						(*nodespdif_encode).BurstPeriod			= SPDIF_AC3_REPITION_TIME;
					}
				if(DTS_StreamType1 == 1)
					{
						(*nodespdif_encode).PreambleC			= 0x0b;
						(*nodespdif_encode).BurstPeriod			= SPDIF_DTS1_REPITION_TIME;
					}

				if( MPEG1_Layer1 == 1)
					{
						(*nodespdif_encode).PreambleC			= 0x04;
						(*nodespdif_encode).BurstPeriod			= SPDIF_MPEG1_2_L1_REPITION_TIME;
					}

				if(MPEG1_Layer2 == 1)
					{
						(*nodespdif_encode).PreambleC			= 0x05;
						(*nodespdif_encode).BurstPeriod			= SPDIF_MPEG1_2_L1_REPITION_TIME;
					}

				if(MPEG1_Layer3 == 1)
					{
						(*nodespdif_encode).PreambleC			= 0x05;
						(*nodespdif_encode).BurstPeriod			= SPDIF_MPEG1_2_L2_L3_REPITION_TIME;
					}

#endif /*defined (ST_7100) || defined (ST_7109)*/

				(*nodespdif_encode).Channel0.Status_0   	= 0x02000006;
    			(*nodespdif_encode).Channel0.Status_1   	= 0x2;
    			(*nodespdif_encode).Channel0.UserStatus 	= 0;
    			(*nodespdif_encode).Channel0.Valid     		= 1;
    			(*nodespdif_encode).Channel0.Pad        	= 0;
    			(*nodespdif_encode).Channel1.Status_0   	= 0x02000006;
    			(*nodespdif_encode).Channel1.Status_1   	= 0x00;
    			(*nodespdif_encode).Channel1.UserStatus 	= 0;
    			(*nodespdif_encode).Channel1.Valid      	= 1;
    			(*nodespdif_encode).Channel1.Pad        	= 0;
				(*nodespdif_encode).Pad1[4]					= 0x0;										//????????????????????

				nodespdif_encode						= nodespdif_encode+1;
			}

		/*******************************************************/
		/*     configuring the Last Node                                                   */
		/*******************************************************/

				(*nodespdif_encode).Next_p				= (NULL);//STFDMA_SPDIFNode_t *)(0x05300000);

				(*nodespdif_encode).Extended			= STFDMA_EXTENDED_NODE;
				(*nodespdif_encode).Type				= STFDMA_EXT_NODE_SPDIF;                                      //SPDIF Node
				(*nodespdif_encode).DReq				= STFDMA_REQUEST_SIGNAL_SPDIF_AUDIO;
				(*nodespdif_encode).Pad					= 0;//((unsigned int)(SPDIF_AC3_REPITION_TIME*4)- 0x8 -frameSize) ;	 //Repetion Period - 8B(Pa,Pb,Pc,Pd)-FrameSize	(Burst PayLoad)
				(*nodespdif_encode).BurstEnd			= TRUE;										//Burst Start or Continuation
				(*nodespdif_encode).Valid				= TRUE;										//Node is Valid
				(*nodespdif_encode).NodeCompletePause	= 0x0;										//Continue processing next node
				(*nodespdif_encode).NodeCompleteNotify	= 0x0;										//No interrupt at end of node
				(*nodespdif_encode).NumberBytes			= frameSize;

				temporary1_spdif_encoded				= (unsigned int)((audiostream_spdif.p_audio_pointer)+((node_spdif_counter-1)*(frameSize/4)));
				(*nodespdif_encode).SourceAddress_p	= (unsigned int *)((temporary1_spdif_encoded) & (0x0FFFFFFF));
				(*nodespdif_encode).DestinationAddress_p	= (unsigned int *)((SPDIF_FIFO_DATA_REG)&(0x1FFFFFFF));

#if defined (ST_7100) || defined (ST_7109)
                (*nodespdif_encode).Data.CompressedMode.PreambleB           = 0x4E1F;   //  0;                          //Sync Word
                (*nodespdif_encode).Data.CompressedMode.PreambleA           = 0xF872;// 0;                              //Sync Word
                (*nodespdif_encode).Data.CompressedMode.PreambleD           = (frameSize*8);//0xFFC0;                                   //Making Busrt period to transfer

				if(AC3_Stream == 1)
					{
                        (*nodespdif_encode).Data.CompressedMode.PreambleC           = 0x01;
                        (*nodespdif_encode).Data.CompressedMode.BurstPeriod         = SPDIF_AC3_REPITION_TIME;
					}
                if(DTS_StreamType1 == 1)
					{
                        (*nodespdif_encode).Data.CompressedMode.PreambleC           = 0x0b;
                        (*nodespdif_encode).Data.CompressedMode.BurstPeriod         = SPDIF_DTS1_REPITION_TIME;
					}

				if( MPEG1_Layer1 == 1)
					{
                        (*nodespdif_encode).Data.CompressedMode.PreambleC           = 0x04;
                        (*nodespdif_encode).Data.CompressedMode.BurstPeriod         = SPDIF_MPEG1_2_L1_REPITION_TIME;
					}

				if(MPEG1_Layer2 == 1)
					{
                        (*nodespdif_encode).Data.CompressedMode.PreambleC           = 0x05;
                        (*nodespdif_encode).Data.CompressedMode.BurstPeriod         = SPDIF_MPEG1_2_L1_REPITION_TIME;
					}

				if(MPEG1_Layer3 == 1)
					{
                        (*nodespdif_encode).Data.CompressedMode.PreambleC           = 0x05;
                        (*nodespdif_encode).Data.CompressedMode.BurstPeriod         = SPDIF_MPEG1_2_L2_L3_REPITION_TIME;
					}
#else
                (*nodespdif_encode).PreambleB           = 0x4E1F;   //  0;                          //Sync Word
				(*nodespdif_encode).PreambleA			= 0xF872;//	0;								//Sync Word
				(*nodespdif_encode).PreambleD			= (frameSize*8);//0xFFC0;									//Making Busrt period to transfer

				if(AC3_Stream == 1)
					{
						(*nodespdif_encode).PreambleC			= 0x01;
						(*nodespdif_encode).BurstPeriod			= SPDIF_AC3_REPITION_TIME;
					}
                if(DTS_StreamType1 == 1)
					{
						(*nodespdif_encode).PreambleC			= 0x0b;
						(*nodespdif_encode).BurstPeriod			= SPDIF_DTS1_REPITION_TIME;
					}

				if( MPEG1_Layer1 == 1)
					{
						(*nodespdif_encode).PreambleC			= 0x04;
						(*nodespdif_encode).BurstPeriod			= SPDIF_MPEG1_2_L1_REPITION_TIME;
					}

				if(MPEG1_Layer2 == 1)
					{
						(*nodespdif_encode).PreambleC			= 0x05;
						(*nodespdif_encode).BurstPeriod			= SPDIF_MPEG1_2_L1_REPITION_TIME;
					}

				if(MPEG1_Layer3 == 1)
					{
						(*nodespdif_encode).PreambleC			= 0x05;
						(*nodespdif_encode).BurstPeriod			= SPDIF_MPEG1_2_L2_L3_REPITION_TIME;
					}

#endif /*defined (ST_7100) || defined (ST_7109)*/

				(*nodespdif_encode).Channel0.Status_0   	= 0x02001906;
   				(*nodespdif_encode).Channel0.Status_1   	= 0x00;
   				(*nodespdif_encode).Channel0.UserStatus 	= 0;
   				(*nodespdif_encode).Channel0.Valid     		= 1;
   				(*nodespdif_encode).Channel0.Pad        	= 0;
   				(*nodespdif_encode).Channel1.Status_0   	= 0x02001906;
   				(*nodespdif_encode).Channel1.Status_1   	= 0x2;
   				(*nodespdif_encode).Channel1.UserStatus 	= 0;
   				(*nodespdif_encode).Channel1.Valid      	= 1;
   				(*nodespdif_encode).Channel1.Pad        	= 0;
				(*nodespdif_encode).Pad1[4]					= 0x0;


				printf("\nAll node made now playing.......................\n");
				node_first_time =FALSE;
		}

		/*******************************************************/
		/*     STFDMA_TransferParams_s						   	     */
		/*******************************************************/
		spdif_transfer_param_encode.ChannelId				= STFDMA_USE_ANY_CHANNEL;/*ChannelId;*/
        //spdif_transfer_param_encode.NodeAddress_p           = (STFDMA_SPDIFNode_t *)((unsigned int)(0xA5300000)&0x0FFFFFFF);
        spdif_transfer_param_encode.NodeAddress_p           = (STFDMA_Node_t *)((unsigned int)(0xA5300000)&0x0FFFFFFF);
		spdif_transfer_param_encode.BlockingTimeout			= 0;
		spdif_transfer_param_encode.CallbackFunction		= NULL;/*pcmp0_FDMAcallback;*/
		spdif_transfer_param_encode.ApplicationData_p		= NULL;

        spdif_transfer_param_encode.FDMABlock               = STFDMA_1;

		/*******************************************************/
		/*     Start the Channel				               */
		/*******************************************************/

		control_start 			=	 *((volatile unsigned int *)(SPDIF_CONTROL_REG));
		spdifcontrol.operation  = 	spdif_encoded_data;
		control_start    		&= 	(0xFFFFFFF8);
		control_start    		|= 	(spdifcontrol.operation);

		set_reg(SPDIF_CONTROL_REG,control_start);    //Starting the SPDIF channel
		printf("\n SPDIFLAYER CONTROL REGISTER =%x\n",control_start );


		*((volatile unsigned int *)(SPDIF_BASE)) = 0x1;      //soft Reset
		*((volatile unsigned int *)(SPDIF_BASE)) = 0x0;
		set_reg(SPDIF_CONTROL_REG,control_start);    //Starting the SPDIF channel



		ErrorCode_spdif_encode = STFDMA_StartTransfer(&spdif_transfer_param_encode,&FDMATransferId_encode);
	  	if(ErrorCode_spdif_encode != ST_NO_ERROR)
				printf(" Failded to start the channel................\n");
  		/***************************************************************************/
#endif  /* LINUX */
	}

 /*******************************************************************/
/*          void create_spdif_pcm_node()			   */
/*******************************************************************/
/*   this function create and configure the nodes 		   */
/*   Link List creation for the FDMA,return the first node pointer */
/*******************************************************************/
void create_spdif_pcm_node()
	{
#ifdef ST_OSLINUX
        printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
        return;
#else
		unsigned int temporary_spdif,temporary1_spdif_pcm;
		unsigned int ChannelId = 8,value;
		unsigned int control_start = 0;
		spdifplayer_control spdifcontrol;

		ST_ErrorCode_t ErrorCode_spdif;
		STFDMA_SPDIFNode_t *nodespdif,*temp_nodespdif;
		STFDMA_TransferParams_t spdif_transfer_param;
		STFDMA_TransferId_t FDMATransferId = 0;


		temp_nodespdif  =  (STFDMA_SPDIFNode_t *)0xA5300000;
		nodespdif	=  (STFDMA_SPDIFNode_t *)0xA5300000;


		/*******************************************************/
		/*     configuring the FIRST node                      */
		/*******************************************************/
		(*nodespdif).Next_p					= (STFDMA_SPDIFNode_t *)(0x05300000);//NULL;
 //		(*nodespdif).Next_p					= NULL ;//(STFDMA_SPDIFNode_t *)(0x05300000);//NULL;
		(*nodespdif).Extended				= STFDMA_EXTENDED_NODE;
		(*nodespdif).Type					= STFDMA_EXT_NODE_SPDIF;                                      //SPDIF Node
		(*nodespdif).DReq					= STFDMA_REQUEST_SIGNAL_SPDIF_AUDIO;
		(*nodespdif).Pad					= 0x00;										//Zero Padding, for PCM kept zero
		(*nodespdif).BurstEnd				= 0x0;										//Burst Start or Continuation
		(*nodespdif).Valid					= 0x1;										//Node is Valid
		(*nodespdif).NodeCompletePause		= 0x0;										//Continue processing next node
		(*nodespdif).NodeCompleteNotify		= 0x0;										//No interrupt at end of node
		(*nodespdif).NumberBytes			= audiostream_spdif.audio_size;
		temporary1_spdif_pcm				= (unsigned int)(audiostream_spdif.p_audio_pointer);

		(*nodespdif).SourceAddress_p		= (unsigned int *)((temporary1_spdif_pcm) & (0x0FFFFFFF));
		(*nodespdif).DestinationAddress_p	= (unsigned int *)((SPDIF_FIFO_DATA_REG)&(0x1FFFFFFF));
#if defined (ST_7100) || defined (ST_7109)
        (*nodespdif).Data.CompressedMode.PreambleB              = 0x4E1F;   //  0;                          //Sync Word
        (*nodespdif).Data.CompressedMode.PreambleA              = 0xF872;// 0;                              //Sync Word
        (*nodespdif).Data.CompressedMode.PreambleD              = 0;//0xFFC0;                                   //Making Busrt period to transfer
        (*nodespdif).Data.CompressedMode.PreambleC              = 0;//0x0001;//0x0000;                                  //
        (*nodespdif).Data.CompressedMode.BurstPeriod            = 1536;                 //Burst Period ... PCM MODE kept as 2048.
#else
		(*nodespdif).PreambleB				= 0x4E1F;	//	0;							//Sync Word
		(*nodespdif).PreambleA				= 0xF872;//	0;								//Sync Word
		(*nodespdif).PreambleD				= 0;//0xFFC0;									//Making Busrt period to transfer
		(*nodespdif).PreambleC				= 0;//0x0001;//0x0000;									//
		(*nodespdif).BurstPeriod			= 1536;					//Burst Period ... PCM MODE kept as 2048.
#endif /*#if defined (ST_7100) || defined (ST_7109)*/
		(*nodespdif).Channel0.Status_0   	= 0;
    	(*nodespdif).Channel0.Status_1   	= 0;
    	(*nodespdif).Channel0.UserStatus 	= 0;
    	(*nodespdif).Channel0.Valid     	= 0;
    	(*nodespdif).Channel0.Pad        	= 0;
    	(*nodespdif).Channel1.Status_0   	= 0;
    	(*nodespdif).Channel1.Status_1   	= 0;
    	(*nodespdif).Channel1.UserStatus 	= 0;
    	(*nodespdif).Channel1.Valid      	= 0;
    	(*nodespdif).Channel1.Pad        	= 0;
		(*nodespdif).Pad1[4]				= 0x0;										//????????????????????


		/*******************************************************/
		/*     STFDMA_TransferParams_s						   */
		/*******************************************************/
		spdif_transfer_param.ChannelId				= STFDMA_USE_ANY_CHANNEL;/*ChannelId;*/
        //spdif_transfer_param.NodeAddress_p          = (STFDMA_SPDIFNode_t *)((unsigned int)(nodespdif)&0x0FFFFFFF);
        spdif_transfer_param.NodeAddress_p          = (STFDMA_Node_t *)((unsigned int)(nodespdif)&0x0FFFFFFF);
        spdif_transfer_param.BlockingTimeout        = 0;
		spdif_transfer_param.CallbackFunction		= NULL;/*pcmp0_FDMAcallback;*/
		spdif_transfer_param.ApplicationData_p		= NULL;

        spdif_transfer_param.FDMABlock                 = STFDMA_1;

		/*******************************************************/
		/*     Start the Channel				               */
		/*******************************************************/
		control_start 			 =	*((volatile unsigned int *)(SPDIF_CONTROL_REG));
		spdifcontrol.operation   = 	spdif_audio_data;
		control_start    		&= 	(0xFFFFFFF8);
		control_start    		|= 	(spdifcontrol.operation);

		*((volatile unsigned int *)(SPDIF_BASE)) = 0x1;      //soft Reset
		*((volatile unsigned int *)(SPDIF_BASE)) = 0x0;

		set_reg(SPDIF_CONTROL_REG,control_start);    //Starting the SPDIF channel
		printf("\n SPDIFLAYER CONTROL REGISTER =%x\n",control_start );

		ErrorCode_spdif = STFDMA_StartTransfer(&spdif_transfer_param,&FDMATransferId);
	  	if(ErrorCode_spdif != ST_NO_ERROR)
				printf(" Failded to start the channel................\n");

  		/***************************************************************************/
#endif  /* LINUX */
    }


/********************************************************************************/
/*             Name          :-  spdifplayer_pcm_encoded                        */
/*             Description :-  Play the spdifplayer both PCM/ENCODED            */
/********************************************************************************/
static boolean STAUD_Spdifplayer_pcm_encoded(parse_t *pars_p, char *result) {
#ifdef ST_OSLINUX
    printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
    return FALSE;
#else

	unsigned int temp;
    BOOL Error;

	Error = cget_integer(pars_p,1, &temp);
	if ( Error == FALSE )
	{
		printf( "The argument extracted = %d\n", temp);
        playspdif_stream(temp,sample);
	}
	return(FALSE);
#endif  /* LINUX */
}

/**********************************************************************/
/*      void playspdif_stream(unsigned int loop,unsigned int sample)  */
/**********************************************************************/
/*  this function will be called when audio play command is given,    */
/*  will create task, semaphore for task completion signaling         */
/**********************************************************************/
void playspdif_stream(unsigned int loop,unsigned int sample)
{
#ifdef ST_OSLINUX
        printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
        return;
#else
		task_t *audio_task_list[5];
		static task_t *spdifplayer;


		audiostream_spdif.p_audio_pointer = ((unsigned int *)(player_buff_address_spdif));  //address from where the audio player will pick the first sample (not the correct one for 7100,change for simulator)

		audiostream_spdif.audio_size=(sample);             //each sample is of 2Bytes data so number of Bytes store = 2*(sample)
		audiostream_spdif.samples = sample/2;              //No of samples to read....will placed into 13:31 bit of control register (each sample will be of 4Bytes)
		audiostream_spdif.loop = loop;                     //No of times playing has to be done

		if( playspdif_first_time==1)
		{
		spdif_feed = semaphore_create_fifo(0);
		spdif_play = semaphore_create_fifo(0);

		// create task.....will call function.....playaudio_spdif();
		spdifplayer = task_create(
					   (void (*)(void *))playaudio_spdif,    	 //Function to call
					   NULL,                                	 //No Arguement
				 	  16384,                                	 //Minimum Define stack size (16KB)
					   MIN_USER_PRIORITY,                                  	 //Priority 254, main runs at 255
				 	  "SPDIF_Player",
                      0/*NULL*/                       //No flag
				 	 );

		if(spdifplayer == NULL)
			printf("\n SPDIF Player Task not created \n");
		else
			printf("\n SPDIF Player Task is running at priority = %d\n",task_priority(spdifplayer));

		}

		audio_task_list[0] = spdifplayer;

		//semaphore will signal to feed the data and configure to pcmp_0
		j_spdif = 1;
		//audio_finish =0;
		semaphore_signal(spdif_feed);

		task_delay(500000);
#endif   /* LINUX */
}

/*********************************************************/
/*     void playaudio_spdif()                            */
/*********************************************************/
/* this is called by the task created in play_stream     */
/* function it configures the SPDIF PLAYER configuration */
/* registers and interrupt initialization                */
/*********************************************************/
void playaudio_spdif()
	{
#ifdef ST_OSLINUX
        printf("\n %s(): feature NOT supported under LINUX !!!\n", __FUNCTION__);
        return;
#else
		unsigned char choicespdif;
		unsigned int mbx_clear_spdif=0;
		unsigned int cmd_status_value_spdif = 0;           //Will hold the value of Command Status register..SPDIF
		unsigned int cmd_status_temp_spdif  = 0;           //Window bit for identifying channel status
		unsigned int cmd_status_case_spdif  = 0;           //To determine the case of the Channel status
        unsigned int Index;
		if(playspdif_first_time == 1)
            playspdif_interrupt_init();          //Initialize and Install the Interrupt

		printf("\n\n Enter the choice -----------\n");
		printf("1 --->SPDIF ENCODED AUDIO \n2 --->SPDIF PCM AUDIO\n");
		fflush(stdin);
		fflush(stdin);
		scanf("%c",&choicespdif);

		if(choicespdif == '1')
			{
				printf("\n *************** INTO THE SPDIF ENCODED AUDIO *******************\n");
				playspdif_encoded_config();             //Configures the SPDIF CONTROL registers (ENCODED MODE)
				mode_encoded = TRUE;
			}
		else
			{
				printf("\n *************** INTO THE SPDIF PCM AUDIO ***************\n");
				playspdif_pcm_config();                 	//Configures the SPDIF CONTROL registers (PCM MODE)
				mode_encoded = FALSE;
			}

        while (1)
            {
				semaphore_wait(spdif_feed);
                for(;j_spdif<=audiostream_spdif.loop;j_spdif++)
					{
					if(mode_encoded == TRUE)
						{
							set_reg(SPDIF_BASE,0x1);      //soft Reset
							set_reg(SPDIF_BASE,0x0);
							create_spdif_encode_node();
						}
					else
						{
							set_reg(SPDIF_BASE,0x1);      //soft Reset
							set_reg(SPDIF_BASE,0x0);
							create_spdif_pcm_node();      //Host creates linked list of nodes describing the transfer in main memory
						}
		    	  	} //For Loop
		    }//End of While Loop
#endif  /* LINUX */
        }


/********************************************************************************/
/*             Name      :-   read_pcmr                                         */
/*             Description :-  read the registers of the pcm reader             */
/********************************************************************************/
static boolean STAUD_freq_synth_32(parse_t *pars_p, char *result)
{
	int ip_choice;
	int freq_choice;
	unsigned int user_cnfgval1 = 0x00007FFE;                      /*  Generic setting   for 256*48Khz         */
	unsigned int user_cnfgval2 = 0x00007FFF;                      /*  Generic setting   for 256*48Khz         */
    int user_val_md = 0x4e;                           				  /*  md_ config            for 256*48Khz      */
    int user_val_pe = 0xD25A;                            			  /*  pe_ config            for 256*48Khz       */
    int user_val_sdiv = 5;                         				  /*  sdiv config            for 256*48Khz       */
    int user_val_prg_en = 0x1;                				 	  /* prg_en_ config            for 256*48Khz  */
    int user_val_fs =0x1000023;
    BOOL Error;

	Error = cget_integer(pars_p,1, &user_val_md);
	if ( Error == FALSE )
	{
		printf( "The argument extracted = %x\n", user_val_md);
	}
	Error = cget_integer(pars_p,1, &user_val_pe);
	if ( Error == FALSE )
	{
		printf( "The argument extracted = %x\n", user_val_pe);
	}
	Error = cget_integer(pars_p,1, &user_val_sdiv);
	if ( Error == FALSE )
	{
		printf( "The argument extracted = %x\n", user_val_sdiv);
	}

    Error = cget_integer( pars_p, 128, &user_val_fs );
    if ( Error == FALSE )
    {
        printf( "The argument extracted = %x\n", user_val_fs);
    }

	set_reg(AUDIO_FSYN_CFG,user_cnfgval2);
	printf(" Value configured the FSYNTH_config = 0x%x\n",user_cnfgval2);

	set_reg(AUDIO_FSYN_CFG,user_cnfgval1);
    printf(" Value configured the FSYNTH_config = 0x%x\n",user_cnfgval1);

	/**************************************************************************/
    /*                          Configuring the PCMPlayer#0 Clock             */
	/**************************************************************************/

	set_reg(AUDIO_FSYN0_PROG_EN,0x0);
	printf(" Value configured the FSYNTH0_PROG_EN = 0x%x\n",0x0);

	set_reg(AUDIO_FSYN0_MD,user_val_md);
	printf(" Value configured the FSYNTH0_MD = 0x%x\n",user_val_md);

	set_reg(AUDIO_FSYN0_PE,user_val_pe);
	printf(" Value configured the FSYNTH0_PE = 0x%x\n",user_val_pe);

	set_reg(AUDIO_FSYN0_SDIV,user_val_sdiv);
	printf(" Value configured the FSYNTH0_SDIV = 0x%x\n",user_val_sdiv);

	set_reg(AUDIO_FSYN0_PROG_EN,user_val_prg_en);
	printf(" Value configured the FSYNTH0_PROG_EN = 0x%x\n",user_val_prg_en);

	set_reg(AUDIO_FSYN2_PROG_EN,0x0);
	set_reg(AUDIO_FSYN2_MD,user_val_md);
	set_reg(AUDIO_FSYN2_PE,user_val_pe);
	set_reg(AUDIO_FSYN2_SDIV,user_val_sdiv);
	set_reg(AUDIO_FSYN2_PROG_EN,user_val_prg_en);

   /* Set spdif control*/
    switch (user_val_fs) {
        case 128 :
             set_reg(SPDIF_CONTROL_REG, 0x1000023);
             break;
        case 256 :
             set_reg(SPDIF_CONTROL_REG, 0x1000043);
             break;
        case 512 :
             set_reg(SPDIF_CONTROL_REG, 0x1000083);
             break;
        case 384 :
             set_reg(SPDIF_CONTROL_REG, 0x1000063);
             break;
          }
    /* Enable the Spdif output on HDMI */
     set_reg(AUDIO_SPDIF_HDMI, 0x1);


    /*Enabling the internal DAC*/
     set_reg(AUDIO_DAC_PCMP1,0x69);

     /* Soft reset of the stereo dacs */
     set_reg(AUDIO_DAC_PCMP1,0x68);
     task_delay(1000);
     set_reg(AUDIO_DAC_PCMP1,0x69);

     /* Enabling the PCM CLK, PCM0 and PCM1 out  */
     set_reg(PCMCLK_IOCTL,0x8);

	return(FALSE);
}







/*#######################################################################*/
/*########################### 7100 and LINUX ############################*/
/*#######################################################################*/


void SetNonSpecificParams( STAUD_InitParams_t * InitParams )
{
#if defined (DVD)
    InitParams->Configuration = STAUD_CONFIG_DVD_COMPACT;
#elif defined (DVD_SERVICE_DIRECTV)
    InitParams->Configuration = STAUD_CONFIG_DIRECTV_COMPACT;
#else /* default to STB */
    InitParams->Configuration = STAUD_CONFIG_DVB_COMPACT;
#endif

    InitParams->MaxOpen         = 1;
    /*InitParams->CPUPartition_p  = (void*)0xFFFFFFFF;*/

    /* Configure interface to AUDIO registers */
    /*InitParams->InterfaceType = STAUD_INTERFACE_EMI;*/

    /* Configure CD data interface */
    /*InitParams->CDInterfaceType = STAUD_INTERFACE_EMI;*/
    InitParams->AVMEMPartition  = 0xFFFFFFFF;
    /*InitParams->CD1BufferAddress_p = NULL;*/
    /*InitParams->CD1BufferSize = 0;*/

/*    strcpy(InitParams->EvtHandlerName, STEVT_HANDLER);*/

}

boolean SetDeviceSpecificParams ( STAUD_InitParams_t * InitParams )
{
#ifdef CLEAN_AUD_CMD
    InitParams->DeviceType = STAUD_DEVICE_STI7100;

    /* DAC configuration is board specific
       Default values can be overridden using AUD_CONFDAC */

    InitParams->InternalPLL  = LocalDACConfiguration.InternalPLL;
    InitParams->DACClockToFsRatio = LocalDACConfiguration.DACClockToFsRatio;
    InitParams->PCMOutParams = LocalDACConfiguration.DACConfiguration;
    InitParams->DeviceType                                   = STAUD_DEVICE_STI7100;
    InitParams->PCMInterruptNumber                           = 530;
    InitParams->PCMInterruptLevel                            = 0;
    InitParams->InterruptNumber                              = 0;
    InitParams->InterruptLevel                               = 0;
    InitParams->InterfaceType                                = STAUD_INTERFACE_EMI;
    InitParams->Configuration                                = STAUD_CONFIG_DVB_COMPACT;
    InitParams->CD1BufferAddress_p                           = NULL;
    InitParams->CD1BufferSize                                = 0;
    InitParams->CD2BufferAddress_p                           = NULL;
    InitParams->CD2BufferSize                                = 0;
    InitParams->CDInterfaceType                              = STAUD_INTERFACE_EMI;
    InitParams->InternalPLL                                  = TRUE;
    InitParams->DACClockToFsRatio                            = 256;
    InitParams->PCMOutParams.InvertWordClock                 = FALSE;
    InitParams->PCMOutParams.Format                          = STAUD_DAC_DATA_FORMAT_I2S;
    InitParams->PCMOutParams.InvertBitClock                  = FALSE;
    InitParams->PCMOutParams.Precision                       = STAUD_DAC_DATA_PRECISION_24BITS;
    InitParams->PCMOutParams.Alignment                       = STAUD_DAC_DATA_ALIGNMENT_LEFT_SIGNED;
    InitParams->PCMOutParams.MSBFirst                        = TRUE;
    InitParams->CD2InterfaceType                             = STAUD_INTERFACE_EMI;
    InitParams->MaxOpen                                      = 1;
    InitParams->CPUPartition_p                               = NULL;
    InitParams->AVMEMPartition                               = (U32)NULL;
    InitParams->CD1InterruptNumber                           = 0;
    InitParams->CD2InterruptNumber                           = 0;
    InitParams->CD1InterruptLevel                            = 0;
    InitParams->InterfaceParams.EMIParams.BaseAddress_p      = NULL;
    InitParams->InterfaceParams.EMIParams.RegisterWordWidth  = STAUD_WORD_WIDTH_32;
    InitParams->PCM1QueueSize                                = 5;
    InitParams->PCMMode                                      = PCM_ON;
    InitParams->SPDIFMode                                    = PCM_OVER_SPDIF;

    return FALSE; /* okay */
#endif
}

/*-------------------------------------------------------------------------
* Function : AUD_Setup
*            Initialise Audio
* Input    : service mode
* Output   :
* Return   : Error Code
* ----------------------------------------------------------------------*/
ST_ErrorCode_t AUD_Setup(void)
{
#ifdef CLEAN_AUD_CMD
    ST_ErrorCode_t           ST_ErrorCode;
    STAUD_InitParams_t       STAUD_InitParams;
    STAUD_OpenParams_t       STAUD_OpenParams;
    STAUD_SpeakerEnable_t    STAUD_SpeakerEnable;
    STAUD_BroadcastProfile_t STAUD_BroadcastProfile;

/*------Setup (hardwired) InitParams based on testtool switch variable-------*/
    SetNonSpecificParams( &STAUD_InitParams );
    if (SetDeviceSpecificParams (&STAUD_InitParams ))
    {
        STTBX_Print(("SetDeviceSpecificParams failed in AUD_Setup!\n"));
        return ST_NO_ERROR+1;
    }

    STAUD_InitParams.DeviceType                   = STAUD_DEVICE_STI7100;
    STAUD_InitParams.InterfaceParams.EMIParams.BaseAddress_p = 0;
    STAUD_InitParams.CD1BufferAddress_p = 0;
    STAUD_InitParams.CD1BufferSize = 0;
    STAUD_InitParams.InterruptNumber = 0;
    STAUD_InitParams.InterruptLevel = 0;
    STAUD_InitParams.MaxOpen = 1;
#ifdef STAUD_REMOVE_CLKRV_SUPPORT
    strcpy(STAUD_InitParams.ClockRecoveryName, "");
#else
    strcpy(STAUD_InitParams.ClockRecoveryName, STCLKRV_DEVICE_NAME);
#endif
    STAUD_InitParams.InternalPLL = TRUE;
    STAUD_InitParams.DACClockToFsRatio = 256;
    STAUD_InitParams.PCMOutParams.InvertWordClock = 0;
    STAUD_InitParams.PCMOutParams.InvertBitClock = 0;
    STAUD_InitParams.PCMOutParams.Format = 0;
    STAUD_InitParams.PCMOutParams.Precision = 3;
    STAUD_InitParams.PCMOutParams.Alignment = 0;
    STAUD_InitParams.PCMOutParams.MSBFirst = 1;
    STAUD_InitParams.PCMMode = PCM_ON;
    STAUD_InitParams.SPDIFMode=PCM_OVER_SPDIF;

    STTBX_Print(("AUD_Setup(%s)=", AUD_DeviceName ));

    ST_ErrorCode = STAUD_Init(AUD_DeviceName, &STAUD_InitParams);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("(error)%d\n", (ST_ErrorCode) ));
        return (FALSE);
    }

    STTBX_Print(("%s\n", STAUD_GetRevision() ));

    STAUD_OpenParams.SyncDelay = 0;

    STTBX_Print(("AUD_Open="));

    ST_ErrorCode = STAUD_Open(AUD_DeviceName,&STAUD_OpenParams, &AudDecHndl0);

    STTBX_Print(("%d\n", (ST_ErrorCode) ));

    if (ST_ErrorCode != ST_NO_ERROR)
    {
        return (FALSE);
    }

    return (TRUE);
#endif
}

static BOOL AUD_DRStart( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_StreamParams_t StreamParams;
    STAUD_Object_t DecoderObject;
    STAUD_Object_t OutputObject;
    boolean RetErr;
    char Buff[160];
    long LVar;

    ErrCode = ST_NO_ERROR;

    RetErr = cget_integer( pars_p, STAUD_STREAM_CONTENT_MPEG1, &LVar );
    if ( RetErr == TRUE )
    {
        sprintf( Buff,"expected Decoder (%d=AC3 %d=DTS %d=MPG1 "
                 "%d=MPG2 %d=CDDA %d=PCM %d=LPCM %d=PINK %d=MP3)",
                 STAUD_STREAM_CONTENT_AC3,
                 STAUD_STREAM_CONTENT_DTS,
                 STAUD_STREAM_CONTENT_MPEG1,
                 STAUD_STREAM_CONTENT_MPEG2,
                 STAUD_STREAM_CONTENT_CDDA,
                 STAUD_STREAM_CONTENT_PCM,
                 STAUD_STREAM_CONTENT_LPCM,
                 STAUD_STREAM_CONTENT_PINK_NOISE,
                 STAUD_STREAM_CONTENT_MP3 );
        tag_current_line( pars_p, Buff );
    }
    else
    {
    	StreamParams.StreamContent = (STAUD_StreamContent_t)LVar;

    	/* get Stream Type (default: STAUD_STREAM_TYPE_PES) */
    	RetErr = cget_integer( pars_p, STAUD_STREAM_TYPE_ES, &LVar );
    	if ( RetErr == TRUE )
    	{
    	    sprintf( Buff,"expected Decoder (%d=ES %d=PES "
    		     "%d=MPEG1_PACKET %d=PES_DVD)",
    		     STAUD_STREAM_TYPE_ES,
    		     STAUD_STREAM_TYPE_PES,
    		     STAUD_STREAM_TYPE_MPEG1_PACKET,
    		     STAUD_STREAM_TYPE_PES_DVD);
    	    tag_current_line( pars_p, Buff );
    	}
    	else
    	{
    	    StreamParams.StreamType = (STAUD_StreamType_t)LVar;

    	    /* get SamplingFrequency (default: 48000) */
    	    RetErr = cget_integer( pars_p, 48000, &LVar );
    	    if ( RetErr == TRUE )
        	{
        		tag_current_line( pars_p, "expected SamplingFrequency "
        				  "(32000, 44100, 48000)" );
        	}
    	    else
        	{
        		StreamParams.SamplingFrequency = (U32)LVar;

        		/* get Stream ID (default: STAUD_IGNORE_ID) */
        		RetErr = cget_integer( pars_p, STAUD_IGNORE_ID, &LVar );
        		if ( RetErr == TRUE )
                {
            		   tag_current_line( pars_p, "expected id (from 0 to 31)" );
            	}
        		else
            	{
            		StreamParams.StreamID = (U8)LVar;
            	}
        	}
	    }
    }


    if ( RetErr == FALSE ) /* only start if all parameters are ok */
    {
        RetErr = TRUE;
        sprintf( AUD_Msg, "STAUD_DRStart(%u,(%d, %d, %d, %02.2x)) : ",
                 (U32)AudDecHndl0, StreamParams.StreamContent,
                 StreamParams.StreamType, StreamParams.SamplingFrequency,
                 StreamParams.StreamID );
        /* set parameters and start */
        ErrCode = STAUD_DRStart(AudDecHndl0, STAUD_OBJECT_DECODER_COMPRESSED1, &StreamParams );
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STAUD_DRStart=%d\n", ErrCode));
        }
    } /* end if */

    return ((BOOL)ErrCode);
} /* end AUD_DRStart */

/*-------------------------------------------------------------------------
 * Function : AUD_DRStop
 *            Stop decoding of input stream
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL AUD_DRStop( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAUD_Stop_t StopMode;
    STAUD_Fade_t Fade;

    ErrCode = ST_NO_ERROR;
    /* Stop mode */
#if !defined ST_OSLINUX
	 task_delay(6250000*2);     /* Timeout value to be checked ... */
#endif

    StopMode = STAUD_STOP_NOW;
    return ((BOOL)STAUD_DRStop( AudDecHndl0, STAUD_OBJECT_DECODER_COMPRESSED1, StopMode, &Fade ));
} /* end AUD_DRStop */

ST_ErrorCode_t AUD_Term(void)
{
    ST_ErrorCode_t ErrCode;
    STAUD_TermParams_t TermParams = {0};

    ErrCode = STAUD_Close( AudDecHndl0 );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STAUD_Close= %d", ErrCode));
        return ErrCode;
    }

    TermParams.ForceTerminate = TRUE;
    ErrCode = STAUD_Term( AUD_DeviceName, &TermParams );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STAUD_Term = %d", ErrCode));
    }
    else
    {
        STTBX_Print(("STAUD_Term = ST_NO_ERROR\n"));
    }
    return ErrCode;
}

/*-------------------------------------------------------------------------
 * Function : AUD_SyncEnable
 *            Enable synchronisation
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL AUD_SyncEnable( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL RetErr;
    long LVar = 0;

    STAUD_Object_t DecodeObject;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED1, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Object expected: STAUD_OBJECT_DECODER_COMPRESSED1\n");
    }
    else
    {
        DecodeObject = (STAUD_Object_t)LVar;
    }

#ifndef ST_OSWINCE
	ErrCode = STAUD_DREnableSynchronization(AudDecHndl0, DecodeObject);
#endif

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

}

/*-------------------------------------------------------------------------
 * Function : AUD_SyncDisable
 *            Enable synchronisation
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL AUD_SyncDisable( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL RetErr;
    long LVar = 0;

    STAUD_Object_t DecodeObject;

    RetErr = cget_integer( pars_p, STAUD_OBJECT_DECODER_COMPRESSED1, (long *)&LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "Object expected: STAUD_OBJECT_DECODER_COMPRESSED1\n");
    }
    else
    {
        DecodeObject = (STAUD_Object_t)LVar;
    }

#ifndef ST_OSWINCE
	ErrCode = STAUD_DRDisableSynchronization(AudDecHndl0, DecodeObject);
#endif

    assign_integer(result_sym_p, (long)ErrCode, false);
    return ( API_EnableError ? RetErr : FALSE );

} /* end AUD_SyncDisable */

static BOOL AUD_Init(parse_t *pars_p, char *result_sym_p)
{
#ifdef CLEAN_AUD_CMD
    U32 DeviceNb = 0;
    BOOL RetErr;
    ST_ErrorCode_t          ST_ErrorCode = ST_NO_ERROR;
    STAUD_StreamParams_t StreamParams;
    STAUD_BufferParams_t    BufferParams;
    char *result;

    StreamParams.StreamContent = STAUD_STREAM_CONTENT_MPEG2;
    StreamParams.StreamType = STAUD_STREAM_TYPE_PES;
	StreamParams.SamplingFrequency = 48000;
	StreamParams.StreamID = 0xff;


    /* Get the PES Buffer params */
    ST_ErrorCode = STAUD_IPGetInputBufferParams(AudDecHndl0, STAUD_OBJECT_DECODER_COMPRESSED1, &BufferParams);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print (("AUD_GetDataInputBufferParams fails \n"));
        printf("Error:%x\n",ST_ErrorCode);
        return(TRUE);
    }
    else
    {
    	STTBX_Print(("BufferBaseAddr_p(0x%x) BufferSize(%d)\n", BufferParams.BufferBaseAddr_p, BufferParams.BufferSize));
    }

    ST_ErrorCode = STPTI_BufferAllocateManual(STPTIHandle[DeviceNb],
                                                BufferParams.BufferBaseAddr_p, BufferParams.BufferSize,
                                                1, &PTI_AudioBufferHandle);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print (("PTI_BufferAllocateManual fails \n"));
        return(TRUE);
    }

    /* No cd-fifo: Initialise the link between pti and Audio */
    ST_ErrorCode = STPTI_SlotLinkToBuffer(SlotHandle[DeviceNb][PTICMD_BASE_AUDIO], PTI_AudioBufferHandle);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print (("PTI_SlotLinkToBuffer fails \n"));
        return(TRUE);
    }

	ST_ErrorCode = STAUD_IPSetDataInputInterface(AudDecHndl0,STAUD_OBJECT_DECODER_COMPRESSED1,
                                                NULL, NULL, (void*)PTI_AudioBufferHandle);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print (("STAUD_IPSetDataInputInterface fails \n"));
        return(TRUE);
    }

    printf("Ready to start...\n");

    return(FALSE);
#endif
 } /* end of AUD_Init() */
#endif  /* defined (ST_7100) || defined (ST_7109) */

/*-------------------------------------------------------------------------
 * Function : AUD_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
static boolean AUD_InitCommand (void)
{
    boolean RetErr;

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
#if defined (ST_7100) || defined (ST_7109)
    RetErr |= register_command("AUD_sync", AUD_SyncEnable,"");
    RetErr |= register_command("AUD_nosync",AUD_SyncDisable,"");
    RetErr |= register_command("AUD_Init", AUD_Init,"");
    RetErr |= register_command( "AUD_START",  AUD_DRStart,
                                "<Stream: Content Type Frequency Id> Start the decoding of input stream");
    RetErr |= register_command("AUD_Stop", AUD_DRStop,"");

    RetErr |= register_command( "AUD_LOADFILE",    AUD_LoadAudioMemory, "<FileName> Load the Audio file into LMI in CD Mode");
    RetErr |= register_command ("AUD_LOAD_SPDIF", STAUD_load_audio_memory_spdif, "Will Load the Audio file into LMI in CD Mode\n");
    RetErr |= register_command( "AUD_SETI2S",    AUD_CommandWriteStatusI2s, "Read all registers of I2S to SPDIF");
    RetErr |= register_command( "AUD_PCM_PLAYER0", AUD_PCMPlayer_Digital_0, "Will play pcmplayer#0 Digital,  pcm0 loop");
    RetErr |= register_command( "AUD_STOP_PCM0", AUD_Stop_PCMPlayer_0, "Stop playing pcmplayer#0 Digital");
    RetErr |= register_command( "AUD_SPDIF", STAUD_Spdifplayer_pcm_encoded, "Will play spdifplayer both encoded/pcm");
    RetErr |= register_command( "AUD_SPDIFCONF", STAUD_freq_synth_32, "will set configuration of spdif player");

#else   /* 7100 */

    RetErr |= register_command( "AUD_CLOSE",         AUD_Close, "Close");
    RetErr |= register_command( "AUD_CONFDAC",       AUD_ConfigDAC, "Set the DAC configuration for use in AUD_Init");
    RetErr |= register_command( "AUD_GETSPEAKER",    AUD_GetSpeaker, "Get the state of each speaker.");
    RetErr |= register_command( "AUD_INIT",          AUD_Init, "[<DeviceName>] [<audio base address>] Initialisation, []:optional");
    RetErr |= register_command( "AUD_MUTE",          AUD_Mute, "<1:mute/0:unmute analogue> <1:mute/0:unmute digital> Mute audio channels");
    RetErr |= register_command( "AUD_NOSYNC",        AUD_DisableSynchronisation, "Disable the video synchronisation with PCR");
    RetErr |= register_command( "AUD_OPEN",          AUD_Open, "<DeviceName> [SyncDelay] Share physical decoder usage, SyncDelay optional");
    RetErr |= register_command( "AUD_SETSPEAKER",    AUD_SetSpeaker, "<Front, LSurr, RSurr, Center, Sub> Define the state [0:off, 1:on] of each speaker ");
    RetErr |= register_command( "AUD_START",         AUD_Start, "<Stream: Content Type Frequency Id> Start the decoding of input stream");
    RetErr |= register_command( "AUD_STOP",          AUD_Stop, "<StopMode>Stop the decoding of input stream");
    RetErr |= register_command( "AUD_SYNC",          AUD_EnableSynchronisation, "Enable video synchronisation with PCR");
    RetErr |= register_command( "AUD_TERM",          AUD_Term, "<DeviceName> <ForceTerminate> Terminate");
    RetErr |= register_command( "AUD_VERBOSE",       AUD_Verbose, "<TRUE |FALSE> makes functions print details or not");
    RetErr |= register_command( "AUD_OPUNMUTE",  AUD_OPUnMute, " <Object> Unmutes output");
    RetErr |= register_command( "AUD_GETFILE",     AUD_Load, "Load a file from disk to memory");
    RetErr |= register_command( "AUD_INJECT",      AUD_MemInject,  "Create DMA Inject Audio in memory Task");
    RetErr |= register_command( "AUD_DRSTART",     AUD_DRStart, "<Stream: Content Type Frequency Object Id> Start the decoding of input stream");
    RetErr |= register_command( "AUD_OPSETDIGOUT",   AUD_OPSetDigitalOutput, "SetDigitalOutput <TRUE/FALSE> [< Source >] [< Copy Right>],[<Latency>] ");
#endif  /* !7100 */

#if defined(ST_7015)
    assign_integer  ("AUDDEVICETYPE", 7015, true );
#elif defined(ST_7020)
    assign_integer  ("AUDDEVICETYPE", 7020, true );
#elif defined(ST_5528)
    assign_integer  ("AUDDEVICETYPE", 5528, true );
#elif defined(ST_4629)
    assign_integer  ("AUDDEVICETYPE", 4629, true );
#elif defined(ST_5517)
    assign_integer  ("AUDDEVICETYPE", 5517, true );
#elif defined(ST_5516)
    assign_integer  ("AUDDEVICETYPE", 5516, true );
#elif defined(ST_5514)
    assign_integer  ("AUDDEVICETYPE", 5514, true );
#elif defined(ST_5512)
    assign_integer  ("AUDDEVICETYPE", 5512, true );
#elif defined(ST_5510)
    assign_integer  ("AUDDEVICETYPE", 5510, true );
#elif defined(ST_5580)
    assign_integer  ("AUDDEVICETYPE", 5580, true );
#elif defined(ST_5518)
    assign_integer  ("AUDDEVICETYPE", 5518, true );
#elif defined(ST_7710)
    assign_integer  ("AUDDEVICETYPE", 7710, true );
#elif defined (ST_7100) || defined (ST_7109)
    assign_integer  ("AUDDEVICETYPE", 7100, true );
#else /* default to STi5508 */
    assign_integer  ("AUDDEVICETYPE", 5508, true );
#endif
    assign_integer  ("INJECT_DEST", 1, false );
    assign_integer  ("OBJ_OPMULTI", STAUD_OBJECT_OUTPUT_MULTIPCM1, true );
    assign_integer  ("OBJ_OPSPDIF", STAUD_OBJECT_OUTPUT_SPDIF1, true );

    assign_integer  ("AC3",  STAUD_STREAM_CONTENT_AC3,   true );
    assign_integer  ("DTS",  STAUD_STREAM_CONTENT_DTS,   true );
    assign_integer  ("MPG1", STAUD_STREAM_CONTENT_MPEG1, true );
    assign_integer  ("MPG2", STAUD_STREAM_CONTENT_MPEG2, true );
    assign_integer  ("MP3",  STAUD_STREAM_CONTENT_MP3,   true );
    assign_integer  ("CDDA", STAUD_STREAM_CONTENT_CDDA,  true );
    assign_integer  ("PCM",  STAUD_STREAM_CONTENT_PCM,   true );
    assign_integer  ("LPCM", STAUD_STREAM_CONTENT_LPCM,  true );
    assign_integer  ("PINK", STAUD_STREAM_CONTENT_PINK_NOISE, true );
    assign_integer  ("BTONE", STAUD_STREAM_CONTENT_BEEP_TONE, true );
    assign_integer  ("AAC",  STAUD_STREAM_CONTENT_MPEG_AAC, true );

    assign_integer  ("ES",       STAUD_STREAM_TYPE_ES, true );
    assign_integer  ("PES",      STAUD_STREAM_TYPE_PES, true );
    assign_integer  ("PKTMPEG1", STAUD_STREAM_TYPE_MPEG1_PACKET, true );
    assign_integer  ("PES_DVD",  STAUD_STREAM_TYPE_PES_DVD, true );
    assign_integer  ("SPDIF",    STAUD_STREAM_TYPE_SPDIF, true );

    assign_integer  ("IGNID",    STAUD_IGNORE_ID, true );

    assign_integer  ("OBJ_DRPCM", STAUD_OBJECT_DECODER_PCMPLAYER1, true );
    assign_integer  ("OBJ_DRCOMP", STAUD_OBJECT_DECODER_COMPRESSED1, true );

    assign_integer  ("COPYRIGHT_FREE",     STAUD_COPYRIGHT_MODE_NO_RESTRICTION, true );
    assign_integer  ("COPYRIGHT_ONE_COPY", STAUD_COPYRIGHT_MODE_ONE_COPY, true );
    assign_integer  ("COPYRIGHT_NO_COPY",  STAUD_COPYRIGHT_MODE_NO_COPY, true );


    AudioPartition_p = SystemPartition_p;


    if ( RetErr == TRUE )
    {
        sprintf(AUD_Msg, "AUD_InitCommand()  \t: failed !\n");
    }
    else
    {
        #if defined (ST_7100) || defined (ST_7109)
        sprintf(AUD_Msg, "AUD_InitCommand()  \t: ok           %s\n", STAUD_GetRevision());
        #else
        sprintf(AUD_Msg, "AUD_InitCommand()  \t: ok           \n");
        #endif
    }
    STTST_Print((AUD_Msg));
    return( !RetErr );

} /* end AUD_InitCommand */



/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

BOOL AUD_RegisterCmd()
{
    BOOL RetOk;
      int i;

#ifdef ST_7710
#if defined(ST_OS21)
    SwitchTaskOKSemaphore_p = semaphore_create_fifo(0);
#else
    semaphore_init_fifo(&SwitchTaskOKSemaphore, 0);
    SwitchTaskOKSemaphore_p = &SwitchTaskOKSemaphore;
#endif

    /* Initialize injection task routines */
    for(i=0; i<MAX_INJECT_TASK; i++)
    {
        InjectTaskTable[i].FileBuffer = NULL;
        InjectTaskTable[i].NbBytes = 0;
        InjectTaskTable[i].TaskID = NULL;
        InjectTaskTable[i].Loop = 0;
        InjectTaskTable[i].Injector_p = NULL;
        InjectTaskTable[i].AllocatedBuffer = NULL;
        InjectTaskTable[i].BBLAddr = 0;
        InjectTaskTable[i].BBTAddr = 0;
        InjectTaskTable[i].CDFIFOAddr = 0;
        InjectTaskTable[i].CDPlayAddr = 0;
    }
    InjectTaskTable[0].Injector_p = &DoAudioInject;
#if defined(ST_7020) || defined(ST_5528) || defined(ST_5100) || \
    defined(ST_5301) || defined(ST_7710)

    InjectTaskTable[0].BBLAddr = AUD1_BASE_ADDRESS + (/*SHIFT*/(AUD_BBL));
    InjectTaskTable[0].BBTAddr = AUD1_BASE_ADDRESS + (/*SHIFT*/(AUD_BBT));
    InjectTaskTable[0].CDPlayAddr = AUD1_BASE_ADDRESS + (/*SHIFT*/(AUD_PLY));
    InjectTaskTable[0].CDFIFOAddr = AUDIO_CD_FIFO1;
#endif
    InjectTaskTable[1].Injector_p = &DoAudioInject2;

#if defined (ST_5100) || defined(ST_7710) || defined(ST_5301)
    InjectTaskTable[1].BBLAddr = InjectTaskTable[0].BBLAddr;
    InjectTaskTable[1].BBTAddr = InjectTaskTable[0].BBTAddr;
    InjectTaskTable[1].CDPlayAddr = InjectTaskTable[0].CDPlayAddr;
    InjectTaskTable[1].CDFIFOAddr = InjectTaskTable[0].CDFIFOAddr;
#elif defined(ST_7020) || defined(ST_5528)
    InjectTaskTable[1].BBLAddr = AUD2_BASE_ADDRESS + (SHIFT(AUD_BBL));
    InjectTaskTable[1].BBTAddr = AUD2_BASE_ADDRESS + (SHIFT(AUD_BBT));
    InjectTaskTable[1].CDPlayAddr = AUD2_BASE_ADDRESS + (SHIFT(AUD_PLY));
    InjectTaskTable[1].CDFIFOAddr = AUDIO_CD_FIFO2;
#endif

    InjectTaskTable[2].Injector_p = NULL; /* Only use for PCM mixing */

#if defined (ST_5528) && defined(ST_5528SPDIF)
    InjectTaskTable[3].Injector_p = &DoAudioInject3;
    InjectTaskTable[3].BBLAddr = AUD3_BASE_ADDRESS + (SHIFT(AUD_BBL));
    InjectTaskTable[3].BBTAddr = AUD3_BASE_ADDRESS + (SHIFT(AUD_BBT));
    InjectTaskTable[3].CDPlayAddr = AUD3_BASE_ADDRESS + (SHIFT(AUD_PLY));
    InjectTaskTable[3].CDFIFOAddr = AUDIO_CD_FIFO3;
#endif
#endif
/*#ifdef USE_HDD_INJECT
 #if defined(ST_OS21)
    LowDataSemaphore_p = semaphore_create_fifo_timeout(0);
#else
    semaphore_init_fifo_timeout(&LowDataSemaphore, 0);
    LowDataSemaphore_p = &LowDataSemaphore
#endif
    InjectTaskTable[0].Injector_p = &DoAudioInjectHDD;
#endif                    */

    LocalDACConfiguration.Initialized = FALSE;
    Verbose = FALSE;

    RetOk = AUD_InitCommand();

    return(RetOk);

} /* end AUD_RegisterCmd */

