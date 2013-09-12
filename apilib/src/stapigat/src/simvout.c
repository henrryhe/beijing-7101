/*******************************************************************************

File name   : simvout.c

Description : STVOUT driver simulator source file.
 * This file is intended to free test application from STVOUT driver to prevent from
 * circular dependencies.
 * VOUT features are basic.

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
21 Feb 2002        Created                                           HS
14 Oct 2002        Add support for 5517                              HSdLM
22 May 2003        Add support for 5528                              HSdLM
14 Aug 2003        Add AUX denc support                              OB
03 dec 2004        Port to 5105 + 5301 + 8010, supposing same as 5100 TA+HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <string.h>
#include <stdio.h>
#include "testcfg.h"

#ifdef USE_VOUTSIM

#include "stddefs.h"
#include "stdevice.h"

#include "stsys.h"
#include "stdenc.h"
#include "testtool.h"
#include "simvout.h"
#ifdef ST_OSLINUX
#include "iocstapigat.h"
#endif


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#if defined (ST_5508) || defined (ST_5518) || defined (ST_5528) || \
    defined (ST_5510) || defined (ST_5512) || \
    (defined (ST_5514) && !defined(mb290)) || defined (ST_5516) || \
    defined (ST_5517) || defined (ST_7710) || defined (ST_7100)|| defined (ST_7109)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#elif defined (ST_7015)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_7015
#elif defined (ST_7020)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_7020
#elif defined (ST_GX1)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_GX1
#elif defined (ST_5100)|| defined (ST_5105)|| defined (ST_5301)|| defined (ST_8010)|| defined (ST_5188)|| defined (ST_5525)\
   || defined (ST_5107)|| defined (ST_7200)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_V13
#else
#error Not defined yet
#endif


/* DEN - Digital encoder registers - standard 8 bit calls     --------_----- */
#define DENC_CFG8   0x08  /* Configuration register 8                [7:0]   */

  /* specific configuration of outputs  dac1  dac2  dac3  dac4  dac5  dac6   */
#define DENC_CFG8_CONF_OUT_0   0x00 /*   Y     C    CVBS   C     Y    CVBS   */
#define DENC_CFG8_CONF_OUT_1   0x10 /*   Y     C    CVBS   V     Y     U     */
#define DENC_CFG8_CONF_OUT_2   0x20 /*   Y     C    CVBS   R     G     B     */
#define DENC_CFG8_MASK_CONF    0xCF /* mask for configuration of outputs     */
#define DENC_CFG13_DAC123_OUT7  0x38 /* Y_aux C_aux CVBS_aux                 */
#define DENC_CFG13_DAC123_OUT7_MAIN  0x00 /* Y_main C_main CVBS_main                 */
#define DENC_CFG13_DAC456_OUT7  0x07 /* Y_main C_main CVBS_main              */

#if defined (ST_7020)
#define DSPCFG_DAC             0x200C       /* Dac configuration */
#define DSPCFG_ANA             0x0008       /* Main analog output config */
#define DSPCFG_DAC_ENABLE_SD   0x00020000   /* Enable SD DAC out */
#define DSPCFG_ANA_RGB         0x00000001   /* Enable RGB output */
#endif /* ST_7020 */

#if defined (ST_7710)
#define VOS_BASE_ADDRESS       ST7710_VOS_BASE_ADDRESS
#ifdef STI7710_CUT2x
#define DSPCFG_DAC_ENABLE_SD   0x0008       /* Enable SD DAC out */
#else
#define DSPCFG_DAC_ENABLE_SD   0x0080       /* Enable SD DAC out */
#endif
#define DSPCFG_DAC             0x00DC       /* Dac configuration */
#define DSPCFG_ANA             0x0078       /* Main analog output config */
#define DSPCFG_DIG             0x0074       /* Digital output config */
#define DSPCFG_ANA_YCbCr       0x0          /* Enable YCbCr output */
#define DSPCFG_ANA_RGB         0x1         /* Enable RGB output */
#define DSPCFG_DIG_HDS         0x1
#define DSPCFG_DIG_EN          0x1
#define DSPCFG_CLK             0x0070      /* Denc select : main/aux compositor*/
#endif /* ST_7710 */

#if defined (ST_7100)
#ifdef ST_OSLINUX
#undef VOS_BASE_ADDRESS
#define VOS_BASE_ADDRESS       OutputStageMap.MappedBaseAddress
#else
#define VOS_BASE_ADDRESS       ST7100_VOS_BASE_ADDRESS
#endif
#define DSPCFG_DAC             0x00DC       /* Dac configuration */
#define DSPCFG_ANA             0x0078       /* Main analog output config */
#define DSPCFG_DIG             0x0074       /* Digital output config */
#define DSPCFG_DAC_ENABLE_SD   0x0008       /* Enable SD DAC out */
#define DSPCFG_ANA_YCbCr       0x0          /* Enable YCbCr output */
#define DSPCFG_ANA_RGB         0x1         /* Enable RGB output */
#define DSPCFG_DIG_HDS         0x1
#define DSPCFG_DIG_EN          0x1
#define DSPCFG_CLK             0x0070      /* Denc select : main/aux compositor*/

#define DEVICE_ID 0x0
#ifndef SYS_CFG_BASE_ADDRESS
#ifdef ST_OSLINUX
#define SYS_CFG_BASE_ADDRESS       SysCfgMap.MappedBaseAddress
#else
#define SYS_CFG_BASE_ADDRESS 0xb9001000
#endif                                  /* ST_OSLINUX */
#endif
#endif /* ST_7100 */

#if defined (ST_7109)
#ifdef ST_OSLINUX
#undef VOS_BASE_ADDRESS
#define VOS_BASE_ADDRESS       OutputStageMap.MappedBaseAddress
#else
#define VOS_BASE_ADDRESS       ST7109_VOS_BASE_ADDRESS
#endif
#define DSPCFG_DAC             0x00DC       /* Dac configuration */
#define DSPCFG_ANA             0x0078       /* Main analog output config */
#define DSPCFG_DIG             0x0074       /* Digital output config */
#define DSPCFG_DAC_ENABLE_SD   0x0008       /* Enable SD DAC out */
#define DSPCFG_ANA_YCbCr       0x0          /* Enable YCbCr output */
#define DSPCFG_ANA_RGB         0x1         /* Enable RGB output */
#define DSPCFG_DIG_HDS         0x1
#define DSPCFG_DIG_EN          0x1
#define DSPCFG_CLK             0x0070      /* Denc select : main/aux compositor*/
#endif /* ST_7109 */

#if defined (ST_7200)
#ifdef ST_OSLINUX
#undef VOS_BASE_ADDRESS
#define VOS_BASE_ADDRESS       OutputStageMap.MappedBaseAddress
#endif/*ST_OSLINUX*/
#define DENC_CFG13          0x5F
#endif /* ST_7200 */


#if defined (ST_5528)
#ifdef ST_OSLINUX
#define LINUX_VOUT_BASE_ADDRESS OutputStageMap.MappedBaseAddress + 0xc00
#define LINUX_DVO_BASE_ADDRESS OutputStageMap.MappedBaseAddress + 0x800

#define VOS_MISC_VDAC0_CTL  (LINUX_VOUT_BASE_ADDRESS +  0x004)
#define VOS_MISC_VDAC1_CTL  (LINUX_VOUT_BASE_ADDRESS +  0x008)
#define VOS_MISC_CTL         LINUX_VOUT_BASE_ADDRESS
#define DENC_CFG13          0x5F
#define DVO_CTL_656         (LINUX_DVO_BASE_ADDRESS)
#define DVO_CTL_PADS        (LINUX_DVO_BASE_ADDRESS + 0x00000004)
#define DVO_VPO_656         (LINUX_DVO_BASE_ADDRESS + 0x00000008)
#define DVO_VPS_656         (LINUX_DVO_BASE_ADDRESS + 0x0000000c)
#else
#define VOS_MISC_VDAC0_CTL  (ST5528_VOUT_BASE_ADDRESS +  0x004)
#define VOS_MISC_VDAC1_CTL  (ST5528_VOUT_BASE_ADDRESS +  0x008)
#define VOS_MISC_CTL         ST5528_VOUT_BASE_ADDRESS
#define DENC_CFG13          0x5F
#define DVO_CTL_656         (ST5528_DVO_BASE_ADDRESS)
#define DVO_CTL_PADS        (ST5528_DVO_BASE_ADDRESS + 0x00000004)
#define DVO_VPO_656         (ST5528_DVO_BASE_ADDRESS + 0x00000008)
#define DVO_VPS_656         (ST5528_DVO_BASE_ADDRESS + 0x0000000c)
#endif
#define DVO_CTL_656_POE             0x00000002
#define DVO_CTL_656_ADDYDS          0x00000008
#define DVO_CTL_656_EN_656_MODE     0x00000001
#define DVO_CTL_656_DIS_1_254       0x00010000
#define DENC_AUX_SEL				0x1
#define DVO_IN_SEL_AUX				0x2
#endif /* ST_5528 */

#if defined (ST_5100)
#define VOS_MISC_VDAC0_CTL  (ST5100_VOUT_BASE_ADDRESS +  0x004)
#define VOS_MISC_VDAC1_CTL  (ST5100_VOUT_BASE_ADDRESS +  0x008)
#define VOS_MISC_CTL         ST5100_VOUT_BASE_ADDRESS
#define DENC_CFG13          0x5F
#define DVO_CTL_656         (ST5100_DVO_BASE_ADDRESS)
#define DVO_CTL_PADS        (ST5100_DVO_BASE_ADDRESS + 0x00000004)
#define DVO_VPO_656         (ST5100_DVO_BASE_ADDRESS + 0x00000008)
#define DVO_VPS_656         (ST5100_DVO_BASE_ADDRESS + 0x0000000c)
#define DVO_CTL_656_POE             0x00000002
#define DVO_CTL_656_ADDYDS          0x00000008
#define DVO_CTL_656_EN_656_MODE     0x00000001
#define DVO_CTL_656_DIS_1_254       0x00010000
#define DENC_AUX_SEL				0x1
#define DVO_IN_SEL_AUX				0x2
#endif /* ST_5100 */

#if defined (ST_5105) || defined (ST_5188)|| defined (ST_5525) || defined (ST_5107)
#define DENC_CFG13           0x5F
/* To be verified */
#endif

#if defined (ST_5301)
#define VOS_MISC_VDAC0_CTL  (ST5301_VOUT_BASE_ADDRESS +  0x004)
#define VOS_MISC_VDAC1_CTL  (ST5301_VOUT_BASE_ADDRESS +  0x008)
#define VOS_MISC_CTL         ST5301_VOUT_BASE_ADDRESS
#define DENC_CFG13           0x5F
#define DVO_CTL_656         (ST5301_DVO_BASE_ADDRESS)
#define DVO_CTL_PADS        (ST5301_DVO_BASE_ADDRESS + 0x00000004)
#define DVO_VPO_656         (ST5301_DVO_BASE_ADDRESS + 0x00000008)
#define DVO_VPS_656         (ST5301_DVO_BASE_ADDRESS + 0x0000000c)
#define DVO_CTL_656_POE             0x00000002
#define DVO_CTL_656_ADDYDS          0x00000008
#define DVO_CTL_656_EN_656_MODE     0x00000001
#define DVO_CTL_656_DIS_1_254       0x00010000
#define DENC_AUX_SEL				0x1
#define DVO_IN_SEL_AUX              0x2

#endif

#if defined (ST_8010)
#define DENC_CFG13           0x5F
/* To be verified */
#endif

/* Private variables ------------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define voutsim_Write32(a, v)     STSYS_WriteRegDev32LE((a), (v))
#define voutsim_Read32(a)         STSYS_ReadRegDev32LE((a))

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : VoutSimSetViewport
Description : Set the top-let and bottom-right of the viewport
Parameters  :
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
#if defined (ST_5528)
void VoutSimSetViewport(const BOOL IsNTSC, const VoutSimSelectOut_t SelectOut)
{
    U32 reg_val=0;
    if ( IsNTSC )
    {
        reg_val = (3 << 8);
        reg_val |= DVO_CTL_656_POE|DVO_CTL_656_ADDYDS|DVO_CTL_656_EN_656_MODE|DVO_CTL_656_DIS_1_254;
        voutsim_Write32((U32)DVO_CTL_656, reg_val);
        voutsim_Write32((U32)DVO_CTL_PADS,0x0);
        voutsim_Write32((U32)DVO_VPO_656,(34 << 16) | 121);
        voutsim_Write32((U32)DVO_VPS_656,(519 << 16) | 842);
    }
    else
    {
        reg_val = (2 << 8);
        if (SelectOut==VOUTSIM_SELECT_OUT_MAIN)
        {
            reg_val |= DVO_CTL_656_EN_656_MODE|DVO_CTL_656_DIS_1_254;

        }
        else
        {
            reg_val |= DVO_CTL_656_DIS_1_254;
        }
        voutsim_Write32((U32)DVO_CTL_656, reg_val);
        voutsim_Write32((U32)DVO_CTL_PADS,0x0);
        voutsim_Write32((U32)DVO_VPO_656, (46 << 16) | 131);
        voutsim_Write32((U32)DVO_VPS_656, (621 << 16) | 852);
    }
} /* end of VoutSimSetViewport */
#endif

/*******************************************************************************
Name        : VoutSimDencOut
Description : Sets DENC tridacs out
Parameters  :
Assumptions : parameters are OK
Limitations :
Returns     : none
*******************************************************************************/
void VoutSimDencOut( const STDENC_Handle_t     DencHnd,
                     const STDENC_DeviceType_t DencType,
                     const VoutSimOutput_t     Output,
                     const VoutSimSelectOut_t  SelectOut)
{
#if defined (ST_7100)
        U32 CutId, DeviceId;
    #endif

#if defined (ST_5528) || defined (ST_5100)|| defined (ST_5301)
    voutsim_Write32((U32)VOS_MISC_CTL, ( SelectOut == VOUTSIM_SELECT_OUT_AUX) ?
										(U32)(DENC_AUX_SEL|DVO_IN_SEL_AUX) : (U32)DENC_AUX_SEL);        /* Select aux output of the compositor */
    STDENC_RegAccessLock(DencHnd);
	if (DencType==STDENC_DEVICE_TYPE_4629)
	{
	    STDENC_WriteReg8(DencHnd, DENC_CFG13, (U32)DENC_CFG13_DAC123_OUT7_MAIN );
	}
	else
	{
        if (DencType== STDENC_DEVICE_TYPE_DENC)
        {
	    voutsim_Write32((U32)VOS_MISC_VDAC0_CTL, 0); /* turn to normal mode, reset: high impedance */
    	voutsim_Write32((U32)VOS_MISC_VDAC1_CTL, 0);
		STDENC_WriteReg8(DencHnd, DENC_CFG13, ( SelectOut == VOUTSIM_SELECT_OUT_AUX) ?
				(U32)DENC_CFG13_DAC123_OUT7 : (U32)DENC_CFG13_DAC456_OUT7);
	}
        else if (DencType== STDENC_DEVICE_TYPE_V13)
        {

            STDENC_WriteReg8(DencHnd, DENC_CFG13, ( SelectOut == VOUTSIM_SELECT_OUT_AUX) ?
                    (U32)DENC_CFG13_DAC123_OUT7 : (U32)DENC_CFG13_DAC456_OUT7);

        }
	}
    STDENC_RegAccessUnlock(DencHnd);
#elif defined (ST_5105) || defined (ST_8010)
    STDENC_RegAccessLock(DencHnd);
    STDENC_WriteReg8(DencHnd, DENC_CFG13, (U32)DENC_CFG13_DAC123_OUT7);
    STDENC_RegAccessUnlock(DencHnd);
#elif defined (ST_5188)|| defined (ST_5525)|| defined (ST_5107)
    STDENC_RegAccessLock(DencHnd);
    STDENC_WriteReg8(DencHnd, DENC_CFG13, (U32)DENC_CFG13_DAC123_OUT7_MAIN);
    STDENC_RegAccessUnlock(DencHnd);
#else
    U8 Value=0;
#if defined (ST_7020)
    voutsim_Write32((U32)STI7020_BASE_ADDRESS + ST7020_DSPCFG_OFFSET + DSPCFG_DAC, DSPCFG_DAC_ENABLE_SD);
    voutsim_Write32((U32)STI7020_BASE_ADDRESS + ST7020_DSPCFG_OFFSET + DSPCFG_ANA, DSPCFG_ANA_RGB);
#elif defined (ST_7710)|| defined (ST_7100) || defined (ST_7109)
    #if defined (ST_7710)
       #ifdef STI7710_CUT2x
          voutsim_Write32((U32)VOS_BASE_ADDRESS + DSPCFG_DAC, 0x0);
       #else
          voutsim_Write32((U32)VOS_BASE_ADDRESS + DSPCFG_DAC, DSPCFG_DAC_ENABLE_SD);
       #endif
    #elif  defined (ST_7100)
       /*get cut Id */
        DeviceId = voutsim_Read32((U32)SYS_CFG_BASE_ADDRESS + DEVICE_ID);
        CutId = (U32)((DeviceId & 0xf0000000) >> 28) + 1 ;

        if( CutId >=2)
        {
            voutsim_Write32((U32)VOS_BASE_ADDRESS + DSPCFG_DAC, 0x180);
        }
        else
        {
            voutsim_Write32((U32)VOS_BASE_ADDRESS + DSPCFG_DAC, 0x0);
        }
    #elif  defined (ST_7109)
        voutsim_Write32((U32)VOS_BASE_ADDRESS + DSPCFG_DAC, 0x180);
    #endif

    voutsim_Write32((U32)VOS_BASE_ADDRESS + DSPCFG_ANA, DSPCFG_ANA_RGB);
    voutsim_Write32((U32)VOS_BASE_ADDRESS + DSPCFG_DIG, (DSPCFG_DIG_HDS | DSPCFG_DIG_EN << 1));

    switch (SelectOut)
    {
        case VOUTSIM_SELECT_OUT_MAIN :
           voutsim_Write32((U32)VOS_BASE_ADDRESS + DSPCFG_CLK, 0x00000001); /* main compositor selected by the DENC */
        break;
        case VOUTSIM_SELECT_OUT_AUX :
           voutsim_Write32((U32)VOS_BASE_ADDRESS + DSPCFG_CLK, 0x00000000); /* aux compositor selected by the DENC */
        break;
    }
#elif defined (ST_7200)
    STDENC_RegAccessLock(DencHnd);
    STDENC_WriteReg8(DencHnd, DENC_CFG13, (U32)DENC_CFG13_DAC123_OUT7);
    STDENC_RegAccessUnlock(DencHnd);
    UNUSED_PARAMETER(SelectOut);
#endif
    switch (Output)
    {
        case VOUTSIM_OUTPUT_RGB :
            Value = DENC_CFG8_CONF_OUT_2;
            break;
        case VOUTSIM_OUTPUT_YUV :
            Value = DENC_CFG8_CONF_OUT_1;
            break;
        case VOUTSIM_OUTPUT_YC :
        case VOUTSIM_OUTPUT_CVBS :
            if ( (DencType == STDENC_DEVICE_TYPE_7015) || ( DencType == STDENC_DEVICE_TYPE_7020))
            {
                Value = DENC_CFG8_CONF_OUT_0;
            }
            else/* STDENC_DEVICE_TYPE_DENC, STDENC_DEVICE_TYPE_GX1 */
            {
                Value = DENC_CFG8_CONF_OUT_2;
            }
            break;
         default:
            break;
    }

    STDENC_RegAccessLock(DencHnd);
    STDENC_MaskReg8(DencHnd, DENC_CFG8, (U8)DENC_CFG8_MASK_CONF, Value);
    STDENC_RegAccessUnlock(DencHnd);
#endif

} /* end of VoutSimDencOut() */

/*-------------------------------------------------------------------------
 * Function : VoutSimDencOutCmd
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL VoutSimDencOutCmd( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_Handle_t DENCHndl;
    STDENC_DeviceType_t DencType;
    VoutSimOutput_t Output;
    BOOL RetErr , IsOutputAux = FALSE;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DENCHndl);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Denc Handle\n");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, DENC_DEVICE_TYPE, (S32*)&DencType);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected DencType\n");
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, VOUTSIM_OUTPUT_CVBS, (S32*)&Output);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected OutputType, from 0 to 3\n"); /* see VoutSimOutput_t definition */
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&IsOutputAux);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected number 0 or 1\n");
        return(TRUE);
    }
    VoutSimDencOut(DENCHndl, DencType, Output, IsOutputAux ? VOUTSIM_SELECT_OUT_AUX : VOUTSIM_SELECT_OUT_MAIN);
    return(FALSE);
} /* end of VoutSimDencOutCmd */

#if defined  (ST_5528)
BOOL VoutSimSetViewportCmd( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL IsNTSC = TRUE;
    S32 LVar;
    ST_ErrorCode_t RetErr = ST_NO_ERROR;

    RetErr = STTST_GetInteger( pars_p, TRUE, (S32*)&IsNTSC);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected number 0 or 1\n");
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0 , (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected MAIN or AUX \n");
        return(TRUE);
    }

    VoutSimSetViewport(IsNTSC, LVar);
    return (FALSE);
} /* end of VoutSimSetViewportCmd */
#endif

/*-------------------------------------------------------------------------
 * Function : VoutSim_RegisterCmd
 *            Definition of register command
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL VoutSim_RegisterCmd(void)
{
    BOOL RetErr = FALSE;

    RetErr  = STTST_RegisterCommand( "VOUTSim_DencOut", VoutSimDencOutCmd, "DENC output configuration by simulation (without STVOUT)");
#if defined  (ST_5528)
    RetErr |= STTST_RegisterCommand( "VOUTSim_SetViewport", VoutSimSetViewportCmd, "Viewport configuration by simulation (without STVOUT)");
#endif
    if ( RetErr )
    {
        printf( "VoutSim_RegisterCmd() \t: failed !\n");
    }
    else
    {
        printf( "VoutSim_RegisterCmd() \t: ok\n");
    }
    return(RetErr ? FALSE : TRUE);
} /* end of VoutSim_RegisterCmd() */

#endif /* USE_VOUTSIM */

/* End of simvout.c */
