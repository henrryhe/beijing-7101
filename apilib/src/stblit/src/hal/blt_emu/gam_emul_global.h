/* CONFIDENTIAL STMicroelectronics */
/******************************************************************** */

#ifndef _GLOBAL_H
#define _GLOBAL_H

/******************************************************************** */
/* Settings structures */

/* BLITTER NODE (register level) */
typedef struct
{
	int nip;
	int usr;
	int ins;
	int s1ba;
	int s1ty;
	int s1xy;
	int s1sz;
	int s1cf;
	int s2ba;
	int s2ty;
	int s2xy;
	int s2sz;
	int s2cf;
	int tba;
	int tty;
	int txy;
	int cwo;
	int cws;
	int cco;
	int cml;
	int rzc;
	int hfp;
	int vfp;
	int rsf;
	int ff0;
	int ff1;
	int ff2;
	int ff3;
	int ack;
	int key1;
	int key2;
	int pmk;
	int rst;
	int xyl;
	int xyp;
	int pkz;
	int ALUT[256];
	int RLUT[256];
	int GLUT[256];
	int BLUT[256];
} gam_emul_BLTNode;

/* BLITTER NODE (detailed parameters level) */
typedef struct
{
	int BLT_instruction;
	int EnaInputColorSpaceConverter;
	int EnaCLUTOperator;
	int Ena2DResampler;
	int EnaFlickerFilter;
	int EnaRectangularClipping;
	int EnaColorKey;
	int EnaOutputColorSpaceConverter;
	int EnaPlaneMask;
	int XYLModeEnable;
	int EnaBlitCompletedIRQ;
	int EnaBlitReadyToStart;
	int EnaBlitSuspended;

	int S1_ba;
	int	S1_pitch;
	int S1_format;
	int S1_hscan;
	int S1_vscan;
	int S1_subformat;
	int S1_RGBExtendMode;
	int S1_bigNotLittle;
	int S1_bpp;			/* nof bits/pixel */
	int	S1_x;
	int	S1_y;
	int	S1_width;
	int	S1_height;
	int S1_colorfill;
	int S1_alphaRange;
	int S1_mbField;


	int S2_ba;
	int	S2_pitch;
	int S2_format;
	int S2_hscan;
	int S2_vscan;
	int S2_chroma_extend;
	int S2_chromalinerepeat;
	int S2_subformat;
	int S2_RGBExtendMode;
	int S2_bigNotLittle;
	int S2_bpp;			/* nof bits/pixel */
	int	S2_x;
	int	S2_y;
	int	S2_width;
	int	S2_height;
	int S2_colorfill;
	int S2_alphaRange;
	int S2_mbField;

	int	T_ba;
	int	T_pitch;
	int T_format;
	int T_hscan;
	int T_vscan;
	int T_dither;
	int T_chromanotluma;
	int T_subformat;
	int T_bigNotLittle;
	int T_bpp;			/* nof bits/pixel */
	int	T_x;
	int	T_y;
	int	T_width;
	int	T_height;
	int T_alphaRange;
	int T_mbField;

	int ALU_opcode;
	int ALU_global_alpha;
	int ALU_rop;

	int CHK_Input;
	int CHK_RMode;
	int CHK_GMode;
	int CHK_BMode;
	int CHK_RMin;
	int CHK_RMax;
	int CHK_GMin;
	int CHK_GMax;
	int CHK_BMin;
	int CHK_BMax;

	int ClipXMin;
	int ClipXMax;
	int ClipYMin;
	int ClipYMax;
	int InternalClip;

	int CSC_input_colorimetry;
	int CSC_input_chroma_format;
	int CSC_input_direction;
	int CSC_output_colorimetry;
	int CSC_output_chroma_format;
	int CSC_matrix_in_ngfx_vid;
	int CSC_matrix_out_ngfx_vid;

	int CLUT_mode;
	int CLUT_enable_update;
	int error_diffusion_weight;
	int error_diffusion_enable_2D;
	int CLUT_memory_location;

	int HF_mode;
	int H_enable;
	int HF_increment;
	int HF_memory_location;
	int HF_initial_phase;
	int VF_mode;
	int V_enable;
	int VF_increment;
	int VF_memory_location;
	int VF_initial_phase;
	int FlickerFilter_mode;
	int AlphaHBorder;
	int AlphaVBorder;

	int FF0;
	int FF1;
	int FF2;
	int FF3;

	int PlaneMask;

	/* Blt_XYL*/

	int XYL_buff_format;
	int XYL_format;
	int XYL_mode;
	int XYL_xyp;

	/* endianess */

	int pkz;

} gam_emul_BlitterParameters;



/******************************************************************** */
/* Define	 */
/******************************************************************** */
#ifndef PI
	#define PI 3.1415926535
#endif
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif
/**********************************************************************/
/* generic compositor*/

/* Gamma RGB */
#define gam_emul_TYPE_RGB565		0
#define gam_emul_TYPE_RGB888		1
#define gam_emul_TYPE_ARGB8565		4
#define gam_emul_TYPE_ARGB8888		5
#define gam_emul_TYPE_ARGB1555		6
#define gam_emul_TYPE_ARGB4444		7

/* Gamma CLUT */
#define gam_emul_TYPE_CLUT1			8
#define gam_emul_TYPE_CLUT2			9
#define gam_emul_TYPE_CLUT4			10
#define gam_emul_TYPE_CLUT8			11
#define gam_emul_TYPE_ACLUT44		12
#define gam_emul_TYPE_ACLUT88		13
#define gam_emul_TYPE_CLUT8_4444    14

/* Gamma YCbCr */
#define gam_emul_TYPE_YCbCr888		16
#define gam_emul_TYPE_YCbCr101010   17
#define gam_emul_TYPE_YCbCr422R	    18
#define gam_emul_TYPE_YCbCr422MB	19
#define gam_emul_TYPE_YCbCr420MB	20
#define gam_emul_TYPE_AYCbCr8888	21
#define gam_emul_TYPE_AYCbCr8101010 32 /* outside color format 5 bits */
#define gam_emul_TYPE_YCbCrMBHDpip	22

/* Gamma Alpha planes */
#define gam_emul_TYPE_A1			24
#define gam_emul_TYPE_A8			25

#define gam_emul_TYPE_BYTE			31



#define	gam_emul_MAX_BLITTER_WIDTH		4096
#define	gam_emul_INPUT_LOCATION			0
#define	gam_emul_OUTPUT_LOCATION			1
#define	gam_emul_RECT_CLIPPING			1
#define	gam_emul_COLOR_KEY				2
#define gam_emul_PRE_RESCALER     FALSE
#define gam_emul_POST_RESCALER     TRUE

#define gam_emul_NUMBER_OF_TAPS			5
#define gam_emul_NUMBER_OF_SUBPOSITIONS	8
#define gam_emul_QUANTIZATION			6

/* Various parameters */
#define gam_emul_SRC1 0
#define gam_emul_SRC2 1
#define gam_emul_TARGET 2
#define gam_emul_LUMA   1
#define gam_emul_CHROMA 2


extern char *gam_emul_BLTMemory;

extern int gam_emul_RescalerCurrentState;


extern int gam_emul_BLTMemSize;


/* Blitter Extern variables */

extern gam_emul_BLTNode CurrentBLTNode;

extern int *gam_emul_HFilter,*gam_emul_VFilter, *gam_emul_Delay;
extern int *gam_emul_VF_Fifo1,*gam_emul_VF_Fifo2,*gam_emul_VF_Fifo3,*gam_emul_VF_Fifo4,*gam_emul_VF_Fifo5,*gam_emul_VF_Output;
extern int *gam_emul_FF_Fifo1,*gam_emul_FF_Fifo2,*gam_emul_FF_Fifo3,*gam_emul_FF_Output;
extern int gam_emul_VF_LineNumber,gam_emul_VF_Counter,gam_emul_VF_Subposition,gam_emul_VF_LineAvailable,gam_emul_NbNewLines;
extern int gam_emul_HF_Counter,gam_emul_HF_Subposition,gam_emul_NbPix;
extern int gam_emul_FF_Flag,gam_emul_NumberOfOutputLines;
extern int gam_emul_FF0[3],gam_emul_FF1[3],gam_emul_FF2[3],gam_emul_FF3[3];
extern int gam_emul_FFThreshold01,gam_emul_FFThreshold12,gam_emul_FFThreshold23;

/*XYL*/
#define gam_emul_FORMATXY		0
#define gam_emul_FORMATXYLu		1
#define gam_emul_FORMATXYuC		2
#define gam_emul_FORMATXYLC		3


/*********************************************************************/
/* Definition of types */
#define U8 unsigned char
#define U16 unsigned short
#define U32 unsigned int
#define S8 char
#define S16 short
#define S32 int



#endif /* _GLOBAL_H */

