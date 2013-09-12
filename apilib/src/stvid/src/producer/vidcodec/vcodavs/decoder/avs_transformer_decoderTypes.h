/*******************************************************************************

File name   : avs_transfomer_decoderTypes.h

Description : AVS Video Decoder specific types for MME

COPYRIGHT (C) STMicroelectronics 2008

Date               Modification                                     Name
----               ------------                                     ----
29 January 2008    First STAPI version                              PLE
*******************************************************************************/

#ifndef _AVS_DECODERTYPES_H_
#define _AVS_DECODERTYPES_H_

#define AVS_MME_TRANSFORMER_NAME    "AVSSD_TRANSFORMER"

#include "mme.h"

#if (AVS_MME_VERSION==11)
#define AVS_MME_API_VERSION "1.1"
#else
#define AVS_MME_API_VERSION "1.0"
#endif /* AVS_MME_VERSION */

/* Avs transformer */
/*--------------------- */
/* should match mp4_vars.h (I,P,B,S_VOP)*/
typedef enum _PICTURETYPE_T_
	{
	IFRAME = 0,
	PFRAME,
	BFRAME
	} PICTURETYPE;
/* Structure reuired by the FW for correcting the INTRA MB's in the second pass on B picture */

typedef struct _INTRA_MB_STRUCT_T_
{
    unsigned int mb_x;          /* Horizantal coardinate of the Intra Mb of B picture */
    unsigned int mb_y;          /* Vertical coardinate of the Intra Mb in B picture */
    short Res_blk[6][64];       /* Residual block */
    char Pred_mode_Luma_blk[4]; /* Prediction modes of the 4 block in an Intra MB of a B picture*/
    char Pred_mode_Chroma; /* Prediction mode of chroma block(Cb/Cr) in an Intra MB of a B picture  */
    char cbp;              /* Coded block Pattern*/
    int curr_mb_no;        /* Current MB Number*/
    int coded_coeff[6];    /* Number of coefficient in the block are coded*/
    int First_MB_In_Current_Slice; /*Flag for the first MB in Slice*/
}IntraMBstruct_t;

typedef struct _AVSVIDEODECODEINITPARAMS_T_
	{
	MME_UINT								width;					/*!< width predetermined from the AVI file */
	MME_UINT								height;					/*!< height predetermined from the AVI file */
	MME_UINT								Progressive_sequence;			/*!< width predetermined from the AVI file */
    IntraMBstruct_t                         *IntraMB_struct_ptr;
	} MME_AVSVideoDecodeInitParams_t;

typedef enum ERROR_CODES_T_
{
	AVS_NO_ERROR,				/* No Error condition detected -- Sucessfull decoding */
	AVS_FIRMWARE_TIME_OUT_ENCOUNTERED	/* Timeout in F/W */
}ERROR_CODES;


typedef struct _AVSVIDEODECODERETURNPARAMS_T_
	{
	PICTURETYPE picturetype;
	/* profiling info */
	MME_UINT pm_cycles;
	MME_UINT pm_dmiss;
	MME_UINT pm_imiss;
	MME_UINT pm_bundles;
	MME_UINT pm_pft;
	MME_UINT Memory_usage;
    ERROR_CODES Errortype;
    
	} MME_AVSVideoDecodeReturnParams_t;

typedef struct
{
	MME_UINT display_buffer_size;	/* Omega2 frame buffer size (luma+chroma) */
	MME_UINT packed_buffer_size;	/* packed-data buffer size (for VIDA) */
	MME_UINT packed_buffer_total;	/* number of packed buffers (for VIDA) */
} MME_AVSVideoDecodeCapabilityParams_t;

typedef struct _AVSVIDEODECODEPARAMS_T_
	{
		MME_UINT	StartOffset;	/*!< Only used when input buffer is circular*/
		MME_UINT	EndOffset;		/*!< Only used when input buffer is circular*/
		/*MME_UINT    Progressive_frame;*/
		MME_UINT    Picture_structure;
		MME_UINT    Fixed_picture_qp;
		MME_UINT    Picture_qp;
		MME_UINT    Skip_mode_flag;
		MME_UINT    Loop_filter_disable;
		MME_UINT    FrameType;
		int			alpha_offset;
		int			beta_offset;
		int			Picture_ref_flag;
		int tr;
		unsigned int topfield_pos;
		unsigned int botfield_pos;
	} MME_AVSVideoDecodeParams_t;

/*! AVS Picture Coding type */
typedef enum 
	{ 
	AVS_PCT_INTRA = 0, 
	AVS_PCT_PREDICTIVE = 1, 
	AVS_PCT_BIDIRECTIONAL = 2, 
	AVS_PCT_SPRITE = 3 
	} MME_AVSPictureCodingType; 


#endif /* _AVS_DECODERTYPES_H_ */

