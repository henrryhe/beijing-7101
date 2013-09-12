/*
**  @file     : H264_VideoTransformerTypes_Preproc.h
**
**  @brief    : H264 Video Preprocessor specific types
**
**  @par OWNER: Jerome Audu / Vincent Capony
**
**  @author   : Jerome Audu / Vincent Capony
**
**  @par SCOPE:
**
**  @date     : April 02th 2004
**
**  &copy; 2004 ST Microelectronics. All Rights Reserved.
*/

#ifndef __H264_TRANSFORMERTYPES_PREPROCESSOR_H
#define __H264_TRANSFORMERTYPES_PREPROCESSOR_H

/*
**        Identifies the name of the transformer
*/
#define H264PP_MME_TRANSFORMER_NAME    "H264PP_TRANSFORMER"

#define H264_PREPROCESSOR_ID         0x0CEA
#define H264_PREPROCESSOR_BASE      (H264_PREPROCESSOR_ID << 16)

/*
** H264_PreprocessingError_t :
** 	Errors returned by the H264 preprocessor in MME_CommandStatus_t.Error when
** 	CmdCode is equal to MME_TRANSFORM.
** 	These errors are bitfields, and several bits can be raised at the
** 	same time.
*/
typedef enum
{
	H264_PREPROCESSOR_ERROR_ERR_SC_DETECTED      = (H264_PREPROCESSOR_BASE + 4),
	H264_PREPROCESSOR_ERROR_ERR_BIT_INSERTED     = (H264_PREPROCESSOR_BASE + 8),
	H264_PREPROCESSOR_ERROR_INT_BUFFER_OVERFLOW  = (H264_PREPROCESSOR_BASE + 16),
	H264_PREPROCESSOR_ERROR_BIT_BUFFER_UNDERFLOW = (H264_PREPROCESSOR_BASE + 32),
	H264_PREPROCESSOR_ERROR_BIT_BUFFER_OVERFLOW  = (H264_PREPROCESSOR_BASE + 64),
	H264_PREPROCESSOR_ERROR_READ_ERROR           = (H264_PREPROCESSOR_BASE + 128),
	H264_PREPROCESSOR_ERROR_WRITE_ERROR          = (H264_PREPROCESSOR_BASE + 256)
} H264_PreprocessingError_t;



/*
** H264_TransformParam_preproc_t :
**		H264 specific parameters required for feeding the preprocessor on a picture prepreprocssing
**     command order.
**
**		Naming convention:
**		-----------------
**			H264_TransformParam_preproc_t
**			               |      |
**			               |      |->  Only the data required by the preprocessor are
**			               |           concerned by this structure.
**			               |
**			               |-------------> structure dealing with the parameters of the TRANSFORM
**			                               command when using the MME_SendCommand()
**
*/
typedef struct {
	/* The following addresses define the bit buffer boundaries */
    /*----- for setting the PP_BBG / PP_BBS resisters ------*/
    U32 *                 BitBufferBeginAddress_p; /* Defines the begin address of the bit buffer */
    U32 *                 BitBufferEndAddress_p;   /* Defines the End address of the bit buffer   */

	/*----- for setting the PP_READ_START / PP_READ_STOP resisters ------*/
	U32 *                 PictureStartAddrBitBuffer_p;      /* start address in the bit buffer        */
	U32 *                 PictureStopAddrBitBuffer_p;       /* stop address in the bit buffer         */

	/*----- for setting the PP_ISBG / PP_IPBG / PP_IBS registers ------*/
	U32 *                 IntPictureBufferBegin_p;          /* write start address in the int. buffer
	                                                         for Picture data. See PP_IPBG register
	                                                      */
	U32 *                 IntSliceErrStatusBufferBegin_p;   /* write start address in the int. buffer
	                                                         For Slice Error Status data. See PP_ISBG
	                                                         Register
	                                                      */
	U32 *                 IntBufferStop_p;                  /* write stop address in the int. buffer
	                                                         For Picture data. See PP_IBS register.
	                                                      */


	/*----- for setting the PP_CFG resister ------*/
	U32 transform_8x8_mode_flag;                /* PPS syntax element */
	U32 direct_8x8_inference_flag;              /* SPS syntax element */
	U32 monochrome;                             /* SPS syntax element */
	U32 constrained_intra_pred_flag;            /* PPS syntax element */
	U32 pic_init_qp;                            /* equal to 26 + pic_init_qp_minus26 PPS syntax element */
	U32 num_ref_idx_l1_active_minus1;           /* PPS syntax element : max reference index for reference
	                                               picture list1 */
	U32 num_ref_idx_l0_active_minus1;           /* PPS syntax element : max reference index for reference
	                                               picture list0 */
	U32 deblocking_filter_control_present_flag; /* PPS syntax element */
	U32 weighted_bipred_idc_flag;               /* PPS syntax element */
                                               /* '1':weighted_bipred_idc PPS syntax elem. equal to 1
                                                  '0':weighted_bipred_idc PPS syntax elem. equal to 0 or 2
	                                            */
	U32 weighted_pred_flag;                     /* PPS syntax element */
	U32 delta_pic_order_always_zero_flag;       /* SPS syntax element */
	U32 pic_order_present_flag;                 /* PPS syntax element */
	U32 pic_order_cnt_type;                     /* SPS syntax element */
	U32 frame_mbs_only_flag;                    /* SPS syntax element */
	U32 entropy_coding_mode_flag;               /* PPS syntax element */
	                                            /* '0' : CAVLC entropy coding mode
	                                               '1' : CABAC entropy coding mode
	                                            */
	U32 mb_adaptive_frame_field_flag;           /* SPS syntax element */
                                               /*'0' : no switch between frame and field macroblocks
                                                       within a picture
	                                              '1' : possible use of switching between frame and field
	                                                 macroblocks within frame picture
	                                               */



	/*----- for setting the PP_PICWIDTH register --------*/
	U32 PictureWidthInMbs;           /* Derived from the SPS field "pic_width_in_mbs_minus1" */
	U32 nb_MB_in_picture_minus1;     /* Number of macroblocks to be treated. Max = 8192      */


	/*----- For setting the PP_CODELENGTH register ------*/
	U32 log2_max_pic_order_cnt_lsb;
	U32 log2_max_frame_num;

} H264_TransformParam_preproc_t;



/*
** H264_InitTransformerParam_preproc_t:
**		Parameters required for a preprocessor transformer initialization.
**
**		Naming convention:
**		-----------------
**			H264_InitTransformerParam_preproc_t
**			       |
**			       |-------------> This structure defines the parameter required when
**		                               using the MME_InitTransformer() function.
*/
typedef struct {
    U32 Garbage;
} H264_InitTransformerParam_preproc_t;



/*
** H264_TransformerCapability_preproc_t:
**		Parameters required for a firmware transformer initialization.
**
**		Naming convention:
**		-----------------
**			H264_TransformerCapability_preproc_t
**			       |
**			       |-------------> This structure defines generic parameter required by the
**		                               MME_TransformerCapability_t in the MME API strucure definition.
*/
typedef struct {
	U32 MaximumFieldPreprocessingLatency90KHz; /* Maximum field preprocessing latency
	                                              expressed in 90kHz unit */
} H264_TransformerCapability_preproc_t;




/*
** H264_CommandStatus_preproc_t:
**		Defines the H264 preprocessor command status.
**
**		Naming convention:
**		-----------------
**			H264_CommandStatus_preproc_t
**			       |
**			       |-------------> This structure defines generic parameter required by the
**		                               MME_CommandStatus_t in the MME API strucure definition.
*/
typedef struct {
	U32 * IntSESBBegin_p;       /* Intermediate Slice Error Status Buffer begin         */
	U32 * IntSESBEnd_p;         /* Intermediate Slice Error Status Buffer begin         */
	U32 * IntPictBufferBegin_p; /* Intermediate Picture Buffer begin                    */
	U32 * IntPictBufferEnd_p;   /* Intermediate Picture Buffer begin - reflects PP_WDL  */
} H264_CommandStatus_preproc_t;





#endif /* #ifndef __H264_TRANSFORMERTYPES_PREPROCESSOR_H */

