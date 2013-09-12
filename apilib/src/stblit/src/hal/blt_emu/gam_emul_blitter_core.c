/************************************************************************************

File Name :  gam_emul_blitter_core.c

Description :

ST Copyright 1999 - CONFIDENTIAL

Reference to the origin of this file within the build system :

References to related design specification, tools : Vide Front End Subsystem
- Register Manual Rev 1.1 and Design Specification Rev 1.1

************************************************************************************/

#include "stdlib.h"

#include "gam_emul_share.h"
#include "gam_emul_util.h"
#include "gam_emul_global.h"
#include "gam_emul_blitter_core.h"
#include "blt_init.h"


/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ExtractBlitterParameters
 * Parameters: NodeAddress,gam_emul_BlitterParameters
 * Return : next node.
 * Description : extract node's blitter parameters and store them in blitterparameters
 * ----------------------------------------------------------------------------------*/

int gam_emul_ExtractBlitterParameters(int NodeAddress,gam_emul_BlitterParameters *Blt)
{
	int NextNodeAddress;
	int ins, s1ba, s1ty, s1xy, s1sz, s1cf, s2ba, s2ty, s2xy, s2sz, s2cf, tba, tty, txy;
	int ack, key1, key2, cwo, cws, cco, cml, rzc, rsf, hfp, vfp, ff0, ff1, ff2, ff3, planeMask;
    int xyl, xyp, nip;
    int bigNotLittle;


    Blt->pkz  = 0;
    bigNotLittle = (Blt->pkz >> 5) & 0x1;

    ins = gam_emul_readInt((int*)(NodeAddress)+2);
	ins = gam_emul_swap_32(ins, bigNotLittle);

	Blt->BLT_instruction				= ins & 0x1F;
	Blt->EnaInputColorSpaceConverter	= (ins >> 6) & 0x1;
	Blt->EnaCLUTOperator				= (ins >> 7) & 0x1;
	Blt->Ena2DResampler					= (ins >> 8) & 0x1;
	Blt->EnaFlickerFilter				= (ins >> 9) & 0x1;
	Blt->EnaRectangularClipping			= (ins >> 10) & 0x1;
	Blt->EnaColorKey					= (ins >> 11) & 0x1;
	Blt->EnaOutputColorSpaceConverter	= (ins >> 12) & 0x1;
	Blt->EnaPlaneMask					= (ins >> 14) & 0x1;
	Blt->XYLModeEnable					= (ins >> 23) & 0x1;
	Blt->EnaBlitCompletedIRQ			= (ins >> 18) & 0x1;
	Blt->EnaBlitReadyToStart			= (ins >> 19) & 0x1;
	Blt->EnaBlitSuspended				= (ins >> 20) & 0x1;

    s1ba = gam_emul_readInt((int*)(NodeAddress)+3);
	s1ba = gam_emul_swap_32(s1ba, bigNotLittle);
	Blt->S1_ba = s1ba;

    s1ty = gam_emul_readInt((int*)(NodeAddress)+4) ;
	s1ty = gam_emul_swap_32(s1ty, bigNotLittle);

	Blt->S1_pitch			= s1ty & 0xFFFF;
	Blt->S1_format			= (s1ty >> 16) & 0x1F;
	Blt->S1_hscan			= (s1ty >> 24) & 0x1;
	Blt->S1_vscan			= (s1ty >> 25) & 0x1;
	Blt->S1_subformat		= (s1ty >> 28) & 0x1;
	Blt->S1_RGBExtendMode	= (s1ty >> 29) & 0x1;
	Blt->S1_bigNotLittle	= (s1ty >> 30) & 0x1;
	Blt->S1_bpp				= gam_emul_GetBpp(Blt->S1_format);
	Blt->S1_alphaRange		= (s1ty >> 21) & 0x1;
	Blt->S1_mbField			= (s1ty >> 23) & 0x1;

    s1xy = gam_emul_readInt((int*)(NodeAddress)+5) ;
	s1xy = gam_emul_swap_32(s1xy, bigNotLittle);

	Blt->S1_x				= s1xy & 0xFFFF;;
	Blt->S1_y				= s1xy >> 16;

    s1sz = gam_emul_readInt((int*)(NodeAddress)+6);
	s1sz = gam_emul_swap_32(s1sz, bigNotLittle);
	Blt->S1_width			= s1sz & 0xFFFF;
	Blt->S1_height			= s1sz >> 16;

    s1cf = gam_emul_readInt((int*)(NodeAddress)+7);
	s1cf = gam_emul_swap_32(s1cf, bigNotLittle);
	Blt->S1_colorfill		= s1cf;

    s2ba = gam_emul_readInt((int*)(NodeAddress)+8);
	s2ba = gam_emul_swap_32(s2ba, bigNotLittle);
	Blt->S2_ba = s2ba;

    s2ty = gam_emul_readInt((int*)(NodeAddress)+9);
	s2ty = gam_emul_swap_32(s2ty, bigNotLittle);

	Blt->S2_pitch				= s2ty & 0xFFFF;
	Blt->S2_format				= (s2ty >> 16) & 0x1F;
	Blt->S2_hscan				= (s2ty >> 24) & 0x1;
	Blt->S2_vscan				= (s2ty >> 25) & 0x1;
	Blt->S2_chroma_extend		= (s2ty >> 26) & 0x1;
	Blt->S2_chromalinerepeat	= (s2ty >> 27) & 0x1;
	Blt->S2_subformat			= (s2ty >> 28) & 0x1;
	Blt->S2_RGBExtendMode		= (s2ty >> 29) & 0x1;
	Blt->S2_bigNotLittle		= (s2ty >> 30) & 0x1;
	Blt->S2_alphaRange		= (s2ty >> 21) & 0x1;
	Blt->S2_mbField			= (s2ty >> 23) & 0x1;
	Blt->S2_bpp					= gam_emul_GetBpp(Blt->S2_format);

    s2xy = gam_emul_readInt((int*)(NodeAddress)+10) ;
	s2xy = gam_emul_swap_32(s2xy, bigNotLittle);

	Blt->S2_x					= s2xy & 0xFFFF;
	Blt->S2_y					= s2xy >> 16;

    s2sz = gam_emul_readInt((int*)(NodeAddress)+11) ;
	s2sz = gam_emul_swap_32(s2sz, bigNotLittle);

	Blt->S2_width				= s2sz & 0xFFFF;
	Blt->S2_height				= s2sz >> 16;

    s2cf = gam_emul_readInt((int*)(NodeAddress)+12);
	s2cf = gam_emul_swap_32(s2cf, bigNotLittle);
	Blt->S2_colorfill = s2cf;


    tba = gam_emul_readInt((int*)(NodeAddress)+13);
	tba = gam_emul_swap_32(tba, bigNotLittle);
	Blt->T_ba = tba;

    tty = gam_emul_readInt((int*)(NodeAddress)+14);
	tty = gam_emul_swap_32(tty, bigNotLittle);

	Blt->T_pitch			= tty & 0xFFFF;
	Blt->T_format			= (tty >> 16) & 0x1F;
	Blt->T_hscan			= (tty >> 24) & 0x1;
	Blt->T_vscan			= (tty >> 25) & 0x1;
	Blt->T_dither			= (tty >> 26) & 0x1;
	Blt->T_chromanotluma	= (tty >> 27) & 0x1;
	Blt->T_subformat		= (tty >> 28) & 0x1;
	Blt->T_bigNotLittle		= (tty >> 30) & 0x1;
	Blt->T_alphaRange		= (tty >> 21) & 0x1;
	Blt->T_mbField			= (tty >> 23) & 0x1;
	Blt->T_bpp				= gam_emul_GetBpp(Blt->T_format);

    txy = gam_emul_readInt((int*)(NodeAddress)+15);
	txy = gam_emul_swap_32(txy, bigNotLittle);

	Blt->T_x				= txy & 0xFFFF;;
	Blt->T_y				= txy >> 16 ;

	Blt->T_width			= s1sz & 0xFFFF;			/* same reg.as S1 */
	Blt->T_height			= s1sz >> 16; /*field access mode (h+1)/2*/
	/*Blt->T_height			= (Blt->S1_mbField  == 1 ) ? *//*field access mode (h+1)/2*/
	/*							((s1sz >> 16) + 1 )>>1 : */
	/*							s1sz >> 16 ;*/

    ack = gam_emul_readInt((int*)(NodeAddress)+28);
	ack = gam_emul_swap_32(ack, bigNotLittle);

	Blt->ALU_opcode			= ack & 0xF;
	Blt->ALU_global_alpha	= (ack >> 8) & 0xFF;
	Blt->ALU_rop			= (ack >> 8) & 0xF;

	Blt->CHK_Input	= (ack >> 22) & 0x3;
	Blt->CHK_RMode	= (ack >> 20) & 0x3;
	Blt->CHK_GMode	= (ack >> 18) & 0x3;
	Blt->CHK_BMode	= (ack >> 16) & 0x3;

    key1 = gam_emul_readInt((int*)(NodeAddress)+29);
	key1 = gam_emul_swap_32(key1, bigNotLittle);

	Blt->CHK_RMin	= (key1 >> 16) & 0xFF;
	Blt->CHK_GMin	= (key1 >> 8) & 0xFF;
	Blt->CHK_BMin	= (key1 >> 0) & 0xFF;

    key2 = gam_emul_readInt((int*)(NodeAddress)+30);
	key2 = gam_emul_swap_32(key2, bigNotLittle);

	Blt->CHK_RMax	= (key2 >> 16) & 0xFF;
	Blt->CHK_GMax	= (key2 >> 8) & 0xFF;
	Blt->CHK_BMax	= (key2 >> 0) & 0xFF;

    cwo = gam_emul_readInt((int*)(NodeAddress)+16);
	cwo = gam_emul_swap_32(cwo, bigNotLittle);

	Blt->ClipXMin		= cwo & 0x7FFF;
	Blt->ClipYMin		= (cwo >> 16) & 0x7FFF;
	Blt->InternalClip	= (cwo >> 31) & 0x1;

    cws = gam_emul_readInt((int*)(NodeAddress)+17);
	cws = gam_emul_swap_32(cws, bigNotLittle);

	Blt->ClipXMax		= cws& 0x7FFF;
	Blt->ClipYMax		= (cws>> 16) & 0x7FFF;

    cco = gam_emul_readInt((int*)(NodeAddress)+18);
	cco = gam_emul_swap_32(cco, bigNotLittle);

	Blt->CSC_input_colorimetry		= (cco>> 0) & 0x1;
	Blt->CSC_input_chroma_format	= (cco>> 1) & 0x1;
	Blt->CSC_input_direction		= (cco>> 2) & 0x1;
	Blt->CSC_matrix_in_ngfx_vid		= (cco>> 3) & 0x1;
	Blt->CSC_output_colorimetry		= (cco>> 8) & 0x1;
	Blt->CSC_output_chroma_format	= (cco>> 9) & 0x1;
	Blt->CSC_matrix_out_ngfx_vid	= (cco>> 10) & 0x1;

	Blt->CLUT_mode					= (cco>> 16) & 0x3;
	Blt->CLUT_enable_update			= (cco>> 18) & 0x1;
	Blt->error_diffusion_weight		= (cco>> 19) & 0x7;
	Blt->error_diffusion_enable_2D	= (cco>> 22) & 0x1;

    cml = gam_emul_readInt((int*)(NodeAddress)+19);
	cml = gam_emul_swap_32(cml, bigNotLittle);

	Blt->CLUT_memory_location = cml;

    rzc = gam_emul_readInt((int*)(NodeAddress)+20);
	rzc = gam_emul_swap_32(rzc, bigNotLittle);

    Blt->HF_mode            = (rzc >> 0) & 0x3;
	Blt->H_enable			= (rzc >> 2) & 0x1;
    rsf = gam_emul_readInt((int*)(NodeAddress)+23);
	rsf = gam_emul_swap_32(rsf, bigNotLittle);
	Blt->HF_increment		= (rsf>> 0) & 0xFFFF;

    hfp = gam_emul_readInt((int*)(NodeAddress)+21);
	hfp = gam_emul_swap_32(hfp, bigNotLittle);
	Blt->HF_memory_location	= hfp;

	Blt->HF_initial_phase	= (rzc >> 16) & 0x7;

    Blt->VF_mode            = (rzc >> 4) & 0x3;
	Blt->V_enable			= (rzc >> 6) & 0x1;

	Blt->VF_increment		= (rsf>> 16) & 0xFFFF;

    vfp = gam_emul_readInt((int*)(NodeAddress)+22);
	vfp = gam_emul_swap_32(vfp, bigNotLittle);
	Blt->VF_memory_location = vfp;

	Blt->VF_initial_phase	= (rzc >> 20) & 0x7;

	if ((Blt->Ena2DResampler == 0) || (Blt->H_enable == 0))
	{
		Blt->HF_mode			= 0;	/* = HF disabled */
		Blt->HF_increment		= 0x400;/* = 1.0 */
		Blt->HF_initial_phase	= 0;
	}
	if ((Blt->Ena2DResampler == 0) || (Blt->V_enable == 0))
	{
		Blt->VF_mode			= 0;	/* = VF disabled */
		Blt->VF_increment		= 0x400;/* = 1.0 */
		Blt->VF_initial_phase	= 0;
	}

	Blt->FlickerFilter_mode	= (rzc >> 8) & 0x3;
	Blt->AlphaHBorder		= (rzc >> 12) & 0x1;
	Blt->AlphaVBorder		= (rzc >> 13) & 0x1;

    ff0 = gam_emul_readInt((int*)(NodeAddress)+24);
	ff0 = gam_emul_swap_32(ff0, bigNotLittle);
	Blt->FF0 = ff0;

    ff1 = gam_emul_readInt((int*)(NodeAddress)+25);
	ff1 = gam_emul_swap_32(ff1, bigNotLittle);
	Blt->FF1 = ff1;

    ff2 = gam_emul_readInt((int*)(NodeAddress)+26);
	ff2 = gam_emul_swap_32(ff2, bigNotLittle);
	Blt->FF2 = ff2;

    ff3 = gam_emul_readInt((int*)(NodeAddress)+27);
	ff3 = gam_emul_swap_32(ff3, bigNotLittle);
	Blt->FF3 = ff3;

    planeMask = gam_emul_readInt((int*)(NodeAddress)+31);
	planeMask = gam_emul_swap_32(planeMask, bigNotLittle);
	Blt->PlaneMask = planeMask;

	/* XYL */

    xyl = gam_emul_readInt((int*)(NodeAddress)+33);
	xyl = gam_emul_swap_32(xyl, bigNotLittle);

	Blt->XYL_buff_format = (xyl >> 16) & 0xFFFF;
	Blt->XYL_format = (xyl >> 8) & 0x3;
	Blt->XYL_mode = (xyl ) & 0x1;

    xyp = gam_emul_readInt((int*)(NodeAddress)+34);
    xyp = gam_emul_swap_32(xyp, bigNotLittle);
	Blt->XYL_xyp = xyp ;

    nip = gam_emul_readInt((int*)(NodeAddress)+0);
	nip = gam_emul_swap_32(nip, bigNotLittle);
	NextNodeAddress = nip;		/* BLT_NIP register */

	return (NextNodeAddress);
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_DirectCopy
 * Parameters: gam_emul_BlitterParameters
 * Return : -
 * Description : Scr1 Direct Copy mode
 * ----------------------------------------------------------------------------------*/

void gam_emul_DirectCopy(gam_emul_BlitterParameters *Blt)
{
	int x,y;
	int S1_datapix;										/* S1_addresspix,T_addresspix; */

	for ( y = 0 ; y < Blt->T_height ; y++ )
	{
		for ( x = 0 ; x < Blt->T_width ; x++ )
		{
			S1_datapix = gam_emul_ReadBlitterPixel(x,y,Blt,gam_emul_SRC1);	/* read pixel from source*/
			gam_emul_WriteBlitterPixel(x,y,Blt,S1_datapix);			/* write pixel in target */
		}
	}
}
/*-----------------------------------------------------------------------------------
 * Function : gam_emul_DirectFill
 * Parameters: gam_emul_BlitterParameters
 * Return : -
 * Description : target is filled with color
 * ----------------------------------------------------------------------------------*/

void gam_emul_DirectFill(gam_emul_BlitterParameters *Blt)
{
	int x,y;
	int S1_datapix;

	for ( y = 0 ; y < Blt->T_height ; y++ )
	{
		for ( x = 0 ; x < Blt->T_width ; x++ )
		{
			/* get source pixel */
			S1_datapix = Blt->S1_colorfill;
			gam_emul_WriteBlitterPixel(x,y,Blt,S1_datapix);		/* write pixel in target */
		}
	}
}


static	int S1LevelA[gam_emul_MAX_BLITTER_WIDTH];	/* The different bus represented as line arrays, */
static	int S2LevelA[gam_emul_MAX_BLITTER_WIDTH];	/* at the different pipeline levels. */
static	int S2LevelB[gam_emul_MAX_BLITTER_WIDTH];
static 	int S2LevelC[gam_emul_MAX_BLITTER_WIDTH];
static 	int S2LevelD[gam_emul_MAX_BLITTER_WIDTH];
static 	int S2LevelE[gam_emul_MAX_BLITTER_WIDTH];
static 	int S2LevelF[gam_emul_MAX_BLITTER_WIDTH];
static 	int TLevelA[gam_emul_MAX_BLITTER_WIDTH];
static 	int TLevelB[gam_emul_MAX_BLITTER_WIDTH];
static 	int TLevelC[gam_emul_MAX_BLITTER_WIDTH];
static 	int SaveUnformattedS1[gam_emul_MAX_BLITTER_WIDTH];
static  int TMask[gam_emul_MAX_BLITTER_WIDTH];          /* Mask line array for handling color key & clipping     */

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_GeneralBlitterOperation
 * Parameters: gam_emul_BlitterParameters
 * Return : -
 * Description : target is filled with color
 * ----------------------------------------------------------------------------------*/

void gam_emul_GeneralBlitterOperation(stblit_Device_t* Device_p, gam_emul_BlitterParameters *Blt)
{
	int x;
	int y_S1,Next_y_S2,y_T,Current_y_S2;
	int RequestS2Line;
	int NewTargetLine;
	int Dummy;
	int S1followsS2;
	int heigthTarget;

	gam_emul_RescalerCurrentState = 0;
	RequestS2Line = 1;
	NewTargetLine = 1;
	y_S1 = Next_y_S2 = y_T = 0;
	Current_y_S2 = -1;

	if   (((Blt->S2_chromalinerepeat == 1)
		&& ((Blt->S1_format) == gam_emul_TYPE_YCbCr420MB))
		|| ((Blt->S2_chromalinerepeat == 1) && ((Blt->S1_format) == gam_emul_TYPE_YCbCrMBHDpip))
		|| ((Blt->S1_format) == gam_emul_TYPE_YCbCr422MB))
		S1followsS2 = 1; /* Luma on Scr1 and Chroma on Scr2 */
	else
		S1followsS2 = 0;

	if ((Blt->S2_mbField == 1) && ( (Blt->S2_format == gam_emul_TYPE_YCbCr422MB) ||
		(((Blt->S2_format == gam_emul_TYPE_YCbCr420MB) || (Blt->S2_format == gam_emul_TYPE_YCbCrMBHDpip)) && (Blt->S2_chromalinerepeat == 1))))
	{
		heigthTarget = (Blt->T_height + 1) >> 1 ; /* (h + 1) / 2*/
	}
	else
	{
		heigthTarget = Blt->T_height;
	}

	while (y_T < heigthTarget)
	{
		if (RequestS2Line == 1)
		{

			gam_emul_FetchNewSourceLine(gam_emul_SRC2,Blt,S2LevelA,NULL,Next_y_S2); /* Bus formatter source 2 */
																			/*	and Scr2 Color file*/

			if(((Blt->S2_format == gam_emul_TYPE_YCbCr420MB) || (Blt->S2_format == gam_emul_TYPE_YCbCrMBHDpip))
				&& (((Blt->ALU_rop == 7) && (Blt->ALU_opcode == 1)) || (Blt->ALU_opcode == 6))) /* concatenation S1/S2 or operation OR */
			{
				for ( x=0 ; x < 2*Blt->S2_width ; x++ ) /* shift values*/
				{
					if ((Blt->S2_x &  0x1) == 0)
					{
						if ( (x & 0x1) == 0x0)
							*(S2LevelA+(x>>1)) = *(S2LevelA+x);
					}
					else
					{
						if ((x & 0x1) == 0x1)
							*(S2LevelA+(x>>1)) = *(S2LevelA+x);
					}
				}
			}

			if (S1followsS2 == 1)
				gam_emul_FetchNewSourceLine(gam_emul_SRC1,Blt,S1LevelA,SaveUnformattedS1,Next_y_S2);

			if ((Blt->S2_mbField == 1) && ( (Blt->S2_format == gam_emul_TYPE_YCbCr422MB) ||
				(((Blt->S2_format == gam_emul_TYPE_YCbCr420MB) || (Blt->S2_format == gam_emul_TYPE_YCbCrMBHDpip)) && (Blt->S2_chromalinerepeat == 1))))
			{
				if (Next_y_S2 < (Blt->S2_height - 1))
				{
					/* MB's organization */
					Current_y_S2 += 2;
					Next_y_S2 += 2;
				}
				else
					Current_y_S2 = Next_y_S2;
			}
			else
			{
				if ((Blt->S2_mbField == 1) && (((Blt->S2_format == gam_emul_TYPE_YCbCr420MB) || (Blt->S2_format == gam_emul_TYPE_YCbCrMBHDpip)) && (Blt->S2_chromalinerepeat == 0)))
				{
					if (Next_y_S2 < (2*Blt->S2_height- 2))
					{
						/* MB's organization */
						Current_y_S2 += 2;
						Next_y_S2 += 2;
					}
					else
						Current_y_S2 = Next_y_S2;
				}
				else
				{
					if (Next_y_S2 < (Blt->S2_height- 1))
					{
						Current_y_S2 +=  1;
						Next_y_S2 += 1;
					}
					else
						Current_y_S2 = Next_y_S2;
				}
			}
		}
		if ((NewTargetLine == 1) && (S1followsS2 == 0)) /* no MB format in entry */
		{
			gam_emul_FetchNewSourceLine(gam_emul_SRC1,Blt,S1LevelA,SaveUnformattedS1,y_S1);
			if ((Blt->S1_mbField == 1) && ( (Blt->S1_format == gam_emul_TYPE_YCbCr422MB) ||
				(((Blt->S1_format == gam_emul_TYPE_YCbCr420MB) || (Blt->S1_format == gam_emul_TYPE_YCbCrMBHDpip)) && (Blt->S2_chromalinerepeat == 1))))
			{
				if (y_S1 < (Blt->S1_height - 1))
				{
					/* MB's organization */
					y_S1 += 2;
				}

			}
			else
			{
				if ((Blt->S1_mbField == 1) && (((Blt->S1_format == gam_emul_TYPE_YCbCr420MB) || (Blt->S1_format == gam_emul_TYPE_YCbCrMBHDpip)) && (Blt->S2_chromalinerepeat == 0)))
				{
					if (y_S1 < (2*Blt->S1_height- 2))
					{
						/* MB's organization */
						y_S1 += 2; /*y_step because MB's organization */
					}
				}
				else
				{
					if (y_S1 < (Blt->S1_height- 1))
					{
						y_S1 += 1;
					}
				}
			}

		}
		/* 4:2:2 to 4:4:4 conversion or S2LevelA -> S2LevelB */

		gam_emul_SamplingPatternConversion42XTo444(Blt,S1LevelA,S2LevelA,S2LevelB,Current_y_S2);

		/* conversion YCbCr <> RGB */

		gam_emul_ColorSpaceConverter(Blt,S2LevelB,S2LevelC,gam_emul_INPUT_LOCATION);

		/* Clut color expansion or color correction */

		gam_emul_CLUTOperator(&CurrentBLTNode, Blt,S2LevelC,S2LevelD,gam_emul_PRE_RESCALER,Current_y_S2);

		/* resize engine */

        NewTargetLine = gam_emul_RescaleOperator(Device_p, Blt,S2LevelD,S2LevelE,&RequestS2Line);

		/* Clut color reduction */

		gam_emul_CLUTOperator(&CurrentBLTNode,Blt,S2LevelE,S2LevelF,gam_emul_POST_RESCALER,y_T);

		if (NewTargetLine == 1) /* Notice : NewTargetLine is initialized by 1 */
		{
			/* ALU */

			gam_emul_ALUOperator(Blt,S1LevelA,S2LevelF,TLevelA,y_T);

			/* mask array is clear */

			gam_emul_ClearMaskArray(Blt,TMask);

			/* write enable - rectangular clipping */

			if (Blt->EnaRectangularClipping == 1)
				gam_emul_SetWriteEnableMask(Blt,TMask,gam_emul_RECT_CLIPPING,&Dummy,&Dummy,&Dummy,y_T);

			/* write enable - color key */

			if (Blt->EnaColorKey == 1)
				gam_emul_SetWriteEnableMask(Blt,TMask,gam_emul_COLOR_KEY,S1LevelA,S2LevelC,S2LevelF,0);

			/* conversion YCbCr <> RGB */

			gam_emul_ColorSpaceConverter(Blt,TLevelA,TLevelB,gam_emul_OUTPUT_LOCATION);

			/* 4:4:4 to 4:2:2 conversion or TLevelB -> TLevelC */

			gam_emul_SamplingPatternConversion444To422(Blt,TLevelB,TLevelC);

			/* output formatter on target */

			gam_emul_WriteTargetLine(Blt,TLevelC,SaveUnformattedS1,TMask,y_T); /* Include PlaneMask & Dither */

			y_T =  y_T + Blt->T_mbField +1;
		}
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ReadBlitterPixel
 * Parameters: x, y,gam_emul_BlitterParameters ,Source
 * Return : int recovered data
 * Description : read data x,y from source
 * ----------------------------------------------------------------------------------*/

int gam_emul_ReadBlitterPixel(int x,int y,gam_emul_BlitterParameters *Blt,int Source)
{
	int SX_addresspix,SX_datapix;
	int SX_absoluteX,SX_absoluteY;
	int bigNotLittleBitmap;

	if (Source == gam_emul_SRC1)
	{
		bigNotLittleBitmap = Blt->S1_bigNotLittle;

		/* in XYL Standard, bits [24,25,] BLT_S1TY have no meaning*/
		if ((Blt->XYLModeEnable == 1) && (Blt->XYL_mode == 0)) /* XYL standard */
		{	/* the register source 1 XY location is unused */
				SX_absoluteX = x ;		/* Given parameter = x left */
				SX_absoluteY = y ;		/* Given parameter = y top */
		}
		else
		{
			/* Let's consider the horizontal scanning direction */
			if (Blt->S1_hscan == 0)
				SX_absoluteX = (x + Blt->S1_x);		/* Given parameter = x left */
			else
				SX_absoluteX = (Blt->S1_x - x);		/* Given parameter = x right */
			if (Blt->S1_vscan == 0)
				SX_absoluteY = (y + Blt->S1_y);		/* Given parameter = y top */
			else
				SX_absoluteY = (Blt->S1_y - y);		/* Given parameter = y bottom */
		}
		switch (Blt->S1_format)
		{
			case gam_emul_TYPE_YCbCr888:
			case gam_emul_TYPE_RGB888:
			case gam_emul_TYPE_ARGB8565:
				/* get source pixel (24 bpp) */
				SX_addresspix = Blt->S1_ba + SX_absoluteY*Blt->S1_pitch + SX_absoluteX*(Blt->S1_bpp>>3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				SX_datapix = gam_emul_swap_24(SX_datapix, bigNotLittleBitmap);
				SX_datapix &= 0xFFFFFF;
				break;
			case gam_emul_TYPE_AYCbCr8888:
			case gam_emul_TYPE_ARGB8888:
				/* get source pixel (32 bpp) */
				SX_addresspix = Blt->S1_ba + SX_absoluteY*Blt->S1_pitch + SX_absoluteX*(Blt->S1_bpp>>3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				SX_datapix = gam_emul_swap_32(SX_datapix, bigNotLittleBitmap);
				break;
			case gam_emul_TYPE_RGB565:
			case gam_emul_TYPE_ARGB1555:
			case gam_emul_TYPE_ARGB4444:
			case gam_emul_TYPE_ACLUT88:
				/* get source pixel (16 bpp) */
				SX_addresspix = Blt->S1_ba + SX_absoluteY*Blt->S1_pitch + SX_absoluteX*(Blt->S1_bpp>>3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				SX_datapix = gam_emul_swap_16(SX_datapix, bigNotLittleBitmap);
				SX_datapix &= 0xFFFF;
				break;
			case gam_emul_TYPE_A8:
			case gam_emul_TYPE_BYTE:
			case gam_emul_TYPE_CLUT8:
			case gam_emul_TYPE_ACLUT44:
				/* get source pixel (8 bpp) */
				SX_addresspix = Blt->S1_ba + SX_absoluteY*Blt->S1_pitch + SX_absoluteX*(Blt->S1_bpp>>3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				SX_datapix &= 0xFF;
				break;
			case gam_emul_TYPE_CLUT4:
                /* get source pixel (4 bpp)  */
                SX_addresspix = (Blt->S1_ba + SX_absoluteY*Blt->S1_pitch) + ((SX_absoluteX*Blt->S1_bpp) >> 3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				if (Blt->S1_subformat == 1)
				{
					if ((SX_absoluteX & 1) == 0)
						SX_datapix >>= 4;
					SX_datapix &= 0xF;
				}
				else
				{
					if ((SX_absoluteX & 1) == 1)
						SX_datapix >>= 4;
					SX_datapix &= 0xF;
				}
				break;
			case gam_emul_TYPE_CLUT2:
                /* get source pixel (2 bpp)  */
                SX_addresspix = (Blt->S1_ba + SX_absoluteY*Blt->S1_pitch)  + ((SX_absoluteX*Blt->S1_bpp) >> 3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				if (Blt->S1_subformat == 1)
				{
					if ((SX_absoluteX & 3) == 0)
						SX_datapix >>= 6;
					if ((SX_absoluteX & 3) == 1)
						SX_datapix >>= 4;
					if ((SX_absoluteX & 3) == 2)
						SX_datapix >>= 2;
					SX_datapix &= 0x3;
				}
				else
				{
					if ((SX_absoluteX & 3) == 3)
						SX_datapix >>= 6;
					if ((SX_absoluteX & 3) == 2)
						SX_datapix >>= 4;
					if ((SX_absoluteX & 3) == 1)
						SX_datapix >>= 2;
					SX_datapix &= 0x3;
				}
				break;
			case gam_emul_TYPE_CLUT1:
			case gam_emul_TYPE_A1:
                /* get source pixel (1 bpp)  */
                SX_addresspix = (Blt->S1_ba + SX_absoluteY*Blt->S1_pitch) + ((SX_absoluteX*Blt->S1_bpp) >> 3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				if (Blt->S1_subformat == 1)
				{
					if ((SX_absoluteX & 7) == 0)
						SX_datapix >>= 7;
					if ((SX_absoluteX & 7) == 1)
						SX_datapix >>= 6;
					if ((SX_absoluteX & 7) == 2)
						SX_datapix >>= 5;
					if ((SX_absoluteX & 7) == 3)
						SX_datapix >>= 4;
					if ((SX_absoluteX & 7) == 4)
						SX_datapix >>= 3;
					if ((SX_absoluteX & 7) == 5)
						SX_datapix >>= 2;
					if ((SX_absoluteX & 7) == 6)
						SX_datapix >>= 1;
					SX_datapix &= 0x1;
				}
				else
				{
					if ((SX_absoluteX & 7) == 7)
						SX_datapix >>= 7;
					if ((SX_absoluteX & 7) == 6)
						SX_datapix >>= 6;
					if ((SX_absoluteX & 7) == 5)
						SX_datapix >>= 5;
					if ((SX_absoluteX & 7) == 4)
						SX_datapix >>= 4;
					if ((SX_absoluteX & 7) == 3)
						SX_datapix >>= 3;
					if ((SX_absoluteX & 7) == 2)
						SX_datapix >>= 2;
					if ((SX_absoluteX & 7) == 1)
						SX_datapix >>= 1;
					SX_datapix &= 0x1;
				}
				break;
			default:
				SX_datapix = 0;
				break;
		}
	}
	else			/* assume SRC2 */
	{
		bigNotLittleBitmap = Blt->S2_bigNotLittle;

		/* in XYL Standard, bits [24,25,26,27,29] BLT_S2TY have no meaning*/
		if ((Blt->XYLModeEnable == 1) && (Blt->XYL_mode == 0)) /* XYL standard */
		{
				SX_absoluteX =  x - Blt->S2_x ;		/* Given parameter = x left */
				SX_absoluteY =  y - Blt->S2_y ;		/* Given parameter = y top */
		}
		else
		{
			if(((Blt->S2_format == gam_emul_TYPE_YCbCr420MB) || (Blt->S2_format == gam_emul_TYPE_YCbCrMBHDpip))
				&& (((Blt->ALU_rop == 7) && (Blt->ALU_opcode == 1)) || (Blt->ALU_opcode == 6))) /* concatenation S1/S2 or operation OR */
			{
				/* CHROMA : (S2) */
				if (Blt->S2_hscan == 0)
					SX_absoluteX = (x + 2*Blt->S2_x);		/* Given parameter = x left */
				else
					SX_absoluteX = (2*Blt->S2_x - x);		/* Given parameter = x right */
			}
			else
			{
				/* CHROMA : (S2) */
				if (Blt->S2_hscan == 0)
					SX_absoluteX = (x + Blt->S2_x);		/* Given parameter = x left */
				else
					SX_absoluteX = (Blt->S2_x - x);		/* Given parameter = x right */
			}
			if (Blt->S2_vscan == 0)
				SX_absoluteY = (y + Blt->S2_y);		/* Given parameter = y top */
			else
				SX_absoluteY = (Blt->S2_y - y);		/* Given parameter = y bottom */


		}
		switch (Blt->S2_format)
		{
			case gam_emul_TYPE_YCbCr422R:
				/* get source pixel (24 bpp or 8 bpp, average 16 bpp) */
				SX_addresspix = Blt->S2_ba + SX_absoluteY*Blt->S2_pitch + SX_absoluteX*(Blt->S2_bpp>>3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				if ((SX_absoluteX & 1) == 0)  /* Absolute x location is even */
				{
					SX_datapix &= 0xFFFFFF;			/* We point a 24 bpp pixel, already right-justified */
					SX_datapix = gam_emul_swap_24(SX_datapix, bigNotLittleBitmap);
				}
				else													/* Absolute x location is odd */
					SX_datapix = (SX_datapix>>8) & 0xFF;	/* We point a 8 bpp pixel, that we right-justify */
				break;
			case gam_emul_TYPE_YCbCr888:
			case gam_emul_TYPE_RGB888:
			case gam_emul_TYPE_ARGB8565:
				/* get source pixel (24 bpp) */
				SX_addresspix = Blt->S2_ba + SX_absoluteY*Blt->S2_pitch + SX_absoluteX*(Blt->S2_bpp>>3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				SX_datapix = gam_emul_swap_24(SX_datapix, bigNotLittleBitmap);
				SX_datapix &= 0xFFFFFF;
				break;
			case gam_emul_TYPE_ARGB8888:
			case gam_emul_TYPE_AYCbCr8888:
				/* get source pixel (32 bpp) */
				SX_addresspix = Blt->S2_ba + SX_absoluteY*Blt->S2_pitch + SX_absoluteX*(Blt->S2_bpp>>3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				SX_datapix = gam_emul_swap_32(SX_datapix, bigNotLittleBitmap);
				break;
			case gam_emul_TYPE_RGB565:
			case gam_emul_TYPE_ARGB1555:
			case gam_emul_TYPE_ARGB4444:
			case gam_emul_TYPE_ACLUT88:
				/* get source pixel (16 bpp) */
				SX_addresspix = Blt->S2_ba + SX_absoluteY*Blt->S2_pitch + SX_absoluteX*(Blt->S2_bpp>>3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				SX_datapix = gam_emul_swap_16(SX_datapix, bigNotLittleBitmap);
				SX_datapix &= 0xFFFF;
				break;
			case gam_emul_TYPE_A8:
			case gam_emul_TYPE_BYTE:
			case gam_emul_TYPE_CLUT8:
			case gam_emul_TYPE_ACLUT44:
				/* get source pixel (8 bpp) */
				SX_addresspix = Blt->S2_ba + SX_absoluteY*Blt->S2_pitch + SX_absoluteX*(Blt->S2_bpp>>3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				SX_datapix &= 0xFF;
				break;
			case gam_emul_TYPE_CLUT4:
                /* get source pixel (4 bpp) */
                SX_addresspix = (Blt->S2_ba + SX_absoluteY*Blt->S2_pitch) + ((SX_absoluteX*Blt->S2_bpp) >> 3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				if (Blt->S2_subformat == 1)
				{
					if ((SX_absoluteX & 1) == 0)
						SX_datapix >>= 4;
					SX_datapix &= 0xF;
				}
				else
				{
					if ((SX_absoluteX & 1) == 1)
						SX_datapix >>= 4;
					SX_datapix &= 0xF;
				}
				break;
			case gam_emul_TYPE_CLUT2:
                /* get source pixel (2 bpp) */
                SX_addresspix = (Blt->S2_ba + SX_absoluteY*Blt->S2_pitch) + ((SX_absoluteX*Blt->S2_bpp) >> 3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				if (Blt->S2_subformat == 1)
				{
					if ((SX_absoluteX & 3) == 0)
						SX_datapix >>= 6;
					if ((SX_absoluteX & 3) == 1)
						SX_datapix >>= 4;
					if ((SX_absoluteX & 3) == 2)
						SX_datapix >>= 2;
					SX_datapix &= 0x3;
				}
				else
				{
					if ((SX_absoluteX & 3) == 3)
						SX_datapix >>= 6;
					if ((SX_absoluteX & 3) == 2)
						SX_datapix >>= 4;
					if ((SX_absoluteX & 3) == 1)
						SX_datapix >>= 2;
					SX_datapix &= 0x3;
				}
				break;
			case gam_emul_TYPE_A1:
			case gam_emul_TYPE_CLUT1:
                /* get source pixel (1 bpp)  */
                SX_addresspix = (Blt->S2_ba + SX_absoluteY*Blt->S2_pitch) + ((SX_absoluteX*Blt->S2_bpp) >> 3);
                SX_addresspix = gam_emul_CheckAddressRange(SX_addresspix,gam_emul_BLTMemSize);
                SX_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+SX_addresspix));
				if (Blt->S2_subformat == 1)
				{
					if ((SX_absoluteX & 7) == 0)
						SX_datapix >>= 7;
					if ((SX_absoluteX & 7) == 1)
						SX_datapix >>= 6;
					if ((SX_absoluteX & 7) == 2)
						SX_datapix >>= 5;
					if ((SX_absoluteX & 7) == 3)
						SX_datapix >>= 4;
					if ((SX_absoluteX & 7) == 4)
						SX_datapix >>= 3;
					if ((SX_absoluteX & 7) == 5)
						SX_datapix >>= 2;
					if ((SX_absoluteX & 7) == 6)
						SX_datapix >>= 1;
					SX_datapix &= 0x1;
				}
				else
				{
					if ((SX_absoluteX & 7) == 7)
						SX_datapix >>= 7;
					if ((SX_absoluteX & 7) == 6)
						SX_datapix >>= 6;
					if ((SX_absoluteX & 7) == 5)
						SX_datapix >>= 5;
					if ((SX_absoluteX & 7) == 4)
						SX_datapix >>= 4;
					if ((SX_absoluteX & 7) == 3)
						SX_datapix >>= 3;
					if ((SX_absoluteX & 7) == 2)
						SX_datapix >>= 2;
					if ((SX_absoluteX & 7) == 1)
						SX_datapix >>= 1;
					SX_datapix &= 0x1;
				}
				break;
			default:
				SX_datapix = 0;
				break;
		}
	}
	return(SX_datapix);
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_WriteBlitterPixel
 * Parameters: x, y,gam_emul_BlitterParameters ,Datapix
 * Return : -
 * Description : to write in target
 * ----------------------------------------------------------------------------------*/

void gam_emul_WriteBlitterPixel(int x,int y,gam_emul_BlitterParameters *Blt,int Datapix)
{
	int T_addresspix,T_datapix;
	int SubBytePosition;
	int T_absoluteX,T_absoluteY;
	int bigNotLittleBitmap;

	bigNotLittleBitmap = Blt->T_bigNotLittle;

	/* in XYL Standard, target XY location is unused and bits [24,25,27] BLT_TTY have no meaning*/
	if ((Blt->XYLModeEnable == 1) && (Blt->XYL_mode == 0)) /* XYL standard */
	{
			T_absoluteX = x ; /* the register Target XY location is unused */
			T_absoluteY = y ;
	}
	else
	{
		/* Let's consider the horizontal scanning direction */
		if (Blt->T_hscan == 0)
			T_absoluteX = (x + Blt->T_x);		/* Given parameter = x left */
		else
			T_absoluteX = (Blt->T_x - x);		/* Given parameter = x right */
		if (Blt->T_vscan == 0)
			T_absoluteY = (y + Blt->T_y);		/* Given parameter = y top */
		else
			T_absoluteY = (Blt->T_y - y);		/* Given parameter = y bottom */
	}
	switch (Blt->T_format)
	{
		case gam_emul_TYPE_YCbCr422R:
			T_addresspix = Blt->T_ba + T_absoluteY*Blt->T_pitch + T_absoluteX*(Blt->T_bpp>>3);
            T_addresspix = gam_emul_CheckAddressRange(T_addresspix,gam_emul_BLTMemSize);
			if ((T_absoluteX & 1) == 0)		/* 24 bits to write */
			{
				Datapix = gam_emul_swap_24(Datapix, bigNotLittleBitmap);
                gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)Datapix);
                gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix+1), (unsigned char)(Datapix >> 8));
                gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix+2), (unsigned char)(Datapix >> 16));
			}
			else
			{
				/* 8 bits to write */
				switch (Blt->BLT_instruction)
				{
						case 0x07:		/* Scr1 Direct Fill mode */
						case 0x0F:
						case 0x17:
                        case 0x1F:  gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix+1), (unsigned char)(Datapix >> 24));
									break;
                        default :   gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix+1), (unsigned char)Datapix);
									break;
				}
			}
			break;
		case gam_emul_TYPE_YCbCr888:
		case gam_emul_TYPE_RGB888:
		case gam_emul_TYPE_ARGB8565:
			/* write destination pixel (24 bpp) */
			T_addresspix = Blt->T_ba + T_absoluteY*Blt->T_pitch + T_absoluteX*(Blt->T_bpp>>3);
            T_addresspix = gam_emul_CheckAddressRange(T_addresspix,gam_emul_BLTMemSize);
			Datapix = gam_emul_swap_24(Datapix, bigNotLittleBitmap);
            gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)Datapix);
            gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix+1), (unsigned char)(Datapix >> 8));
            gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix+2), (unsigned char)(Datapix >> 16));
			break;
		case gam_emul_TYPE_AYCbCr8888:
		case gam_emul_TYPE_ARGB8888:
			/* write destination pixel (32 bpp) */
			T_addresspix = Blt->T_ba + T_absoluteY*Blt->T_pitch + T_absoluteX*(Blt->T_bpp>>3);
            T_addresspix = gam_emul_CheckAddressRange(T_addresspix,gam_emul_BLTMemSize);
			Datapix = gam_emul_swap_32(Datapix, bigNotLittleBitmap);
            gam_emul_writeInt((int*)(gam_emul_BLTMemory+T_addresspix), Datapix);
			break;
		case gam_emul_TYPE_RGB565:
		case gam_emul_TYPE_ARGB1555:
		case gam_emul_TYPE_ARGB4444:
		case gam_emul_TYPE_ACLUT88:
			/* write destination pixel (16 bpp) */
			T_addresspix = Blt->T_ba + T_absoluteY*Blt->T_pitch + T_absoluteX*(Blt->T_bpp>>3);
            T_addresspix = gam_emul_CheckAddressRange(T_addresspix,gam_emul_BLTMemSize);
			Datapix = gam_emul_swap_16(Datapix, bigNotLittleBitmap);
            gam_emul_writeShort((short*)(gam_emul_BLTMemory+T_addresspix), (short)Datapix);
			break;
		case gam_emul_TYPE_A8:
		case gam_emul_TYPE_BYTE:
		case gam_emul_TYPE_CLUT8:
		case gam_emul_TYPE_ACLUT44:
			/* write destination pixel (8 bpp) */
			T_addresspix = Blt->T_ba + T_absoluteY*Blt->T_pitch + T_absoluteX*(Blt->T_bpp>>3);
            T_addresspix = gam_emul_CheckAddressRange(T_addresspix,gam_emul_BLTMemSize);
            gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)Datapix);
			break;
		case gam_emul_TYPE_CLUT4:
			/* write destination pixel (4 bpp, so read modify write) */
            T_addresspix = (Blt->T_ba + T_absoluteY*Blt->T_pitch) + ((T_absoluteX*Blt->T_bpp) >> 3);
            T_addresspix = gam_emul_CheckAddressRange(T_addresspix,gam_emul_BLTMemSize);
            T_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+T_addresspix));
			SubBytePosition = (T_absoluteX & 1);
			if (Blt->T_subformat == 0)
				SubBytePosition = 1 - SubBytePosition;
			switch (SubBytePosition)
			{
				case 0:
					T_datapix &= 0xF;
					Datapix = (((Datapix << 4) & 0xF0) | T_datapix);
					break;
				case 1:
					T_datapix &= 0xF0;
					Datapix = ((Datapix & 0x0F) | T_datapix);
					break;
				default:
					break;
			}
            gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)Datapix);
			break;
		case gam_emul_TYPE_CLUT2:
			/* write destination pixel (2 bpp, so read modify write) */
            T_addresspix = (Blt->T_ba + T_absoluteY*Blt->T_pitch) + ((T_absoluteX*Blt->T_bpp) >> 3);
            T_addresspix = gam_emul_CheckAddressRange(T_addresspix,gam_emul_BLTMemSize);
            T_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+T_addresspix));
			SubBytePosition = (T_absoluteX & 3);
			if (Blt->T_subformat == 0)
				SubBytePosition = 3 - SubBytePosition;
			switch (SubBytePosition)
			{
				case 0:
					T_datapix &= 0x3F;
					Datapix = (((Datapix << 6) & 0xC0) | T_datapix);
					break;
				case 1:
					T_datapix &= 0xCF;
					Datapix = (((Datapix << 4) & 0x30) | T_datapix);
					break;
				case 2:
					T_datapix &= 0xF3;
					Datapix = (((Datapix << 2) & 0x0C) | T_datapix);
					break;
				case 3:
					T_datapix &= 0xFC;
					Datapix = ((Datapix & 0x03) | T_datapix);
					break;
				default:
					break;
			}
            gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)Datapix);
			break;
		case gam_emul_TYPE_A1:
		case gam_emul_TYPE_CLUT1:
			/* write destination pixel (1 bpp, so read modify write) */
            T_addresspix = (Blt->T_ba + T_absoluteY*Blt->T_pitch) + ((T_absoluteX*Blt->T_bpp) >> 3);
            T_addresspix = gam_emul_CheckAddressRange(T_addresspix,gam_emul_BLTMemSize);
            T_datapix = gam_emul_readInt((int*)(gam_emul_BLTMemory+T_addresspix));
			SubBytePosition = (T_absoluteX & 7);
			if (Blt->T_subformat == 0)
				SubBytePosition = 7 - SubBytePosition;
			switch (SubBytePosition)
			{
				case 0:
					T_datapix &= 0x7F;
					Datapix = (((Datapix << 7) & 0x80) | T_datapix);
					break;
				case 1:
					T_datapix &= 0xBF;
					Datapix = (((Datapix << 6) & 0x40)  | T_datapix);
					break;
				case 2:
					T_datapix &= 0xDF;
					Datapix = (((Datapix << 5) & 0x20) | T_datapix);
					break;
				case 3:
					T_datapix &= 0xEF;
					Datapix = (((Datapix << 4) & 0x10) | T_datapix);
					break;
				case 4:
					T_datapix &= 0xF7;
					Datapix = (((Datapix << 3) & 0x08) | T_datapix);
					break;
				case 5:
					T_datapix &= 0xFB;
					Datapix = (((Datapix << 2) & 0x04) | T_datapix);
					break;
				case 6:
					T_datapix &= 0xFD;
					Datapix = (((Datapix << 1) & 0x02)| T_datapix);
					break;
				case 7:
					T_datapix &= 0xFE;
					Datapix = ((Datapix & 0x01) | T_datapix);
					break;
				default:
					break;
			}
            gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)Datapix);
			break;
		default:
			break;
	}
}
/*-----------------------------------------------------------------------------------
 * Function : gam_emul_FetchNewSourceLine
 * Parameters: Source,gam_emul_BlitterParameters ,iSXOut,iSaveBuffer,y
 * Return : SXout is formatted ou and SaveBuffer is unformatted out
 * Description : read the line y, clip alpha, convert the line to the internal 32-bit pixel bus
 (bus formatter)
 * ----------------------------------------------------------------------------------*/

void gam_emul_FetchNewSourceLine(int Source, gam_emul_BlitterParameters *Blt, int *SXOut,int *SaveBuffer,int y)
{
	int x;
	int a,r,g,b;
	int InputFormat,InputWidth;
	int RGBExtendMode = 0;
	/*int format = Blt->XYL_format;*/

	if (Source == gam_emul_SRC1)
	{
		/* We read the line */
		InputFormat = Blt->S1_format;
		switch (InputFormat)
		{
			case gam_emul_TYPE_YCbCr422MB:
			case gam_emul_TYPE_YCbCr420MB:
				if ((Blt->ALU_opcode == 0x7)) /* bypass Scr2 */
				{
					if ((Blt->BLT_instruction & 0x7) == 1)		/* fetch from memory */
						for ( x=0 ; x < Blt->S2_width ; x++ )	/* size in S2 for 4:2:0 !!!*/
							*(SXOut+x) = gam_emul_ReadBlitter42XPixel(x,y,Blt,gam_emul_LUMA);
					else										/* disabled or color fill register */
						for ( x=0 ; x < Blt->S2_width ; x++ )
							*(SXOut+x) = Blt->S1_colorfill;
					InputWidth = Blt->S2_width;
				}
				else
				{
					if ((Blt->BLT_instruction & 0x7) == 1)		/* fetch from memory */
						for ( x=0 ; x < Blt->S1_width ; x++ )	/* size in S2 for 4:2:0 !!!*/
							*(SXOut+x) = gam_emul_ReadBlitter42XPixel(x,y,Blt,gam_emul_LUMA);
					else										/* disabled or color fill register */
						for ( x=0 ; x < Blt->S1_width ; x++ )
							*(SXOut+x) = Blt->S1_colorfill;
					InputWidth = Blt->S1_width;

				}
				break;
			case gam_emul_TYPE_YCbCrMBHDpip:
				if ((Blt->ALU_opcode == 0x7)) /* bypass Scr2 */
				{

					if ((Blt->BLT_instruction & 0x7) == 1)		/* fetch from memory */
						for ( x=0 ; x < Blt->S2_width ; x++ )
							*(SXOut+x) = gam_emul_ReadBlitterHDpipPixel(x,y,Blt,gam_emul_LUMA);
					else										/* disabled or color fill register */
						for ( x=0 ; x < Blt->S2_width ; x++ )
							*(SXOut+x) = Blt->S1_colorfill;
					InputWidth = Blt->S2_width;
				}
				else
				{
					if ((Blt->BLT_instruction & 0x7) == 1)		/* fetch from memory */
						for ( x=0 ; x < Blt->S1_width ; x++ )	/* size in S2 for 4:2:0 !!!*/
							*(SXOut+x) = gam_emul_ReadBlitterHDpipPixel(x,y,Blt,gam_emul_LUMA);
					else										/* disabled or color fill register */
						for ( x=0 ; x < Blt->S1_width ; x++ )
							*(SXOut+x) = Blt->S1_colorfill;
					InputWidth = Blt->S1_width;

				}
				break;
			default:
				if ((Blt->BLT_instruction & 0x7) == 1)		/* fetch from memory */
					for ( x=0 ; x < Blt->S1_width ; x++ )
					{
						*(SXOut+x) = gam_emul_ReadBlitterPixel(x,y,Blt,gam_emul_SRC1);
						*(SaveBuffer+x) = *(SXOut+x);		/* for the PlaneMask feature  */
					}
				else										/* disabled or color fill */
					for ( x=0 ; x < Blt->S1_width ; x++ )
					{
						*(SXOut+x) = Blt->S1_colorfill;
						*(SaveBuffer+x) = Blt->S1_colorfill;/* for the PlaneMask feature  */
					}
				InputWidth = Blt->S1_width;
				RGBExtendMode = Blt->S1_RGBExtendMode;
				break;
		}

	}
	else		/* SRC2 */
	{
		InputFormat = Blt->S2_format;
		switch (InputFormat)
		{
			case gam_emul_TYPE_YCbCr420MB:
			case gam_emul_TYPE_YCbCr422MB:
				if ((Blt->BLT_instruction & 0x18) == 8)		/* fetch from memory */
				{
					if(((Blt->ALU_rop == 7) && (Blt->ALU_opcode == 1)) || (Blt->ALU_opcode == 6)) /* concatenation S1/S2 or operation OR */
					{
						for ( x=0 ; x < 2*Blt->S2_width ; x++ )
							*(SXOut+x) = gam_emul_ReadBlitter42XPixel(x,y,Blt,gam_emul_CHROMA);
					}
					else
					{
						for ( x=0 ; x < Blt->S2_width ; x++ )
							*(SXOut+x) = gam_emul_ReadBlitter42XPixel(x,y,Blt,gam_emul_CHROMA);
					}
				}
				else										/* disabled or color fill register */
					for ( x=0 ; x < Blt->S2_width ; x++ )
						*(SXOut+x) = Blt->S2_colorfill;		/* S1 or S2 ?? */
				InputWidth = Blt->S1_width;
				break;
			case gam_emul_TYPE_YCbCrMBHDpip:
				if ((Blt->BLT_instruction & 0x18) == 8)		/* fetch from memory */
				{
					if(((Blt->ALU_rop == 7) && (Blt->ALU_opcode == 1)) || (Blt->ALU_opcode == 6)) /* concatenation S1/S2 or operation OR */
					{
						for ( x=0 ; x < 2*Blt->S2_width ; x++ )
							*(SXOut+x) = gam_emul_ReadBlitterHDpipPixel(x,y,Blt,gam_emul_CHROMA);
					}
					else
					{
						for ( x=0 ; x < Blt->S2_width ; x++ )
							*(SXOut+x) = gam_emul_ReadBlitterHDpipPixel(x,y,Blt,gam_emul_CHROMA);
					}
				}
				else										/* disabled or color fill register */
					for ( x=0 ; x < Blt->S2_width ; x++ )
						*(SXOut+x) = Blt->S2_colorfill;		/* S1 or S2 ?? */
				InputWidth = Blt->S1_width;
				break;
			default:
				InputFormat = Blt->S2_format;
				if ((Blt->BLT_instruction & 0x18) == 8)							/* fetch from memory */
					for ( x=0 ; x < Blt->S2_width ; x++ )
						*(SXOut+x) = gam_emul_ReadBlitterPixel(x,y,Blt,gam_emul_SRC2);
				else															/* disabled or color fill */
					for ( x=0 ; x < Blt->S2_width ; x++ )
						*(SXOut+x) = Blt->S2_colorfill;

				InputWidth = Blt->S2_width;
				RGBExtendMode = Blt->S2_RGBExtendMode;
				break;
		}
	}


	/* We convert the line to the internal 32-bit pixel bus (bus formatter) */
	for ( x=0 ; x < InputWidth ; x++ )
	{
		switch (InputFormat)
		{
			case gam_emul_TYPE_YCbCr422R:
				if ((Blt->BLT_instruction & 0x18) != 8)			/* disabled or color fill */
				{
					if ((((Blt->T_x & 1) == 0) && ((x&1) == 1))
						|| (((Blt->T_x & 1) == 1) && ((x&1) == 0)))
						*(SXOut+x) = (*(SXOut+x) >> 8) & 0xFF;	/* should be a luma-only pixel */
				}
				break;
			case gam_emul_TYPE_RGB888:
			case gam_emul_TYPE_YCbCr888:
				*(SXOut+x) |= 0x80000000;			/* add opaque alpha channel */
				break;
			case gam_emul_TYPE_ARGB8565:
				a = (*(SXOut+x) >> 16) & 0xFF;
				if ( ((Blt->S1_alphaRange == 0x1) && (Source == gam_emul_SRC1))
					|| ( (Blt->S2_alphaRange == 0x1) && (Source == gam_emul_SRC2)))
				{
					a = (a + 1) >>1; /* to have a range between 0...128 */
				}
				r = ((*(SXOut+x) & 0xF800) >> 8) & 0xFF;
				g = ((*(SXOut+x) & 0x07E0) >> 3) & 0xFF;
				b = ((*(SXOut+x) & 0x001F) << 3) & 0xFF;
				if (RGBExtendMode == 0)
				{
					r = r | (r>>5);					/* Report the MSBs on LSBs */
					g = g | (g>>6);
					b = b | (b>>5);
				}
				*(SXOut+x) = (a<<24) | (r<<16) | (g<<8) | b;
				break;
			case gam_emul_TYPE_ARGB8888:
			case gam_emul_TYPE_AYCbCr8888:
				if ( ((Blt->S1_alphaRange == 0x1) && (Source == gam_emul_SRC1))
					|| ( (Blt->S2_alphaRange == 0x1) && (Source == gam_emul_SRC2)))
				{
					a = (*(SXOut+x) >> 24) & 0xFF;
					*(SXOut+x) &= 0xFFFFFF;
					a = (a + 1) >>1; /* to have a range between 0...128 */
					*(SXOut+x) |= (a<<24);
				}
				break;
			case gam_emul_TYPE_RGB565:
				a = 128;											/* add opaque alpha channel */
				r = ((*(SXOut+x) & 0xF800) >> 8) & 0xFF;
				g = ((*(SXOut+x) & 0x07E0) >> 3) & 0xFF;
				b = ((*(SXOut+x) & 0x001F) << 3) & 0xFF;
				if (RGBExtendMode == 0)
				{
					r = r | (r>>5);															/* Report the MSBs on LSBs */
					g = g | (g>>6);
					b = b | (b>>5);
				}
				*(SXOut+x) = (a<<24) | (r<<16) | (g<<8) | b;
				break;
			case gam_emul_TYPE_ARGB1555:
				a = ((*(SXOut+x) & 0x8000) >> 8) & 0xFF;
				r = ((*(SXOut+x) & 0x7C00) >> 7) & 0xFF;
				g = ((*(SXOut+x) & 0x03E0) >> 2) & 0xFF;
				b = ((*(SXOut+x) & 0x001F) << 3) & 0xFF;
				if (RGBExtendMode == 0)
				{
					r = r | (r>>5);															/* Report the MSBs on LSBs */
					g = g | (g>>5);
					b = b | (b>>5);
				}
				*(SXOut+x) = (a<<24) | (r<<16) | (g<<8) | b;
				break;
			case gam_emul_TYPE_ARGB4444:
				a = ((*(SXOut+x) & 0xF000) >> 9) & 0xFF;
				if (a==0x78)
					a = 128;
				else
					if (a!=0)
						a += 4;
				r = ((*(SXOut+x) & 0x0F00) >> 4) & 0xFF;
				g = ((*(SXOut+x) & 0x00F0) >> 0) & 0xFF;
				b = ((*(SXOut+x) & 0x000F) << 4) & 0xFF;
				if (RGBExtendMode == 0)
				{
					r = r | (r>>4);															/* Report the MSBs on LSBs */
					g = g | (g>>4);
					b = b | (b>>4);
				}
				*(SXOut+x) = (a<<24) | (r<<16) | (g<<8) | b;
				break;
			case gam_emul_TYPE_ACLUT88:
				a = ((*(SXOut+x) & 0xFF00) >> 8) & 0xFF;
				if ( ((Blt->S1_alphaRange == 0x1) && (Source == gam_emul_SRC1))
					|| ( (Blt->S2_alphaRange == 0x1) && (Source == gam_emul_SRC2)))
				{
					a = (a + 1) >>1; /* to have a range between 0...128 */
				}
				*(SXOut+x) &= 0xFF;
				*(SXOut+x) |= (a<<24);
				break;
			case gam_emul_TYPE_ACLUT44:
				a = ((*(SXOut+x) & 0xF0) >> 1) & 0xFF; /* multiplied by 8 - if alpha 0 -> 0
													   if alpha = 15 -> 128 else alpha is included between 8 + 4 (12) at 14*8 +4 (116)*/
				if (a==0x78) /* 15 is opaque then alpha = 128  */
					a = 128;
				else
					if (a!=0) /* a = 0 is unaltered */
						a += 4;
				*(SXOut+x) &= 0xF;
				*(SXOut+x) |= (a<<24);
				break;
			case gam_emul_TYPE_A8:
				a = *(SXOut+x) & 0xFF;
				if ( ((Blt->S1_alphaRange == 0x1) && (Source == gam_emul_SRC1))
					|| ( (Blt->S2_alphaRange == 0x1) && (Source == gam_emul_SRC2)))
				{
					a = (a + 1) >>1; /* to have a range between 0...128 */
				}
				*(SXOut+x) = (a<<24);
				break;
			case gam_emul_TYPE_A1:
				a = *(SXOut+x) & 0x1;
				*(SXOut+x) = (a<<31);
				break;
			case gam_emul_TYPE_CLUT8:										/* ok */
			case gam_emul_TYPE_CLUT4:
			case gam_emul_TYPE_CLUT2:
			case gam_emul_TYPE_CLUT1:
			case gam_emul_TYPE_BYTE:
				break;
			default:
				break;
		}
	}
	/* clip alpha to 128, except if 255 */
	for ( x=0 ; x < InputWidth ; x++ )
	{
		switch (InputFormat)
		{
			case gam_emul_TYPE_YCbCr422R:
			case gam_emul_TYPE_YCbCr422MB:
			case gam_emul_TYPE_YCbCr420MB:
			case gam_emul_TYPE_YCbCrMBHDpip:
				break;
			default:
				a = (*(SXOut+x) >> 24) & 0xFF;
				*(SXOut+x) &= 0xFFFFFF;
				if ((a > 128) && (a < 255))
					a = 128;
				*(SXOut+x) |= (a<<24);
				break;
		}
	}


}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_SamplingPatternConversion42XTo444
 * Parameters: gam_emul_BlitterParameters ,S1In,S2In,S2Out,y
 * Return : S2Out in format 4:4:4 or S2In (copy)
 * Description : convert 4:2:X to 4:4:4 or copy input to output directly if format is not MB
 * ----------------------------------------------------------------------------------*/

void gam_emul_SamplingPatternConversion42XTo444(gam_emul_BlitterParameters *Blt,int *S1In,int *S2In,int *S2Out,int y)
{
	int x;
	int luma,cb,cr,cb1,cr1;
	int tmp;

	switch (Blt->S2_format)
	{
		case gam_emul_TYPE_YCbCr422R:
			for ( x=0 ; x < Blt->S2_width ; x++ )
			{
				if (((x & 1)^(Blt->S2_x & 1)) == 1)	/* this is a Y-only pixel */
				{
					if (x == 0)												/* first pixel / Y-only	 */
					{
						if ((Blt->S2_chroma_extend == 1) && (Blt->S2_hscan == 0))		/* Forward scan */
						{
							tmp = gam_emul_ReadBlitterPixel(x-1,y,Blt,gam_emul_SRC2);	/* we read the "previous" pixel */
							cb  =  tmp & 0xFF;
							cr  = (tmp >> 16) & 0xFF;
							if (Blt->S2_width != 1)
							{
								cb1 = (*(S2In+x+1) & 0xFF);
								cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
							}
							else
							{
								cb1 = cb;
								cr1 = cr;
							}
						}
						else														/* all pix except first / Y-only */
						{
							cb = cb1 = (*(S2In+x+1) & 0xFF);
							cr = cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
						}
					}
					else
					{
						if (x == (Blt->S2_width-1))			/* last pixel / Y-only */
						{
							if ((Blt->S2_chroma_extend == 1) && (Blt->S2_hscan == 1))		/* Reverse scan */
							{
								tmp = gam_emul_ReadBlitterPixel(x+1,y,Blt,gam_emul_SRC2);		/* we read the "next" pixel */
								cb1  =  tmp & 0xFF;
								cr1  = (tmp >> 16) & 0xFF;
								cb = (*(S2In+x-1) & 0xFF);
								cr = ((*(S2In+x-1) >> 16) & 0xFF);
							}
							else
							{
								cb = cb1 = (*(S2In+x-1) & 0xFF);
								cr = cr1 = ((*(S2In+x-1) >> 16) & 0xFF);
							}
						}
						else
						{
							cb  = (*(S2In+x-1) & 0xFF);
							cr  = ((*(S2In+x-1) >> 16) & 0xFF);
							cb1 = (*(S2In+x+1) & 0xFF);
							cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
						}
					}
					luma = *(S2In+x);
					cb = (cb+cb1+1)/2;
					cr = (cr+cr1+1)/2;
					*(S2Out+x) = 0x80000000 | cb | (luma<<8) | (cr<<16);	/* YCbCr pixel + opaque alpha channel added */
				}
				else
					*(S2Out+x) = 0x80000000 | *(S2In+x);	/* YCbCr pixel + opaque alpha channel added */
			}
			break;
	case gam_emul_TYPE_YCbCr422MB:
			/* We make a complete CrYCb bus from S1/S2 input buses */
			for ( x=0 ; x < Blt->S2_width ; x++ )
				*(S2In+x) = *(S1In+x) | *(S2In+x);

			for ( x=0 ; x < Blt->S2_width ; x++ )
				{
					if (((x & 1)^(Blt->S2_x & 1)) == 1)	/* this is a Y-only pixel */
					{
						if (x == 0)
						{
							if ((Blt->S2_chroma_extend == 1) && (Blt->S2_hscan == 0))		/* Forward scan */
							{
								tmp = gam_emul_ReadBlitter42XPixel(x-1,y,Blt,gam_emul_CHROMA);	/* we read the "previous" pixel */
								cb  =  tmp & 0xFF;
								cr  = (tmp >> 16) & 0xFF;
								if (Blt->S2_width != 1)
								{
									cb1 = (*(S2In+x+1) & 0xFF);
									cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
								}
								else
								{
									cb1 = cb;
									cr1 = cr;
								}
							}
							else
							{
								cb = cb1 = (*(S2In+x+1) & 0xFF);
								cr = cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
							}
						}
						else
						{
							if (x == (Blt->S2_width-1))
							{
								if ((Blt->S2_chroma_extend == 1) && (Blt->S2_hscan == 1))		/* Reverse scan */
								{
									tmp = gam_emul_ReadBlitter42XPixel(x+1,y,Blt,gam_emul_CHROMA);   /* we read the "next" pixel */
									cb1  =  tmp & 0xFF;
									cr1  = (tmp >> 16) & 0xFF;
									cb = (*(S2In+x-1) & 0xFF);
									cr = ((*(S2In+x-1) >> 16) & 0xFF);
								}
								else
								{
									cb = cb1 = (*(S2In+x-1) & 0xFF);
									cr = cr1 = ((*(S2In+x-1) >> 16) & 0xFF);
								}
							}
							else
							{
								cb  = (*(S2In+x-1) & 0xFF);
								cr  = ((*(S2In+x-1) >> 16) & 0xFF);
								cb1 = (*(S2In+x+1) & 0xFF);
								cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
							}
						}
						luma = (*(S2In+x) >> 8) & 0xFF;
						cb = (cb+cb1+1)/2;
						cr = (cr+cr1+1)/2;
						*(S2Out+x) = 0x80000000 | cb | (luma<<8) | (cr<<16);	/* YCbCr pixel + opaque alpha channel added */
					}
					else
						*(S2Out+x) = 0x80000000 | *(S2In+x);	/* YCbCr pixel + opaque alpha channel added */
				}
				break;

		case gam_emul_TYPE_YCbCr420MB:
			if (Blt->ALU_opcode == 0x7) /* bypass SCR2 */
			{
				/* We make a complete CrYCb bus from S1/S2 input buses, if ChromalineRepeat is ON */
					if (Blt->S2_chromalinerepeat == 1)
						for ( x=0 ; x < Blt->S2_width ; x++ )
							*(S2In+x) = *(S1In+x) | *(S2In+x);

				for ( x=0 ; x < Blt->S2_width ; x++ )
				{
					if (((x & 1)^(Blt->S2_x & 1)) == 1)	/* this is a Y-only pixel */
					{
						if (x == 0)
						{
							if ((Blt->S2_chroma_extend == 1) && (Blt->S2_hscan == 0))		/* Forward scan */
							{
								tmp = gam_emul_ReadBlitter42XPixel(x-1,y,Blt,gam_emul_CHROMA);	/* we read the "previous" pixel */
								cb  =  tmp & 0xFF;
								cr  = (tmp >> 16) & 0xFF;
								if (Blt->S2_width != 1)
								{
									cb1 = (*(S2In+x+1) & 0xFF);
									cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
								}
								else
								{
									cb1 = cb;
									cr1 = cr;
								}
							}
							else
							{
								cb = cb1 = (*(S2In+x+1) & 0xFF);
								cr = cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
							}
						}
						else
						{
							if (x == (Blt->S2_width-1))
							{
								if ((Blt->S2_chroma_extend == 1) && (Blt->S2_hscan == 1))		/* Reverse scan */
								{
									tmp = gam_emul_ReadBlitter42XPixel(x+1,y,Blt,gam_emul_CHROMA);   /* we read the "next" pixel */
									cb1  =  tmp & 0xFF;
									cr1  = (tmp >> 16) & 0xFF;
									cb = (*(S2In+x-1) & 0xFF);
									cr = ((*(S2In+x-1) >> 16) & 0xFF);
								}
								else
								{
									cb = cb1 = (*(S2In+x-1) & 0xFF);
									cr = cr1 = ((*(S2In+x-1) >> 16) & 0xFF);
								}
							}
							else
							{
								cb  = (*(S2In+x-1) & 0xFF);
								cr  = ((*(S2In+x-1) >> 16) & 0xFF);
								cb1 = (*(S2In+x+1) & 0xFF);
								cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
							}
						}
						if (Blt->S2_chromalinerepeat == 1)
							luma = (*(S2In+x) >> 8) & 0xFF;
						else
							luma = 0;
						cb = (cb+cb1+1)/2;
						cr = (cr+cr1+1)/2;
						*(S2Out+x) = 0x80000000 | cb | (luma<<8) | (cr<<16);	/* YCbCr pixel + opaque alpha channel added */
					}
					else
						*(S2Out+x) = 0x80000000 | *(S2In+x);	/* YCbCr pixel + opaque alpha channel added */
				}
			}
			else
			{
				for ( x=0 ; x < Blt->S2_width ; x++ ) /* copy input to output directly if format is not MB */
				*(S2Out+x) = *(S2In+x);
			}
			break;

		case gam_emul_TYPE_YCbCrMBHDpip:
			if (Blt->ALU_opcode == 0x7)
			{
				/* We make a complete CrYCb bus from S1/S2 input buses, if ChromalineRepeat is ON */
				if (Blt->S2_chromalinerepeat == 1)
					for ( x=0 ; x < Blt->S2_width ; x++ )
						*(S2In+x) = *(S1In+x) | *(S2In+x);

				for ( x=0 ; x < Blt->S2_width ; x++ )
				{
					if (((x & 1)^(Blt->S2_x & 1)) == 1)	/* this is a Y-only pixel */
					{
						if (x == 0)
						{
							if ((Blt->S2_chroma_extend == 1) && (Blt->S2_hscan == 0))		/* Forward scan */
							{
								tmp = gam_emul_ReadBlitterHDpipPixel(x-1,y,Blt,gam_emul_CHROMA);	/* we read the "previous" pixel */
								cb  =  tmp & 0xFF;
								cr  = (tmp >> 16) & 0xFF;
								if (Blt->S2_width != 1)
								{
									cb1 = (*(S2In+x+1) & 0xFF);
									cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
								}
								else
								{
									cb1 = cb;
									cr1 = cr;
								}
							}
							else
							{
								cb = cb1 = (*(S2In+x+1) & 0xFF);
								cr = cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
							}
						}
						else
						{
							if (x == (Blt->S2_width-1))
							{
								if ((Blt->S2_chroma_extend == 1) && (Blt->S2_hscan == 1))		/* Reverse scan */
								{
									tmp = gam_emul_ReadBlitterHDpipPixel(x+1,y,Blt,gam_emul_CHROMA);   /* we read the "next" pixel */
									cb1  =  tmp & 0xFF;
									cr1  = (tmp >> 16) & 0xFF;
									cb = (*(S2In+x-1) & 0xFF);
									cr = ((*(S2In+x-1) >> 16) & 0xFF);
								}
								else
								{
									cb = cb1 = (*(S2In+x-1) & 0xFF);
									cr = cr1 = ((*(S2In+x-1) >> 16) & 0xFF);
								}
							}
							else
							{
								cb  = (*(S2In+x-1) & 0xFF);
								cr  = ((*(S2In+x-1) >> 16) & 0xFF);
								cb1 = (*(S2In+x+1) & 0xFF);
								cr1 = ((*(S2In+x+1) >> 16) & 0xFF);
							}
						}
						if (Blt->S2_chromalinerepeat == 1)
							luma = (*(S2In+x) >> 8) & 0xFF;
						else
							luma = 0;
						cb = (cb+cb1+1)/2;
						cr = (cr+cr1+1)/2;
						*(S2Out+x) = 0x80000000 | cb | (luma<<8) | (cr<<16);	/* YCbCr pixel + opaque alpha channel added */
					}
					else
						*(S2Out+x) = 0x80000000 | *(S2In+x);	/* YCbCr pixel + opaque alpha channel added */
				}
			}
			else
			{
				for ( x=0 ; x < Blt->S2_width ; x++ ) /* copy input to output directly if format is not MB */
				*(S2Out+x) = *(S2In+x);
			}
			break;
		default:
			for ( x=0 ; x < Blt->S2_width ; x++ ) /* copy input to output directly if format is not MB */
				*(S2Out+x) = *(S2In+x);
			break;
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ColorSpaceConverter
 * Parameters: gam_emul_BlitterParameters, In,Out,InputOutput
 * Return : out
 * Description : YCbCR <> RGB ( InputOutput : in function of conversion YCbCR <> RGB
 location) or copy input to output directly if color space converter is disabled
 * ----------------------------------------------------------------------------------*/

void gam_emul_ColorSpaceConverter(gam_emul_BlitterParameters *Blt,int *In,int *Out,int InputOutput)
{
	int x;
	int a,r,g,b,luma,cb,cr;

	if (InputOutput == gam_emul_INPUT_LOCATION) /* case conversion YCbCR <> RGB in input */
	{
		if (Blt->EnaInputColorSpaceConverter == 1)
		{
			if (Blt->CSC_input_direction == 0)				/* 0 : Input -> YCbCr to RGB */
			{
				for ( x=0 ; x < Blt->S2_width ; x++ )
				{
					a = (*(In+x) >> 24) & 0xFF;
					cr = (*(In+x) >> 16) & 0xFF;
					luma = (*(In+x) >> 8) & 0xFF;
					cb = (*(In+x) >> 0) & 0xFF;
					gam_emul_ConvertYCbCr2RGB (&luma,&cb,&cr,&r,&g,&b,
														Blt->CSC_input_colorimetry,8,8,Blt->CSC_input_chroma_format, Blt->CSC_matrix_in_ngfx_vid);
					*(Out+x) = (a<<24) | (r<<16) | (g<<8) | b;
				}
			}
			else																			/* 1 : Input -> RGB to YCbCr */
			{
				for ( x=0 ; x < Blt->S2_width ; x++ )
				{
					a = (*(In+x) >> 24) & 0xFF;
					r = (*(In+x) >> 16) & 0xFF;
					g = (*(In+x) >> 8) & 0xFF;
					b = (*(In+x) >> 0) & 0xFF;
					gam_emul_ConvertRGB2YCbCr (&r,&g,&b,&luma,&cb,&cr,
														Blt->CSC_input_colorimetry,Blt->CSC_input_chroma_format,8, Blt->CSC_matrix_in_ngfx_vid);
					*(Out+x) = (a<<24) | (cr<<16) | (luma<<8) | cb;
				}
			}
		}
		else
			for ( x=0 ; x < Blt->S2_width ; x++ )		/* Disabled -> BYPASS */
				*(Out+x) = *(In+x);
	}

	if (InputOutput == gam_emul_OUTPUT_LOCATION)
	{
		if (Blt->EnaOutputColorSpaceConverter == 1)
		{
			if (Blt->CSC_input_direction == 1)			/* 1 : Output -> YCbCr to RGB */
			{
				for ( x=0 ; x < Blt->T_width ; x++ )

				{
					if (x==0)
						x = x;

					a = (*(In+x) >> 24) & 0xFF;
					cr = (*(In+x) >> 16) & 0xFF;
					luma = (*(In+x) >> 8) & 0xFF;
					cb = (*(In+x) >> 0) & 0xFF;
					gam_emul_ConvertYCbCr2RGB (&luma,&cb,&cr,&r,&g,&b,
														Blt->CSC_output_colorimetry,8,8,Blt->CSC_output_chroma_format, Blt->CSC_matrix_out_ngfx_vid);
					*(Out+x) = (a<<24) | (r<<16) | (g<<8) | b;
				}
			}
			else										/* 0 : Output -> RGB to YCbCr */
			{
				for ( x=0 ; x < Blt->T_width ; x++ )
				{
					a = (*(In+x) >> 24) & 0xFF;
					r = (*(In+x) >> 16) & 0xFF;
					g = (*(In+x) >> 8) & 0xFF;
					b = (*(In+x) >> 0) & 0xFF;
					gam_emul_ConvertRGB2YCbCr (&r,&g,&b,&luma,&cb,&cr,
														Blt->CSC_output_colorimetry,Blt->CSC_output_chroma_format,8, Blt->CSC_matrix_out_ngfx_vid);
					*(Out+x) = (a<<24) | (cr<<16) | (luma<<8) | cb;
				}
			}
		}
		else
			for ( x=0 ; x < Blt->T_width ; x++ )		/* Disabled -> BYPASS */
				*(Out+x) = *(In+x);
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_CLUTOperator
 * Parameters: gam_emul_BLTNode, gam_emul_BlitterParameters, S2In, S2Out, PostResizer, y
 * Return : S2out
 * Description : to apply clut operator (reduction after resize)
 * ----------------------------------------------------------------------------------*/

void gam_emul_CLUTOperator(gam_emul_BLTNode* Blitter, gam_emul_BlitterParameters *Blt,int *S2In,int *S2Out,int PostResizer,int y)
{
	int x,i;
	int *palette;
	int a,r,g,b,index;
	int ShortestDistance,Distance,BestIndex,NumberOfColors;
	int r_err,g_err,b_err,rpal,bpal,gpal,ErrorDiffusionMode;
	int bigNotLittle = (Blt->pkz >> 5 ) & 0x1;

	if (Blt->EnaCLUTOperator == 1)
	{
		/* do we reload the CLUT ? */
		if (Blt->CLUT_enable_update == 1)
		{
            palette = (int*)(gam_emul_BLTMemory+Blt->CLUT_memory_location);
			for ( index=0 ; index<256 ; index++ )		/* we always load 256 32-bit words */
			{
				if (bigNotLittle == 0)
				{
					Blitter->ALUT[index] = (gam_emul_readInt(palette+index) >> 24) & 0xFF;
					Blitter->RLUT[index] = (gam_emul_readInt(palette+index) >> 16) & 0xFF;
					Blitter->GLUT[index] = (gam_emul_readInt(palette+index) >> 8) & 0xFF;
					Blitter->BLUT[index] = (gam_emul_readInt(palette+index) & 0xFF);
				}
				else
				{
					Blitter->ALUT[index] = gam_emul_readInt(palette+index)  & 0xFF;
					Blitter->RLUT[index] = (gam_emul_readInt(palette+index) >> 8) & 0xFF;
					Blitter->GLUT[index] = (gam_emul_readInt(palette+index) >>16) & 0xFF;
					Blitter->BLUT[index] = (gam_emul_readInt(palette+index) >> 24) & 0xFF;
				}
			}
		}
		switch (Blt->CLUT_mode)
		{
			case 0:			/* color expansion */
				if (PostResizer == FALSE)	/* before resizing */
				{
					for ( x=0 ; x < Blt->S2_width ; x++ )
					{
						index = *(S2In+x) & 0xFF;
						a = (*(S2In+x) >> 24) & 0xFF;
						if (((Blt->S2_format)==gam_emul_TYPE_ACLUT88) || ((Blt->S2_format)==gam_emul_TYPE_ACLUT44))
							a = Blitter->ALUT[a];		/* Alpha channel correction in these modes */
						else
							a = Blitter->ALUT[index];
						r = Blitter->RLUT[index];
						g = Blitter->GLUT[index];
						b = Blitter->BLUT[index];
						*(S2Out+x) = (a<<24) | (r<<16) | (g<<8) | b;
					}
				}
				else
				{
					for ( x=0 ; x < Blt->S2_width ; x++ ) /* no Blt->T_width */
						*(S2Out+x) = *(S2In+x);
				}
				break;
			case 1:			/* color correction */
				if (PostResizer == FALSE)	/* before resizing */
				{
					for ( x=0 ; x < Blt->S2_width ; x++ )
					{
						a = (*(S2In+x) >> 24) & 0xFF;
						r = (*(S2In+x) >> 16) & 0xFF;
						g = (*(S2In+x) >> 8) & 0xFF;
						b = (*(S2In+x) >> 0) & 0xFF;
						a = Blitter->ALUT[a];
						r = Blitter->RLUT[r];
						g = Blitter->GLUT[g];
						b = Blitter->BLUT[b];
						*(S2Out+x) = (a<<24) | (r<<16) | (g<<8) | b;
					}
				}
				else
				{
					for ( x=0 ; x < Blt->S2_width ; x++ )
						*(S2Out+x) = *(S2In+x);
				}
				break;
			case 2:			/* color reduction, only possible after the rescaler */
				if (PostResizer == TRUE)
				{
					switch (Blt->T_format)
					{
						case gam_emul_TYPE_CLUT8:
						case gam_emul_TYPE_ACLUT88:
							NumberOfColors = 256;
							break;
						case gam_emul_TYPE_CLUT4:
						case gam_emul_TYPE_ACLUT44:
							NumberOfColors = 16;
							break;
						case gam_emul_TYPE_CLUT2:
							NumberOfColors = 4;
							break;
						case gam_emul_TYPE_CLUT1:
							NumberOfColors = 2;
							break;
						default:
							NumberOfColors = 16;
							break;
					}
					for ( x=0 ; x < Blt->T_width ; x++ )
					{
						a = (*(S2In+x) >> 24) & 0xFF;
						r = (*(S2In+x) >> 16) & 0xFF;
						g = (*(S2In+x) >> 8) & 0xFF;
						b = (*(S2In+x) >> 0) & 0xFF;

						ShortestDistance = 0x7FFFFFFF; /*max of distance */
						BestIndex = -1;
						/* to search best index for each pixel in clut */
						for ( i=0 ; ((i<NumberOfColors) && (ShortestDistance!=0)) ; i++ )
						{
							rpal = Blitter->RLUT[i];
							gpal = Blitter->GLUT[i];
							bpal = Blitter->BLUT[i];
							r_err = r - rpal;
							g_err = g - gpal;
							b_err = b - bpal;
							Distance = abs(r_err) + abs(g_err) + abs(b_err);
							if (Distance<ShortestDistance)
							{
								ShortestDistance = Distance;
								BestIndex = i;
							}
						}
						*(S2Out+x) = (a<<24) | BestIndex;/* to memorize alpha and index */

						/* Error diffusion process : */
						/* We recompute the errors on each component for the best index */
						rpal = Blitter->RLUT[BestIndex];
						gpal = Blitter->GLUT[BestIndex];
						bpal = Blitter->BLUT[BestIndex];
						r_err = r - rpal;
						g_err = g - gpal;
						b_err = b - bpal;
						ErrorDiffusionMode = Blt->error_diffusion_weight
															 + Blt->error_diffusion_enable_2D * 8;
						switch (ErrorDiffusionMode)
						{
							case 0:			/* Disabled */
								r_err = 0;
								g_err = 0;
								b_err = 0;
								break;
							case 1:			/* simple diffusion 100% */
/*							r_err = r_err;								 */
/*							g_err = g_err;								 */
/*							b_err = b_err;								 */
								break;
							case 2:			/* simple diffusion 75% */
								r_err = ((r_err<<1) + (r_err<<0)) >> 2;
								g_err = ((g_err<<1) + (g_err<<0)) >> 2;
								b_err = ((b_err<<1) + (b_err<<0)) >> 2;
								break;
							case 3:			/* simple diffusion 50% */
								r_err = r_err >> 1;
								g_err = g_err >> 1;
								b_err = b_err >> 1;
								break;
							case 4:			/* simple diffusion 25% */
								r_err = r_err >> 2;
								g_err = g_err >> 2;
								b_err = b_err >> 2;
								break;
							case 8:			/* 2D alternate diffusion rate */
								if (((x&1) == 0) && ((y&1) == 0))		/* 25% */
								{
									r_err = r_err >> 2;
									g_err = g_err >> 2;
									b_err = b_err >> 2;
								}
								if (((x&1) == 0) && ((y&1) == 1))		/* 100%				 */
								{
/*								r_err = r_err;								 */
/*								g_err = g_err;								 */
/*								b_err = b_err;		 */
								}
								if (((x&1) == 1) && ((y&1) == 0))		/* 75%				 */
								{
									r_err = ((r_err<<1) + (r_err<<0)) >> 2;
									g_err = ((g_err<<1) + (g_err<<0)) >> 2;
									b_err = ((b_err<<1) + (b_err<<0)) >> 2;
								}
								if (((x&1) == 1) && ((y&1) == 1))		/* 50%				 */
								{
									r_err = r_err >> 1;
									g_err = g_err >> 1;
									b_err = b_err >> 1;
								}
								break;
							default:
								r_err = 0;
								g_err = 0;
								b_err = 0;
								break;
						}
						/* error diffusion on pixel y/x+1 */
						if (x < (Blt->T_width-1))
						{
							a = (*(S2In+x+1) >> 24) & 0xFF;			/* get pixel x+1 */
							r = (*(S2In+x+1) >> 16) & 0xFF;
							g = (*(S2In+x+1) >> 8) & 0xFF;
							b = (*(S2In+x+1) >> 0) & 0xFF;
							r += r_err; if (r<0) r = 0; if (r>255) r = 255;
							g += g_err; if (g<0) g = 0; if (g>255) g = 255;
							b += b_err; if (b<0) b = 0; if (b>255) b = 255;
							*(S2In+x+1) = (a<<24) | (r<<16) | (g<<8) | b;
						}
					}
				}
				else						/* BYPASS */
				{
					for ( x=0 ; x < Blt->S2_width  ; x++ )
						*(S2Out+x) = *(S2In+x);
				}
				break;
		}
	}
	else						/* BYPASS */
	{
		if (PostResizer == FALSE)
		{
			for ( x=0 ; x < Blt->S2_width ; x++ )
				*(S2Out+x) = *(S2In+x);
		}
		else
		{
			for ( x=0 ; x < Blt->T_width ; x++ )
				*(S2Out+x) = *(S2In+x);
		}
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ALUOperator
 * Parameters: gam_emul_BlitterParameters, S1In, S2In, TOut, y
 * Return : -
 * Description :
 * ----------------------------------------------------------------------------------*/

void gam_emul_ALUOperator(gam_emul_BlitterParameters *Blt,int *S1In,int *S2In,int *TOut,int y)
{
	int x,alpha;
	int S1a,S1r,S1g,S1b;
	int S2a,S2r,S2g,S2b;
	int Ta,Tr,Tg,Tb;


	/* H/V Border pre-processing if enabled */
	if (Blt->EnaFlickerFilter == 1)
	{
		for ( x=0 ; x < Blt->T_width ; x++ )
		{
			if (((y==0) && (Blt->AlphaVBorder==1))					/* first line */
			 || ((y==(Blt->T_height-1)) && (Blt->AlphaVBorder==1))	/* last line */
			 || ((x==0) && (Blt->AlphaHBorder==1))					/* first pixel */
			 || ((x==(Blt->T_width-1)) && (Blt->AlphaHBorder==1)))  /* last pixel */
			{
				alpha = (*(S2In+x) >> 24) & 0xFF;
				alpha >>= 1;
				*(S2In+x) &= 0xFFFFFF;
				*(S2In+x) |= (alpha << 24);
			}
		}
	}

	switch (Blt->ALU_opcode)
	{
		case 0:		/* BYPASS S1 */
			for ( x=0 ; x < Blt->S1_width ; x++ )
				*(TOut+x) = *(S1In+x);
			break;
		case 1:		/* Logical operator */
			for ( x=0 ; x < Blt->T_width ; x++ )
			{

				switch (Blt->ALU_rop)
				{
					case 0:		/* result = all 0 */
						*(TOut+x) = 0;
						break;
					case 1:		/* result = S2 AND S1 */
						*(TOut+x) = *(S2In+x) & *(S1In+x);
						break;
					case 2:		/* result = S2 AND (NOT S1) */
						*(TOut+x) = *(S2In+x) & (~(*(S1In+x)));
						break;
					case 3:		/* result = S2 */
							*(TOut+x) = *(S2In+x);
						break;
					case 4:		/* result = (NOT S2)  AND S1 */
						*(TOut+x) = (~(*(S2In+x))) & *(S1In+x);
						break;
					case 5:		/* result =  S1 */
						*(TOut+x) = *(S1In+x);
						break;
					case 6:		/* result = S2 XOR S1 */
						*(TOut+x) = *(S2In+x) ^ *(S1In+x);
						break;
					case 7:		/* result = S2 OR S1 */
						*(TOut+x) = *(S2In+x) | *(S1In+x);
						break;
					case 8:		/* result = (NOT S2)  AND (NOT S1) */
						*(TOut+x) = (~(*(S2In+x))) & (~(*(S1In+x)));
						break;
					case 9:		/* result = (NOT S2)  XOR S1 */
						*(TOut+x) = (~(*(S2In+x))) ^ *(S1In+x);
						break;
					case 10:	/* result = (NOT S1) */
						*(TOut+x) = (~(*(S1In+x)));
						break;
					case 11:	/* result = S2 OR (NOT S1) */
						*(TOut+x) = *(S2In+x) | (~(*(S1In+x)));
						break;
					case 12:	/* result = (NOT S2) */
						*(TOut+x) = (~(*(S2In+x)));
						break;
					case 13:	/* result = (NOT S2)  OR S1 */
						*(TOut+x) = (~(*(S2In+x))) | *(S1In+x);
						break;
					case 14:	/* result = (NOT S2)  OR (NOT S1) */
						*(TOut+x) = (~(*(S2In+x))) | (~(*(S1In+x)));
						break;
					case 15:	/* result = all 1 */
						*(TOut+x) = (int) 0xFFFFFFFF;
						break;
					default:
						break;
				}
			}
			break;
		case 2:		/* BLENDING S1/S2, S2 unweighted */
			for ( x=0 ; x < Blt->T_width ; x++ )
			{
				S1a = (*(S1In+x) >> 24) & 0xFF;
				S1r = (*(S1In+x) >> 16) & 0xFF;
				S1g = (*(S1In+x) >> 8) & 0xFF;
				S1b =  *(S1In+x) & 0xFF;

				S2a = (*(S2In+x) >> 24) & 0xFF;
				S2r = (*(S2In+x) >> 16) & 0xFF;
				S2g = (*(S2In+x) >> 8) & 0xFF;
				S2b =  *(S2In+x) & 0xFF;

				S2a = (((S2a * Blt->ALU_global_alpha) >> 6) + 1) >> 1;

				Ta = 128 - (((((128 - S1a) * (128 - S2a)) >> 6) + 1) >> 1);
				Tr = ((((S2a * S2r) + (128 - S2a) * S1r) >> 6) + 1) >> 1;
				Tg = ((((S2a * S2g) + (128 - S2a) * S1g) >> 6) + 1) >> 1;
				Tb = ((((S2a * S2b) + (128 - S2a) * S1b) >> 6) + 1) >> 1;

				if (Ta<0) Ta = 0; if (Ta>128) Ta = 128;
				if (Tr<0) Tr = 0; if (Tr>255) Tr = 255;
				if (Tg<0) Tg = 0; if (Tg>255) Tg = 255;
				if (Tb<0) Tb = 0; if (Tb>255) Tb = 255;
				*(TOut+x) = (Ta<<24) | (Tr<<16) | (Tg<<8) | Tb;
			}
			break;
		case 3:		/* BLENDING S1/S2, S2 already weighted */
			for ( x=0 ; x < Blt->T_width ; x++ )
			{
				S1a = (*(S1In+x) >> 24) & 0xFF;
				S1r = (*(S1In+x) >> 16) & 0xFF;
				S1g = (*(S1In+x) >> 8) & 0xFF;
				S1b =  *(S1In+x) & 0xFF;

				S2a = (*(S2In+x) >> 24) & 0xFF;
				S2r = (*(S2In+x) >> 16) & 0xFF;
				S2g = (*(S2In+x) >> 8) & 0xFF;
				S2b =  *(S2In+x) & 0xFF;

				S2a = (((S2a * Blt->ALU_global_alpha) >> 6) + 1) >> 1;

				Ta = 128 - (((((128 - S1a) * (128 - S2a)) >> 6) + 1) >> 1);
				Tr = ((((Blt->ALU_global_alpha * S2r) + (128 - S2a) * S1r) >> 6) + 1) >> 1;
				Tg = ((((Blt->ALU_global_alpha * S2g) + (128 - S2a) * S1g) >> 6) + 1) >> 1;
				Tb = ((((Blt->ALU_global_alpha * S2b) + (128 - S2a) * S1b) >> 6) + 1) >> 1;
				if (Ta<0) Ta = 0; if (Ta>128) Ta = 128;
				if (Tr<0) Tr = 0; if (Tr>255) Tr = 255;
				if (Tg<0) Tg = 0; if (Tg>255) Tg = 255;
				if (Tb<0) Tb = 0; if (Tb>255) Tb = 255;
				*(TOut+x) = (Ta<<24) | (Tr<<16) | (Tg<<8) | Tb;
			}
			break;
		case 4:		/* CLIPMASK PASS IN LOGICAL MODE, PASS 1 */
			for ( x=0 ; x < Blt->T_width ; x++ )
			{
				S1a = (*(S1In+x) >> 24) & 0xFF;			/* S1 = texture	 */
				S1r = (*(S1In+x) >> 16) & 0xFF;
				S1g = (*(S1In+x) >> 8) & 0xFF;
				S1b =  *(S1In+x) & 0xFF;
				S2a = (*(S2In+x) >> 24) & 0xFF;			/* mask alpha, RGB on S2 are "don't care" */

				if (S2a == 128)							/* means 1 for an A1 S2 format */
					Ta = S1a;
				else
					Ta = 255;							/* flag for the second pass */
				*(TOut+x) = (Ta<<24) | (S1r<<16) | (S1g<<8) | S1b;
			}
			break;
		case 5:		/* CLIPMASK PASS IN BLENDING MODE (S1=texture / S2=blending mask) */
			for ( x=0 ; x < Blt->T_width ; x++ )
			{
				S1a = (*(S1In+x) >> 24) & 0xFF;			/* texture alpha */
				S1r = (*(S1In+x) >> 16) & 0xFF;
				S1g = (*(S1In+x) >> 8) & 0xFF;
				S1b =  *(S1In+x) & 0xFF;
				S2a = (*(S2In+x) >> 24) & 0xFF;			/* mask alpha */
				S2r = (*(S2In+x) >> 16) & 0xFF;			/* must be 0 */
				S2g = (*(S2In+x) >> 8) & 0xFF;			/* must be 0 */
				S2b =  *(S2In+x) & 0xFF;						/* must be 0 */

				S2a = (((S2a * Blt->ALU_global_alpha) >> 6) + 1) >> 1; /* Global_alpha must be 128 */

				Ta = ((((S1a * S2a) >> 6) + 1) >> 1);			/* all (128-A) blocks are bypassed */
				Tr = S1r;
				Tg = S1g;
				Tb = S1b;
				*(TOut+x) = (Ta<<24) | (Tr<<16) | (Tg<<8) | Tb;
			}
			break;
		case 6:		/* YCbCr concatenation for 4:2:0 source : TO BE VERIFIED */
			for ( x=0 ; x < Blt->T_width ; x++ )
				*(TOut+x) = *(S2In+x) | *(S1In+x);
			break;
		case 7:		/* BYPASS S2 */
			for ( x=0 ; x < Blt->T_width ; x++ )
			{
					*(TOut+x) = *(S2In+x);
			}
			break;
		case 8:		/* CLIPMASK PASS IN LOGICAL MODE, PASS 2 */
			for ( x=0 ; x < Blt->T_width ; x++ )
			{
				S2a = (*(S2In+x) >> 24) & 0xFF;		/* 0xFF flag, or original texture alpha */
				if (S2a == 255)
					*(TOut+x) = *(S1In+x);			/* Background is unchanged */
				else
				{
					switch (Blt->ALU_rop)
						{
						case 0:		/* result = all 0 */
							*(TOut+x) = 0;
							break;
						case 1:		/* result = S2 AND S1 */
							*(TOut+x) = *(S2In+x) & *(S1In+x);
							break;
						case 2:		/* result = S2 AND (NOT S1) */
							*(TOut+x) = *(S2In+x) & (~(*(S1In+x)));
							break;
						case 3:		/* result = S2 */
							*(TOut+x) = *(S2In+x);
							break;
						case 4:		/* result = (NOT S2)  AND S1 */
							*(TOut+x) = (~(*(S2In+x))) & *(S1In+x);
							break;
						case 5:		/* result =  S1 */
							*(TOut+x) = *(S1In+x);
							break;
						case 6:		/* result = S2 XOR S1 */
							*(TOut+x) = *(S2In+x) ^ *(S1In+x);
							break;
						case 7:		/* result = S2 OR S1 */
							*(TOut+x) = *(S2In+x) | *(S1In+x);
							break;
						case 8:		/* result = (NOT S2)  AND (NOT S1) */
							*(TOut+x) = (~(*(S2In+x))) & (~(*(S1In+x)));
							break;
						case 9:		/* result = (NOT S2)  XOR S1 */
							*(TOut+x) = (~(*(S2In+x))) ^ *(S1In+x);
							break;
						case 10:	/* result = (NOT S1) */
							*(TOut+x) = (~(*(S1In+x)));
							break;
						case 11:	/* result = S2 OR (NOT S1) */
							*(TOut+x) = *(S2In+x) | (~(*(S1In+x)));
							break;
						case 12:	/* result = (NOT S2) */
							*(TOut+x) = (~(*(S2In+x)));
							break;
						case 13:	/* result = (NOT S2)  OR S1 */
							*(TOut+x) = (~(*(S2In+x))) | *(S1In+x);
							break;
						case 14:	/* result = (NOT S2)  OR (NOT S1) */
							*(TOut+x) = (~(*(S2In+x))) | (~(*(S1In+x)));
							break;
						case 15:	/* result = all 1 */
							*(TOut+x) = (int) 0xFFFFFFFF;
							break;
						default:
							break;
					}
				}
			}
			break;

		default:
			break;
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_SetWriteEnableMask
 * Parameters: gam_emul_BlitterParameters, TMask, Type, Sa, Sb, Sc, y
 * Return : -
 * Description : modify mask to validate rectangular clipping and color key for each pixel
 by line
 * ----------------------------------------------------------------------------------*/

void gam_emul_SetWriteEnableMask(gam_emul_BlitterParameters *Blt,int *TMask,int Type,int *Sa,int *Sb,int *Sc,int y)
{
	int x,T_absoluteX,T_absoluteY,Width;
	int *Input;
	int r,EnableRKey,ROutRange;
	int g,EnableGKey,GOutRange;
	int b,EnableBKey,BOutRange;
	int CKmatch;

	Width = 0;
	Input = 0;

	if (Type == gam_emul_RECT_CLIPPING)
	{
		/* Let's consider the target vertical scanning direction */
		if (Blt->T_vscan == 0)
			T_absoluteY = (y + Blt->T_y);		/* Given parameter = y top */
		else
			T_absoluteY = (Blt->T_y - y);		/* Given parameter = y bottom */
		for ( x=0 ; x < Blt->T_width ; x++ )
		{
			/* Let's consider the horizontal scanning direction */
			if (Blt->T_hscan == 0)
				T_absoluteX = (x + Blt->T_x);		/* Given parameter = x left */
			else
				T_absoluteX = (Blt->T_x - x);		/* Given parameter = x right */
			if (Blt->InternalClip == 0)
			{
				if  ( (T_absoluteX < Blt->ClipXMin)		/* Write allowed on the borders in normal mode */
					 || (T_absoluteX > Blt->ClipXMax)
					 || (T_absoluteY < Blt->ClipYMin)
					 || (T_absoluteY > Blt->ClipYMax))
				*(TMask+x) |= gam_emul_RECT_CLIPPING;					/* Set Clipping Bit filter */
			}
			else
			{
				if ( (T_absoluteX >= Blt->ClipXMin)		/* Write NOT allowed on the borders in reverse mode */
					&& (T_absoluteX <= Blt->ClipXMax)
				  && (T_absoluteY >= Blt->ClipYMin)
				  && (T_absoluteY <= Blt->ClipYMax))
				*(TMask+x) |= gam_emul_RECT_CLIPPING;					/* Set Clipping Bit filter */
			}
			if ((T_absoluteX < 0) || (T_absoluteY < 0))
				*(TMask+x) |= gam_emul_RECT_CLIPPING;					/* Set Clipping Bit filter */
		}
	}

	if (Type == gam_emul_COLOR_KEY)
	{
		switch (Blt->CHK_Input)
		{
			case 0:					/* destination color key (S1) */
			case 3:					/* reserved */
				Width = Blt->T_width;
				Input = Sa;
				break;
			case 1:					/* source color key (S2 before CLUT) */
				Width = Blt->S2_width;
				Input = Sb;
				break;
			case 2:					/* source color key (S2 after CLUT & RESIZE) */
				Width = Blt->T_width;
				Input = Sc;
				break;
			default :
				Width = 0;
				Input = 0;
				break;
		}
		EnableRKey = Blt->CHK_RMode & 1;
		EnableGKey = Blt->CHK_GMode & 1;
		EnableBKey = Blt->CHK_BMode & 1;
		ROutRange = (Blt->CHK_RMode >> 1) & 1;
		GOutRange = (Blt->CHK_GMode >> 1) & 1;
		BOutRange = (Blt->CHK_BMode >> 1) & 1;
		for ( x=0 ; x < Width ; x++ )
		{
			r = (*(Input+x) >> 16) & 0xFF;
			g = (*(Input+x) >> 8) & 0xFF;
			b =  *(Input+x) & 0xFF;
			if (
				((((g<Blt->CHK_GMin)||(g>Blt->CHK_GMax)) && (GOutRange==1) && (EnableGKey==1))
			||((g>=Blt->CHK_GMin) && (g<=Blt->CHK_GMax) && (GOutRange==0) && (EnableGKey==1))
			||(EnableGKey==0))
			&&
				((((b<Blt->CHK_BMin)||(b>Blt->CHK_BMax)) && (BOutRange==1) && (EnableBKey==1))
			||((b>=Blt->CHK_BMin) && (b<=Blt->CHK_BMax) && (BOutRange==0) && (EnableBKey==1))
			||(EnableBKey==0))
			&&
				((((r<Blt->CHK_RMin)||(r>Blt->CHK_RMax)) && (ROutRange==1) && (EnableRKey==1))
			||((r>=Blt->CHK_RMin) && (r<=Blt->CHK_RMax) && (ROutRange==0) && (EnableRKey==1))
			||(EnableRKey==0))
			)
				CKmatch = 1;
			else
				CKmatch = 0;
			if (Blt->CHK_Input == 0)					/* destination color key, write only if match */
				CKmatch = 1 - CKmatch;
			if (CKmatch == 1)
				*(TMask+x) |= gam_emul_COLOR_KEY;				/* Set ColorKey Bit filter */
		}
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_SamplingPatternConversion444To422
 * Parameters: gam_emul_BlitterParameters , TIn, TOut
 * Return : -
 * Description : to format and to write in target buffer
 * ----------------------------------------------------------------------------------*/

void gam_emul_SamplingPatternConversion444To422(gam_emul_BlitterParameters *Blt,int *TIn,int *TOut)
{
	int x,TargetFormat;
	int luma,cb1,cb2,cb3,cr1,cr2,cr3;

	TargetFormat = Blt->T_format;
	switch (TargetFormat)
	{
		case gam_emul_TYPE_YCbCr422R:
			if ((Blt->T_x & 1) == 0)		/* First pixel is on an even x location */
			{
				for ( x=0 ; x < Blt->T_width ; x++ )
				{
					if ((x & 1) == 0)				/* This pixel must be stored as a 24 bpp pixel */
					{
						if ( x==0 )
						{
							cb1 = cb2 = *(TIn+x) & 0xFF;
							cr1 = cr2 = (*(TIn+x) >> 16) & 0xFF;
							cb3 = *(TIn+x+1) & 0xFF;
							cr3 = (*(TIn+x+1) >> 16) & 0xFF;
						}
						else
						{
							if ( x==(Blt->T_width - 1))
							{
								cb1 = *(TIn+x-1) & 0xFF;
								cr1 = (*(TIn+x-1) >> 16) & 0xFF;
								cb2 = cb3 = *(TIn+x) & 0xFF;
								cr2 = cr3 = (*(TIn+x) >> 16) & 0xFF;
							}
							else
							{
								cb1 = *(TIn+x-1) & 0xFF;
								cb2 = *(TIn+x) & 0xFF;
								cb3 = *(TIn+x+1) & 0xFF;
								cr1 = (*(TIn+x-1) >> 16) & 0xFF;
								cr2 = (*(TIn+x) >> 16) & 0xFF;
								cr3 = (*(TIn+x+1) >> 16) & 0xFF;
							}
						}
						luma = (*(TIn+x) >> 8) & 0xFF;
						cb1 = (cb1 + 2*cb2 + cb3 + 2) >> 2;
						cr1 = (cr1 + 2*cr2 + cr3 + 2) >> 2;
						*(TOut+x) = (cr1<<16) | (luma<<8) | cb1;
					}
					else										/* This pixel must be stored as a 8 bpp pixel (luma) */
						*(TOut+x) = (*(TIn+x) >> 8) & 0xFF;
				}
			}
			else		/* First pixel is on an odd x location */
			{
				for ( x=0 ; x < Blt->T_width ; x++ )
				{
					if ((x & 1) == 1)				/* This pixel must be stored as a 24 bpp pixel */
					{
						if ( x==(Blt->T_width - 1))
						{
							cb1 = *(TIn+x-1) & 0xFF;
							cr1 = (*(TIn+x-1) >> 16) & 0xFF;
							cb2 = cb3 = *(TIn+x) & 0xFF;
							cr2 = cr3 = (*(TIn+x) >> 16) & 0xFF;
						}
						else
						{
							cb1 = *(TIn+x-1) & 0xFF;
							cb2 = *(TIn+x) & 0xFF;
							cb3 = *(TIn+x+1) & 0xFF;
							cr1 = (*(TIn+x-1) >> 16) & 0xFF;
							cr2 = (*(TIn+x) >> 16) & 0xFF;
							cr3 = (*(TIn+x+1) >> 16) & 0xFF;
						}
						luma = (*(TIn+x) >> 8) & 0xFF;
						cb1 = (cb1 + 2*cb2 + cb3 + 2) >> 2;
						cr1 = (cr1 + 2*cr2 + cr3 + 2) >> 2;
						*(TOut+x) = (cr1<<16) | (luma<<8) | cb1;
					}
					else										/* This pixel must be stored as a 8 bpp pixel (luma) */
						*(TOut+x) = (*(TIn+x) >> 8) & 0xFF;
				}
			}
			break;
		default:
			for ( x=0 ; x < Blt->T_width ; x++ )
				*(TOut+x) = *(TIn+x);
			break;
	}
}
/*-----------------------------------------------------------------------------------
 * Function : gam_emul_WriteTargetLine
 * Parameters: gam_emul_BlitterParameters, TIn, S1In, TMask, y
 * Return : -
 * Description : to format and to write in target buffer
 * ----------------------------------------------------------------------------------*/

void gam_emul_WriteTargetLine(gam_emul_BlitterParameters *Blt,int *TIn,int *S1In,int *TMask,int y)
{
	int x;
	int a,r,g,b,index;
	int DitherPattern;

	/* control the PlaneMask value if x=y=0 */
	x=0;
	if (y == 0)
	{
		switch (Blt->T_format)
		{
			case gam_emul_TYPE_ARGB8888:		/* 32 bpp */
			case gam_emul_TYPE_AYCbCr8888:
				Blt->PlaneMask &= 0xFFFFFFFF;
				break;
			case gam_emul_TYPE_YCbCr888:		/* 24 bpp */
			case gam_emul_TYPE_RGB888:
			case gam_emul_TYPE_ARGB8565:
				Blt->PlaneMask &= 0xFFFFFF;
				break;
			case gam_emul_TYPE_RGB565:			/* 16 bpp */
			case gam_emul_TYPE_ARGB1555:
			case gam_emul_TYPE_ARGB4444:
			case gam_emul_TYPE_ACLUT88:
				Blt->PlaneMask &= 0xFFFF;
				break;
			case gam_emul_TYPE_A8:					/* 8 bpp */
			case gam_emul_TYPE_ACLUT44:
			case gam_emul_TYPE_CLUT8:
			case gam_emul_TYPE_BYTE:
				Blt->PlaneMask &= 0xFF;
				break;
			case gam_emul_TYPE_CLUT4:			/* 4 bpp */
				Blt->PlaneMask &= 0xF;
				break;
			case gam_emul_TYPE_CLUT2:			/* 2 bpp */
				Blt->PlaneMask &= 0x3;
				break;
			case gam_emul_TYPE_CLUT1:			/* 1 bpp */
			case gam_emul_TYPE_A1:
				Blt->PlaneMask &= 0x1;
				break;
			default:
				break;
		}
	}

	/* We convert the internal 32-bit pixel buffer into a target memory format pixel buffer */
	for ( x=0 ; x < Blt->T_width ; x++ )
	{
		if (Blt->T_dither == 1)		/* spacial dithering on RGB components ? */
		/* dither bit 1 = x0^y0 */
		/* dither bit 0 = y0 */
			DitherPattern = (y&1) + (((x&1)^(y&1))<<1);
		else				/* normal rounding  (0.5) */
			DitherPattern = 2; /* round off 1/2*/
		switch (Blt->T_format)
		{
			case gam_emul_TYPE_YCbCr422R:					/* ok */
				break;
			case gam_emul_TYPE_YCbCr888:
			case gam_emul_TYPE_RGB888:
				*(TIn+x) &= 0x00FFFFFF;			/* reset alpha channel */
				break;
			case gam_emul_TYPE_ARGB8565:
				a = (*(TIn+x) >> 24) & 0xFF;
				if (Blt->T_alphaRange == 0x1)
				{
					if ( a != 0x0 )
					{
						a = ((a - 1) << 1) + 1; /* to have a range between 0...255 */
					}
				}
				r = (*(TIn+x) >> 16) & 0xFF;
				g = (*(TIn+x) >> 8) & 0xFF;
				b = (*(TIn+x) >> 0) & 0xFF;
				r = r + (DitherPattern<<1); if (r>255) r = 255; r = r>>3;		/* 8-bit to 5-bit component */
				g = g +  DitherPattern;     if (g>255) g = 255; g = g>>2;		/* 8-bit to 6-bit component */
				b = b + (DitherPattern<<1); if (b>255) b = 255; b = b>>3;		/* 8-bit to 5-bit component */
				*(TIn+x) = (a<<16) | (r<<11) | (g<<5) | b;									/* Reformat */
				break;
			case gam_emul_TYPE_ARGB8888:							/* ok */
			case gam_emul_TYPE_AYCbCr8888:
				if (Blt->T_alphaRange == 0x1)
				{
					a = (*(TIn+x) >> 24) & 0xFF;
					*(TIn+x) &= 0xFFFFFF;
					if ( a != 0x0 )
					{
						a = ((a - 1) << 1) + 1; /* to have a range between 0...255 */
					}
					*(TIn+x) |= (a<<24);
				}
				break;
			case gam_emul_TYPE_RGB565:
				r = (*(TIn+x) >> 16) & 0xFF;
				g = (*(TIn+x) >> 8) & 0xFF;
				b = (*(TIn+x) >> 0) & 0xFF;
				r = r + (DitherPattern<<1); if (r>255) r = 255; r = r>>3;		/* 8-bit to 5-bit component */
				g = g +  DitherPattern;     if (g>255) g = 255; g = g>>2;		/* 8-bit to 6-bit component */
				b = b + (DitherPattern<<1); if (b>255) b = 255; b = b>>3;		/* 8-bit to 5-bit component */
				*(TIn+x) = (r<<11) | (g<<5) | b;														/* Reformat */
				break;
			case gam_emul_TYPE_ARGB1555:
				a = (*(TIn+x) >> 24) & 0xFF;
				if (a==128)			/* opaque */
					a = 1;
				else						/* not opaque -> transparent */
					a = 0;
				r = (*(TIn+x) >> 16) & 0xFF;
				g = (*(TIn+x) >> 8) & 0xFF;
				b = (*(TIn+x) >> 0) & 0xFF;
				r = r + (DitherPattern<<1); if (r>255) r = 255; r = r>>3;		/* 8-bit to 5-bit component */
				g = g + (DitherPattern<<1); if (g>255) g = 255; g = g>>3;		/* 8-bit to 5-bit component */
				b = b + (DitherPattern<<1); if (b>255) b = 255; b = b>>3;		/* 8-bit to 5-bit component */
				*(TIn+x) = (a<<15) | (r<<10) | (g<<5) | b;									/* Reformat */
				break;
			case gam_emul_TYPE_ARGB4444:
				a = (*(TIn+x) >> 24) & 0xFF;
				a = a>>3;
				if (a==16)			/* opaque */
					a = 15;
				r = (*(TIn+x) >> 16) & 0xFF;
				g = (*(TIn+x) >> 8) & 0xFF;
				b = (*(TIn+x) >> 0) & 0xFF;
				r = r + (DitherPattern<<2); if (r>255) r = 255; r = r>>4;		/* 8-bit to 4-bit component */
				g = g + (DitherPattern<<2); if (g>255) g = 255; g = g>>4;		/* 8-bit to 4-bit component */
				b = b + (DitherPattern<<2); if (b>255) b = 255; b = b>>4;		/* 8-bit to 4-bit component */
				*(TIn+x) = (a<<12) | (r<<8) | (g<<4) | b;										/* Reformat */
				break;
			case gam_emul_TYPE_A8:
				a = (*(TIn+x) >> 24) & 0xFF;
				if (Blt->T_alphaRange == 0x1)
				{
					if ( a != 0x0 )
					{
						a = ((a - 1) << 1) + 1; /* to have a range between 0...255 */
					}
				}
				*(TIn+x) = a;									/* Reformat */
				break;
			case gam_emul_TYPE_A1:
				a = (*(TIn+x) >> 24) & 0xFF;
				if (a != 128)
					a = 0;
				else
					a = 1;
				*(TIn+x) = a;									/* Reformat */
				break;
			case gam_emul_TYPE_ACLUT88:
				a = (*(TIn+x) >> 24) & 0xFF;
				if (Blt->T_alphaRange == 0x1)
				{
					if ( a != 0x0 )
					{
						a = ((a - 1) << 1) + 1; /* to have a range between 0...255 */
					}
				}
				index = *(TIn+x) & 0xFF;
				*(TIn+x) = (a<<8) | index;		/* Reformat */
				break;
			case gam_emul_TYPE_ACLUT44:
				a = (*(TIn+x) >> 24) & 0xFF;
				a = a>>3;
				if (a==16)			/* opaque */
					a = 15;
				index = *(TIn+x) & 0xF;
				*(TIn+x) = (a<<4) | index;		/* Reformat */
				break;
			case gam_emul_TYPE_CLUT8:
			case gam_emul_TYPE_BYTE:
				*(TIn+x) &= 0xFF;			/* Keep 8 LSBs */
				break;
			case gam_emul_TYPE_CLUT4:
				*(TIn+x) &= 0xF;			/* Keep 4 LSBs */
				break;
			case gam_emul_TYPE_CLUT2:
				*(TIn+x) &= 0x3;			/* Keep 2 LSBs */
				break;
			case gam_emul_TYPE_CLUT1:
				*(TIn+x) &= 0x1;			/* Keep 1 LSBs */
				break;
			default:
				break;
		}
		if (Blt->EnaPlaneMask == 1)
		{
			*(TIn+x) = ( Blt->PlaneMask & *(TIn+x))			/* Reset unmodified bits in the result */
							 | (~Blt->PlaneMask & *(S1In+x));		/* Reset modified bits in S1 value, */
																									/* then combine */
		}
	}
	/* Then we write the target line buffer */
	switch (Blt->T_format)
	{
		case gam_emul_TYPE_YCbCr420MB:
		case gam_emul_TYPE_YCbCr422MB:
			/* Luma pass */
			if (Blt->T_chromanotluma == 0)
				for ( x = 0 ; x < Blt->T_width ; x++ )
					gam_emul_Write42XLuma(x,y,Blt,*(TIn+x));
			/* Chroma pass */
			else
			{
				for ( x = 0 ; x < Blt->T_width ; x++ )
					gam_emul_Write42XCbCr(x,y,Blt,*(TIn+x));
			}
			break;
		case gam_emul_TYPE_YCbCrMBHDpip:
			/* Luma pass */
			if (Blt->T_chromanotluma == 0)
			{
				for ( x = 0 ; x < Blt->T_width ; x++ )
					gam_emul_WriteHDpipLuma(x,y,Blt,*(TIn+x));
			}
			/* Chroma pass */
			else
			{
				for ( x = 0 ; x < Blt->T_width ; x++ )
					gam_emul_WriteHDpipCbCr(x,y,Blt,*(TIn+x));
			}
			break;
		default:
			for ( x = 0 ; x < Blt->T_width ; x++ )
				if (*(TMask+x) == 0)
					gam_emul_WriteBlitterPixel(x,y,Blt,*(TIn+x));
			break;
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ClearMaskArray
 * Parameters: gam_emul_BlitterParameters, TMask
 * Return : -
 * Description : clear mask array
 * ----------------------------------------------------------------------------------*/

void gam_emul_ClearMaskArray(gam_emul_BlitterParameters *Blt,int *TMask)
{
	int x;

	for ( x = 0 ; x < Blt->T_width ; x++ )
		*(TMask+x) = 0;
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ConvertRGB2YCbCr
 * Parameters: r,g,b,luma,cb,cr,type,c_format,nob_in
 * Return : -
 * Description : RGB to YCbCr
 * ----------------------------------------------------------------------------------*/

void gam_emul_ConvertRGB2YCbCr (int *r,int *g,int *b,int *luma,int *cb,int *cr,int type,int c_format,int nob_in, int matrix_ngfx_vid)
/* Coefficient format : 1.0 = 1024 */
{
	int chroma_offset,luma_offset;

	if (c_format == 0)			/* means cb/cr in offset binary */
		chroma_offset = 128;
	else										/* cb/cr signed */
		chroma_offset = 0;
	luma_offset = 16;

	if (matrix_ngfx_vid == 0) /* Graphic RGB color space */
	{
		switch (type)
		{
			case 0:				/* 601 matrix, fixed coefficients */
				*luma =  263 * (*r) + 516 * (*g) + 100 * (*b);
				*cb   = -152 * (*r) - 298 * (*g) + 450 * (*b);
				*cr   =  450 * (*r) - 377 * (*g) -  73 * (*b);
				break;
			case 1:				/* 709 matrix, fixed coefficients */
				*luma =  187 * (*r) + 629 * (*g) +  63 * (*b);
				*cb   = -103 * (*r) - 347 * (*g) + 450 * (*b);
				*cr   =  450 * (*r) - 409 * (*g) -  41 * (*b);
				break;
			default:
				break;
		}
	}
	else
	{
		switch (type)
		{
			case 0:				/* 601 matrix, fixed coefficients */
				*luma =  306 * (*r) + 601 * (*g) + 117 * (*b);
				*cb   = -177 * (*r) - 347 * (*g) + 523 * (*b);
				*cr   =  523 * (*r) - 438 * (*g) -  85 * (*b);
				break;
			case 1:				/* 709 matrix, fixed coefficients */
				*luma =  218 * (*r) + 732 * (*g) +  74 * (*b);
				*cb   = -120 * (*r) - 404 * (*g) + 524 * (*b);
				*cr   =  524 * (*r) - 476 * (*g) -  48 * (*b);
				break;
			default:
				break;
		}

	}


	if (matrix_ngfx_vid == 0) /* Graphic RGB color space */
	{

		*luma = ((*luma >> (nob_in+1)) + 1) >> 1;
		(*luma) += luma_offset;
		if (*luma >235)
			*luma = 235;


		*cb = ((*cb >> (nob_in+1)) + 1) >> 1;
		if (*cb < -112) /* 240 -16 /2 = 112*/
			*cb = -112;
		if (*cb > 112)
			*cb = 112;
		(*cb) += chroma_offset;
		(*cb) &= 0xFF;

		*cr = ((*cr >> (nob_in+1)) + 1) >> 1;
		if (*cr < -112)
			*cr = -112;
		if (*cr > 112)
			*cr = 112;
		(*cr) += chroma_offset;
		(*cr) &= 0xFF;
	}
	else
	{
		*luma = ((*luma >> (nob_in+1)) + 1) >> 1;
		if (*luma > 254)
			*luma = 254; /* 0xFF is a value forbidden */
		if (*luma < 1)
			*luma = 1;	/* 0x1 is a value forbidden */
		(*luma) &= 0xFF;

		*cb = ((*cb >> (nob_in+1)) + 1) >> 1;
		if (*cb < -127) /* (254 - 1) / 2  = 126,5 */
			*cb = -127;
		if (*cb > 126)
			*cb = 126;
		(*cb) += chroma_offset;
		(*cb) &= 0xFF;

		*cr = ((*cr >> (nob_in+1)) + 1) >> 1;
		if (*cr < -127)
			*cr = -127;
		if (*cr > 126)
			*cr = 126;
		(*cr) += chroma_offset;
		(*cr) &= 0xFF;
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_RescaleOperator
 * Parameters: gam_emul_BlitterParameters, *S2In, *S2Out, *RequestS2Line
 * Return : -
 * Description : 2D resize engine if no resize copy input  else resize and flicher
 * ----------------------------------------------------------------------------------*/

int gam_emul_RescaleOperator(stblit_Device_t* Device_p, gam_emul_BlitterParameters *Blt,int *S2In,int *S2Out,int *RequestS2Line)
{
	int x,xs,i;
	int S2Request,TargetAvailability;
	int data;
	int bigNotLittle = (Blt->pkz >> 5) &0x1;
	S2Request = FALSE;
	TargetAvailability = FALSE;

	if ((Blt->Ena2DResampler == 1) || (Blt->EnaFlickerFilter == 1))
	{
/*		return(0);		 */
		switch (gam_emul_RescalerCurrentState)
		{
			case 0:				/* Main Init */
				/* We format the selected filters in HFilter and VFilter arrays */
                gam_emul_HFilter = (int*)gam_emul_malloc(Device_p, sizeof(int) * gam_emul_NUMBER_OF_TAPS  * gam_emul_NUMBER_OF_SUBPOSITIONS );
				for ( i=0 ; i< gam_emul_NUMBER_OF_TAPS  * gam_emul_NUMBER_OF_SUBPOSITIONS ; i+=4 )
				{
                    data = gam_emul_readInt((int*)(gam_emul_BLTMemory+Blt->HF_memory_location+i));
					data  =  gam_emul_swap_32(data, bigNotLittle);
					*(gam_emul_HFilter+i) = (data & 0xFF)>= 0x80 ? (data & 0xFF) | 0xFFFFFF00 : (data & 0xFF) ;
					*(gam_emul_HFilter+i+1) = ((data >> 8) & 0xFF) >= 0x80 ? ((data >> 8) & 0xFF) | 0xFFFFFF00 : ((data >> 8) & 0xFF) ;
					*(gam_emul_HFilter+i+2) = ((data >> 16) & 0xFF) >= 0x80 ? ((data >> 16) & 0xFF) | 0xFFFFFF00 : ((data >> 16) & 0xFF) ;
					*(gam_emul_HFilter+i+3) = ((data >> 24) & 0xFF) >= 0x80 ? ((data >> 24) & 0xFF) | 0xFFFFFF00 : ((data >> 24) & 0xFF) ;

                    /**(HFilter+i) = (int)(gam_emul_readChar(gam_emul_BLTMemory+Blt->HF_memory_location+i));*/
				}
                gam_emul_VFilter = (int*)gam_emul_malloc(Device_p, sizeof(int) * gam_emul_NUMBER_OF_TAPS  * gam_emul_NUMBER_OF_SUBPOSITIONS );
				for ( i=0 ; i< gam_emul_NUMBER_OF_TAPS  * gam_emul_NUMBER_OF_SUBPOSITIONS ; i+=4 )
				{
                    data = gam_emul_readInt((int*)(gam_emul_BLTMemory+Blt->VF_memory_location+i));
					data  =  gam_emul_swap_32(data, bigNotLittle);
					*(gam_emul_VFilter+i) = (data & 0xFF)>= 0x80 ? (data & 0xFF) | 0xFFFFFF00 : (data & 0xFF) ;
					*(gam_emul_VFilter+i+1) = ((data >> 8) & 0xFF) >= 0x80 ? ((data >> 8) & 0xFF) | 0xFFFFFF00 : ((data >> 8) & 0xFF) ;
					*(gam_emul_VFilter+i+2) = ((data >> 16) & 0xFF) >= 0x80 ? ((data >> 16) & 0xFF) | 0xFFFFFF00 : ((data >> 16) & 0xFF) ;
					*(gam_emul_VFilter+i+3) = ((data >> 24) & 0xFF) >= 0x80 ? ((data >> 24) & 0xFF) | 0xFFFFFF00 : ((data >> 24) & 0xFF) ;

                    /**(VFilter+i) = (int)(gam_emul_readChar(gam_emul_BLTMemory+Blt->VF_memory_location+i));*/
				}
				/* Let's acquire the flicker filter coefficients/threshold parameters */
				gam_emul_FF0[0] = ((Blt->FF0) >> 0) & 0xFF;
				gam_emul_FF0[1] = ((Blt->FF0) >> 8) & 0xFF;
				gam_emul_FF0[2] = ((Blt->FF0) >> 16) & 0xFF;
				gam_emul_FFThreshold01 = ((Blt->FF0) >> 24) & 0xF;
				gam_emul_FF1[0] = ((Blt->FF1) >> 0) & 0xFF;
				gam_emul_FF1[1] = ((Blt->FF1) >> 8) & 0xFF;
				gam_emul_FF1[2] = ((Blt->FF1) >> 16) & 0xFF;
				gam_emul_FFThreshold12 = ((Blt->FF1) >> 24) & 0xF;
				gam_emul_FF2[0] = ((Blt->FF2) >> 0) & 0xFF;
				gam_emul_FF2[1] = ((Blt->FF2) >> 8) & 0xFF;
				gam_emul_FF2[2] = ((Blt->FF2) >> 16) & 0xFF;
				gam_emul_FFThreshold23 = ((Blt->FF2) >> 24) & 0xF;
				gam_emul_FF3[0] = ((Blt->FF3) >> 0) & 0xFF;
				gam_emul_FF3[1] = ((Blt->FF3) >> 8) & 0xFF;
				gam_emul_FF3[2] = ((Blt->FF3) >> 16) & 0xFF;

				/* The shift register for the horizontal SRC */
                gam_emul_Delay = (int*)gam_emul_malloc(Device_p, sizeof(int) * gam_emul_NUMBER_OF_TAPS);

				/* Line-Fifo memories allocation for the Vertical Filter */
                gam_emul_VF_Fifo1  = (int*)gam_emul_malloc(Device_p, sizeof(int) * Blt->S2_width);
                gam_emul_VF_Fifo2  = (int*)gam_emul_malloc(Device_p, sizeof(int) * Blt->S2_width);
                gam_emul_VF_Fifo3  = (int*)gam_emul_malloc(Device_p, sizeof(int) * Blt->S2_width);
                gam_emul_VF_Fifo4  = (int*)gam_emul_malloc(Device_p, sizeof(int) * Blt->S2_width);
                gam_emul_VF_Fifo5  = (int*)gam_emul_malloc(Device_p, sizeof(int) * Blt->S2_width);
                /* For the flicker-filter */
                gam_emul_FF_Fifo1  = (int*)gam_emul_malloc(Device_p, sizeof(int) * Blt->S2_width);
                gam_emul_FF_Fifo2  = (int*)gam_emul_malloc(Device_p, sizeof(int) * Blt->S2_width);
                gam_emul_FF_Fifo3  = (int*)gam_emul_malloc(Device_p, sizeof(int) * Blt->S2_width);
                gam_emul_FF_Output = (int*)gam_emul_malloc(Device_p, sizeof(int) * Blt->S2_width);

				gam_emul_VF_Counter = Blt->VF_initial_phase << 7;	/* Set vertical counter initial value, 6.10 */
				gam_emul_VF_LineNumber = 0;							/* Line number produced by the vertical filter */
				gam_emul_NumberOfOutputLines = 0;				/* Number of valid lines output by the rescaler block */
				gam_emul_FF_Flag = 0;
				gam_emul_Newline (S2In,Blt->S2_width);		/* First line is entered 3 times */
				gam_emul_Newline (S2In,Blt->S2_width);
				gam_emul_Newline (S2In,Blt->S2_width);
				/* Go to next state */
				gam_emul_RescalerCurrentState = 1;
				TargetAvailability = FALSE;
				S2Request = TRUE;
				break;

			case 1:				/* End of Init (we still need one line before we can start) */
				gam_emul_Newline (S2In,Blt->S2_width);
				gam_emul_NbNewLines = 1;
				/* Go to next state */
				gam_emul_RescalerCurrentState = 2;
				TargetAvailability = FALSE;
				S2Request = TRUE;
				break;

			case 2:				/* Main processing loop */
				/* Check if we need new line before we can go on */
				if (gam_emul_NbNewLines != 0)
				{
					gam_emul_Newline (S2In,Blt->S2_width);
					gam_emul_NbNewLines--;
				}
				if (gam_emul_NbNewLines == 0)		/* That means the VF can produce a new line */
				{
					/* 5-tap vertical 8-phase filter */
					if (gam_emul_VF_LineNumber < Blt->T_height)
					{
						gam_emul_VF_Output = gam_emul_FF_Fifo3;
						gam_emul_FF_Fifo3  = gam_emul_FF_Fifo2;
						gam_emul_FF_Fifo2  = gam_emul_FF_Fifo1;
						gam_emul_FF_Fifo1  = gam_emul_VF_Output;
					  gam_emul_VF_Subposition = (gam_emul_VF_Counter >> 7) & 0x7;	/* shift 7 bits						 */
						gam_emul_VerticalFilter(gam_emul_VFilter, gam_emul_NUMBER_OF_TAPS,gam_emul_QUANTIZATION,gam_emul_VF_Subposition,Blt);
						gam_emul_VF_LineNumber++;
						gam_emul_VF_Counter = gam_emul_VF_Counter & 0x3FF;		/* keep only 10-bit fractional part */
					  gam_emul_VF_Counter += Blt->VF_increment;
					  gam_emul_NbNewLines = (gam_emul_VF_Counter>>10);
					}
					TargetAvailability = gam_emul_FlickerFilter(gam_emul_VF_LineNumber,Blt);
					/* 5-taps horizontal n-phases filter, if this line is selected */
					if (TargetAvailability == TRUE)
					{
						/* Horizontal filter */
						gam_emul_HF_Counter = Blt->HF_initial_phase << 7;	/* Set vertical counter initial value, 6.10 */
						/* init the N-pixel delay line that feeds the filter */
						/* (pipeline overhead)							 */
						xs = 0;
						for ( i=0 ; i<gam_emul_NUMBER_OF_TAPS/2 ; i++ )
							gam_emul_GetNewPixel(Blt->S2_width,0,gam_emul_Delay);					/* 0,0 if 5 taps */
						for ( xs=0 ; xs<gam_emul_NUMBER_OF_TAPS/2+1 ; xs++ )
							gam_emul_GetNewPixel(Blt->S2_width,xs,gam_emul_Delay);				/* 0,1,2 if 5 taps */
						xs = gam_emul_NUMBER_OF_TAPS/2+1;											/* xs = 3 if 5 taps */
						x = 0;
						while (x < Blt->T_width)
						{
							gam_emul_HF_Subposition = (gam_emul_HF_Counter >> 7) & 0x7;		/* shift 7 bits						 */
							*(S2Out+x) = gam_emul_HorizontalFilter(gam_emul_HFilter,gam_emul_NUMBER_OF_TAPS,gam_emul_QUANTIZATION,gam_emul_HF_Subposition,gam_emul_Delay,Blt);
							gam_emul_HF_Counter = gam_emul_HF_Counter & 0x3FF;			/* keep only fractional part */
							gam_emul_HF_Counter += Blt->HF_increment;
						  gam_emul_NbPix = (gam_emul_HF_Counter>>10);
							for ( i=0 ; i<gam_emul_NbPix ; i++)
							{
								gam_emul_GetNewPixel(Blt->S2_width,xs,gam_emul_Delay);
								xs++;
							}
							x++;
						}
						gam_emul_NumberOfOutputLines++;
					}
				}
				else
					TargetAvailability = FALSE;

				if (gam_emul_NbNewLines != 0)
					S2Request = TRUE;
				else
					S2Request = FALSE;
				break;
			default:
				break;
		}
		if (gam_emul_NumberOfOutputLines == Blt->T_height)		/* This the last line to be output */
		{												/* so we free all allocated buffers */
            gam_emul_free(Device_p,gam_emul_HFilter);
            gam_emul_free(Device_p,gam_emul_VFilter);
            gam_emul_free(Device_p,gam_emul_Delay);
            gam_emul_free(Device_p,gam_emul_VF_Fifo1);
            gam_emul_free(Device_p,gam_emul_VF_Fifo2);
            gam_emul_free(Device_p,gam_emul_VF_Fifo3);
            gam_emul_free(Device_p,gam_emul_VF_Fifo4);
            gam_emul_free(Device_p,gam_emul_VF_Fifo5);
            gam_emul_free(Device_p,gam_emul_FF_Fifo1);
            gam_emul_free(Device_p,gam_emul_FF_Fifo2);
            gam_emul_free(Device_p,gam_emul_FF_Fifo3);
            gam_emul_free(Device_p,gam_emul_FF_Output);
		}
		*RequestS2Line = S2Request;
		return(TargetAvailability);
	}
	else /* if no resize or flicker filter */
	{
		*RequestS2Line = TRUE;
		for ( x=0 ; x < Blt->S2_width ; x++ )
			*(S2Out+x) = *(S2In+x);
		return(TargetAvailability = TRUE);
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_gam_emul_Newline
 * Parameters: source,width
 * Return : -
 * Description : to get a new line for vertcal filter
 * ----------------------------------------------------------------------------------*/

void gam_emul_Newline (int *source,int width)
{
	int i;
	int *pt_tmp;

	pt_tmp   = gam_emul_VF_Fifo5;												/* push data through FIFOs !! */
	gam_emul_VF_Fifo5 = gam_emul_VF_Fifo4;
	gam_emul_VF_Fifo4 = gam_emul_VF_Fifo3;
	gam_emul_VF_Fifo3 = gam_emul_VF_Fifo2;
	gam_emul_VF_Fifo2 = gam_emul_VF_Fifo1;
	gam_emul_VF_Fifo1 = pt_tmp;
	for ( i=0 ; i<width ; i++ )									/* enter new line in FIFO1 */
		*(gam_emul_VF_Fifo1+i) = *(source + i);
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_VerticalFilter
 * Parameters:  VFilter, nbtaps,quantization,subposition,gam_emul_BlitterParameters
 * Return : -
 * Description : to apply vertical filter
 * ----------------------------------------------------------------------------------*/

void gam_emul_VerticalFilter(int* VFilter,int nbtaps,int quantization,int subposition,gam_emul_BlitterParameters *Blt)
{
	int i;
	int TA,TR,TG,TB;

	for ( i=0 ; i<Blt->S2_width ; i++ )
	{
		if ((Blt->VF_mode) & 1)			/* Filter active on RGB components */
		{
			TR = ((*(gam_emul_VF_Fifo1+i) & 0xFF0000)>>16) * (*(VFilter + nbtaps*subposition + 0))
				 + ((*(gam_emul_VF_Fifo2+i) & 0xFF0000)>>16) * (*(VFilter + nbtaps*subposition + 1))
				 + ((*(gam_emul_VF_Fifo3+i) & 0xFF0000)>>16) * (*(VFilter + nbtaps*subposition + 2))
				 + ((*(gam_emul_VF_Fifo4+i) & 0xFF0000)>>16) * (*(VFilter + nbtaps*subposition + 3))
				 + ((*(gam_emul_VF_Fifo5+i) & 0xFF0000)>>16) * (*(VFilter + nbtaps*subposition + 4));

			TG = ((*(gam_emul_VF_Fifo1+i) & 0xFF00)>>8) * (*(VFilter + nbtaps*subposition + 0))
				 + ((*(gam_emul_VF_Fifo2+i) & 0xFF00)>>8) * (*(VFilter + nbtaps*subposition + 1))
				 + ((*(gam_emul_VF_Fifo3+i) & 0xFF00)>>8) * (*(VFilter + nbtaps*subposition + 2))
				 + ((*(gam_emul_VF_Fifo4+i) & 0xFF00)>>8) * (*(VFilter + nbtaps*subposition + 3))
				 + ((*(gam_emul_VF_Fifo5+i) & 0xFF00)>>8) * (*(VFilter + nbtaps*subposition + 4));

			TB = (*(gam_emul_VF_Fifo1+i) & 0xFF) * (*(VFilter + nbtaps*subposition + 0))
				 + (*(gam_emul_VF_Fifo2+i) & 0xFF) * (*(VFilter + nbtaps*subposition + 1))
				 + (*(gam_emul_VF_Fifo3+i) & 0xFF) * (*(VFilter + nbtaps*subposition + 2))
				 + (*(gam_emul_VF_Fifo4+i) & 0xFF) * (*(VFilter + nbtaps*subposition + 3))
				 + (*(gam_emul_VF_Fifo5+i) & 0xFF) * (*(VFilter + nbtaps*subposition + 4));
			TR = TR >> quantization;
			TG = TG >> quantization;
			TB = TB >> quantization;
			if (TR<0) TR = 0; if (TR>255) TR = 255;
			if (TG<0) TG = 0; if (TG>255) TG = 255;
			if (TB<0) TB = 0; if (TB>255) TB = 255;
		}
		else												/* No filter on RGB components */
		{
			if (subposition <= 3)
			{
				TR = (*(gam_emul_VF_Fifo3+i) & 0xFF0000)>>16;
				TG = (*(gam_emul_VF_Fifo3+i) & 0xFF00)>>8;
				TB =  *(gam_emul_VF_Fifo3+i) & 0xFF;
			}
			else
			{
				TR = (*(gam_emul_VF_Fifo2+i) & 0xFF0000)>>16;
				TG = (*(gam_emul_VF_Fifo2+i) & 0xFF00)>>8;
				TB =  *(gam_emul_VF_Fifo2+i) & 0xFF;
			}
		}
		if ((Blt->VF_mode) & 2)			/* Filter active on alpha component */
		{
			TA = ((*(gam_emul_VF_Fifo1+i) >> 24) & 0xFF) * (*(VFilter + nbtaps*subposition + 0))
				 + ((*(gam_emul_VF_Fifo2+i) >> 24) & 0xFF) * (*(VFilter + nbtaps*subposition + 1))
				 + ((*(gam_emul_VF_Fifo3+i) >> 24) & 0xFF) * (*(VFilter + nbtaps*subposition + 2))
				 + ((*(gam_emul_VF_Fifo4+i) >> 24) & 0xFF) * (*(VFilter + nbtaps*subposition + 3))
				 + ((*(gam_emul_VF_Fifo5+i) >> 24) & 0xFF) * (*(VFilter + nbtaps*subposition + 4));
			TA = TA >> quantization;
			if (TA<0) TA = 0; if (TA>128) TA = 128;
		}
		else												/* No filter on alpha component */
		{
			if (subposition <= 3)
				TA = (*(gam_emul_VF_Fifo3+i) >> 24) & 0xFF;
			else
				TA = (*(gam_emul_VF_Fifo2+i) >> 24) & 0xFF;
		}
		*(gam_emul_VF_Output+i) = (TA<<24) | (TR<<16) | (TG<<8) | TB;
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_FlickerFilter
 * Parameters:  LineNumber,gam_emul_BlitterParameters
 * Return : -
 * Description : to apply flicker filter
 * ----------------------------------------------------------------------------------*/

int gam_emul_FlickerFilter(int LineNumber,gam_emul_BlitterParameters *Blt)
{
	int i;
	int *PTlm1,*PTlp0,*PTlp1;
	int Am1,Rm1,Gm1,Bm1,Ap0,Rp0,Gp0,Bp0,Ap1,Rp1,Gp1,Bp1;
	int TA,TR,TG,TB;
	int dNNm1,dNp1N;
	int FilterLevel;
	int *kF;

	FilterLevel = 0;
	kF = NULL;

	if (Blt->EnaFlickerFilter == 0)				/* Flicker filter is disabled */
	{
		for ( i=0 ; i<Blt->S2_width ; i++ )
			*(gam_emul_FF_Output+i) = *(gam_emul_VF_Output+i);
		return(1);								/* The block is bypassed */
	}
	else										/* Flicker filter is enabled */
	{
		if (LineNumber == 1)
		{
			for ( i=0 ; i<Blt->S2_width ; i++ )
				*(gam_emul_FF_Fifo2+i) = *(gam_emul_FF_Fifo1+i);		/* First line is duplicated */
			return(0);			/* No valid result yet, as we need at least 2 lines */
		}
		else
		{
			if (gam_emul_FF_Flag == 1)
			{
				PTlm1 = gam_emul_FF_Fifo2;
				PTlp0 = gam_emul_FF_Fifo1;
				PTlp1 = gam_emul_FF_Fifo1;		/* Last-line used twice !! */
			}
			else
			{
				PTlm1 = gam_emul_FF_Fifo3;		/* Normal filtering mode */
				PTlp0 = gam_emul_FF_Fifo2;
				PTlp1 = gam_emul_FF_Fifo1;
			}
			/* Let's apply the filter */
			for ( i=0 ; i<Blt->S2_width ; i++ )
			{
				Am1 = (*(PTlm1+i) >> 24) & 0xFF;
				Rm1 = (*(PTlm1+i) & 0xFF0000)>>16;
				Gm1 = (*(PTlm1+i) & 0xFF00)>>8;
				Bm1 =  *(PTlm1+i) & 0xFF;
				Ap0 = (*(PTlp0+i) >> 24) & 0xFF;
				Rp0 = (*(PTlp0+i) & 0xFF0000)>>16;
				Gp0 = (*(PTlp0+i) & 0xFF00)>>8;
				Bp0 =  *(PTlp0+i) & 0xFF;
				Ap1 = (*(PTlp1+i) >> 24) & 0xFF;
				Rp1 = (*(PTlp1+i) & 0xFF0000)>>16;
				Gp1 = (*(PTlp1+i) & 0xFF00)>>8;
				Bp1 =  *(PTlp1+i) & 0xFF;

				/* we approximate luma(N) - luma(N-1) */
				/* luma , 0.25*R + 0.5*G + 0.25*B */
				dNNm1 = ((Rp0>>1) + (Gp0>>0) + (Bp0>>1))
							- ((Rm1>>1) + (Gm1>>0) + (Bm1>>1));
				dNNm1 = abs(dNNm1);
				dNNm1 = dNNm1>>5;			/* -15..+15 */
				/* we approximate luma(N+1) - luma(N) */
				/* luma , 0.25*R + 0.5*G + 0.25*B */
				dNp1N = ((Rp1>>1) + (Gp1>>0) + (Bp1>>1))
							- ((Rp0>>1) + (Gp0>>0) + (Bp0>>1));
				dNp1N = abs(dNp1N);
				dNp1N = dNp1N>>5;			/* -15..+15 */

				switch (Blt->FlickerFilter_mode)
				{
					case 0:					/* We force filter 0 */
					case 3:					/* Reserved : We force filter 0 */
						kF = &gam_emul_FF0[0];
						FilterLevel = 0;
						break;
					case 1:					/* Adaptive filtering / normal mode */
					case 2:					/* Adaptive filtering / test mode */
						if ( (abs(dNNm1)>=gam_emul_FFThreshold23) || (abs(dNp1N)>= gam_emul_FFThreshold23) )
						{
							kF = &gam_emul_FF3[0];
							FilterLevel = 3;
						}
						else
							if ( (abs(dNNm1)>=gam_emul_FFThreshold12) || (abs(dNp1N)>= gam_emul_FFThreshold12) )
							{
								kF = &gam_emul_FF2[0];
								FilterLevel = 2;
							}
							else
								if ( (abs(dNNm1)>=gam_emul_FFThreshold01) || (abs(dNp1N)>= gam_emul_FFThreshold01) )
								{
									kF = &gam_emul_FF1[0];
									FilterLevel = 1;
								}
								else
								{
									kF = &gam_emul_FF0[0];
									FilterLevel = 0;
								}
						break;
				}
				/* Compute filtered target pixel */
				TA = (Am1 * (*kF)
					 +  Ap0 * (*(kF+1))
					 +  Ap1 * (*(kF+2))) >> 7;
				TR = (Rm1 * (*kF)
					 +  Rp0 * (*(kF+1))
					 +  Rp1 * (*(kF+2))) >> 7;
				TG = (Gm1 * (*kF)
					 +  Gp0 * (*(kF+1))
					 +  Gp1 * (*(kF+2))) >> 7;
				TB = (Bm1 * (*kF)
					 +  Bp0 * (*(kF+1))
					 +  Bp1 * (*(kF+2))) >> 7;

				if (TA<0) TA = 0; if (TA>128) TA = 128;
				if (TR<0) TR = 0; if (TR>255) TR = 255;
				if (TG<0) TG = 0; if (TG>255) TG = 255;
				if (TB<0) TB = 0; if (TB>255) TB = 255;

				if (Blt->FlickerFilter_mode == 2)		/* provides an image of the locally applied filter */
				{
					TA = 128;
					switch (FilterLevel)
					{
						case 0:
							TR = TG = TB = 0;
							break;
						case 1:
							TR = 250; TG = TB = 0;
							break;
						case 2:
							TR = TB = 0; TG = 250;
							break;
						case 3:
							TR = TG = TB = 250;
							break;
						default:
							break;
					}
				}
				*(gam_emul_FF_Output+i) = (TA<<24) | (TR<<16) | (TG<<8) | TB;
			}
			if (LineNumber == Blt->T_height)
				gam_emul_FF_Flag = 1;
			return(1);
		}
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_GetNewPixel
 * Parameters: width, x, *delay
 * Return : -
 * Description : to get a new pixel - use in horizontal filter
 * ----------------------------------------------------------------------------------*/

int gam_emul_GetNewPixel(int width,int x,int *delay)
{
	int i;

	/* the delay buffer is handled just like a N-tap shift register (N = number of taps) */
	for ( i=gam_emul_NUMBER_OF_TAPS-1 ; i>0 ; i-- )				/* shift the delay line */
		*(delay+i) = *(delay+i-1);
	if (x>=width)
	{
		*(delay+0) = *(gam_emul_FF_Output + width - 1);
		return(1);
	}
	else
	{
		*(delay+0) = *(gam_emul_FF_Output + x);		/* new pixel in location 0 */
		return(0);
	}
}
/*-----------------------------------------------------------------------------------
 * Function : gam_emul_HorizontalFilter
 * Parameters: Hfilter, nbtaps,  quantization,subposition,delay,gam_emul_BlitterParameters
 * Return : -
 * Description : to apply horizontal filter
 * ----------------------------------------------------------------------------------*/

int gam_emul_HorizontalFilter(int* Hfilter,int nbtaps, int quantization,int subposition,int* delay,gam_emul_BlitterParameters *Blt)
{
	int TA,TR,TG,TB;
	int i;

	if ((Blt->HF_mode) & 1)			/* Filter active on RGB components */
	{
		TR = TG = TB = 0;
		for ( i=0 ; i<nbtaps ; i++ )
		{
			TR += ((*(delay+i) & 0xFF0000)>>16) * (*(Hfilter + nbtaps*subposition + i));
			TG += ((*(delay+i) & 0x00FF00)>> 8) * (*(Hfilter + nbtaps*subposition + i));
			TB += ((*(delay+i) & 0x0000FF)>> 0) * (*(Hfilter + nbtaps*subposition + i));
		}
		TR = TR >> quantization;
		TG = TG >> quantization;
		TB = TB >> quantization;
		if (TR<0) TR = 0; if (TR>255) TR = 255;
		if (TG<0) TG = 0; if (TG>255) TG = 255;
		if (TB<0) TB = 0; if (TB>255) TB = 255;
	}
	else
	{
		if (subposition<=3)
		{
			TR = ((*(delay + nbtaps/2) & 0xFF0000)>>16);
			TG = ((*(delay + nbtaps/2) & 0x00FF00)>> 8);
			TB = ((*(delay + nbtaps/2) & 0x0000FF)>> 0);
		}
		else
		{
			TR = ((*(delay + (nbtaps/2) - 1) & 0xFF0000)>>16);
			TG = ((*(delay + (nbtaps/2) - 1) & 0x00FF00)>> 8);
			TB = ((*(delay + (nbtaps/2) - 1) & 0x0000FF)>> 0);
		}
	}
	if ((Blt->HF_mode) & 2)			/* Filter active on alpha component */
	{
		TA = 0;
		for ( i=0 ; i<nbtaps ; i++ )
			TA += ((*(delay+i) >> 24) & 0xFF) * (*(Hfilter + nbtaps*subposition + i));
		TA = TA >> quantization;
		if (TA<0) TA = 0; if (TA>128) TA = 128;
	}
	else
	{
		if (subposition<=3)
			TA = ((*(delay + nbtaps/2) >> 24) & 0xFF);
		else
			TA = ((*(delay + (nbtaps/2) - 1) >> 24) & 0xFF);
	}
	return((TA<<24) | (TR<<16) | (TG<<8) | TB);
}


/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ReadBlitter42XPixel
 * Parameters: x, y,gam_emul_BlitterParameters , MASK
 * Return : -
 * Description : to read luma or chroma from pixel format 4:2:X MB
 * ----------------------------------------------------------------------------------*/

int gam_emul_ReadBlitter42XPixel(int x,int y,gam_emul_BlitterParameters *Blt,int MASK)
{
	int AbsoluteX,AbsoluteY;
	int luma,cr_cb;

	luma = cr_cb = 0;

	if ((MASK & gam_emul_LUMA) != 0)
	{
		/* LUMA : (S1) */
		if (Blt->S1_hscan == 0)
			AbsoluteX = (x + Blt->S1_x);		/* Given parameter = x left */
		else
			AbsoluteX = (Blt->S1_x - x);		/* Given parameter = x right */
		if (Blt->S1_vscan == 0)
			AbsoluteY = (y + Blt->S1_y);		/* Given parameter = y top */
		else
			AbsoluteY = (Blt->S1_y - y);		/* Given parameter = y bottom */
		luma = gam_emul_Read42XLuma(AbsoluteX,AbsoluteY,Blt->S1_ba,Blt->S1_pitch);
	}

	if ((MASK & gam_emul_CHROMA) != 0)
	{
		if(((Blt->ALU_rop == 7) && (Blt->ALU_opcode == 1)) || (Blt->ALU_opcode == 6)) /* concatenation S1/S2 or operation OR */
		{
			/* CHROMA : (S2) */
			if (Blt->S2_hscan == 0)
				AbsoluteX = (x + 2*Blt->S2_x);		/* Given parameter = x left */
			else
				AbsoluteX = (2*Blt->S2_x - x);		/* Given parameter = x right */
		}
		else
		{
			/* CHROMA : (S2) */
			if (Blt->S2_hscan == 0)
				AbsoluteX = (x + Blt->S2_x);		/* Given parameter = x left */
			else
				AbsoluteX = (Blt->S2_x - x);		/* Given parameter = x right */
		}

		if (Blt->S2_vscan == 0)
				AbsoluteY = (y + Blt->S2_y);		/* Given parameter = y top */
			else
				AbsoluteY = (Blt->S2_y - y);		/* Given parameter = y bottom */

		if ((Blt->S2_format) == gam_emul_TYPE_YCbCr420MB)
		{
			if (Blt->S2_chromalinerepeat == 1)
				cr_cb = gam_emul_Read42XCbCr(AbsoluteX>>1,AbsoluteY>>1,Blt->S2_ba,Blt->S2_pitch);
			else
				cr_cb = gam_emul_Read42XCbCr(AbsoluteX>>1,AbsoluteY,Blt->S2_ba,Blt->S2_pitch);
		}
		else	/* 4:2:2 MB */
			cr_cb = gam_emul_Read42XCbCr(AbsoluteX>>1,AbsoluteY,Blt->S2_ba,Blt->S2_pitch);
	}
	return(cr_cb | (luma<<8));
}
/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ReadBlitterHDpipPixel
 * Parameters: x, y,gam_emul_BlitterParameters , MASK
 * Return : -
 * Description : to read luma or chroma from pixel format 4:2:X MB
 * ----------------------------------------------------------------------------------*/

int gam_emul_ReadBlitterHDpipPixel(int x,int y,gam_emul_BlitterParameters *Blt,int MASK)
{
	int AbsoluteX,AbsoluteY;
	int luma,cr_cb;

	luma = cr_cb = 0;

	if ((MASK & gam_emul_LUMA) != 0)
	{
		/* LUMA : (S1) */
		if (Blt->S1_hscan == 0)
			AbsoluteX = (x + Blt->S1_x);		/* Given parameter = x left */
		else
			AbsoluteX = (Blt->S1_x - x);		/* Given parameter = x right */
		if (Blt->S1_vscan == 0)
			AbsoluteY = (y + Blt->S1_y);		/* Given parameter = y top */
		else
			AbsoluteY = (Blt->S1_y - y);		/* Given parameter = y bottom */
		luma = gam_emul_ReadHDpipLuma(AbsoluteX,AbsoluteY,Blt->S1_ba,Blt->S1_pitch);
	}

	if ((MASK & gam_emul_CHROMA) != 0)
	{
		if(((Blt->ALU_rop == 7) && (Blt->ALU_opcode == 1)) || (Blt->ALU_opcode == 6)) /* concatenation S1/S2 or operation OR */
		{
			/* CHROMA : (S2) */
			if (Blt->S2_hscan == 0)
				AbsoluteX = (x + 2*Blt->S2_x);		/* Given parameter = x left */
			else
				AbsoluteX = (2*Blt->S2_x - x);		/* Given parameter = x right */
		}
		else
		{
			/* CHROMA : (S2) */
			if (Blt->S2_hscan == 0)
				AbsoluteX = (x + Blt->S2_x);		/* Given parameter = x left */
			else
				AbsoluteX = (Blt->S2_x - x);		/* Given parameter = x right */
		}

		if (Blt->S2_vscan == 0)
			AbsoluteY = (y + Blt->S2_y);		/* Given parameter = y top */
		else
			AbsoluteY = (Blt->S2_y - y);		/* Given parameter = y bottom */

		if (Blt->S2_chromalinerepeat == 1)
			cr_cb = gam_emul_ReadHDpipCbCr(AbsoluteX>>1,AbsoluteY>>1,Blt->S2_ba,Blt->S2_pitch);
		else
			cr_cb = gam_emul_ReadHDpipCbCr(AbsoluteX>>1,AbsoluteY,Blt->S2_ba,Blt->S2_pitch);

	}

	return(cr_cb | (luma<<8));
}
/*-----------------------------------------------------------------------------------
 * Function : gam_emul_GetBpp
 * Parameters: Format
 * Return : int : number of bits in function of format
 * Description :
 * ----------------------------------------------------------------------------------*/

int gam_emul_GetBpp(int Format)
{
	int bpp;

	switch (Format)
	{
		case gam_emul_TYPE_ARGB8888:
		case gam_emul_TYPE_AYCbCr8888:
			bpp = 32;
			break;
		case gam_emul_TYPE_RGB888:
		case gam_emul_TYPE_ARGB8565:
		case gam_emul_TYPE_YCbCr888:
			bpp = 24;
			break;
		case gam_emul_TYPE_RGB565:
		case gam_emul_TYPE_ARGB1555:
		case gam_emul_TYPE_ARGB4444:
		case gam_emul_TYPE_ACLUT88:
		case gam_emul_TYPE_YCbCr422R:
			bpp = 16;
			break;
		case gam_emul_TYPE_ACLUT44:
		case gam_emul_TYPE_CLUT8:
		case gam_emul_TYPE_A8:
		case gam_emul_TYPE_BYTE:
			bpp = 8;
			break;
		case gam_emul_TYPE_CLUT4:
			bpp = 4;
			break;
		case gam_emul_TYPE_CLUT2:
			bpp = 2;
			break;
		case gam_emul_TYPE_CLUT1:
		case gam_emul_TYPE_A1:
			bpp = 1;
			break;
		default:
			bpp = 0;
			break;
	}
	return(bpp);
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_SearchXYL
 * Parameters: position x,y, gam_emul_BlitterParameters, length, color
 * Return : int 1 if x,y are found else 0
 * Description : search x,y and return 1 if value is correct
 * ----------------------------------------------------------------------------------*/

int gam_emul_SearchXYL(int x, int y,gam_emul_BlitterParameters *Blt, int *length_P, int *color_P)
{
	int countIns;
	int X, Y;
	int format;
	int find = 0;

	int alignedByteAddress = Blt->XYL_xyp & 0xFFFFFFFC;

	format = Blt->XYL_format;


	for (countIns = 0 ; countIns <= Blt->XYL_buff_format; countIns++)
	{
            Y =  gam_emul_readInt((int*)(gam_emul_BLTMemory + alignedByteAddress + 16 * countIns + 4));
			Y = gam_emul_swap_32 (Y, (Blt->pkz >> 5) &0x1);
            X =  gam_emul_readInt((int*)(gam_emul_BLTMemory + alignedByteAddress + 16 * countIns ));
			X = gam_emul_swap_32 (X, (Blt->pkz >> 5) &0x1);

			if ( (x == X) && ( y==Y ))
			{
				/* to recover L */

				if ((format == gam_emul_FORMATXYLu) || (format == gam_emul_FORMATXYLC)) /* XYLu or XYLC */
				{
                    *length_P = gam_emul_readInt((int*)(gam_emul_BLTMemory + alignedByteAddress + 16 * countIns + 8));
					*length_P = gam_emul_swap_32 (*length_P, (Blt->pkz >> 5) &0x1);
				}
				else
				{
					*length_P = 1;
					*length_P = gam_emul_swap_32 (*length_P, (Blt->pkz >> 5) &0x1);
				}

				/* to recover C */

				if ( ( format == gam_emul_FORMATXYuC) || (format == gam_emul_FORMATXYLC))
				{
                    *color_P = gam_emul_readInt((int*)(gam_emul_BLTMemory + alignedByteAddress + 16 * countIns + 12));
					*color_P = gam_emul_swap_32 (*color_P, (Blt->pkz >> 5) &0x1);
				}
				else
				{
					*color_P = Blt->S2_colorfill;
					*color_P = gam_emul_swap_32 (*color_P, (Blt->pkz >> 5) &0x1);
				}
				find = 1;
			}
	}
	return (find);

}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_XYLBlitterOperation
 * Parameters: gam_emul_BlitterParameters
 * Return : -
 * Description : target is filled with color
 * ----------------------------------------------------------------------------------*/

void gam_emul_XYLBlitterOperation(gam_emul_BlitterParameters *Blt)
{
	int X,Y,length, color;
	int nbSubInsCounter;
	int counterLength;
	int alignedByteAddress;
	int newGlobalAlpha;

	int Dummy;
	int alpha;
	int TMask;
	int S1A;
	int S2A;
	int TA;
	int TB;
    int SaveUnformattedS1A;


	alignedByteAddress = Blt->XYL_xyp & 0xFFFFFFFC;
	nbSubInsCounter = 0;
	newGlobalAlpha = 0;

    while (nbSubInsCounter < Blt->XYL_buff_format)
	{
        Y =  gam_emul_readInt((int*)(gam_emul_BLTMemory + alignedByteAddress + 16 * nbSubInsCounter + 4 ));
		Y = gam_emul_swap_32 (Y, (Blt->pkz >> 5) &0x1);
        X =  gam_emul_readInt((int*)(gam_emul_BLTMemory + alignedByteAddress + 16 * nbSubInsCounter ));
		X = gam_emul_swap_32 (X, (Blt->pkz >> 5) &0x1);

        /*  to recover L */
        if ((Blt->XYL_format == gam_emul_FORMATXYLu) || (Blt->XYL_format == gam_emul_FORMATXYLC)) /* XYLu or XYLC */
        {
            length = gam_emul_readInt((int*)(gam_emul_BLTMemory + alignedByteAddress + 16 *  nbSubInsCounter + 8));
            length = gam_emul_swap_32 (length, (Blt->pkz >> 5) &0x1);
        }
        else
        {
            length = 1;
            length = gam_emul_swap_32 (length, (Blt->pkz >> 5) &0x1);
        }

        /* to recover C */

        if ( ( Blt->XYL_format == gam_emul_FORMATXYuC) || (Blt->XYL_format == gam_emul_FORMATXYLC))
        {
            color = gam_emul_readInt((int*)(gam_emul_BLTMemory + alignedByteAddress + 16 *  nbSubInsCounter + 12));
            color = gam_emul_swap_32 (color, (Blt->pkz >> 5) &0x1);
        }
        else
        {
            color = Blt->S2_colorfill;
            color = gam_emul_swap_32 (color, (Blt->pkz >> 5) &0x1);
        }



		counterLength = 0;

		while (counterLength < length) /* only one pixel is processed */
		{
			/* process of differents XYL mode */

			if ((Blt->ALU_opcode) == 9) /* CLIPMASK IN XYL mode, logical operation	*/
			{
				gam_emul_FetchPixel(gam_emul_SRC2, Blt,&S2A,&Dummy, X + counterLength, Y);
				if ((((S2A) >> 31) & 0x1) == 0x1)
				{
					S2A= color;
				}
				else
				{
					S2A= 0xFF000000 | color; /* FF -> flag for the second pass */
				}
			}
			else
			{
					if (((Blt->ALU_opcode) == 10) || ((Blt->ALU_opcode) == 11))/* CLIPMASK IN XYL mode, scr2 unweighted/weighted	*/
					{

						gam_emul_FetchPixel(gam_emul_SRC2, Blt,&alpha ,&Dummy, X + counterLength, Y); /* format A1, A8 - source 2 */

						newGlobalAlpha = (((((alpha >> 24) & 0x0FF) * Blt->ALU_global_alpha) >> 6) + 1) >> 1;
						S2A = color;
					}
					else /* other mode XYL */
					{
						S2A = color;
					}
			}


			/* Pixel formatter source 1 */

			gam_emul_FetchPixel(gam_emul_SRC1, Blt, &S1A, &SaveUnformattedS1A, X + counterLength, Y);

			/* ALU */

			gam_emul_ALUOperatorXYL(Blt, &S1A, &S2A, &TA, newGlobalAlpha);

			/* mask is clear */

			TMask = 0 ;

			/* write enable - rectangular clipping */

			if (Blt->EnaRectangularClipping == 1)
				gam_emul_SetWriteEnableMaskXYL(Blt,&TMask,gam_emul_RECT_CLIPPING,&Dummy,X + counterLength,Y);

			/* write enable - color key */

			if (Blt->EnaColorKey == 1)
				gam_emul_SetWriteEnableMaskXYL(Blt,&TMask,gam_emul_COLOR_KEY,&S1A,0,0);

			/* conversion YCbCr <> RGB */

			gam_emul_ColorSpaceConverterXYL(Blt,&TA,&TB);

			/* output formatter on target */

			gam_emul_WriteTargetPixel(Blt,&TB,&SaveUnformattedS1A,&TMask,X + counterLength,Y); /* Include PlaneMask & Dither */


			counterLength++;

		}
		nbSubInsCounter++;
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_FetchPixel
 * Parameters: Source,gam_emul_BlitterParameters ,SXOut,SaveBuffer,x, y
 * Return : SXout is formatted ou and SaveBuffer is unformatted out
 * Description : read the pixel (x,y), clip alpha, convert the pixel to the internal 32-bit
 pixel (pixel formatter)
 * ----------------------------------------------------------------------------------*/

void gam_emul_FetchPixel(int source, gam_emul_BlitterParameters *Blt,int *SXOut,int *SaveBuffer,int x, int y)
{
	int a,r,g,b;
	int InputFormat,RGBExtendMode;
	/*int format = Blt->XYL_format;*/

	/* Source 1*/

	if (source == gam_emul_SRC1)
	{
		InputFormat = Blt->S1_format;

		*SXOut = gam_emul_ReadBlitterPixel(x,y,Blt,gam_emul_SRC1);
		*SaveBuffer = *SXOut;		/* for the PlaneMask feature  */

		RGBExtendMode = Blt->S1_RGBExtendMode;
	}
	else /* Source 2 */
	{
		InputFormat = Blt->S2_format;

		*SXOut = gam_emul_ReadBlitterPixel(x,y,Blt,gam_emul_SRC2);
		*SaveBuffer = *SXOut;		/* for the PlaneMask feature  */

		RGBExtendMode = Blt->S2_RGBExtendMode;
	}

	/* clip alpha for plane mask */

	/*
	switch (InputFormat)
	{
		case gam_emul_TYPE_ARGB8888:
			a = (*SaveBuffer >> 24) & 0xFF;
			*SaveBuffer &= 0xFFFFFF;
			if ((a > 128) && (a < 255))
				a = 128;
			*SaveBuffer |= (a<<24);
			break;
		case gam_emul_TYPE_ARGB8565:
			a = (*SaveBuffer >> 16) & 0xFF;
			*SaveBuffer &= 0xFFFF;
			if ((a > 128) && (a < 255))
				a = 128;
			*SaveBuffer |= (a<<16);
			break;
		case gam_emul_TYPE_ACLUT88:
			a = (*SaveBuffer >> 8) & 0xFF;
			*SaveBuffer &= 0xFF;
			if ((a > 128) && (a < 255))
				a = 128;
			*SaveBuffer |= (a<<8);
			break;
		default:
			break;
	}
	*/
	/* We convert the pixel to the internal 32-bit pixel (bus formatter) only SCR1*/
	switch (InputFormat)
	{
		case gam_emul_TYPE_YCbCr422R:
			if ((Blt->BLT_instruction & 0x18) != 8)			/* disabled or color fill */
			{
				if ((((Blt->T_x & 1) == 0) && ((x&1) == 1))
					|| (((Blt->T_x & 1) == 1) && ((x&1) == 0)))
					*SXOut = (*SXOut >> 8) & 0xFF;	/* should be a luma-only pixel */
			}
			break;
		case gam_emul_TYPE_RGB888:
		case gam_emul_TYPE_YCbCr888:
			*SXOut |= 0x80000000;			/* add opaque alpha channel */
			break;
		case gam_emul_TYPE_ARGB8565:
			a = (*SXOut >> 16) & 0xFF;
			if ( ((Blt->S1_alphaRange == 0x1) && (source == gam_emul_SRC1))
					|| ( (Blt->S2_alphaRange == 0x1) && (source == gam_emul_SRC2)))
			{
					a = (a + 1) >>1; /* to have a range between 0...128 */
			}
			r = ((*SXOut & 0xF800) >> 8) & 0xFF;
			g = ((*SXOut & 0x07E0) >> 3) & 0xFF;
			b = ((*SXOut & 0x001F) << 3) & 0xFF;
			if (RGBExtendMode == 0)
			{
				r = r | (r>>5);					/* Report the MSBs on LSBs */
				g = g | (g>>6);
				b = b | (b>>5);
			}
			*SXOut = (a<<24) | (r<<16) | (g<<8) | b;
			break;
		case gam_emul_TYPE_AYCbCr8888:
		case gam_emul_TYPE_ARGB8888:							/* ok */
			if ( ((Blt->S1_alphaRange == 0x1) && (source == gam_emul_SRC1))
					|| ( (Blt->S2_alphaRange == 0x1) && (source == gam_emul_SRC2)))
			{
				a = (*SXOut >> 24) & 0xFF;
				*SXOut &= 0xFFFFFF;
				a = (a + 1) >>1; /* to have a range between 0...128 */
				*SXOut |= (a<<24);
			}
			break;
		case gam_emul_TYPE_RGB565:
			a = 128;											/* add opaque alpha channel */
			r = ((*SXOut & 0xF800) >> 8) & 0xFF;
			g = ((*SXOut & 0x07E0) >> 3) & 0xFF;
			b = ((*SXOut & 0x001F) << 3) & 0xFF;
			if (RGBExtendMode == 0)
			{
				r = r | (r>>5);									/* Report the MSBs on LSBs */
				g = g | (g>>6);
				b = b | (b>>5);
			}
			*SXOut = (a<<24) | (r<<16) | (g<<8) | b;
			break;
		case gam_emul_TYPE_ARGB1555:
			a = ((*SXOut & 0x8000) >> 8) & 0xFF;
			r = ((*SXOut & 0x7C00) >> 7) & 0xFF;
			g = ((*SXOut & 0x03E0) >> 2) & 0xFF;
			b = ((*SXOut & 0x001F) << 3) & 0xFF;
			if (RGBExtendMode == 0)
			{
				r = r | (r>>5);									/* Report the MSBs on LSBs */
				g = g | (g>>5);
				b = b | (b>>5);
			}
			*SXOut = (a<<24) | (r<<16) | (g<<8) | b;
			break;
		case gam_emul_TYPE_ARGB4444:
			a = ((*SXOut & 0xF000) >> 9) & 0xFF;
			if (a==0x78)
				a = 128;
			else
				if (a!=0)
					a += 4;
			r = ((*SXOut & 0x0F00) >> 4) & 0xFF;
			g = ((*SXOut & 0x00F0) >> 0) & 0xFF;
			b = ((*SXOut & 0x000F) << 4) & 0xFF;
			if (RGBExtendMode == 0)
			{
				r = r | (r>>4);															/* Report the MSBs on LSBs */
				g = g | (g>>4);
				b = b | (b>>4);
			}
			*SXOut = (a<<24) | (r<<16) | (g<<8) | b;
			break;
		case gam_emul_TYPE_ACLUT88:
			a = ((*SXOut & 0xFF00) >> 8) & 0xFF;
			if ( ((Blt->S1_alphaRange == 0x1) && (source == gam_emul_SRC1))
					|| ( (Blt->S2_alphaRange == 0x1) && (source == gam_emul_SRC2)))
			{
					a = (a + 1) >>1; /* to have a range between 0...128 */
			}
			*SXOut &= 0xFF;
			*SXOut |= (a<<24);
			break;
		case gam_emul_TYPE_ACLUT44:
			a = ((*SXOut & 0xF0) >> 1) & 0xFF; /* multiplied by 8 - if alpha 0 -> 0
												   if alpha = 15 -> 128 else alpha is included between 8 + 4 (12) at 14*8 +4 (116)*/
			if (a==0x78) /* 15 is opaque then alpha = 128  */
				a = 128;
			else
				if (a!=0) /* a = 0 is unaltered */
					a += 4;
			*SXOut &= 0xF;
			*SXOut |= (a<<24);
			break;
		case gam_emul_TYPE_A8:
			a = *SXOut & 0xFF;
			if ( ((Blt->S1_alphaRange == 0x1) && (source == gam_emul_SRC1))
					|| ( (Blt->S2_alphaRange == 0x1) && (source == gam_emul_SRC2)))
			{
					a = (a + 1) >>1; /* to have a range between 0...128 */
			}
			*SXOut = (a<<24);
			break;
		case gam_emul_TYPE_A1:
			a = *SXOut & 0x1;
			*SXOut = (a<<31);
			break;
		case gam_emul_TYPE_CLUT8:										/* ok */
		case gam_emul_TYPE_CLUT4:
		case gam_emul_TYPE_CLUT2:
		case gam_emul_TYPE_CLUT1:
		case gam_emul_TYPE_BYTE:
			break;
		default:
			break;
	}
}


/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ALUOperatorXYL
 * Parameters: gam_emul_BlitterParameters, S1In, S2In, TOut, newGlobalAlpha;
 * Return : -
 * Description : ALU operators in mode XYL
 * ----------------------------------------------------------------------------------*/

void gam_emul_ALUOperatorXYL(gam_emul_BlitterParameters *Blt,int *S1In,int *S2In,int *TOut, int newGlobalAlpha)
{

	int S1a,S1r,S1g,S1b;
	int S2a,S2r,S2g,S2b;
	int Ta,Tr,Tg,Tb;

	switch (Blt->ALU_opcode)
	{
		case 0:		/* BYPASS S1 */
			*TOut = *S1In;
			break;
		case 1:		/* Logical operator */
			switch (Blt->ALU_rop)
			{
				case 0:		/* result = all 0 */
					*TOut = 0;
					break;
				case 1:		/* result = S2 AND S1 */
					*TOut = *S2In & *S1In;
					break;
				case 2:		/* result = S2 AND (NOT S1) */
					*TOut = *S2In & (~(*S1In));
					break;
				case 3:		/* result = S2 */
						*TOut = *S2In;
					break;
				case 4:		/* result = (NOT S2)  AND S1 */
					*TOut = (~(*S2In)) & *S1In;
					break;
				case 5:		/* result =  S1 */
					*TOut = *S1In;
					break;
				case 6:		/* result = S2 XOR S1 */
					*TOut = *S2In ^ *S1In;
					break;
				case 7:		/* result = S2 OR S1 */
					*TOut = *S2In | *S1In;
					break;
				case 8:		/* result = (NOT S2)  AND (NOT S1) */
					*TOut = (~(*S2In)) & (~(*S1In));
					break;
				case 9:		/* result = (NOT S2)  XOR S1 */
					*TOut = (~(*S2In)) ^ *S1In;
					break;
				case 10:	/* result = (NOT S1) */
					*TOut = (~(*S1In));
					break;
				case 11:	/* result = S2 OR (NOT S1) */
					*TOut = *S2In | (~(*S1In));
					break;
				case 12:	/* result = (NOT S2) */
					*TOut = (~(*S2In));
					break;
				case 13:	/* result = (NOT S2)  OR S1 */
					*TOut = (~(*S2In)) | *S1In;
					break;
				case 14:	/* result = (NOT S2)  OR (NOT S1) */
					*TOut = (~(*S2In)) | (~(*S1In));
					break;
				case 15:	/* result = all 1 */
					*TOut = (int) 0xFFFFFFFF;
					break;
				default:
					break;
			}
			break;
		case 2:		/* BLENDING S1/S2, S2 unweighted */
			S1a = (*S1In >> 24) & 0xFF;
			S1r = (*S1In >> 16) & 0xFF;
			S1g = (*S1In >> 8) & 0xFF;
			S1b =  *S1In & 0xFF;

			S2a = (*S2In >> 24) & 0xFF;
			S2r = (*S2In >> 16) & 0xFF;
			S2g = (*S2In >> 8) & 0xFF;
			S2b =  *S2In & 0xFF;

			S2a = (((S2a * Blt->ALU_global_alpha) >> 6) + 1) >> 1;

			Ta = 128 - (((((128 - S1a) * (128 - S2a)) >> 6) + 1) >> 1);
			Tr = ((((S2a * S2r) + (128 - S2a) * S1r) >> 6) + 1) >> 1;
			Tg = ((((S2a * S2g) + (128 - S2a) * S1g) >> 6) + 1) >> 1;
			Tb = ((((S2a * S2b) + (128 - S2a) * S1b) >> 6) + 1) >> 1;

			if (Ta<0) Ta = 0; if (Ta>128) Ta = 128;
			if (Tr<0) Tr = 0; if (Tr>255) Tr = 255;
			if (Tg<0) Tg = 0; if (Tg>255) Tg = 255;
			if (Tb<0) Tb = 0; if (Tb>255) Tb = 255;
			*TOut = (Ta<<24) | (Tr<<16) | (Tg<<8) | Tb;
			break;
		case 3:		/* BLENDING S1/S2, S2 already weighted */
			S1a = (*S1In >> 24) & 0xFF;
			S1r = (*S1In >> 16) & 0xFF;
			S1g = (*S1In >> 8) & 0xFF;
			S1b =  *S1In & 0xFF;

			S2a = (*S2In >> 24) & 0xFF;
			S2r = (*S2In >> 16) & 0xFF;
			S2g = (*S2In >> 8) & 0xFF;
			S2b =  *S2In & 0xFF;

			S2a = (((S2a * Blt->ALU_global_alpha) >> 6) + 1) >> 1;

			Ta = 128 - (((((128 - S1a) * (128 - S2a)) >> 6) + 1) >> 1);
			Tr = ((((Blt->ALU_global_alpha * S2r) + (128 - S2a) * S1r) >> 6) + 1) >> 1;
			Tg = ((((Blt->ALU_global_alpha * S2g) + (128 - S2a) * S1g) >> 6) + 1) >> 1;
			Tb = ((((Blt->ALU_global_alpha * S2b) + (128 - S2a) * S1b) >> 6) + 1) >> 1;
			if (Ta<0) Ta = 0; if (Ta>128) Ta = 128;
			if (Tr<0) Tr = 0; if (Tr>255) Tr = 255;
			if (Tg<0) Tg = 0; if (Tg>255) Tg = 255;
			if (Tb<0) Tb = 0; if (Tb>255) Tb = 255;
			*TOut = (Ta<<24) | (Tr<<16) | (Tg<<8) | Tb;
			break;
		case 7:		/* BYPASS S2 */
			*TOut = *S2In;
			break;
		case 9:			/* CLIPMASK IN XYL mode, logical operation	*/
			if (((*S2In >> 24) & 0x0FF) == 0x0FF)
			{
				*TOut = *S1In;
			}
			else
			{
				switch (Blt->ALU_rop)
				{
					case 0:		/* result = all 0 */
						*TOut = 0;
						break;
					case 1:		/* result = S2 AND S1 */
						*TOut = *S2In & *S1In;
						break;
					case 2:		/* result = S2 AND (NOT S1) */
						*TOut = *S2In & (~(*S1In));
						break;
					case 3:		/* result = S2 */
						*TOut = *S2In;
						break;
					case 4:		/* result = (NOT S2)  AND S1 */
						*TOut = (~(*S2In)) & *S1In;
						break;
					case 5:		/* result =  S1 */
						*TOut = *S1In;
						break;
					case 6:		/* result = S2 XOR S1 */
						*TOut = *S2In ^ *S1In;
						break;
					case 7:		/* result = S2 OR S1 */
						*TOut = *S2In | *S1In;
						break;
					case 8:		/* result = (NOT S2)  AND (NOT S1) */
						*TOut = (~(*S2In)) & (~(*S1In));
						break;
					case 9:		/* result = (NOT S2)  XOR S1 */
						*TOut = (~(*S2In)) ^ *S1In;
						break;
					case 10:	/* result = (NOT S1) */
						*TOut = (~(*S1In));
						break;
					case 11:	/* result = S2 OR (NOT S1) */
						*TOut = *S2In | (~(*S1In));
						break;
					case 12:	/* result = (NOT S2) */
						*TOut = (~(*S2In));
						break;
					case 13:	/* result = (NOT S2)  OR S1 */
						*TOut = (~(*S2In)) | *S1In;
						break;
					case 14:	/* result = (NOT S2)  OR (NOT S1) */
						*TOut = (~(*S2In)) | (~(*S1In));
						break;
					case 15:	/* result = all 1 */
						*TOut = (int) 0xFFFFFFFF;
						break;
					default:
						break;
				}
			}
			break;
		case 10:		/* CLIPMASK IN XYL mode, scr2 unweighted	*/
			S1a = (*S1In >> 24) & 0xFF;
			S1r = (*S1In >> 16) & 0xFF;
			S1g = (*S1In >> 8) & 0xFF;
			S1b =  *S1In & 0xFF;

			S2a = (*S2In >> 24) & 0xFF;
			S2r = (*S2In >> 16) & 0xFF;
			S2g = (*S2In >> 8) & 0xFF;
			S2b =  *S2In & 0xFF;

			S2a = (((S2a * newGlobalAlpha) >> 6) + 1) >> 1;

			Ta = 128 - (((((128 - S1a) * (128 - S2a)) >> 6) + 1) >> 1);
			Tr = ((((S2a * S2r) + (128 - S2a) * S1r) >> 6) + 1) >> 1;
			Tg = ((((S2a * S2g) + (128 - S2a) * S1g) >> 6) + 1) >> 1;
			Tb = ((((S2a * S2b) + (128 - S2a) * S1b) >> 6) + 1) >> 1;

			if (Ta<0) Ta = 0; if (Ta>128) Ta = 128;
			if (Tr<0) Tr = 0; if (Tr>255) Tr = 255;
			if (Tg<0) Tg = 0; if (Tg>255) Tg = 255;
			if (Tb<0) Tb = 0; if (Tb>255) Tb = 255;
			*TOut = (Ta<<24) | (Tr<<16) | (Tg<<8) | Tb;
			break;
		case 11:		/* CLIPMASK IN XYL mode, scr2 weighted		*/
			S1a = (*S1In >> 24) & 0xFF;
			S1r = (*S1In >> 16) & 0xFF;
			S1g = (*S1In >> 8) & 0xFF;
			S1b =  *S1In & 0xFF;

			S2a = (*S2In >> 24) & 0xFF;
			S2r = (*S2In >> 16) & 0xFF;
			S2g = (*S2In >> 8) & 0xFF;
			S2b =  *S2In & 0xFF;


			S2a = (((S2a * newGlobalAlpha) >> 6) + 1) >> 1;

			Ta = 128 - (((((128 - S1a) * (128 - S2a)) >> 6) + 1) >> 1);
			Tr = (((( newGlobalAlpha * S2r) + (128 - S2a) * S1r) >> 6) + 1) >> 1;
			Tg = (((( newGlobalAlpha * S2g) + (128 - S2a) * S1g) >> 6) + 1) >> 1;
			Tb = (((( newGlobalAlpha * S2b) + (128 - S2a) * S1b) >> 6) + 1) >> 1;
			if (Ta<0) Ta = 0; if (Ta>128) Ta = 128;
			if (Tr<0) Tr = 0; if (Tr>255) Tr = 255;
			if (Tg<0) Tg = 0; if (Tg>255) Tg = 255;
			if (Tb<0) Tb = 0; if (Tb>255) Tb = 255;
			*TOut = (Ta<<24) | (Tr<<16) | (Tg<<8) | Tb;
			break;
		default:
			break;
	}
}
/*-----------------------------------------------------------------------------------
 * Function : gam_emul_WriteTargetPixel
 * Parameters: gam_emul_BlitterParameters, TIn, S1In, TMask, y
 * Return : -
 * Description : to format and to write in target buffer
 * ----------------------------------------------------------------------------------*/

void gam_emul_WriteTargetPixel(gam_emul_BlitterParameters *Blt,int *TIn,int *S1In,int *TMask,int x, int y)
{
	int a,r,g,b,index;
	int DitherPattern;

	/* control the PlaneMask */

	switch (Blt->T_format)
	{
		case gam_emul_TYPE_ARGB8888:		/* 32 bpp */
		case gam_emul_TYPE_AYCbCr8888:
			Blt->PlaneMask &= 0xFFFFFFFF;
			break;
		case gam_emul_TYPE_YCbCr888:		/* 24 bpp */
		case gam_emul_TYPE_RGB888:
		case gam_emul_TYPE_ARGB8565:
			Blt->PlaneMask &= 0xFFFFFF;
			break;
		case gam_emul_TYPE_RGB565:		/* 16 bpp */
		case gam_emul_TYPE_ARGB1555:
		case gam_emul_TYPE_ARGB4444:
		case gam_emul_TYPE_ACLUT88:
			Blt->PlaneMask &= 0xFFFF;
			break;
		case gam_emul_TYPE_A8:			/* 8 bpp */
		case gam_emul_TYPE_ACLUT44:
		case gam_emul_TYPE_CLUT8:
		case gam_emul_TYPE_BYTE:
			Blt->PlaneMask &= 0xFF;
			break;
		case gam_emul_TYPE_CLUT4:		/* 4 bpp */
				Blt->PlaneMask &= 0xF;
			break;
		case gam_emul_TYPE_CLUT2:		/* 2 bpp */
				Blt->PlaneMask &= 0x3;
			break;
		case gam_emul_TYPE_CLUT1:		/* 1 bpp */
		case gam_emul_TYPE_A1:
			Blt->PlaneMask &= 0x1;
			break;
		default:
			break;
	}

	/* We convert the internal 32-bit pixel buffer into a target memory format pixel buffer */

	if (Blt->T_dither == 1)		/* spacial dithering on RGB components ? */
	/* dither bit 1 = x0^y0 */
	/* dither bit 0 = y0 */
		DitherPattern = (y&1) + (((x&1)^(y&1))<<1);
	else				/* normal rounding  (0.5) */
		DitherPattern = 2; /* round off 1/2*/
	switch (Blt->T_format)
	{
		case gam_emul_TYPE_YCbCr422R:					/* ok */
			break;
		case gam_emul_TYPE_YCbCr888:
		case gam_emul_TYPE_RGB888:
			*TIn &= 0x00FFFFFF;			/* reset alpha channel */
			break;
		case gam_emul_TYPE_ARGB8565:
			a = (*TIn >> 24) & 0xFF;
			if (Blt->T_alphaRange == 0x1)
			{
				if ( a != 0x0 )
				{
					a = ((a - 1) << 1) + 1; /* to have a range between 0...255 */
				}
			}
			r = (*TIn >> 16) & 0xFF;
			g = (*TIn >> 8) & 0xFF;
			b = (*TIn >> 0) & 0xFF;
			r = r + (DitherPattern<<1); if (r>255) r = 255; r = r>>3;		/* 8-bit to 5-bit component */
			g = g +  DitherPattern;     if (g>255) g = 255; g = g>>2;		/* 8-bit to 6-bit component */
			b = b + (DitherPattern<<1); if (b>255) b = 255; b = b>>3;		/* 8-bit to 5-bit component */
			*TIn = (a<<16) | (r<<11) | (g<<5) | b;									/* Reformat */
			break;
		case gam_emul_TYPE_ARGB8888:							/* ok */
		case gam_emul_TYPE_AYCbCr8888:
			if (Blt->T_alphaRange == 0x1)
			{
				a = (*(TIn) >> 24) & 0xFF;
				*(TIn) &= 0xFFFFFF;
				if ( a != 0x0 )
				{
					a = ((a - 1) << 1) + 1; /* to have a range between 0...255 */
				}
				*(TIn) |= (a<<24);
			}
			break;
		case gam_emul_TYPE_RGB565:
			r = (*TIn >> 16) & 0xFF;
			g = (*TIn >> 8) & 0xFF;
			b = (*TIn >> 0) & 0xFF;
			r = r + (DitherPattern<<1); if (r>255) r = 255; r = r>>3;		/* 8-bit to 5-bit component */
			g = g +  DitherPattern;     if (g>255) g = 255; g = g>>2;		/* 8-bit to 6-bit component */
			b = b + (DitherPattern<<1); if (b>255) b = 255; b = b>>3;		/* 8-bit to 5-bit component */
			*TIn = (r<<11) | (g<<5) | b;														/* Reformat */
			break;
		case gam_emul_TYPE_ARGB1555:
			a = (*TIn >> 24) & 0xFF;
			if (a==128)			/* opaque */
				a = 1;
			else						/* not opaque -> transparent */
				a = 0;
			r = (*TIn >> 16) & 0xFF;
			g = (*TIn >> 8) & 0xFF;
			b = (*TIn >> 0) & 0xFF;
			r = r + (DitherPattern<<1); if (r>255) r = 255; r = r>>3;		/* 8-bit to 5-bit component */
			g = g + (DitherPattern<<1); if (g>255) g = 255; g = g>>3;		/* 8-bit to 5-bit component */
			b = b + (DitherPattern<<1); if (b>255) b = 255; b = b>>3;		/* 8-bit to 5-bit component */
			*TIn = (a<<15) | (r<<10) | (g<<5) | b;									/* Reformat */
			break;
		case gam_emul_TYPE_ARGB4444:
			a = (*TIn >> 24) & 0xFF;
			a = a>>3;
			if (a==16)			/* opaque */
				a = 15;
			r = (*TIn >> 16) & 0xFF;
			g = (*TIn >> 8) & 0xFF;
			b = (*TIn >> 0) & 0xFF;
			r = r + (DitherPattern<<2); if (r>255) r = 255; r = r>>4;		/* 8-bit to 4-bit component */
			g = g + (DitherPattern<<2); if (g>255) g = 255; g = g>>4;		/* 8-bit to 4-bit component */
			b = b + (DitherPattern<<2); if (b>255) b = 255; b = b>>4;		/* 8-bit to 4-bit component */
			*TIn = (a<<12) | (r<<8) | (g<<4) | b;										/* Reformat */
			break;
		case gam_emul_TYPE_A8:
			a = (*TIn >> 24) & 0xFF;
			if (Blt->T_alphaRange == 0x1)
			{
				if ( a != 0x0 )
				{
					a = ((a - 1) << 1) + 1; /* to have a range between 0...255 */
				}
			}
			*TIn = a;									/* Reformat */
			break;
		case gam_emul_TYPE_A1:
			a = (*TIn >> 24) & 0xFF;
			if (a != 128)
				a = 0;
			else
				a = 1;
			*TIn = a;									/* Reformat */
			break;
		case gam_emul_TYPE_ACLUT88:
			a = (*TIn >> 24) & 0xFF;
			if (Blt->T_alphaRange == 0x1)
			{
				if ( a != 0x0 )
				{
					a = ((a - 1) << 1) + 1; /* to have a range between 0...255 */
				}
			}
			index = *TIn & 0xFF;
			*TIn = (a<<8) | index;		/* Reformat */
			break;
		case gam_emul_TYPE_ACLUT44:
			a = (*TIn >> 24) & 0xFF;
			a = a>>3;
			if (a==16)			/* opaque */
				a = 15;
			index = *TIn & 0xF;
			*TIn = (a<<4) | index;		/* Reformat */
			break;
		case gam_emul_TYPE_CLUT8:
		case gam_emul_TYPE_BYTE:
			*TIn &= 0xFF;			/* Keep 8 LSBs */
			break;
		case gam_emul_TYPE_CLUT4:
			*TIn &= 0xF;			/* Keep 4 LSBs */
			break;
		case gam_emul_TYPE_CLUT2:
			*TIn &= 0x3;			/* Keep 2 LSBs */
			break;
		case gam_emul_TYPE_CLUT1:
			*TIn &= 0x1;			/* Keep 1 LSBs */
			break;
		default:
			break;
	}
	if (Blt->EnaPlaneMask == 1)
	{
		*TIn = ( Blt->PlaneMask & *TIn)			/* Reset unmodified bits in the result */
						 | (~Blt->PlaneMask & *S1In);		/* Reset modified bits in S1 value, */
																								/* then combine */
	}
	/* Then we write the target line buffer */
	if (*TMask == 0)
		gam_emul_WriteBlitterPixel(x,y,Blt,*TIn);

}
/*-----------------------------------------------------------------------------------
 * Function : gam_emul_SetWriteEnableMaskXYL
 * Parameters: gam_emul_BlitterParameters, TMask, Type, Sa, x, y
 * Return : -
 * Description : modify mask to validate rectangular clipping and color key for one pixel
 * ----------------------------------------------------------------------------------*/

void gam_emul_SetWriteEnableMaskXYL(gam_emul_BlitterParameters *Blt,int *TMask,int Type,int *Sa,int x, int y)
{
	int T_absoluteX,T_absoluteY;
	int r,EnableRKey,ROutRange;
	int g,EnableGKey,GOutRange;
	int b,EnableBKey,BOutRange;
	int CKmatch;

	/* in XYL, bits [24,25,27] Of BLT_TTY have no meaning */

	if (Type == gam_emul_RECT_CLIPPING) /* the register Target XY location is unused */
	{
		T_absoluteY = y ;		/* Given parameter = y top */

		T_absoluteX = x ;		/* Given parameter = x left */

		if (Blt->InternalClip == 0)
		{
			if  ( (T_absoluteX < Blt->ClipXMin)		/* Write allowed on the borders in normal mode */
				 || (T_absoluteX > Blt->ClipXMax)
				 || (T_absoluteY < Blt->ClipYMin)
				 || (T_absoluteY > Blt->ClipYMax))
			*TMask |= gam_emul_RECT_CLIPPING;					/* Set Clipping Bit filter */
		}
		else
		{
			if ( (T_absoluteX >= Blt->ClipXMin)		/* Write NOT allowed on the borders in reverse mode */
				&& (T_absoluteX <= Blt->ClipXMax)
			  && (T_absoluteY >= Blt->ClipYMin)
			  && (T_absoluteY <= Blt->ClipYMax))
			*TMask |= gam_emul_RECT_CLIPPING;					/* Set Clipping Bit filter */
		}
			if ((T_absoluteX < 0) || (T_absoluteY < 0))
				*TMask |= gam_emul_RECT_CLIPPING;					/* Set Clipping Bit filter */
	}
	/* for the color key feature, only mode 0 can be used (destination color key), and only in XYL standard mode */

	if ((Type == gam_emul_COLOR_KEY) && (Blt->CHK_Input == 0))
	{
		EnableRKey = Blt->CHK_RMode & 1;
		EnableGKey = Blt->CHK_GMode & 1;
		EnableBKey = Blt->CHK_BMode & 1;
		ROutRange = (Blt->CHK_RMode >> 1) & 1;
		GOutRange = (Blt->CHK_GMode >> 1) & 1;
		BOutRange = (Blt->CHK_BMode >> 1) & 1;

		r = (*Sa >> 16) & 0xFF;
		g = (*Sa >> 8) & 0xFF;
		b =  *Sa & 0xFF;
		if (
			((((g<Blt->CHK_GMin)||(g>Blt->CHK_GMax)) && (GOutRange==1) && (EnableGKey==1))
		||((g>=Blt->CHK_GMin) && (g<=Blt->CHK_GMax) && (GOutRange==0) && (EnableGKey==1))
		||(EnableGKey==0))
		&&
			((((b<Blt->CHK_BMin)||(b>Blt->CHK_BMax)) && (BOutRange==1) && (EnableBKey==1))
		||((b>=Blt->CHK_BMin) && (b<=Blt->CHK_BMax) && (BOutRange==0) && (EnableBKey==1))
		||(EnableBKey==0))
		&&
			((((r<Blt->CHK_RMin)||(r>Blt->CHK_RMax)) && (ROutRange==1) && (EnableRKey==1))
		||((r>=Blt->CHK_RMin) && (r<=Blt->CHK_RMax) && (ROutRange==0) && (EnableRKey==1))
		||(EnableRKey==0))
				)
			CKmatch = 1;
		else
			CKmatch = 0;

		/* destination color key, write only if match */
			CKmatch = 1 - CKmatch;

		if (CKmatch == 1)
			*TMask |= gam_emul_COLOR_KEY;		/* Set ColorKey Bit filter */
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_ColorSpaceConverterXYL
 * Parameters: gam_emul_BlitterParameters, In,Out
 * Return : out
 * Description : YCbCR <> RGB ( InputOutput : in function of conversion YCbCR <> RGB
 location) or copy input to output directly if color space converter is disabled
 * ----------------------------------------------------------------------------------*/

void gam_emul_ColorSpaceConverterXYL(gam_emul_BlitterParameters *Blt,int *In,int *Out)
{
	int a,r,g,b,luma,cb,cr;


	if (Blt->EnaOutputColorSpaceConverter == 1)
	{
		if (Blt->CSC_input_direction == 1)			/* 1 : Output -> YCbCr to RGB */
		{
				a = (*In >> 24) & 0xFF;
				cr = (*In >> 16) & 0xFF;
				luma = (*In >> 8) & 0xFF;
				cb = (*In >> 0) & 0xFF;
				gam_emul_ConvertYCbCr2RGB (&luma,&cb,&cr,&r,&g,&b,
													Blt->CSC_output_colorimetry,8,8,Blt->CSC_output_chroma_format, Blt->CSC_matrix_out_ngfx_vid);
				*Out = (a<<24) | (r<<16) | (g<<8) | b;
		}
		else										/* 0 : Output -> RGB to YCbCr */
		{
				a = (*In >> 24) & 0xFF;
				r = (*In >> 16) & 0xFF;
				g = (*In >> 8) & 0xFF;
				b = (*In >> 0) & 0xFF;
				gam_emul_ConvertRGB2YCbCr (&r,&g,&b,&luma,&cb,&cr,
													Blt->CSC_output_colorimetry,Blt->CSC_output_chroma_format,8, Blt->CSC_matrix_out_ngfx_vid);
				*Out = (a<<24) | (cr<<16) | (luma<<8) | cb;
		}
	}
	else
	{
		/* Disabled -> BYPASS */

		*Out = *In;
	}
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_Write42XLuma
 * Parameters: x, y,gam_emul_BlitterParameters, Datapix
 * Return : -
 * Description : write luma in 4.2.X MB
 * ----------------------------------------------------------------------------------*/

void gam_emul_Write42XLuma(int x,int y,gam_emul_BlitterParameters *Blt,int Datapix)
{
	int T_addresspix;
	int T_absoluteX,T_absoluteY;
	int Offset,Block16x32Address;

	Offset = 0;

	/* Let's consider the horizontal scanning direction */
	if (Blt->T_hscan == 0)
		T_absoluteX = (x + Blt->T_x);		/* Given parameter = x left */
	else
		T_absoluteX = (Blt->T_x - x);		/* Given parameter = x right */
	if (Blt->T_vscan == 0)
		T_absoluteY = (y + Blt->T_y);		/* Given parameter = y top */
	else
		T_absoluteY = (Blt->T_y - y);		/* Given parameter = y bottom */

	Block16x32Address = ((T_absoluteX/16) + ((T_absoluteY/32) * (Blt->T_pitch/16))) << 9;
	if ((T_absoluteY & 0x10) == 0)		/* MB of rows 0,2,4... */
	{
		if (((T_absoluteX & 0xF) <8) && ((T_absoluteY & 1) == 0)) 	/* MB even row ..Area 1 */
			Offset = 0;
		if (((T_absoluteX & 0xF)>=8) && ((T_absoluteY & 1) == 0))		/* MB even row ..Area 2 */
			Offset = 64;
		if (((T_absoluteX & 0xF) <8) && ((T_absoluteY & 1) == 1))		/* MB even row ..Area 3 */
			Offset = 128;
		if (((T_absoluteX & 0xF)>=8) && ((T_absoluteY & 1) == 1))		/* MB even row ..Area 4 */
			Offset = 192;
	}
	else															/* MB of rows 1,3,5... */
	{
		if (((T_absoluteX & 0xF) <8) && ((T_absoluteY & 1) == 0))		/* MB odd row  ..Area 1 */
			Offset = 256;
		if (((T_absoluteX & 0xF)>=8) && ((T_absoluteY & 1) == 0))		/* MB odd row  ..Area 2 */
			Offset = 320;
		if (((T_absoluteX & 0xF) <8) && ((T_absoluteY & 1) == 1))		/* MB odd row  ..Area 3 */
			Offset = 384;
		if (((T_absoluteX & 0xF)>=8) && ((T_absoluteY & 1) == 1))		/* MB odd row  ..Area 4 */
			Offset = 448;
	}
	T_addresspix = Blt->T_ba
							 + Block16x32Address + Offset
							 + (((T_absoluteY & 0xF) >> 1) << 3)
							 + (T_absoluteX & 7);
	/* Now the endianess issue !!! */
	T_addresspix = (T_addresspix & 0xFFFFFFF0) + (15 - (T_addresspix & 0xF));

    gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)((Datapix >> 8) & 0xFF));
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_Write42XCbCr
 * Parameters: x, y,gam_emul_BlitterParameters, Width
 * Return : -
 * Description : write cb and cr in 4:2:X MacroBlock
 * ----------------------------------------------------------------------------------*/

void gam_emul_Write42XCbCr(int x,int y,gam_emul_BlitterParameters *Blt,int Datapix)
{
	int T_addresspix,xc,yc;
	int T_absoluteX,T_absoluteY;
	int Offset,Block16x16Address;

	/* Let's consider the horizontal scanning direction */
	if (Blt->T_hscan == 0)
		T_absoluteX = (2*x + Blt->T_x);		/* Given parameter = x left */
	else
		T_absoluteX = (Blt->T_x - x);		/* Given parameter = x right */
	if (Blt->T_vscan == 0)
		T_absoluteY = (2*y + Blt->T_y);		/* Given parameter = y top */
	else
		T_absoluteY = (Blt->T_y - y);		/* Given parameter = y bottom */

	T_absoluteX >>= 1;
	T_absoluteY >>= 1;

	/* Find equivalent xc/yc  */
	xc = T_absoluteX + ((T_absoluteY/16) * (Blt->T_pitch/2));
	yc = T_absoluteY & 0xF;

	/* A 16x16 block represents 4 chroma MB (cb+cr), so 64x4x2 = 512 bytes */
	Block16x16Address = ((xc/16) + ((yc/16) * (Blt->T_pitch/32))) * 512;

	if ((yc&0xF) <8)  /* 0..255 */
		Block16x16Address += 0;
	else							/* 256..511 */
		Block16x16Address += 256;

	if ((xc&0xF) <8)
		Block16x16Address += 0;
	else
		Block16x16Address += 128;

	if ((yc&0x1) == 0)
	{
		if ((xc&0x7) <4)
			Offset = (xc & 7) + ((yc & 7)/2) * 8;
		else
			Offset = ((xc-4) & 7) + ((yc & 7)/2) * 8 + 32;
	}
	else
	{
		if ((xc&0x7) <4)
			Offset = (xc & 7) + ((yc & 7)/2) * 8 + 64;
		else
			Offset = ((xc-4) & 7) + ((yc & 7)/2) * 8 + 96;
	}
	T_addresspix = Blt->T_ba
							 + Block16x16Address + Offset;
	/* Write Cb */
	T_addresspix = (T_addresspix & 0xFFFFFFF0) + (15 - (T_addresspix & 0xF));
    gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)(Datapix & 0xFF));

	/* Now write Cr */
	T_addresspix = Blt->T_ba
							 + Block16x16Address + Offset + 4;
	T_addresspix = (T_addresspix & 0xFFFFFFF0) + (15 - (T_addresspix & 0xF));
    gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)((Datapix >> 16) & 0xFF));
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_WriteHDpipLuma
 * Parameters: x, y,gam_emul_BlitterParameters, Datapix
 * Return : -
 * Description : write luma in 4.2.X MB
 * ----------------------------------------------------------------------------------*/

void gam_emul_WriteHDpipLuma(int x,int y,gam_emul_BlitterParameters *Blt,int Datapix)
{
	int T_addresspix;
	int T_absoluteX,T_absoluteY;
	int Offset,Block16x32Address;

	Offset = 0;

	/* Let's consider the horizontal scanning direction */
	if (Blt->T_hscan == 0)
		T_absoluteX = (x + Blt->T_x);		/* Given parameter = x left */
	else
		T_absoluteX = (Blt->T_x - x);		/* Given parameter = x right */
	if (Blt->T_vscan == 0)
		T_absoluteY = (y + Blt->T_y);		/* Given parameter = y top */
	else
		T_absoluteY = (Blt->T_y - y);		/* Given parameter = y bottom */

	Block16x32Address = ((T_absoluteX/16) + ((T_absoluteY/32) * (Blt->T_pitch/16))) << 10;
	if ((T_absoluteY & 0x10) == 0)		/* MB of rows 0,2,4... */
	{
		if (((T_absoluteX & 0xF) <8) && ((T_absoluteY & 1) == 0)) 	/* MB even row ..Area 1 */
			Offset = 0;
		if (((T_absoluteX & 0xF)>=8) && ((T_absoluteY & 1) == 0))		/* MB even row ..Area 2 */
			Offset = 64;
		if (((T_absoluteX & 0xF) <8) && ((T_absoluteY & 1) == 1))		/* MB odd row ..Area 3 */
			Offset = 128;
		if (((T_absoluteX & 0xF)>=8) && ((T_absoluteY & 1) == 1))		/* MB odd row ..Area 4 */
			Offset = 192;
	}
	else															/* MB of rows 1,3,5... */
	{
		if (((T_absoluteX & 0xF) <8) && ((T_absoluteY & 1) == 0))		/* MB even row  ..Area 1 */
			Offset = 256;
		if (((T_absoluteX & 0xF)>=8) && ((T_absoluteY & 1) == 0))		/* MB even row  ..Area 2 */
			Offset = 320;
		if (((T_absoluteX & 0xF) <8) && ((T_absoluteY & 1) == 1))		/* MB odd row  ..Area 3 */
			Offset = 384;
		if (((T_absoluteX & 0xF)>=8) && ((T_absoluteY & 1) == 1))		/* MB odd row  ..Area 4 */
			Offset = 448;
	}
	T_addresspix = Blt->T_ba
							 + Block16x32Address + Offset
							 + (((T_absoluteY & 0xF) >> 1) << 3)
							 + (T_absoluteX & 7);
	/* Now the endianess issue !!! */
	T_addresspix = (T_addresspix & 0xFFFFFFF0) + (15 - (T_addresspix & 0xF));

    gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)((Datapix >> 8) & 0xFF));
}

/*-----------------------------------------------------------------------------------
 * Function : gam_emul_WriteHDpipCbCr
 * Parameters: x, y,gam_emul_BlitterParameters, Width
 * Return : -
 * Description : write cb and cr in 4:2:X MacroBlock
 * ----------------------------------------------------------------------------------*/

void gam_emul_WriteHDpipCbCr(int x,int y,gam_emul_BlitterParameters *Blt,int Datapix)
{
	int T_addresspix;
	int T_absoluteX,T_absoluteY;
	int Offset,Block16x32Address;

	Offset = 0;

	/* Let's consider the horizontal scanning direction */
	if (Blt->T_hscan == 0)
		T_absoluteX = (2*x + Blt->T_x);		/* Given parameter = x left */
	else
		T_absoluteX = (Blt->T_x - x);		/* Given parameter = x right */
	if (Blt->T_vscan == 0)
		T_absoluteY = (2*y + Blt->T_y);		/* Given parameter = y top */
	else
		T_absoluteY = (Blt->T_y - y);		/* Given parameter = y bottom */

	T_absoluteX >>= 1;
	T_absoluteY >>= 1;

	Block16x32Address = ((T_absoluteX/8) + ((T_absoluteY/16) * (Blt->T_pitch/16))) << 10; /* *1024 each 1024 byte we can find macro-block*/

	if ((T_absoluteY & 0x8) == 0)
	{
		if (((T_absoluteX&0x7) <4) && ((T_absoluteY&1) == 0)) 	/* MB even row ..Area 1 */
			Offset = 0;
		if (((T_absoluteX&0x7)>=4) && ((T_absoluteY&1) == 0))		/* MB even row ..Area 2 */
			Offset = 32;
		if (((T_absoluteX&0x7) <4) && ((T_absoluteY&1) == 1))		/* MB odd row ..Area 3 */
			Offset = 64;
		if (((T_absoluteX&0x7)>=4) && ((T_absoluteY&1) == 1))		/* MB odd row ..Area 4 */
			Offset = 96;
	}
	else
	{
		if (((T_absoluteX&0x7) <4) && ((T_absoluteY&1) == 0))		/* MB even row  ..Area 1 */
			Offset = 256;
		if (((T_absoluteX&0x7)>=4) && ((T_absoluteY&1) == 0))		/* MB even row  ..Area 2 */
			Offset = 288;
		if (((T_absoluteX&0x7) <4) && ((T_absoluteY&1) == 1))		/* MB odd row  ..Area 3 */
			Offset = 320;
		if (((T_absoluteX&0x7)>=4) && ((T_absoluteY&1) == 1))		/* MB odd row  ..Area 4 */
			Offset = 352;
	}

	T_addresspix = Blt->T_ba
							 + Block16x32Address + Offset
							 + (((T_absoluteY & 0x7) >> 1) << 3)
							 + ((T_absoluteX & 7) < 4 ? (T_absoluteX & 0x7) : (T_absoluteX & 0x7)-4 ) ;
	/* Write Cb */
	T_addresspix = (T_addresspix & 0xFFFFFFF0) + (15 - (T_addresspix & 0xF));
    gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)(Datapix & 0xFF));

	/* Now write Cr */
	T_addresspix -=  4;
    gam_emul_writeUChar((unsigned char*)(gam_emul_BLTMemory+T_addresspix), (unsigned char)((Datapix >> 16) & 0xFF));
}
