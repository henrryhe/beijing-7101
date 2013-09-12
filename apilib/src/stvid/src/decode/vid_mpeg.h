/*******************************************************************************

File name   : vid_mpeg.h

Description : MPEG norm structure header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
22 Feb 2000        Created                                          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VID_MPEG_H
#define __VID_MPEG_H


/* Includes ----------------------------------------------------------------- */


#include "stddefs.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Max sizes in bytes */
/*#define MAX_SEQUENCE                            136
#define MAX_GROUP_OF_PICTURE                    4
#define MAX_SEQUENCE_EXTENSION                  6
#define MAX_SEQUENCE_DISPLAY_EXTENSION          8
#define MAX_SEQUENCE_SCALABLE_EXTENSION         9
#define MAX_PICTURE                             (6 + (9 * MAX_EXTRA_PICTURE_INFORMATION) / 8)
#define MAX_PICTURE_CODING_EXTENSION            6
#define MAX_QUANTIZATION_MATRIX_EXTENSION       257
#define MAX_PICTURE_DISPLAY_EXTENSION           13
#define MAX_PICTURE_TEMPORAL_SCALABLE_EXTENSION 4
#define MAX_PICTURE_SPATIAL_SCALABLE_EXTENSION  7*/


/* Definition of start code values */
#define PICTURE_START_CODE               0x00
#define FIRST_SLICE_START_CODE           0x01
#define GREATEST_SLICE_START_CODE        0xAF
#define RESERVED_START_CODE_0            0xB0
#define RESERVED_START_CODE_1            0xB1
#define USER_DATA_START_CODE             0xB2
#define SEQUENCE_HEADER_CODE             0xB3
#define SEQUENCE_ERROR_CODE              0xB4
#define EXTENSION_START_CODE             0xB5
#define RESERVED_START_CODE_6            0xB6
#define SEQUENCE_END_CODE                0xB7
#define GROUP_START_CODE                 0xB8
#define SMALLEST_SYSTEM_START_CODE       0xB9
#define SMALLEST_AUDIO_PACKET_START_CODE 0xC0
#define GREATEST_AUDIO_PACKET_START_CODE 0xDF
#define SMALLEST_VIDEO_PACKET_START_CODE 0xE0
#define GREATEST_VIDEO_PACKET_START_CODE 0xEF
#define GREATEST_SYSTEM_START_CODE       0xFF

/* Definition of ST internal start codes: Caution this is not in MPEG standard ! */
#define DISCONTINUITY_START_CODE         RESERVED_START_CODE_6


/* Definition of extension_start_code_identifier values */
#define SEQUENCE_EXTENSION_ID                   0x01
#define SEQUENCE_DISPLAY_EXTENSION_ID           0x02
#define QUANT_MATRIX_EXTENSION_ID               0x03
#define COPYRIGHT_EXTENSION_ID                  0x04
#define SEQUENCE_SCALABLE_EXTENSION_ID          0x05
#define PICTURE_DISPLAY_EXTENSION_ID            0x07
#define PICTURE_CODING_EXTENSION_ID             0x08
#define PICTURE_SPATIAL_SCALABLE_EXTENSION_ID   0x09
#define PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID  0x0A

#define QUANTISER_MATRIX_SIZE 64

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

/* Definition of the maximum picture size according to the profile and the level */
#define MAX_VIDEO_ES_SIZE_PROFILE_MAIN_LEVEL_MAIN (229376 + 50000) /* SD */
#define MAX_VIDEO_ES_SIZE_PROFILE_MAIN_LEVEL_HIGH 1222656 /* HD */

/* Exported Types ----------------------------------------------------------- */

typedef enum BitStreamProgress_e
{
    NO_BITSTREAM,
    AFTER_FIRST_SEQUENCE_HEADER,
    AFTER_REPEAT_SEQUENCE_HEADER,
    AFTER_MPEG1_REPEAT_SEQUENCE_HEADER,
    AFTER_SEQUENCE_EXTENSION,
    AFTER_GROUP_OF_PICTURES_HEADER,
    AFTER_PICTURE_HEADER,
    AFTER_PICTURE_CODING_EXTENSION,
    AFTER_PICTURE_DATA
} BitStreamProgress_t;

/* Type for quatisation matrix, 64 values stored in the default zigzag scanning order */
typedef U8 QuantiserMatrix_t[QUANTISER_MATRIX_SIZE];

typedef struct Sequence_s {
    U16  horizontal_size_value;                           /*   12 bits */
    U16  vertical_size_value;                             /*   12 bits */
    U8   aspect_ratio_information;                        /*    4 bits */
    U8   frame_rate_code;                                 /*    4 bits */
    U32  bit_rate_value;                                  /*   18 bits */
    BOOL marker_bit;                                      /*    1 bit  */
    U16  vbv_buffer_size_value;                           /*   10 bits */
    BOOL constrained_parameters_flag;  /* 0 in MPEG2 */   /*    1 bit  */
    BOOL load_intra_quantizer_matrix;                     /*    1 bit  */
    QuantiserMatrix_t intra_quantizer_matrix;             /* 64*8 bits */
    BOOL load_non_intra_quantizer_matrix;                 /*    1 bit  */
    QuantiserMatrix_t non_intra_quantizer_matrix;         /* 64*8 bits */
                                                          /* --------- */
                                                          /* 1088 bits -> 136 bytes from stream */
                                                       /* or  576 bits ->  72 bytes from stream */
                                                       /* or   64 bits ->   8 bytes from stream */
} Sequence_t;

typedef struct UserData_s {
    U8 * Data_p;    /* Pointer to a buffer of bytes */
    U32  TotalSize; /* Size of the buffer of bytes */
    BOOL Overflow;  /* Set TRUE if there was more bytes than the size of the buffer */
    U32  UsedSize;  /* Size of data used in buffer */
                                         /* ------- */
                                         /* X*8 bits -> X bytes from stream */
} UserData_t;

typedef struct SequenceExtension_s {
    /*U8   extension_start_code_identifier; *//*  4 bits */
    U8   profile_and_level_indication;    /*  8 bits */
    BOOL progressive_sequence;            /*  1 bit  */
    U8   chroma_format;                   /*  2 bits */
    U8   horizontal_size_extension;       /*  2 bits */
    U8   vertical_size_extension;         /*  2 bits */
    U16  bit_rate_extension;              /* 12 bits */
    BOOL marker_bit;                      /*  1 bit  */
    U8   vbv_buffer_size_extension;       /*  8 bits */
    BOOL low_delay;                       /*  1 bit  */
    U8   frame_rate_extension_n;          /*  2 bits */
    U8   frame_rate_extension_d;          /*  5 bits */
                                          /* ------- */
                                          /* 48 bits -> 6 bytes from stream */
} SequenceExtension_t;

typedef struct SequenceDisplayExtension_s {
    /*U8   extension_start_code_identifier; *//*  4 bits */
    U8   video_format;                    /*  3 bits */
    BOOL colour_description;              /*  1 bit  */
    U8   colour_primaries;                /*  8 bits */
    U8   transfer_characteristics;        /*  8 bits */
    U8   matrix_coefficients;             /*  8 bits */
    U16  display_horizontal_size;         /* 14 bits */
    BOOL marker_bit;                      /*  1 bit  */
    U16  display_vertical_size;           /* 14 bits */
                                          /* ------- */
                                          /* 61 bits -> 7.625 bytes from stream */
} SequenceDisplayExtension_t;

typedef struct SequenceScalableExtension_s {
    /*U8   extension_start_code_identifier;        *//*  4 bits */
    U8   scalable_mode;                          /*  2 bits */
    U8   layer_id;                               /*  4 bits */
    U16  lower_layer_prediction_horizontal_size; /* 14 bits */
    BOOL marker_bit;                             /*  1 bit  */
    U16  lower_layer_prediction_vertical_size;   /* 14 bits */
    U8   horizontal_subsampling_factor_m;        /*  5 bits */
    U8   horizontal_subsampling_factor_n;        /*  5 bits */
    U8   vertical_subsampling_factor_m;          /*  5 bits */
    U8   vertical_subsampling_factor_n;          /*  5 bits */
    BOOL picture_mux_enable;                     /*  1 bit  */
    BOOL mux_to_progressive_sequence;            /*  1 bit  */
    U8   picture_mux_order;                      /*  3 bits */
    U8   picture_mux_factor;                     /*  3 bits */
                                                 /* ------- */
                                                 /* 67 bits -> 8.375 bytes from stream */
} SequenceScalableExtension_t;

typedef struct GroupOfPictures_s {
    struct {
        BOOL drop_frame_flag; /*  1 bit  */
        U8   hours;           /*  5 bits */
        U8   minutes;         /*  6 bits */
        BOOL marker_bit;      /*  1 bit  */
        U8   seconds;         /*  6 bits */
        U8   pictures;        /*  6 bits */
    } time_code;
    BOOL closed_gop;        /*  1 bit  */
    BOOL broken_link;       /*  1 bit  */
                            /* ------- */
                            /* 27 bits -> 3.375 bytes from stream */
} GroupOfPictures_t;

typedef struct Picture_s {
    U16  temporal_reference;                                       /*  10 bits */
    U8   picture_coding_type; /* D pictures only allowed in MPEG1 */ /*   3 bits */
    U16  vbv_delay;                                                /*  16 bits */
    BOOL full_pel_forward_vector;  /* 0 in MPEG2 */                /*   1 bit  */
    U8   forward_f_code;           /* 0x7 in MPEG2 */              /*   3 bits */
    BOOL full_pel_backward_vector; /* 0 in MPEG2 */                /*   1 bit  */
    U8   backward_f_code;          /* 0x7 in MPEG2 */              /*   3 bits */
    /* extra_information is reserved for future evolution of MPEG */
    /*BOOL extra_bit_picture[MAX_EXTRA_PICTURE_INFORMATION];         *//* X*1 bit  */
    /*U8   extra_information_picture[MAX_EXTRA_PICTURE_INFORMATION]; *//* X*8 bits */
    /*U8   Count;                                                    *//* nbr of extra infos */
/* defined nowhere: this is for future evolution of MPEG */
#ifdef ProcessPictureExtraInformation
    struct {
        U8  num_timecodes;                  /*   2 bits */
        struct {
            BOOL time_discontinuity;        /*   1 bit  */
            BOOL reserved_bit;              /*   1 bit  */
            U32  time_offset;               /*  26 bits */
            U8   units_of_seconds;          /*   4 bits */
            U8   tens_of_seconds;           /*   3 bits */
            U8   units_of_minutes;          /*   4 bits */
            U8   tens_of_minutes;           /*   3 bits */
            U8   units_of_hours;            /*   4 bits */
            U8   tens_of_hours;             /*   2 bits */
        } frame_field_capture_timestamp[2];
        U8  padding_bits;                   /*   6 bits */
    } capture_timecode;
    struct {
        U8   aspect_ratio_information;      /*   4 bits */
        BOOL display_size_present;          /*   1 bit  */
        U16  display_horizontal_size;       /*  14 bits */
        U16  display_vertical_size;         /*  14 bits */
        U8   padding_bits;                  /*   4 bits */
        U16  frame_centre_horizontal_offset[MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS]; /* X*16 bits */
        U16  frame_centre_vertical_offset[MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS]; /* X*16 bits */
        U8   padding_bits;                  /*   3 bits */
    } additional_pan-scan_parameters;
    struct {
        U16 top_left_x;                     /*   16 bits */
        U16 top_left_y;                     /*   16 bits */
        U16 active_region_horizontal_size;  /*   16 bits */
        U16 active_region_vertical_size;    /*   16 bits */
    } active_region_window;
    struct {
        U32 picture_byte_count;             /*   32 bits */
    } coded_picture_length;
#endif
                                                                   /* -------- */
                                                                   /*  37 + X * 9 bits -> 4.625 + (9 * X) / 8 bytes */
                                                                   /* If no extra info : */
                                                                   /*  38 bits -> 4.75 bytes from stream */
                                                                 /* or 34 bits -> 4.25 bytes from stream */
                                                                 /* or 30 bits -> 3.75 bytes from stream */
} Picture_t;

typedef struct PictureCodingExtension_s {
    /*U8   extension_start_code_identifier; *//*  4 bits */
    U8   f_code[2][2];                    /* 4*4 bits */
    U8   intra_dc_precision;              /*  2 bits */
    U8   picture_structure;               /*  2 bits */
    BOOL top_field_first;                 /*  1 bit  */
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
                                          /* ------- */
                                          /*  54 bits -> 6.75 bytes from stream */
} PictureCodingExtension_t;

typedef struct QuantMatrixExtension_s {
    /*U8   extension_start_code_identifier;        */         /*    4 bits */
    BOOL load_intra_quantizer_matrix;                     /*    1 bit  */
    QuantiserMatrix_t intra_quantizer_matrix;             /* 64*8 bits */
    BOOL load_non_intra_quantizer_matrix;                 /*    1 bit  */
    QuantiserMatrix_t non_intra_quantizer_matrix;         /* 64*8 bits */
    BOOL load_chroma_intra_quantizer_matrix;              /*    1 bit  */
    QuantiserMatrix_t chroma_intra_quantizer_matrix;      /* 64*8 bits */
    BOOL load_chroma_non_intra_quantizer_matrix;          /*    1 bit  */
    QuantiserMatrix_t chroma_non_intra_quantizer_matrix;  /* 64*8 bits */
                                                          /* --------- */
                                                          /* 2056 bits -> 257 bytes from stream */
                                                       /* or 1032 bits -> 129 bytes from stream */
                                                       /* or  520 bits ->  65 bytes from stream */
                                                       /* or    8 bits ->   1 bytes from stream */
} QuantMatrixExtension_t;

typedef struct CopyrightExtension_s {
    /*U8   extension_start_code_identifier;   *//*  4 bits */
    BOOL copyright_flag;                    /*  1 bit  */
    U8   copyright_identifier;              /*  8 bits */
    BOOL original_or_copy;                  /*  1 bit  */
    U8   reserved;                          /*  7 bits */
    BOOL marker_bit_1;                      /*  1 bit  */
    U32  copyright_number_1;                /* 20 bits */
    BOOL marker_bit_2;                      /*  1 bit  */
    U32  copyright_number_2;                /* 22 bits */
    BOOL marker_bit_3;                      /*  1 bit  */
    U32  copyright_number_3;                /* 22 bits */
                                            /* ----------*/
                                            /*  88 bits -> 11 bytes from stream */
} CopyrightExtension_t;

typedef struct FrameCentreOffsets_s
{
    S16  frame_centre_horizontal_offset;    /* MPEG type */
    S16  frame_centre_vertical_offset;      /* MPEG type */
} FrameCentreOffsets_t;

typedef struct PictureDisplayExtension_s {
    /*U8   extension_start_code_identifier;   *//*  4 bits */
    U8   number_of_frame_centre_offsets;    /* computed from other values,  1 <= X <= 3 */
    FrameCentreOffsets_t FrameCentreOffsets[MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS]; /* 96 bits */
    BOOL marker_bit_1[MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS];                   /*  3 bit  */
    BOOL marker_bit_2[MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS];                   /*  3 bit  */
                                            /* ----------*/
                                            /* 106 bits -> 13.25 bytes from stream */
                                         /* or  72 bits ->  9 bytes from stream */
                                         /* or  38 bits ->  4.75 bytes from stream */
} PictureDisplayExtension_t;

typedef struct PictureTemporalScalableExtension_s {
    /*U8   extension_start_code_identifier; *//*  4 bits */
    U8   reference_select_code;           /*  2 bits */
    U16  forward_temporal_reference;      /* 10 bits */
    BOOL marker_bit;                      /*  1 bit  */
    U16  backward_temporal_reference;     /* 10 bits */
                                          /* ------- */
                                          /* 27 bits -> 3.375 bytes from stream */
} PictureTemporalScalableExtension_t;

typedef struct PictureSpatialScalableExtension_s {
    /*U8   extension_start_code_identifier;          *//*  4 bits */
    U16  lower_layer_temporal_reference;           /* 10 bits */
    BOOL marker_bit_1;                             /*  1 bit  */
    U16  lower_layer_horizontal_offset;            /* 15 bits */
    BOOL marker_bit_2;                             /*  1 bit  */
    U16  lower_layer_vertical_offset;              /* 15 bits */
    U8   spatial_temporal_weight_code_table_index; /*  2 bits */
    BOOL lower_layer_progressive_frame;            /*  1 bit  */
    BOOL lower_layer_deinterlaced_field_select;    /*  1 bit  */
                                                   /* ------- */
                                                   /* 50 bits -> 6.25 bytes from stream */
} PictureSpatialScalableExtension_t;


typedef struct MPEG2BitStream_s {
    /* Fields to be initialised by user when declaring a variable of this type:
    they should be allocated outside this structure to avoid having them allocated many times */
    UserData_t                          UserData;

    /* Fields to be initialised */
    BitStreamProgress_t                 BitStreamProgress;
    const QuantiserMatrix_t *           DefaultIntraQuantMatrix_p;
    const QuantiserMatrix_t *           DefaultNonIntraQuantMatrix_p;
    const QuantiserMatrix_t *           LumaIntraQuantMatrix_p;     /* Shall be set DefaultIntraQuantMatrix_p at each sequence_header,
                                                                                    &(Sequence.intra_quantizer_matrix) after the matrix is loaded in sequence_header,
                                                                                    &(QuantMatrixExtension.intra_quantizer_matrix) after the matrix is loaded in quant_matrix_extension,
                                                                       Used to always point to the right quant matrix to use (default or loaded) */
    const QuantiserMatrix_t *           LumaNonIntraQuantMatrix_p;  /* Shall be set DefaultNonIntraQuantMatrix_p at each sequence_header,
                                                                                    &(Sequence.non_intra_quantizer_matrix) after the matrix is loaded in sequence_header,
                                                                                    &(QuantMatrixExtension.non_intra_quantizer_matrix) after the matrix is loaded in quant_matrix_extension,
                                                                       Used to always point to the right quant matrix to use (default or loaded) */
    const QuantiserMatrix_t *           ChromaIntraQuantMatrix_p;   /* Shall be set DefaultIntraQuantMatrix_p at each sequence_header,
                                                                                    &(Sequence.intra_quantizer_matrix) after the matrix is loaded in sequence_header,
                                                                                    &(QuantMatrixExtension.chroma_intra_quantizer_matrix) after the matrix is loaded in quant_matrix_extension,
                                                                       Used to always point to the right quant matrix to use (default or loaded) */
    const QuantiserMatrix_t *           ChromaNonIntraQuantMatrix_p; /* Shall be set DefaultNonIntraQuantMatrix_p at each sequence_header,
                                                                                    &(Sequence.non_intra_quantizer_matrix) after the matrix is loaded in sequence_header,
                                                                                    &(QuantMatrixExtension.chroma_non_intra_quantizer_matrix) after the matrix is loaded in quant_matrix_extension,
                                                                       Used to always point to the right quant matrix to use (default or loaded) */
    BOOL                                IntraQuantMatrixHasChanged;  /* Shall be set TRUE after LumaIntraQuantMatrix_p or ChromaIntraQuantMatrix_p has changed,
                                                                                     FALSE after the new matrix have been taken into account for decode
                                                                       Used to know if new matrix have to be taken into account for decode */
    BOOL                                NonIntraQuantMatrixHasChanged; /* Shall be set TRUE after first sequence_header,
                                                                                     FALSE after the new matrix have been taken into account for decode
                                                                       Used to know if new matrix have to be taken into account for decode */
    BOOL                                HasSequence;                /* Shall be set TRUE after first sequence_header,
                                                                                    FALSE after sequence_end,
                                                                       Used to know if sequence is ended or not: it is TRUE within a sequence, FALSE outside a sequence */

    /* Information fields to be accessed by user: */
    BOOL                                MPEG1BitStream;             /* Shall be set TRUE if there is no sequence_extension after the first sequence_header,
                                                                                    FALSE otherwise,
                                                                       Used to differenciate MPEG1/MPEG2 syntax: only sequence_header, group_of_pictures_header and picture_header are valid in MPEG1 */
    /*BOOL                                HasSequenceUserData;*/        /* Shall be set TRUE after first user_data following sequence_extension,
                                                                                    FALSE after each sequence_header or sequence_extension,
                                                                       ? Used to discard repeated sequence user_data (MPEG2 allows only one extension of each type) or keep last one ? */
    BOOL                                HasSequenceDisplay;         /* Shall be set TRUE after first sequence_display_extension after the first sequence_header,
                                                                                    FALSE after first sequence_header or sequence_extension,
                                                                       Used to know if there is sequence_display_extension in the stream
                                                                       ? Used to discard repeated sequence_display_extension (MPEG2 allows only one extension of each type) or keep last one ? */
    BOOL                                IsScalable;                 /* Shall be set TRUE after first sequence_scalable_extension after the first sequence_header,
                                                                                    FALSE after first sequence_header or sequence_extension,
                                                                       Used to know if there is sequence_scalable_extension in the stream
                                                                       ? Used to discard repeated sequence_scalable_extension (MPEG2 allows only one extension of each type) or keep last one ? */
    BOOL                                HasGroupOfPictures;         /* Shall be set TRUE after each group_of_picture_header,
                                                                                    FALSE after sequence_header or sequence_extension,
                                                                       Used to know if temporal_reference is reset */
    /*BOOL                                HasGroupOfPicturesUserData;*/ /* Shall be set TRUE after first user_data following group_of_pictures_header,
                                                                                    FALSE after each group_of_pictures_header,
                                                                       ? Used to discard repeated user_data (MPEG2 allows only one extension of each type) or keep last one ? */
    /*BOOL                                HasPictureUserData;*/         /* Shall be set TRUE after first user_data following picture_coding_extension,
                                                                                    FALSE after each picture_header or picture_coding_extension,
                                                                       ? Used to discard repeated user_data (MPEG2 allows only one extension of each type) or keep last one ? */
    BOOL                                HasQuantMatrixExtension;    /* Shall be set TRUE after first quant_matrix_extension following picture_coding_extension,
                                                                                    FALSE after each picture_header or picture_coding_extension,
                                                                       ? Used to discard repeated quant_matrix_extension (MPEG2 allows only one extension of each type) or keep last one ? */
    BOOL                                HasCopyrightExtension;      /* Shall be set TRUE after first copyright_extension following picture_coding_extension,
                                                                                    FALSE after each picture_header or picture_coding_extension,
                                                                       ? Used to discard repeated copyright_extension (MPEG2 allows only one extension of each type) or keep last one ? */
    /* picture_display_extension considered always present by MPEG2: reset values at each sequence_header, always take latest loaded values. */
/*    BOOL                                HasPictureDisplay;*/      /* Shall be set TRUE after first picture_display_extension following picture_coding_extension,
                                                                                    FALSE after each picture_header or picture_coding_extension,
                                                                       ? Used to discard repeated picture_display_extension (MPEG2 allows only one extension of each type) or keep last one ? */
    BOOL                                HasPictureSpatialScalableExtension;  /* Shall be set TRUE after first picture_spatial_scalable_extension following picture_coding_extension,
                                                                                    FALSE after each picture_header or picture_coding_extension,
                                                                       ? Used to discard repeated picture_spatial_scalable_extension (MPEG2 allows only one extension of each type) or keep last one ? */
    BOOL                                HasPictureTemporalScalableExtension; /* Shall be set TRUE after first picture_temporal_scalable_extension following picture_coding_extension,
                                                                                    FALSE after each picture_header or picture_coding_extension,
                                                                       ? Used to discard repeated picture_temporal_scalable_extension (MPEG2 allows only one extension of each type) or keep last one ? */
    Sequence_t                          Sequence;
    SequenceExtension_t                 SequenceExtension;
    SequenceDisplayExtension_t          SequenceDisplayExtension;
    SequenceScalableExtension_t         SequenceScalableExtension;
    GroupOfPictures_t                   GroupOfPictures;
    Picture_t                           Picture;
    PictureCodingExtension_t            PictureCodingExtension;
    QuantMatrixExtension_t              QuantMatrixExtension;
    CopyrightExtension_t                CopyrightExtension;
    PictureDisplayExtension_t           PictureDisplayExtension;
    PictureSpatialScalableExtension_t   PictureSpatialScalableExtension;
    PictureTemporalScalableExtension_t  PictureTemporalScalableExtension;
} MPEG2BitStream_t;


/* Information for the picture that is necessary to decode it */
typedef struct StreamInfoForDecode_s {
    const QuantiserMatrix_t *   LumaIntraQuantMatrix_p;
    const QuantiserMatrix_t *   LumaNonIntraQuantMatrix_p;
    const QuantiserMatrix_t *   ChromaIntraQuantMatrix_p;
    const QuantiserMatrix_t *   ChromaNonIntraQuantMatrix_p;
    BOOL                        IntraQuantMatrixHasChanged;
    BOOL                        NonIntraQuantMatrixHasChanged;
    BOOL                        MPEG1BitStream;
    U16                         HorizontalSize;
    U16                         VerticalSize;
    Picture_t                   Picture;
    PictureCodingExtension_t    PictureCodingExtension; /* not valid in MPEG1 */
    QuantiserMatrix_t           intra_quantizer_matrix;
    QuantiserMatrix_t           non_intra_quantizer_matrix;
    QuantiserMatrix_t           chroma_intra_quantizer_matrix;
    QuantiserMatrix_t           chroma_non_intra_quantizer_matrix;
    BOOL                        ExpectingSecondField;       /* TRUE for first single field picture, because another single field must follow ! */
    BOOL                        progressive_sequence;/* not valid in MPEG1 */
    U8                          chroma_format; /* not valid in MPEG1 */
} StreamInfoForDecode_t;

/* Stream information for the picture that is not contained in STVID_PictureInfos_t */
typedef struct PictureStreamInfo_s {
    BOOL                        HasSequenceDisplay;
    SequenceDisplayExtension_t  SequenceDisplayExtension;
    PictureDisplayExtension_t   PictureDisplayExtension;
    U32                         ExtendedTemporalReference;
    U32                         TopFieldCounter;            /* According to the stream, number of times the top field must be displayed */
    U32                         BottomFieldCounter;         /* According to the stream, number of times the bottom field must be displayed */
    U32                         RepeatFieldCounter;         /* According to the stream, number of times the first field must be displayed (repeated) */
    U32                         RepeatProgressiveCounter;   /* According to the stream, number of times the (same) picture must be displayed */
    BOOL                        ExpectingSecondField;       /* TRUE for first single field picture, because another single field must follow ! */
    BOOL                        IsBitStreamMPEG1;   /* TRUE if MPEG-1, FALSE if MPEG-2 */
} PictureStreamInfo_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VID_MPEG_H */

/* End of vid_mpeg.h */
