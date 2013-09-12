/*******************************************************************************
File Name   : vcodmpg2.h

Description : Video driver CODEC MPEG-2 (also MPEG-1) header file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
26 Jan 2004        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVIDCOD_MPEG2_H
#define __STVIDCOD_MPEG2_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stos.h"

#if !defined ST_OSLINUX
#include "stlite.h"
#endif

#include "stevt.h"
#include "stavmem.h"

#include "stgxobj.h"
#include "stlayer.h"

#include "stvid.h"

#include "vidcodec.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Exported Constants ------------------------------------------------------- */

#define MPEG2_Q_MATRIX_SIZE 64

#define FRAME_CENTRE_OFFSET_FIRST               0
#define FRAME_CENTRE_OFFSET_SECOND              1
#define FRAME_CENTRE_OFFSET_THIRD_OR_REPEAT     2
#define MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS      3


/* Definition of aspect_ratio_information values */
#define ASPECT_RATIO_INFO_FORBIDDEN 0
#define ASPECT_RATIO_INFO_SQUARE    1
#define ASPECT_RATIO_INFO_4TO3      2
#define ASPECT_RATIO_INFO_16TO9     3
#define ASPECT_RATIO_INFO_221TO1    4

/* Definition of frame_rate_code values */
#define FRAME_RATE_CODE_24000_1001  1
#define FRAME_RATE_CODE_24          2
#define FRAME_RATE_CODE_25          3
#define FRAME_RATE_CODE_30000_1001  4
#define FRAME_RATE_CODE_30          5
#define FRAME_RATE_CODE_50          6
#define FRAME_RATE_CODE_60000_1001  7
#define FRAME_RATE_CODE_60          8

/* Definition of profile_and_level_indication */
#define PROFILE_SIMPLE                          0x50
#define PROFILE_MAIN                            0x40
#define PROFILE_SNR_SCALABLE                    0x30
#define PROFILE_SPACIALLY_SCALABLE              0x20
#define PROFILE_HIGH                            0x10
#define LEVEL_LOW                               0x0A
#define LEVEL_MAIN                              0x08
#define LEVEL_HIGH_1440                         0x06
#define LEVEL_HIGH                              0x04
#define PROFILE_AND_LEVEL_IDENTIFICATION_MASK   0x7F

/* Definition of chroma_format */
#define CHROMA_FORMAT_4_2_0         1
#define CHROMA_FORMAT_4_2_2         2
#define CHROMA_FORMAT_4_4_4         3

/* Definition of video_format */
#define VIDEO_FORMAT_COMPONENT      0
#define VIDEO_FORMAT_PAL            1
#define VIDEO_FORMAT_NTSC           2
#define VIDEO_FORMAT_SECAM          3
#define VIDEO_FORMAT_MAC            4
#define VIDEO_FORMAT_UNSPECIFIED    5

/* Definition of colour_primaries values */
#define COLOUR_PRIMARIES_ITU_R_BT_709       1
#define COLOUR_PRIMARIES_UNSPECIFIED        2
#define COLOUR_PRIMARIES_ITU_R_BT_470_2_M   4
#define COLOUR_PRIMARIES_ITU_R_BT_470_2_BG  5
#define COLOUR_PRIMARIES_SMPTE_170M         6
#define COLOUR_PRIMARIES_SMPTE_240M         7

/* Definition of transfer_characteristics values */
#define TRANSFER_ITU_R_BT_709       1
#define TRANSFER_UNSPECIFIED        2
#define TRANSFER_ITU_R_BT_470_2_M   4
#define TRANSFER_ITU_R_BT_470_2_BG  5
#define TRANSFER_SMPTE_170M         6
#define TRANSFER_SMPTE_240M         7
#define TRANSFER_LINEAR             8

/* Definition of matrix_coefficients values */
#define MATRIX_COEFFICIENTS_ITU_R_BT_709        1
#define MATRIX_COEFFICIENTS_UNSPECIFIED         2
#define MATRIX_COEFFICIENTS_FCC                 4
#define MATRIX_COEFFICIENTS_ITU_R_BT_470_2_BG   5
#define MATRIX_COEFFICIENTS_SMPTE_170M          6
#define MATRIX_COEFFICIENTS_SMPTE_240M          7

/* Definition of scalable_mode */
#define SCALABLE_MODE_DATA_PARTITIONING     0
#define SCALABLE_MODE_SPATIAL_SCALABILITY   1
#define SCALABLE_MODE_SNR_SCALABILITY       2
#define SCALABLE_MODE_TEMPORAL_SCALABILITY  3

/* Definition for f_code */
#define F_CODE_FORWARD      0
#define F_CODE_BACKWARD     1
#define F_CODE_HORIZONTAL   0
#define F_CODE_VERTICAL     1

/* Definition of temporal_reference */
#define TEMPORAL_REFERENCE_MODULO       1024

/* Definition of picture_coding_type values */
#define PICTURE_CODING_TYPE_I           1
#define PICTURE_CODING_TYPE_P           2
#define PICTURE_CODING_TYPE_B           3
#define PICTURE_CODING_TYPE_D_IN_MPEG1  4

/* Definition of intra_dc_precision values */
#define INTRA_DC_PRECISION_8_BITS   0
#define INTRA_DC_PRECISION_9_BITS   1
#define INTRA_DC_PRECISION_10_BITS  2
#define INTRA_DC_PRECISION_11_BITS  3

/* Definition of picture_structure values */
#define PICTURE_STRUCTURE_TOP_FIELD     1
#define PICTURE_STRUCTURE_BOTTOM_FIELD  2
#define PICTURE_STRUCTURE_FRAME_PICTURE 3

#define FRAME_CENTRE_OFFSET_FIRST               0
#define FRAME_CENTRE_OFFSET_SECOND              1
#define FRAME_CENTRE_OFFSET_THIRD_OR_REPEAT     2
#define MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS      3

/* Definition of variable bitrate streams vbv delay */
#define VARIABLE_BITRATE_STREAMS_VBV_DELAY      0xFFFF

/*
Video Sequence Start Codes
*/
#define  PICTURE_START_CODE         0x00000000
#define  FIRST_SLICE_START_CODE     0x00000001
#define  GREATEST_SLICE_START_CODE  0x000000AF
#define  RESERVED_START_CODE_0      0x000000B0
#define  RESERVED_START_CODE_1      0x000000B1
#define  USER_DATA_START_CODE       0x000000B2
#define  SEQUENCE_HEADER_CODE       0x000000B3
#define  SEQUENCE_ERROR_CODE        0x000000B4
#define  EXTENSION_START_CODE       0x000000B5
#define  RESERVED_START_CODE_6      0x000000B6
#define  SEQUENCE_END_CODE          0x000000B7
#define  GROUP_START_CODE           0x000000B8

/* Definition of ST internal start codes: Caution this is not in MPEG standard ! */
#define DISCONTINUITY_START_CODE         RESERVED_START_CODE_6

#define SMALLEST_SYSTEM_START_CODE       0xB9
#define SMALLEST_AUDIO_PACKET_START_CODE 0xC0
#define GREATEST_AUDIO_PACKET_START_CODE 0xDF
#define SMALLEST_VIDEO_PACKET_START_CODE 0xE0
#define GREATEST_VIDEO_PACKET_START_CODE 0xEF
#define GREATEST_SYSTEM_START_CODE       0xFF


/*
Video Sequence Extension Start Code Identifiers
*/
#define  ES_EXT_RESERVED0                 0x0
#define  ES_EXT_SEQUENCE                  0x1
#define  ES_EXT_SEQ_DISPLAY               0x2
#define  ES_EXT_QUANT_MATRIX              0x3
#define  ES_EXT_COPYRIGHT                 0x4
#define  ES_EXT_SEQ_SCALABLE              0x5
#define  ES_EXT_RESERVED6                 0x6
#define  ES_EXT_PICTURE_DISPLAY           0x7
#define  ES_EXT_PICTURE_CODING            0x8
#define  ES_EXT_PICTURE_SPATIAL_SCALABLE  0x9
#define  ES_EXT_PICTURE_TEMPORAL_SCALABLE 0xA
#define  ES_EXT_RESERVEDB                 0xB
#define  ES_EXT_RESERVEDC                 0xC
#define  ES_EXT_RESERVEDD                 0xD
#define  ES_EXT_RESERVEDE                 0xE
#define  ES_EXT_RESERVEDF                 0xF


extern const DECODER_FunctionsTable_t   DECODER_Mpeg2Functions;
extern const PARSER_FunctionsTable_t    PARSER_Mpeg2Functions;


/* Exported Types ----------------------------------------------------------- */

/* Type for quatisation matrix, 64 values stored in the default zigzag scanning order */
typedef U8 QuantiserMatrix_t[MPEG2_Q_MATRIX_SIZE];

typedef struct FrameCentreOffsets_s
{
    S16  frame_centre_horizontal_offset;    /* MPEG type */
    S16  frame_centre_vertical_offset;      /* MPEG type */
} FrameCentreOffsets_t;



typedef struct MPEG2_GlobalDecodingContextSpecificData_s
{
    U16  vbv_buffer_size_value;                           /*   10 bits */
    BOOL load_intra_quantizer_matrix;                     /*    1 bit  */
    QuantiserMatrix_t intra_quantizer_matrix;             /* 64*8 bits */
    BOOL load_non_intra_quantizer_matrix;                 /*    1 bit  */
    QuantiserMatrix_t non_intra_quantizer_matrix;         /* 64*8 bits */
    BOOL load_chroma_intra_quantizer_matrix;              /*    1 bit  */
    QuantiserMatrix_t chroma_intra_quantizer_matrix;      /* 64*8 bits */
    BOOL load_chroma_non_intra_quantizer_matrix;          /*    1 bit  */
    QuantiserMatrix_t chroma_non_intra_quantizer_matrix;  /* 64*8 bits */

    U8   chroma_format;                   /*  2 bits */
    U8   matrix_coefficients;             /*  8 bits */
    U16  display_horizontal_size;         /* 14 bits */
    U16  display_vertical_size;           /* 14 bits */
    BOOL IsBitStreamMPEG1;   /* TRUE if MPEG-1, FALSE if MPEG-2 */
    BOOL StreamTypeChange;   /* TRUE if the stream type changed */
    BOOL NewSeqHdr;          /* TRUE if a new seq hdr arrived before this picture */
    BOOL NewQuantMxt;        /* TRUE if a new quant mtx extension arrived before this picture */
    BOOL DiscontinuityStartCodeDetected;        /* TRUE if a discontinuity start code is detected*/
    BOOL HasSequenceDisplay;
    BOOL FirstPictureEver;
    U32  ExtendedTemporalReference;
    U32  PreviousExtendedTemporalReference;
    U32  TemporalReferenceOffset;
    BOOL OneTemporalReferenceOverflowed;
    U32  PreviousGreatestExtendedTemporalReference;
    U32  PictureCounter;
    U32  CalculatedPictureDisplayDuration;
    BOOL HasGroupOfPictures;
    BOOL tempRefReachedZero;
    BOOL GetSecondReferenceAfterOverflowing;
} MPEG2_GlobalDecodingContextSpecificData_t;

typedef struct MPEG2_PictureDecodingContextSpecificData_s
{
    BOOL closed_gop;        /*  1 bit  */
    BOOL broken_link;       /*  1 bit  */
    BOOL GotFirstReferencePicture;
    BOOL IsBrokenLinkActive;
} MPEG2_PictureDecodingContextSpecificData_t;

typedef struct MPEG2_PictureSpecificData_s
{
    U16  temporal_reference;                                       /*  10 bits */
    U16  vbv_delay; /* !!! generic */
    BOOL full_pel_forward_vector;  /* 0 in MPEG2 */                /*   1 bit  */
    U8   forward_f_code;           /* 0x7 in MPEG2 */              /*   3 bits */
    BOOL full_pel_backward_vector; /* 0 in MPEG2 */                /*   1 bit  */
    U8   backward_f_code;          /* 0x7 in MPEG2 */              /*   3 bits */
    U8   f_code[2][2];                    /* 4*4 bits */
    U8   intra_dc_precision;              /*  2 bits */
/*    U8   picture_structure;      */         /*  2 bits */
/*    BOOL top_field_first;        */         /*  1 bit  */
    BOOL frame_pred_frame_dct;            /*  1 bit  */
    BOOL concealment_motion_vectors;      /*  1 bit  */
    BOOL q_scale_type;                    /*  1 bit  */
    BOOL intra_vlc_format;                /*  1 bit  */
    BOOL alternate_scan;                  /*  1 bit  */
    BOOL repeat_first_field;              /*  1 bit  */
    BOOL chroma_420_type;                 /*  1 bit  */
    BOOL progressive_frame;               /*  1 bit  */
    BOOL composite_display_flag;          /*  1 bit  */
    BOOL v_axis;                          /*  1 bit  */
    U8   field_sequence;                  /*  3 bits */
    BOOL sub_carrier;                     /*  1 bit  */
    U8   burst_amplitude;                 /*  7 bits */
    U8   sub_carrier_phase;               /*  8 bits */
    U8   number_of_frame_centre_offsets;    /* computed from other values,  1 <= X <= 3 */
    FrameCentreOffsets_t FrameCentreOffsets[MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS]; /* 96 bits */
    const QuantiserMatrix_t *   LumaIntraQuantMatrix_p;
    QuantiserMatrix_t           LumaIntraQuantMatrix;
    const QuantiserMatrix_t *   LumaNonIntraQuantMatrix_p;
    QuantiserMatrix_t           LumaNonIntraQuantMatrix;
    const QuantiserMatrix_t *   ChromaIntraQuantMatrix_p;
    QuantiserMatrix_t           ChromaIntraQuantMatrix;
    const QuantiserMatrix_t *   ChromaNonIntraQuantMatrix_p;
    QuantiserMatrix_t           ChromaNonIntraQuantMatrix;
    BOOL                        IntraQuantMatrixHasChanged;
    BOOL                        NonIntraQuantMatrixHasChanged;
    U32  PreviousExtendedTemporalReference;
    U16  PreviousTemporalReference;
    U16  LastReferenceTemporalReference;
    U16  LastButOneReferenceTemporalReference;
    U8   PreviousMPEGFrame;
    S32  BackwardTemporalReferenceValue;  /* Temporal Reference Values for the decoder. (-1=none) */
    S32  ForwardTemporalReferenceValue;
} MPEG2_PictureSpecificData_t;


/* Exported Macros ---------------------------------------------------------- */
#define  MPEGFrame2Char(Type)  (                                        \
               (  (Type) == STVID_MPEG_FRAME_I)                         \
                ? 'I' : (  ((Type) == STVID_MPEG_FRAME_P)               \
                         ? 'P' : (  ((Type) == STVID_MPEG_FRAME_B)      \
                                  ? 'B'  : 'U') ) )


/* Exported Functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __STVIDCOD_MPEG2_H */

/* End of vcodmpg2.h */
