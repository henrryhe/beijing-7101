#ifndef _ACF_ST20_DEFINES_H_
#define _ACF_ST20_DEFINES_H_

/* Constant Defination*/
#define ACF_ST20_NB_MAX_CHANNELS        2   /*Max supported channels                    */
#define ACF_ST20_NB_MAX_BITSTREAM_INFO  5   /*Max bitstream info return                 */
#define ACF_ST20_NB_MAX_PCMPROCESS      7   /*Maximum supported PCM process             */
#define ACF_ST20_MP3_NbSample			1152 			/* Mp3 sample per fraem for  full freq rate */
#ifdef _VCONTROL_
#define BASSMGT_0dB_Q31 0x7FFFFFFF
#define BASSMGT_M0p5dB  0x78D6FC9E
#define BASSMGT_ATTENUATION_MUTE 70
#define BASSMGT_MAX_ATTENUATION (6 * 24)
#endif

#ifdef _DEBUG_
#define DP printf
#endif
/* Enum defination   */
#ifndef _ST20_C_
enum ACF_ST20_eBoolean
{
    ACF_ST20_FALSE,		  	/* false = 0	*/
    ACF_ST20_TRUE		 		 	/* true = 1		*/
};

/* Codec Codes*/
enum ACF_ST20_eCodecCode
{
    ACF_ST20_Unknown_Codec,	  	/* Unknown codec       */
    ACF_ST20_PCM_Bypass,	  		/* PCM Bypass          */
    ACF_ST20_MP3,               /* MP3 decoder         */
    ACF_ST20_Reserved0,	        /* Reserved for future */
    ACF_ST20_Last_Codec 	  		/* Last codec id   		 */
};
/* PCM Process Codes*/
enum ACF_ST20_ePCMProcessCode
{
     ACF_ST20_Unknown_PostProcess,   	/* Unknown Post process */
     ACF_ST20_DeEmphasis,             /* deempsis						  */
     ACF_ST20_Reserved1,             	/* Reserved for future  */
     ACF_ST20_Reserved2,		 					/* Reserved for future  */
     ACF_ST20_Volume_control,        	/* volume control				*/
     ACF_ST20_Reserved3,            	/* Reserved for future	*/
     ACF_ST20_Last_PostProcess     	  /* last PostProcess Id	*/
};
/*  Cammand code*/
enum ACF_ST20_eCodecCmd
{
    ACF_ST20_CMD_PLAY = 0,
    ACF_ST20_CMD_MUTE = 1,
    ACF_ST20_CMD_SKIP = 2,
    ACF_ST20_CMD_PAUSE= 4
};

/* Channel index*/
enum ACF_ST20_eChannelIdx
{
    ACF_ST20_LEFT,												/* Left Channel index   */
    ACF_ST20_RGHT                         /* Right Channel index	*/
};

/* Sampling freq Code	*/
enum ACF_ST20_eFsCode
{
    /* Normal sampling freq code */
    ACF_ST20_FS48k,
    ACF_ST20_FS44k,
    ACF_ST20_FS32k,
    ACF_ST20_FS_reserved_3,
    /* Half rate sampling freq code */
    ACF_ST20_FS24k,
    ACF_ST20_FS22k,
    ACF_ST20_FS16k,
    ACF_ST20_FS_reserved_7,
    /* Quarter Rate Sampling Freq Code */
    ACF_ST20_FS12k,
    ACF_ST20_FS11k,
    ACF_ST20_FS8k,
    ACF_ST20_FS_reserved_11,
    ACF_ST20_FS_ID = 0xFF

};
/* Audio Coding mode	*/
enum ACF_ST20_eAcMode
{
ACF_ST20_eAcMode0,			/*	Audio coding mode 1+1 (2chan)   */
ACF_ST20_eAcMode1,			/*	Audio coding mode 1/0 (mono)    */
ACF_ST20_eAcMode2				/*	Audio Coding mode 2/0 (stereo)	*/
};
/* Error Codes		*/
enum ACF_ST20_eErrorCode
{
    ACF_ST20_EC_OK                  = 0,
    ACF_ST20_EC_ERROR               = 1,
    ACF_ST20_EC_ERROR_SYNC          = 2,
    ACF_ST20_EC_ERROR_CMP           = 4,
    ACF_ST20_EC_END_OF_FILE         = 8,
    ACF_ST20_EC_ERROR_MEM_ALLOC     = 16,
    ACF_ST20_EC_ERROR_PARSE_COMMAND = 32,
    ACF_ST20_EC_EXIT                = 64,
    ACF_ST20_EC_ERROR_CRC           = 128,
    ACF_ST20_EC_ERROR_FATAL 		= 1024
};
enum ACF_ST20_eWordSizeCode
{
    ACF_ST20_WS32,					/* 32bit PCM sample resolution			*/
    ACF_ST20_WS16,    			/* 16bit PCM sample resolution      */
    ACF_ST20_WS8						/* 8bit PCM sample resolution				*/
};

#else
/*In case of ST20 compile*/
typedef enum tACF_ST20_eWordSizeCode
{
    ACF_ST20_WS32,						/* 32bit PCM sample resolution			*/
    ACF_ST20_WS16,    				/* 16bit PCM sample resolution      */
    ACF_ST20_WS8							/* 8bit PCM sample resolution				*/
}ACF_ST20_eWordSizeCode;

typedef enum tACF_ST20_eBoolean
{
    ACF_ST20_FALSE,		  			/* false = 0	*/
    ACF_ST20_TRUE		  				/* true = 1		*/
}ACF_ST20_eBoolean;

/* Codec Codes		*/
typedef enum tACF_ST20_eCodecCode
{
    ACF_ST20_Unknown_Codec,	  	/* Unknown codec        */
    ACF_ST20_PCM_Bypass,	  		/* PCM Bypass           */
    ACF_ST20_MP3,               /* MP3 decoder          */
    ACF_ST20_Reserved0,	        /* Reserved for future  */
    ACF_ST20_Last_Codec 	  		/* Last codec id   			*/
}ACF_ST20_eCodecCode;
/* PCM Process Codes	*/
typedef enum tACF_ST20_ePCMProcessCode
{
     ACF_ST20_Unknown_PostProcess,   	/* Unknown Post process		*/
     ACF_ST20_DeEmphasis,             /* Reserved for future    */
     ACF_ST20_Reserved1,             	/* Reserved for future 		*/
     ACF_ST20_Reserved2,		 					/* Reserved for future    */
     ACF_ST20_Volume_control,        	/* Reserved for future    */
     ACF_ST20_Reserved3,             	/* Reserved for future		*/
     ACF_ST20_Last_PostProcess       	/* last PostProcess Id		*/
}ACF_ST20_ePCMProcessCode;
/*  Cammand code	*/
typedef enum tACF_ST20_eCodecCmd
{
    ACF_ST20_CMD_PLAY = 0,
    ACF_ST20_CMD_MUTE = 1,
    ACF_ST20_CMD_SKIP = 2,
    ACF_ST20_CMD_PAUSE= 4
}ACF_ST20_eCodecCmd;

/* Channel index*/
typedef enum tACF_ST20_eChannelIdx
{
    ACF_ST20_LEFT,												/* Left Channel index			*/
    ACF_ST20_RGHT                         /* Right Channel index	  */
}ACF_ST20_eChannelIdx;

/* Sampling freq Code*/
typedef enum tACF_ST20_eFsCode
{
    /* Normal sampling freq code */
    ACF_ST20_FS48k,
    ACF_ST20_FS44k,
    ACF_ST20_FS32k,
    ACF_ST20_FS_reserved_3,
        /* Quarter Rate Sampling Freq Code */
    ACF_ST20_FS12k,
    ACF_ST20_FS11k,
    ACF_ST20_FS8k,
    ACF_ST20_FS_reserved_7,
    /* Half rate sampling freq code */
    ACF_ST20_FS24k,
    ACF_ST20_FS22k,
    ACF_ST20_FS16k,
    ACF_ST20_FS_reserved_11,
    ACF_ST20_FS_ID = 0xFF

}ACF_ST20_eFsCode;
/* Audio Coding mode*/
typedef enum tACF_ST20_eAcMode
{
ACF_ST20_eAcMode0,			/*Audio coding mode 1+1 (2chan)   */
ACF_ST20_eAcMode1,			/*Audio coding mode 1/0 (mono)    */
ACF_ST20_eAcMode2				/*Audio Coding mode 2/0 (stereo)	*/
}ACF_ST20_eAcMode;
/* Error Codes	*/
typedef enum tACF_ST20_eErrorCode
{
    ACF_ST20_EC_OK                  = 0,
    ACF_ST20_EC_ERROR               = 1,
    ACF_ST20_EC_ERROR_SYNC          = 2,
    ACF_ST20_EC_ERROR_CMP           = 4,
    ACF_ST20_EC_END_OF_FILE         = 8,
    ACF_ST20_EC_ERROR_MEM_ALLOC     = 16,
    ACF_ST20_EC_ERROR_PARSE_COMMAND = 32,
    ACF_ST20_EC_EXIT                = 64,
    ACF_ST20_EC_ERROR_CRC           = 128,
    ACF_ST20_EC_ERROR_FATAL 		= 1024
}ACF_ST20_eErrorCode;
#endif
/* data type definations	*/
typedef struct
{
	int  						FrameSize;								/*	size of input data in  bytes                    */
	uchar          				* DataPtr;					/*	Address of input buffer 32bit aligned           */
#ifndef _ST20_C_	                          /*                                                  */
	enum ACF_ST20_eFsCode   	SamplingFreq;		/* 	sampling freq                                   */
#else                                       /*                                                  */
	ACF_ST20_eFsCode   	SamplingFreq;					/* 	sampling freq                                   */
#endif		                                  /*                                                  */
   int               			NbSamples;				/* 	no of PCM sample                                */
#ifndef _ST20_C_                            /*                                                  */
   enum ACF_ST20_eAcMode		AudioMode;			/* 	audio coding mode                               */
   enum ACF_ST20_eCodecCode		StreamType;   /*                                                  */
#else                                       /*                                                  */
		ACF_ST20_eAcMode		AudioMode;					/* 	audio coding mode                               */
   		ACF_ST20_eCodecCode		StreamType;     /*                                                  */
#endif   		                                /*                                                  */
   int               			PTSflag;					/* PTS validity bits                                */
   int               			PTS;							/* PTS associated 																	*/

}ACF_ST20_tFrameBuffer;

typedef struct
{
   tSample *          			SamplesPtr[ACF_ST20_NB_MAX_CHANNELS];
											  													/* Pointers to each Channel																	*/
   int                			NbSamples;   				 	/* Number of Sample Per Channel                             */
   short              			NbChannels;   				/* Number of Channels                                       */
   short              			Offset;       				/* WS-based Address-Offset between samples of same channel  */
#ifndef _ST20_C_                                  /*                                                          */
   enum ACF_ST20_eFsCode		SamplingFreq;  				/* Sampling frequency of the PCM Samples                    */
   enum ACF_ST20_eWordSizeCode	WordSize;     		/* 32/16 bit word Left-aligned                              */
   enum ACF_ST20_eAcMode		AudioMode;     				/* Audio Configuration Mode.                                */
   enum ACF_ST20_eBoolean		ChannelExist[ACF_ST20_NB_MAX_CHANNELS];  /* Presence of channels                       */
#else
    	ACF_ST20_eFsCode		SamplingFreq;  					/* Sampling frequency of the PCM Samples										*/
   		ACF_ST20_eWordSizeCode	WordSize;     			/* 32/16 bit word Left-aligned                              */
   		ACF_ST20_eAcMode		AudioMode;     					/* Audio Configuration Mode.                                */
   		ACF_ST20_eBoolean		ChannelExist[ACF_ST20_NB_MAX_CHANNELS];  /* Presence of channels                         */
#endif                                                        /*                                              */
   int                			BufferSize; 		/* PcmBuffer Allocated Size in samples per channel                */
   int                			PTSflag;     		/*PTS validity bits                                               */
   int                			PTS;        		/*PTS associated to the buffer(if any) 														*/
}
ACF_ST20_tPcmBuffer;

typedef struct
{
        /* Pcm Process selection*/
#ifndef _ST20_C_
  enum ACF_ST20_ePCMProcessCode PCMProcessCode;
#else
	   ACF_ST20_ePCMProcessCode PCMProcessCode;
#endif
	  	/* PCM Process Internal parameters	*/
  int                  		   * ProcessMem;
  int                    	   ProcessMemSize;
  tScratchBuffer          	   * ScratchBuffer;
	  /* Input parameters											*/
  tPcmProcessingConfig 		   * Config;
/* Pointer on a PcmProcessing Configuration*/

}
ACF_ST20_PcmProcessInterface;


typedef struct
{
		/*Command Parameters for Decoder  */
#ifndef _ST20_C_
	enum ACF_ST20_eCodecCode  DecoderSelection;
	enum ACF_ST20_eCodecCmd   SkipMuteCmd;
	enum ACF_ST20_eBoolean    CrcOff;

#else
		 ACF_ST20_eCodecCode  DecoderSelection;
		 ACF_ST20_eCodecCmd   SkipMuteCmd;
		 ACF_ST20_eBoolean    CrcOff;
#endif
	uchar      		        * PDC;											/*Decoder configuration Information							*/
																										/* Decoder Internal parameters                  */
	int 				  * DecoderMem;                       /*                                              */
	int 				  DecoderMemSize;                     /*                                              */
	tScratchBuffer            * ScratchBuffer; 	      /*                                              */
						/* input parameters  */                 /*                                              */
#ifndef _ST20_C_						                        /*                                              */
	enum ACF_ST20_eBoolean    FirstTime; 							/* Initilization of codec                       */
#else                                               /*                                              */
		ACF_ST20_eBoolean    FirstTime; 								/* Initilization of codec                       */
#endif	                                            /*                                              */
	ACF_ST20_tFrameBuffer     * FrameIn; 							/* Address of frame buffer                      */
						/* Output parameters                                                                    */
	ACF_ST20_tPcmBuffer 	  * PcmOut; 								/* Address of PCM buffer                        */
	int 				  RemainingBlocks;                    /*                                              */
#ifndef _ST20_C_	                                  /*                                              */
	enum ACF_ST20_eBoolean    OutputPcmEnable;        /*                                              */
#else                                               /*                                              */
		 ACF_ST20_eBoolean    OutputPcmEnable;	        /*                                              */
#endif		                                          /*                                              */
	int 				  NbNewOutSamples;                    /*                                              */
	int 			BitStreamInformations[SYS_NB_MAX_BITSTREAM_INFO];					/*PCM Processing Parameters   */
	int                       PcmProcessCount; 													/*Total no of PCM Process     */
	tDecoderInterface  * DecInterfaceMem; /* ACCUMUALTED INTERNAL MEMORY */
	ACF_ST20_PcmProcessInterface	* PcmProcessInterface[ACF_ST20_NB_MAX_PCMPROCESS]; /*               */
              /*Parameters for each PCM Process                                                     */
}
ACF_ST20_tDecoderInterface;

#endif /*_ACC_DEFINES_H_ */
