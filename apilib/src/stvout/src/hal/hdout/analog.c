/*******************************************************************************

File name   : analog.c

Description : VOUT driver module for analog output.

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
25 Jul 2000        Created                                           JG
16 Oct 2000        release 2.0.0                                     JG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "vout_drv.h"
#include "analog.h"
#include "hd_reg.h"
#include "stcommon.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#if defined (ST_7710) || defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#define BLANKDAC 0x00000002
#else
#define BLANKDAC 0x00000100
#endif

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : stvout_HalSetAnalogRgbYcbcr
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvout_HalSetAnalogRgbYcbcr( void* BaseAddress, U8 RgbYcbcr)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    #if defined(ST_7200) || defined(ST_7111)|| defined (ST_7105)
        UNUSED_PARAMETER(BaseAddress);
        UNUSED_PARAMETER(RgbYcbcr);
    #else
        U32 Value ;
        U32 Offset = DSPCFG_ANA;
        U32 Add = (U32)BaseAddress;

        STOS_InterruptLock();
        Value = stvout_rd32( Add + Offset);
        switch ( RgbYcbcr) {
            case VOUT_OUTPUTS_HD_RGB :
                Value |= DSPCFG_ANA_RGB;
                break;
            case VOUT_OUTPUTS_HD_YCBCR :
                Value &= (U32)~DSPCFG_ANA_RGB;
                break;
            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;

        }
        stvout_wr32( Add + Offset, Value);
        STOS_InterruptUnlock();
    #endif
    return (ErrorCode);
}

/*******************************************************************************
Name        : stvout_HalSetAnalogSDRgbYuv
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvout_HalSetAnalogSDRgbYuv( void* BaseAddress, U8 RgbYuv)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    #if defined(ST_7200) || defined(ST_7111) || defined (ST_7105)
        UNUSED_PARAMETER(BaseAddress);
        UNUSED_PARAMETER(RgbYuv);
    #else
        U32 Value ;
        U32 Offset = DSPCFG_CLK;
        U32 Add = (U32)BaseAddress;
        U32 Ups_Sel=0x0;

        STOS_InterruptLock();
        Value = stvout_rd32( Add + Offset);

        switch ( RgbYuv) {
            case VOUT_OUTPUTS_RGB :
            case VOUT_OUTPUTS_YUV :
            case VOUT_OUTPUTS_CVBS :
            case VOUT_OUTPUTS_YC :
                Ups_Sel=0x1;
                Value|=(Ups_Sel<<3);
                break;
            case VOUT_OUTPUTS_HD_RGB :
            case VOUT_OUTPUTS_HD_YCBCR :
                Ups_Sel=0x0;
                Value&=(U32)~DSPCFG_CLK_UPSEL_MASK;
                Value|=(Ups_Sel<<3);
                break;

            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;

        }
        stvout_wr32( Add + Offset, Value);
        STOS_InterruptUnlock();
    #endif
    return (ErrorCode);
}

/*******************************************************************************
Name        : stvout_HalSetAnalogEmbeddedSync
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t stvout_HalSetAnalogEmbeddedSync( void* BaseAddress, BOOL EmbeddedSync)
{
    #if defined(ST_7200) || defined (ST_7111) || defined (ST_7105)
        UNUSED_PARAMETER(BaseAddress);
        UNUSED_PARAMETER(EmbeddedSync);
    #else
        U32 Value;
        U32 Offset = DSPCFG_ANA;
        U32 Add = (U32)BaseAddress;

        STOS_InterruptLock();
        Value = stvout_rd32( Add + Offset);
        if ( EmbeddedSync)
        {
            Value |= DSPCFG_ANA_SYHD | DSPCFG_ANA_RSC;
        }
        else
        {
            Value &= (U32)~(DSPCFG_ANA_SYHD | DSPCFG_ANA_RSC);
        }
        stvout_wr32( Add + Offset, Value);
        STOS_InterruptUnlock();
    #endif /*ST_7200||ST_7111||ST_7105*/
    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : stvout_HalSetSyncroInChroma
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t stvout_HalSetSyncroInChroma( void* BaseAddress, BOOL SyncroInChroma)
{
    #if defined(ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        UNUSED_PARAMETER(BaseAddress);
        UNUSED_PARAMETER(SyncroInChroma);
    #else
        U32 Value;
        U32 Offset = DHDO_ACFG;
        U32 Add = (U32)BaseAddress;

        STOS_InterruptLock();
        Value = stvout_rd32( Add + Offset);
        if ( SyncroInChroma)
        {
            Value |= DHDO_ACFG_SIC;
        }
        else
        {
            Value &= (U32)~DHDO_ACFG_SIC;
        }
        stvout_wr32( Add + Offset, Value);
        STOS_InterruptUnlock();
    #endif
    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : stvout_HalSetColorSpace
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvout_HalSetColorSpace( void* BaseAddress, U8 ColorSpace)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    #if defined(ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        UNUSED_PARAMETER(BaseAddress);
        UNUSED_PARAMETER(ColorSpace);
    #else
        U32 Value;
        U32 Offset = DHDO_COLOR;
        U32 Add = (U32)BaseAddress;

        STOS_InterruptLock();
        Value = stvout_rd32( Add + Offset);
        switch ( ColorSpace) {
            case 1 : /* SMPTE240M */
                Value |= 0x2;
                break;
            case 2 : /* ITU-R-601 */
                Value &= 0xFFFC;
                Value |= 0x1;
                break;
            case 4 : /* ITU-R-709 */
                Value &= 0xFFFC;
                break;
            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }
        stvout_wr32( Add + Offset, Value);
        STOS_InterruptUnlock();
    #endif /*ST_7200||ST_7111||ST_7105*/
    return (ErrorCode);
}

/*******************************************************************************
Name        : stvout_HalEnableRGBDac
Description : Disable/Enable the output signals by writing useful Analog register
              parameter Number : 0-4 -> Dac 0-4
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t stvout_HalEnableRGBDac( void* BaseAddress, U8 Dac)
{
    #if defined (ST_7200)|| defined (ST_7109)|| defined (ST_7111)|| defined (ST_7105)
        UNUSED_PARAMETER(BaseAddress);
        UNUSED_PARAMETER(Dac);
    #else
        U32 Value;
        U32 Offset = DSPCFG_DAC;
        U32 Add = (U32)BaseAddress;
        #if defined (ST_7100)
            U32 CutId;
            /* get the cut ID*/
            CutId = ST_GetCutRevision();
            if( CutId >= 0xB0)
            {
                UNUSED_PARAMETER(BaseAddress);
                UNUSED_PARAMETER(Dac);
            }
            else
            {
                STOS_InterruptLock();
                Value = stvout_rd32( Add + Offset);
                Value &= (U32)~DSPCFG_DAC_BLRGB;         /* don't use BLRGB bit */
                Value &= (U32)~(BLANKDAC << Dac) ;
                stvout_wr32( Add + Offset, Value);
                STOS_InterruptUnlock();
            }
        #else

            STOS_InterruptLock();
            Value = stvout_rd32( Add + Offset);
            Value &= (U32)~DSPCFG_DAC_BLRGB;         /* don't use BLRGB bit */
            Value &= (U32)~(BLANKDAC << Dac) ;
            #if !defined (ST_7710)
            Value &= (U32)~DSPCFG_DAC_PD;
            #endif
            stvout_wr32( Add + Offset, Value);
            STOS_InterruptUnlock();
        #endif
    #endif
    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : stvout_HalDisableRGBDac
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvout_HalDisableRGBDac( void* BaseAddress, U8 Dac)
{
    #if defined (ST_7200)|| defined (ST_7109)|| defined (ST_7111)|| defined (ST_7105)
        UNUSED_PARAMETER(BaseAddress);
        UNUSED_PARAMETER(Dac);
    #else
        U32 Value;
        U32 Offset = DSPCFG_DAC;
        U32 Add = (U32)BaseAddress;
        #if defined (ST_7100)
            U32 CutId;
            /* get the cut ID*/
            CutId = ST_GetCutRevision();
            if( CutId >= 0xB0)
            {
                UNUSED_PARAMETER(BaseAddress);
                UNUSED_PARAMETER(Dac);
            }
            else
            {
                STOS_InterruptLock();
                Value = stvout_rd32( Add + Offset);
                Value &= (U32)~DSPCFG_DAC_BLRGB;         /* don't use BLRGB bit */
                Value |= (BLANKDAC << Dac);
                stvout_wr32( Add + Offset, Value);
                STOS_InterruptUnlock();
            }
        #else
            STOS_InterruptLock();
            Value = stvout_rd32( Add + Offset);
            Value &= (U32)~DSPCFG_DAC_BLRGB;         /* don't use BLRGB bit */
            Value |= (BLANKDAC << Dac);
            #if !defined (ST_7710)
                Value |= (U32)DSPCFG_DAC_PD;    /* set PD bit : power down   */
            #endif
            stvout_wr32( Add + Offset, Value);
            STOS_InterruptUnlock();
        #endif
    #endif
    return (ST_NO_ERROR);
}
#ifdef STVOUT_HDDACS
/*******************************************************************************
Name        : stvout_HalSetUpsampler
Description : Set upsampler coefficient
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvout_HalSetUpsampler( stvout_Device_t * Device_p, U8 RgbYcbcr)
{
    ST_ErrorCode_t Ec = ST_NO_ERROR;
#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
    STVTG_Handle_t               VTGHandle;
    STVTG_OpenParams_t           VTGOpenParams;
    STVTG_TimingMode_t           VTGTimingMode;
    STVTG_ModeParams_t           VTGModeParams;
#endif
    U32 Add ;
    U32 Value;
    #if defined(ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        Add  = (U32)Device_p->HDFBaseAddress_p;
        /*Value = stvout_rd32( Add + ANA_SRC_CONFIG);*/
        switch ( RgbYcbcr)
        {
            case VOUT_OUTPUTS_HD_RGB :
            case VOUT_OUTPUTS_HD_YCBCR :
                 Ec = STVTG_Open(Device_p->VTGName, &VTGOpenParams, &VTGHandle);
                 if ( Ec!= ST_NO_ERROR )
                 {
                     return(ST_ERROR_OPEN_HANDLE);
                 }
                 else
                 {
                   Ec = STVTG_GetMode (VTGHandle, &VTGTimingMode, &VTGModeParams);
                   if ( Ec!= ST_NO_ERROR )
                   {
                   STVTG_Close(VTGHandle);
                   return( ST_ERROR_BAD_PARAMETER);
                   }
                   else
                   {
                      if(Device_p->Format == STVOUT_ED_MODE)
                      {

                       switch (VTGTimingMode)
                       {
                       case STVTG_TIMING_MODE_480P59940_27000 :
                       case STVTG_TIMING_MODE_480P60000_27027 :
                            /*ANA_CFG Configuration*/
                            Value = ANA_SEL_MAIN_YCBCR_INPUT| ANA_RECORDER_CTRL_R_SEL2| ANA_RECORDER_CTRL_B_SEL1|
                            PBPR_SYNC_OFFSET_ED_480P|AWG_ASYNC_EN|AWG_ASYNC_VSYNC_FIELD|AWG_ASYNC_FILTER_MODE_1;
                            stvout_wr32(  Add + ANA_CFG, Value);

                            /*ANA_SCALE_CTRL_C configuration*/
                            Value = C_SCALE_COEFF_ED_480P|C_OFFESET_ED;
                            stvout_wr32(  Add + ANA_SCALE_CTRL_C, Value);
                            break;
                       case STVTG_TIMING_MODE_576P50000_27000 :
                            /*ANA_CFG Configuration*/
                            Value = ANA_SEL_MAIN_YCBCR_INPUT| ANA_RECORDER_CTRL_R_SEL2| ANA_RECORDER_CTRL_B_SEL1|
                            PBPR_SYNC_OFFSET_ED_576P|AWG_ASYNC_EN|AWG_ASYNC_VSYNC_FIELD|AWG_ASYNC_FILTER_MODE_1;
                            stvout_wr32(  Add + ANA_CFG, Value);

                            /*ANA_SCALE_CTRL_C configuration*/
                            Value = C_SCALE_COEFF_ED_576P|C_OFFESET_ED;
                            stvout_wr32(  Add + ANA_SCALE_CTRL_C, Value);
                            break;
                        default:
                            break;
                      }
                       /*ANA_SCALE_CTRL_Y configuration*/
                       Value = Y_SCALE_COEFF_ED|Y_OFFESET_ED;
                       stvout_wr32(  Add + ANA_SCALE_CTRL_Y, Value);


                       /* ANA_ANCILLARY_CTRL configuration*/
                       Value = stvout_rd32( Add + ANA_ANCILLARY_CTRL);
                       Value &=~ANA_ANC_MACRIVISION_EN;
                       stvout_wr32(  Add + ANA_ANCILLARY_CTRL, Value);

                       /*ANA_SRC_CONFIG configuration*/
                       Value =  SRC_UPSAMPLE_CONF_BY_4| SRC_GAIN_FACTOR_BY_256|COEFF_PHASE1_T7_SD;
                       stvout_wr32(  Add + ANA_SRC_CONFIG, Value);

                       /*Hereafter the upsampler coeff used for ED display format */
                       stvout_wr32(  Add + COEFF_PHASE1_TAP123 ,EDFORM_COEFF_PHASE1_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE1_TAP456 ,EDFORM_COEFF_PHASE1_TAP456);
                       stvout_wr32(  Add + COEFF_PHASE2_TAP123 ,EDFORM_COEFF_PHASE2_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE2_TAP456 ,EDFORM_COEFF_PHASE2_TAP456);
                       stvout_wr32(  Add + COEFF_PHASE3_TAP123 ,EDFORM_COEFF_PHASE3_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE3_TAP456 ,EDFORM_COEFF_PHASE3_TAP456);
                       stvout_wr32(  Add + COEFF_PHASE4_TAP123 ,EDFORM_COEFF_PHASE4_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE4_TAP456 ,EDFORM_COEFF_PHASE4_TAP456);

                       }
                       else if(Device_p->Format == STVOUT_HD_MODE)
                       {

                       switch (VTGTimingMode)
                       {
                       case STVTG_TIMING_MODE_1080I60000_74250 : /* no break */
                       case STVTG_TIMING_MODE_1080I59940_74176 : /* no break */
                       case STVTG_TIMING_MODE_1080I50000_74250_1 :

                            /*ANA_CFG Configuration*/
                            Value = ANA_SEL_MAIN_YCBCR_INPUT| ANA_RECORDER_CTRL_R_SEL2|ANA_RECORDER_CTRL_B_SEL1|
                            PBPR_SYNC_OFFSET_HD_1080I|AWG_ASYNC_EN|AWG_ASYNC_FILTER_MODE_2;
                            stvout_wr32(  Add + ANA_CFG, Value);

                             /*ANA_SCALE_CTRL_C configuration*/
                             Value = C_SCALE_COEFF_HD_1080I|C_OFFESET_HD;
                             stvout_wr32(  Add + ANA_SCALE_CTRL_C, Value);

                             /* AWG Analog ancillary instruction*/
                             stvout_wr32(  Add + AWG_INST_ANC, AWG_INST_ANC_VALUE_1080I);
                            break;
                       case STVTG_TIMING_MODE_720P60000_74250 :
                       case STVTG_TIMING_MODE_720P59940_74176 :
                       case STVTG_TIMING_MODE_720P50000_74250 :
                             /*ANA_CFG Configuration*/
                            Value = ANA_SEL_MAIN_YCBCR_INPUT| ANA_RECORDER_CTRL_R_SEL2|ANA_RECORDER_CTRL_B_SEL1|
                            PBPR_SYNC_OFFSET_HD_720P|AWG_ASYNC_EN|AWG_ASYNC_VSYNC_FIELD|AWG_ASYNC_FILTER_MODE_2;
                            stvout_wr32(  Add + ANA_CFG, Value);

                            /*ANA_SCALE_CTRL_C configuration*/
                             Value = C_SCALE_COEFF_HD_720P|C_OFFESET_HD;
                             stvout_wr32(  Add + ANA_SCALE_CTRL_C, Value);

                            /* AWG Analog ancillary instruction*/
                             stvout_wr32(  Add + AWG_INST_ANC, AWG_INST_ANC_VALUE_720P);
                            break;
                       default:
                            break;
                       }

                       /*ANA_SCALE_CTRL_Y configuration*/
                       Value = Y_SCALE_COEFF_HD|Y_OFFESET_HD;
                       stvout_wr32(  Add + ANA_SCALE_CTRL_Y, Value);

                       /* ANA_ANCILLARY_CTRL configuration*/
                       Value = stvout_rd32( Add + ANA_ANCILLARY_CTRL);
                       Value &=~ANA_ANC_MACRIVISION_EN;
                       stvout_wr32(  Add + ANA_ANCILLARY_CTRL, Value);

                       /*ANA_SRC_CONFIG configuration*/
                       Value =  SRC_GAIN_FACTOR_BY_256|COEFF_PHASE1_T7_HD;
                       stvout_wr32(  Add + ANA_SRC_CONFIG, Value);


                       /*Hereafter the upsampler coeff used for HD display format 1080I and 720P*/
                       stvout_wr32(  Add + COEFF_PHASE1_TAP123 ,HDFORM_COEFF_PHASE1_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE1_TAP456 ,HDFORM_COEFF_PHASE1_TAP456);
                       stvout_wr32(  Add + COEFF_PHASE2_TAP123 ,HDFORM_COEFF_PHASE2_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE2_TAP456 ,HDFORM_COEFF_PHASE2_TAP456);
                       stvout_wr32(  Add + COEFF_PHASE3_TAP123 ,HDFORM_COEFF_PHASE3_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE3_TAP456 ,HDFORM_COEFF_PHASE3_TAP456);
                       stvout_wr32(  Add + COEFF_PHASE4_TAP123 ,HDFORM_COEFF_PHASE4_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE4_TAP456 ,HDFORM_COEFF_PHASE4_TAP456);
                       }
					   else if(Device_p->Format == STVOUT_HDRQ_MODE)
                       {
                       		switch (VTGTimingMode)
                       		{
								case STVTG_TIMING_MODE_1080I60000_74250 : /* no break */
                       			case STVTG_TIMING_MODE_1080I59940_74176 : /* no break */
                       			case STVTG_TIMING_MODE_1080I50000_74250_1 :

                            	/*ANA_CFG Configuration*/
                            	Value = ANA_SEL_MAIN_YCBCR_INPUT| ANA_RECORDER_CTRL_R_SEL2|ANA_RECORDER_CTRL_B_SEL1|
                            	PBPR_SYNC_OFFSET_HD_1080I|AWG_ASYNC_EN|AWG_ASYNC_FILTER_MODE_2;
                            	stvout_wr32(  Add + ANA_CFG, Value);

                             	/*ANA_SCALE_CTRL_C configuration*/
                             	Value = C_SCALE_COEFF_HD_1080I|C_OFFESET_HD;
                             	stvout_wr32(  Add + ANA_SCALE_CTRL_C, Value);

    	                        /* AWG Analog ancillary instruction*/
        	                    stvout_wr32(  Add + AWG_INST_ANC, AWG_INST_ANC_VALUE_1080I);
            	               	break;
                		       	case STVTG_TIMING_MODE_720P60000_74250 :
                       		   	case STVTG_TIMING_MODE_720P59940_74176 :
                       			case STVTG_TIMING_MODE_720P50000_74250 :
                             	/*ANA_CFG Configuration*/
                            	Value = ANA_SEL_MAIN_YCBCR_INPUT| ANA_RECORDER_CTRL_R_SEL2|ANA_RECORDER_CTRL_B_SEL1|
                            	PBPR_SYNC_OFFSET_HD_720P|AWG_ASYNC_EN|AWG_ASYNC_VSYNC_FIELD|AWG_ASYNC_FILTER_MODE_2;
                            	stvout_wr32(  Add + ANA_CFG, Value);

                            	/*ANA_SCALE_CTRL_C configuration*/
                             	Value = C_SCALE_COEFF_HD_720P|C_OFFESET_HD;
                             	stvout_wr32(  Add + ANA_SCALE_CTRL_C, Value);

                            	/* AWG Analog ancillary instruction*/
                             	stvout_wr32(  Add + AWG_INST_ANC,
								AWG_INST_ANC_VALUE_720P);
                            	break;

						 		default:
                               		break;
                       		}
					   /*ANA_SCALE_CTRL_Y configuration*/
                       Value = Y_SCALE_COEFF_HD|Y_OFFESET_HD;
                       stvout_wr32(  Add + ANA_SCALE_CTRL_Y, Value);

                       /* ANA_ANCILLARY_CTRL configuration*/
                       Value = stvout_rd32( Add + ANA_ANCILLARY_CTRL);
                       Value &=~ANA_ANC_MACRIVISION_EN;
                       stvout_wr32(  Add + ANA_ANCILLARY_CTRL, Value);

                       /*ANA_SRC_CONFIG configuration*/
					   Value =  SRC_GAIN_FACTOR_BY_512|COEFF_PHASE1_T7_HDRQ;
                       stvout_wr32(  Add + ANA_SRC_CONFIG, Value);


                       /*Hereafter the upsampler coeff used for HDRQ display format 1080I and 720P*/
                       stvout_wr32(  Add + COEFF_PHASE1_TAP123 ,HDRQFORM_COEFF_PHASE1_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE1_TAP456 ,HDRQFORM_COEFF_PHASE1_TAP456);
                       stvout_wr32(  Add + COEFF_PHASE2_TAP123 ,HDRQFORM_COEFF_PHASE2_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE2_TAP456 ,HDRQFORM_COEFF_PHASE2_TAP456);
                       stvout_wr32(  Add + COEFF_PHASE3_TAP123 ,HDRQFORM_COEFF_PHASE3_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE3_TAP456 ,HDRQFORM_COEFF_PHASE3_TAP456);
                       stvout_wr32(  Add + COEFF_PHASE4_TAP123 ,HDRQFORM_COEFF_PHASE4_TAP123);
                       stvout_wr32(  Add + COEFF_PHASE4_TAP456 ,HDRQFORM_COEFF_PHASE4_TAP456);
					   }
                       else
                       {
                           Ec = ST_ERROR_BAD_PARAMETER;
                       }
                   }
                   /* Close STVTG ...*/
                   STVTG_Close(VTGHandle);
                 }
                 break;

            case VOUT_OUTPUTS_RGB :
            case VOUT_OUTPUTS_YUV :
            case VOUT_OUTPUTS_CVBS :
            case VOUT_OUTPUTS_YC :
                /*to be defined*/
                /* if(Device_p->Format == STVOUT_SD_MODE)
                 {
                 }
                 else
                 {
                     Ec = ST_ERROR_BAD_PARAMETER;
                 }*/
                 break;
            default:
                Ec = ST_ERROR_BAD_PARAMETER;
                break;
        }
    #else

        U8 Sel_HD4x;
        #if defined (ST_7100)
            U32 CutId;
        #endif
        Add = (U32)Device_p->BaseAddress_p;
        STOS_InterruptLock();
        Value = stvout_rd32( Add + DSPCFG_CLK);
        #if defined (ST_7710)
            #ifndef STI7710_CUT2x
                Sel_HD4x = (U8)((Value & DSPCFG_CLK_HD4x_MASK)>>7);
            #else
                Sel_HD4x = 0;
            #endif /*STI7710_CUT2x*/
        #elif defined (ST_7100)
            /* get the cut ID*/
            CutId = ST_GetCutRevision();
            if( CutId >= 0xB0)
            {
                /*for 7100 cut2.x and +*/
                Sel_HD4x = (U8)((Value & DSPCFG_CLK_HD4x_MASK)>>10);
            }
            else
            {
                Sel_HD4x = 0;
            }
        #elif defined (ST_7109)
            Sel_HD4x = (U8)((Value & DSPCFG_CLK_HD4x_MASK)>>10);
        #endif /*ST_7710*/

        if((Sel_HD4x)&&(Device_p->Format != STVOUT_ED_MODE))
        {
            Device_p->Format = STVOUT_ED_MODE;
        }
        switch ( RgbYcbcr) {
            case VOUT_OUTPUTS_HD_RGB :
            case VOUT_OUTPUTS_HD_YCBCR :
                    if(Device_p->Format == STVOUT_ED_MODE)
                    {
                        Value = Value & DSPCFG_CLK_UPS_DIV_MASK;
                        Value = Value|(DSPCFG_CLK_NUP_SD<<2)|(DSPCFG_CLK_UPS_DIV_SD << 4);
                        stvout_wr32(  Add + DSPCFG_CLK, Value);
                        stvout_wr32(  Add + COEFF_SET1_1, ED_COEFF_SET1_1);
                        stvout_wr32(  Add + COEFF_SET1_2, ED_COEFF_SET1_2);
                        stvout_wr32(  Add + COEFF_SET1_3, ED_COEFF_SET1_3);
                        stvout_wr32(  Add + COEFF_SET1_4, ED_COEFF_SET1_4);
                        stvout_wr32(  Add + COEFF_SET2_1, ED_COEFF_SET2_1);
                        stvout_wr32(  Add + COEFF_SET2_2, ED_COEFF_SET2_2);
                        stvout_wr32(  Add + COEFF_SET2_3, ED_COEFF_SET2_3);
                        stvout_wr32(  Add + COEFF_SET2_4, ED_COEFF_SET2_4);
                        stvout_wr32(  Add + COEFF_SET3_1, ED_COEFF_SET3_1);
                        stvout_wr32(  Add + COEFF_SET3_2, ED_COEFF_SET3_2);
                        stvout_wr32(  Add + COEFF_SET3_3, ED_COEFF_SET3_3);
                        stvout_wr32(  Add + COEFF_SET3_4, ED_COEFF_SET3_4);
                        stvout_wr32(  Add + COEFF_SET4_1, ED_COEFF_SET4_1);
                        stvout_wr32(  Add + COEFF_SET4_2, ED_COEFF_SET4_2);
                        stvout_wr32(  Add + COEFF_SET4_3, ED_COEFF_SET4_3);
                        stvout_wr32(  Add + COEFF_SET4_4, ED_COEFF_SET4_4);
                    }
                    else if( (Device_p->Format == STVOUT_HDRQ_MODE))
                    {
                        Value = Value & DSPCFG_CLK_UPS_DIV_MASK;
                        Value = Value|(DSPCFG_CLK_NUP_SD<<2)|(DSPCFG_CLK_UPS_DIV_HDRQ<<4);
                        stvout_wr32(  Add + DSPCFG_CLK, Value);
                        stvout_wr32(  Add + COEFF_SET1_1, HDRQ_COEFF_SET1_1 );
                        stvout_wr32(  Add + COEFF_SET1_2, HDRQ_COEFF_SET1_2 );
                        stvout_wr32(  Add + COEFF_SET1_3, HDRQ_COEFF_SET1_3 );
                        stvout_wr32(  Add + COEFF_SET1_4, HDRQ_COEFF_SET1_4 );
                        stvout_wr32(  Add + COEFF_SET2_1, HDRQ_COEFF_SET2_1 );
                        stvout_wr32(  Add + COEFF_SET2_2, HDRQ_COEFF_SET2_2 );
                        stvout_wr32(  Add + COEFF_SET2_3, HDRQ_COEFF_SET2_3 );
                        stvout_wr32(  Add + COEFF_SET2_4, HDRQ_COEFF_SET2_4 );
                        stvout_wr32(  Add + COEFF_SET3_1, HDRQ_COEFF_SET3_1 );
                        stvout_wr32(  Add + COEFF_SET3_2, HDRQ_COEFF_SET3_2 );
                        stvout_wr32(  Add + COEFF_SET3_3, HDRQ_COEFF_SET3_3 );
                        stvout_wr32(  Add + COEFF_SET3_4, HDRQ_COEFF_SET3_4 );
                        stvout_wr32(  Add + COEFF_SET4_1, HDRQ_COEFF_SET4_1 );
                        stvout_wr32(  Add + COEFF_SET4_2, HDRQ_COEFF_SET4_2 );
                        stvout_wr32(  Add + COEFF_SET4_3, HDRQ_COEFF_SET4_3 );
                        stvout_wr32(  Add + COEFF_SET4_4, HDRQ_COEFF_SET4_4 );
                    }
                    else if(Device_p->Format == STVOUT_HD_MODE)
                    {
                        Value = Value & DSPCFG_CLK_UPS_DIV_MASK;
                        Value = Value|(DSPCFG_CLK_NUP_SD<<2)|(DSPCFG_CLK_UPS_DIV_HD<<4);
                        stvout_wr32(  Add + DSPCFG_CLK, Value);
                        stvout_wr32(  Add + COEFF_SET1_1, HD_COEFF_SET1_1);
                        stvout_wr32(  Add + COEFF_SET1_2, HD_COEFF_SET1_2);
                        stvout_wr32(  Add + COEFF_SET1_3, HD_COEFF_SET1_3);
                        stvout_wr32(  Add + COEFF_SET1_4, HD_COEFF_SET1_4);
                        stvout_wr32(  Add + COEFF_SET2_1, HD_COEFF_SET2_1);
                        stvout_wr32(  Add + COEFF_SET2_2, HD_COEFF_SET2_2);
                        stvout_wr32(  Add + COEFF_SET2_3, HD_COEFF_SET2_3);
                        stvout_wr32(  Add + COEFF_SET2_4, HD_COEFF_SET2_4);
                        stvout_wr32(  Add + COEFF_SET3_1, HD_COEFF_SET3_1);
                        stvout_wr32(  Add + COEFF_SET3_2, HD_COEFF_SET3_2);
                        stvout_wr32(  Add + COEFF_SET3_3, HD_COEFF_SET3_3);
                        stvout_wr32(  Add + COEFF_SET3_4, HD_COEFF_SET3_4);
                        stvout_wr32(  Add + COEFF_SET4_1, HD_COEFF_SET4_1);
                        stvout_wr32(  Add + COEFF_SET4_2, HD_COEFF_SET4_2);
                        stvout_wr32(  Add + COEFF_SET4_3, HD_COEFF_SET4_3);
                        stvout_wr32(  Add + COEFF_SET4_4, HD_COEFF_SET4_4);
                    }
                    else
                    {
                        Ec = ST_ERROR_BAD_PARAMETER;
                    }
                    break;

            case VOUT_OUTPUTS_RGB :
            case VOUT_OUTPUTS_YUV :
            case VOUT_OUTPUTS_CVBS :
            case VOUT_OUTPUTS_YC :
                    if(Device_p->Format == STVOUT_SD_MODE)
                    {
                        Value = Value & DSPCFG_CLK_UPS_DIV_MASK;
                        Value = Value|(DSPCFG_CLK_NUP_SD<<2)|(DSPCFG_CLK_UPSEL<<3)|(DSPCFG_CLK_UPS_DIV_SD<<4);
                        stvout_wr32(  Add + DSPCFG_CLK, Value);
                        stvout_wr32(  Add + COEFF_SET1_1, SD_COEFF_SET1_1);
                        stvout_wr32(  Add + COEFF_SET1_2, SD_COEFF_SET1_2);
                        stvout_wr32(  Add + COEFF_SET1_3, SD_COEFF_SET1_3);
                        stvout_wr32(  Add + COEFF_SET1_4, SD_COEFF_SET1_4);
                        stvout_wr32(  Add + COEFF_SET2_1, SD_COEFF_SET2_1);
                        stvout_wr32(  Add + COEFF_SET2_2, SD_COEFF_SET2_2);
                        stvout_wr32(  Add + COEFF_SET2_3, SD_COEFF_SET2_3);
                        stvout_wr32(  Add + COEFF_SET2_4, SD_COEFF_SET2_4);
                        stvout_wr32(  Add + COEFF_SET3_1, SD_COEFF_SET3_1);
                        stvout_wr32(  Add + COEFF_SET3_2, SD_COEFF_SET3_2);
                        stvout_wr32(  Add + COEFF_SET3_3, SD_COEFF_SET3_3);
                        stvout_wr32(  Add + COEFF_SET3_4, SD_COEFF_SET3_4);
                        stvout_wr32(  Add + COEFF_SET4_1, SD_COEFF_SET4_1);
                        stvout_wr32(  Add + COEFF_SET4_2, SD_COEFF_SET4_2);
                        stvout_wr32(  Add + COEFF_SET4_3, SD_COEFF_SET4_3);
                        stvout_wr32(  Add + COEFF_SET4_4, SD_COEFF_SET4_4);
                    }
                    else
                    {
                        Ec = ST_ERROR_BAD_PARAMETER;
                    }

                break;

            default:
                Ec = ST_ERROR_BAD_PARAMETER;
                break;

        }

        STOS_InterruptUnlock();
    #endif /*ST_7200||ST_7111||ST_7105*/
    return (Ec);
}

/*******************************************************************************
Name        : stvout_HalGetUpsampler
Description : Get enbaled output for Upsampler
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvout_HalGetUpsampler( void* BaseAddress, U8 RgbYcbcr)
{
    ST_ErrorCode_t Ec = ST_NO_ERROR;
    #if defined(ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        UNUSED_PARAMETER(BaseAddress);
        UNUSED_PARAMETER(RgbYcbcr);
    #else
        U32 Add = (U32)BaseAddress;
        U32 Value;
        U8 DspcfgClk,Upsample;
        STOS_InterruptLock();
        Value = stvout_rd32( Add + DSPCFG_CLK);
        DspcfgClk = (U8) ((Value & DSPCFG_CLK_UPSEL_MASK)>>3);
        Upsample  = (U8) ((Value & DSPCFG_CLK_NUP_SD_MASK)>>2);

        if( Upsample & DSPCFG_CLK_NUP_SD )
        {
            switch ( RgbYcbcr) {
                case VOUT_OUTPUTS_HD_RGB :
                case VOUT_OUTPUTS_HD_YCBCR :
                    if ( DspcfgClk & DSPCFG_CLK_UPSEL )
                    {
                        /* HD_DACS was selected to output SD mode */
                        Ec = ST_ERROR_BAD_PARAMETER ;
                    }
                    break;
                case VOUT_OUTPUTS_RGB :
                case VOUT_OUTPUTS_YUV :
                case VOUT_OUTPUTS_CVBS :
                case VOUT_OUTPUTS_YC :
                    if (!DspcfgClk)
                    {
                    /* HD_DACS was selected to output HD mode */
                    Ec = ST_ERROR_BAD_PARAMETER ;
                    }
                    break;

                default:
                    Ec = ST_ERROR_BAD_PARAMETER;
                    break;

            }
        }
        STOS_InterruptUnlock();
      #endif /*ST_7200||ST_7111||ST_7105*/
      return (Ec);
 }
/*******************************************************************************
Name        : stvout_HalSetUpsamplerOff
Description : Disable Upsampler coefficient
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvout_HalSetUpsamplerOff(stvout_Device_t *Device_p)
{
  ST_ErrorCode_t Ec = ST_NO_ERROR;
  #if defined(ST_7200)|| defined (ST_7111)|| defined (ST_7105)
     UNUSED_PARAMETER(Device_p);
  #else
     stvout_wr32( (U32)Device_p->BaseAddress_p  + DSPCFG_CLK, DSPCFG_CLK_RST);
  #endif /*ST_7200||ST_7111||ST_7105*/
  return (Ec);
}
#endif

/* ----------------------------- End of file ------------------------------ */
