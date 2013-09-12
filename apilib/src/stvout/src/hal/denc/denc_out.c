/*******************************************************************************

File name   : denc_out.c

Description : VOUT driver module for denc

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
22 Fev 2001        Created                                          JG
13 Sep 2001        Update STDENC macro names                        HSdLM
23 Apr 2002        New DeviceType for STi7020, SD DAC enable        HSdLM
14 Mar 2003        Add support for DENC cell version 12             HSdLM
13 Aug 2003        Add support for 4629                             OB
01 Mar 2004        Add support for STi5100                          HSdLM
05 Jan 2005        Add support for STi5105                          AC
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#if !defined (ST_OSLINUX)
#include <string.h>
#endif

#include "stsys.h"

#include "vout_drv.h"
#include "dnc_reg.h"
#include "denc_out.h"

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

#define BLKDAC                 0x80
#if defined (ST_7710)|| defined (ST_7100) || defined (ST_7109)
#define DSPCFG_DAC      0x00DC  /* Dac configuration */
#elif defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
#define DSPCFG_DAC      0     /* Not used */
#else /*7020 only*/
#define DSPCFG_DAC      0x200C      /* Dac configuration */
#endif

#if defined (ST_7710)
#define DSPCFG_DAC_ENABLE_SD   0x00000080  /* Enable SD DAC out on 7710 cut 3.0 */
#elif defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
#define DSPCFG_DAC_ENABLE_SD   0     /* Not used */
#else
#define DSPCFG_DAC_ENABLE_SD   0x00020000  /* Enable SD DAC out, 7020 only */
#endif

#define DSPCFG_DAC_PD          0x00002000  /* power down , Disable HD DAC out, 7020 only  */

#define VOS_MISC_VDAC0_CTL     0x004             /* 5528 only */
#define VOS_MISC_VDAC0_CTL_HIGH_IMPEDANCE 0x02  /* 5528 only */
#define VOS_MISC_VDAC1_CTL     0x008             /* 5528 only */
#define VOS_MISC_VDAC1_CTL_HIGH_IMPEDANCE 0x02  /* 5528 only */
#define TRIDAC_MAX_ROW         9                                      /* Denc V12 only */
#define TRIDAC_MAX_ROW_4629    3                                      /* ST4629 only */
#define VDAC_CFG_ADR                     0x230          /* ST4629 only */
#define VOS_MISC_VDAC_CTL_HIGH_IMPEDANCE 0x02          /* 4629 only */

/* Private variables (static) --------------------------------------------- */

/* Denc V12 */


static const U8 MainChromaFilterReg[9] = { DENC_CHRM0, DENC_CHRM1, DENC_CHRM2, DENC_CHRM3, DENC_CHRM4,
                                           DENC_CHRM5, DENC_CHRM6, DENC_CHRM7, DENC_CHRM8};

static const U8 AuxChromaFilterReg[9] =  { DENC_AUX_CHRM0, DENC_AUX_CHRM1, DENC_AUX_CHRM2, DENC_AUX_CHRM3, DENC_AUX_CHRM4,
                                           DENC_AUX_CHRM5, DENC_AUX_CHRM6, DENC_AUX_CHRM7, DENC_AUX_CHRM8};

/* DEN_CFG13 */
static const STVOUT_DAC_CONF_t AuthorizedDacs123Configuration5525[3]=
{
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT0},
    {{STVOUT_DAC_OUTPUT_RGB, STVOUT_DAC_OUTPUT_RGB, STVOUT_DAC_OUTPUT_RGB}, DENC_CFG13_DAC123_OUT1},
    {{STVOUT_DAC_OUTPUT_YUV_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YUV_MAIN}, DENC_CFG13_DAC123_OUT2},
};

static const STVOUT_DAC_CONF_t AuthorizedDacs4Configuration5525[3]=
{
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC4_OUT0},
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC4_OUT1},
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC4_OUT2},
};


static const STVOUT_DAC_CONF_t AuthorizedDacs123Configuration[8]=
{
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT0},
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC123_OUT1},
    {{STVOUT_DAC_OUTPUT_RGB, STVOUT_DAC_OUTPUT_RGB, STVOUT_DAC_OUTPUT_RGB}, DENC_CFG13_DAC123_OUT2},
    {{STVOUT_DAC_OUTPUT_YUV_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YUV_MAIN}, DENC_CFG13_DAC123_OUT3},
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_AUX, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC123_OUT4},
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT5},
    {{STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT6},
    {{STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC123_OUT7}
};


static const STVOUT_DAC_CONF_t AuthorizedDacs456Configuration[8]=
{
    {{STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC456_OUT0},
    {{STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC456_OUT1},
    {{STVOUT_DAC_OUTPUT_RGB, STVOUT_DAC_OUTPUT_RGB, STVOUT_DAC_OUTPUT_RGB}, DENC_CFG13_DAC456_OUT2},
    {{STVOUT_DAC_OUTPUT_YUV_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YUV_AUX}, DENC_CFG13_DAC456_OUT3},
    {{STVOUT_DAC_OUTPUT_CVBS_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC456_OUT4},
    {{STVOUT_DAC_OUTPUT_CVBS_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC456_OUT5},
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC456_OUT6},
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC456_OUT7}
};

/* END DEN_CFG13 */

/* For STi4629 */
 static const STVOUT_DAC_CONF_t AuthorizedDacs123Configuration4629[3]=
 {
    {{STVOUT_DAC_OUTPUT_YC_MAIN,STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT0},
    {{STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB, STVOUT_DAC_OUTPUT_RGB}, DENC_CFG13_DAC123_OUT2},
    {{STVOUT_DAC_OUTPUT_YUV_MAIN,STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YUV_MAIN}, DENC_CFG13_DAC123_OUT3}
 };
/* End STi4629 */

/* For STi7710 doesn't support RGB output.*/
static const STVOUT_DAC_CONF_t AuthorizedDacsConfiguration7710[13]=
{
    /* Use of DAC456_CONF for YC and CVBS outsputs on DACS 4,5 and 6... */
    {{STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC456_OUT1},
    {{STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB}, DENC_CFG13_DAC456_OUT2},
    {{STVOUT_DAC_OUTPUT_CVBS_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC456_OUT4},
    {{STVOUT_DAC_OUTPUT_CVBS_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC456_OUT5},
    {{STVOUT_DAC_OUTPUT_YC_MAIN,STVOUT_DAC_OUTPUT_YC_MAIN,STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC456_OUT6},
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC456_OUT7},
     /* Use of DAC123_CONF for YPbPr output on HD DACS which are connected to DACS 1,2 and 3... */
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT0},
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC123_OUT1},
    {{STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB}, DENC_CFG13_DAC123_OUT2},
    {{STVOUT_DAC_OUTPUT_YUV_MAIN,STVOUT_DAC_OUTPUT_YC_MAIN,STVOUT_DAC_OUTPUT_YUV_MAIN}, DENC_CFG13_DAC123_OUT3},
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN,STVOUT_DAC_OUTPUT_CVBS_AUX,STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC123_OUT4},
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN,STVOUT_DAC_OUTPUT_CVBS_AUX,STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT5},
    {{STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT6}

};
/* End STi7710 */

/* For STi7100 .*/
static const STVOUT_DAC_CONF_t AuthorizedDacsConfiguration7100[13]=
{
    /* Use of DAC456_CONF for YC and CVBS outsputs on DACS 4,5 and 6... */
    {{STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC456_OUT1},
    {{STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB}, DENC_CFG13_DAC456_OUT2},
    {{STVOUT_DAC_OUTPUT_CVBS_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN}, DENC_CFG13_DAC456_OUT4},
    {{STVOUT_DAC_OUTPUT_CVBS_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC456_OUT5},
    {{STVOUT_DAC_OUTPUT_YC_MAIN,STVOUT_DAC_OUTPUT_YC_MAIN,STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC456_OUT6},
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC456_OUT7},
    /* Use of DAC123_CONF for YPbPr ,RGB output DACS 1,2 and 3... */
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT0},
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC123_OUT1},
    {{STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB}, DENC_CFG13_DAC123_OUT2},
    {{STVOUT_DAC_OUTPUT_YUV_MAIN,STVOUT_DAC_OUTPUT_YC_MAIN,STVOUT_DAC_OUTPUT_YUV_MAIN}, DENC_CFG13_DAC123_OUT3},
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN,STVOUT_DAC_OUTPUT_CVBS_AUX,STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC123_OUT4},
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN,STVOUT_DAC_OUTPUT_CVBS_AUX,STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT5},
    {{STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT6}

};
/* End STi7100 */

/* For STi7200 .*/
static const STVOUT_DAC_CONF_t AuthorizedDacsConfiguration7200[13]=
{
    /* Use of DAC123_CONF for YC, RGB, YUV and CVBS outputs on DACS 1,2 and 3... */
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT0},
    {{STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_YC_MAIN, STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC123_OUT1},
    {{STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB,STVOUT_DAC_OUTPUT_RGB}, DENC_CFG13_DAC123_OUT2},
    {{STVOUT_DAC_OUTPUT_YUV_MAIN,STVOUT_DAC_OUTPUT_YUV_MAIN,STVOUT_DAC_OUTPUT_YUV_MAIN}, DENC_CFG13_DAC123_OUT3},
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN,STVOUT_DAC_OUTPUT_CVBS_AUX,STVOUT_DAC_OUTPUT_CVBS_AUX}, DENC_CFG13_DAC123_OUT4},
    {{STVOUT_DAC_OUTPUT_CVBS_MAIN,STVOUT_DAC_OUTPUT_CVBS_AUX,STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT5},
    {{STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC123_OUT6},
    /* Use of DAC456_CONF for CVBS outputs on DACS 6... */
    {{STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_YC_AUX, STVOUT_DAC_OUTPUT_CVBS_MAIN}, DENC_CFG13_DAC456_OUT1}
};
/* End STi7200 */

/* Global Variables --------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */

#define stvout_rd32(a)     STSYS_ReadRegDev32LE((void*)(a))
#define stvout_wr32(a,v)   STSYS_WriteRegDev32LE((void*)(a),(v))
#define stvout_rd8(a)     STSYS_ReadRegDev8((void*)(a))
#define stvout_wr8(a,v)   STSYS_WriteRegDev8((void*)(a),(v))

/* Private Function prototypes -------------------------------------------- */
ST_ErrorCode_t stvout_HalCheckOptionIsSupported(stvout_Device_t * Device_p, const STVOUT_OptionParams_t * const OptionParams_p);
ST_ErrorCode_t stvout_HalDencWrite8(stvout_Device_t * Device_p, U32 Reg, U8 RegMask, U8 Value);
ST_ErrorCode_t stvout_HalDencRead8(stvout_Device_t * Device_p, U32 Reg, U8* Value_p);

/*
******************************************************************************
Exported Functions
******************************************************************************
*/

/* CFG5 : DAC configuration
    -------------------------------------------------
bit |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
    -------------------------------------------------
    |     |     |     |     |     |     |     |     | DENC Version 3
    |  0  | cvbs|  .  |  .  |  y  |  c  | cvbs| inv |
    |  1  | cvbs|  .  |  .  |  g  |  r  |  b  | inv |
    -------------------------------------------------
    |     |     |     |     |     |     |     |     | DENC Version 5
    |  .  | cvbs|  y  |  c  |  r  |  g  |  b  | inv |
    -------------------------------------------------
    |     |     |     |     |     |     |     |     | DENC Version 5 to 11
    |  .  |  y  |  c  | cvbs|  c  |  y  | cvbs| inv | Conf 0 (cfg8)
    |  .  |  y  |  c  | cvbs|  v  |  y  |  u  | inv | Conf 1 (cfg8)
    |  .  |  y  |  c  | cvbs|  r  |  g  |  b  | inv | Conf 2 (cfg8)
    -------------------------------------------------
 */

ST_ErrorCode_t stvout_HalSetOutputsConfiguration(
        STDENC_Handle_t             Handle,
        U8                          Version,
        STVOUT_DeviceType_t         Type,
        STVOUT_DAC_t                Dac,
        VOUT_OutputsConfiguration_t OutputsConfiguration)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 Value;

    if ( (Error = OKHandle( Handle)) == ST_NO_ERROR)
    {
        STDENC_RegAccessLock( Handle);
        switch ( Version)
        {
            case 3 :   /* typically STV0119 case */
                Error = STDENC_ReadReg8( Handle, DENC_CFG5, &Value);
                Error = ERROR(Error);
                if (Error == ST_NO_ERROR)
                {
                    Value &= DENC_CFG5_MASK_CONF;
                    switch ( OutputsConfiguration)
                    {
                        case VOUT_OUTPUTS_RGB :
                            Value |= DENC_CFG5_RGB;
                            break;
                        case VOUT_OUTPUTS_YUV :
                            break;
                        case VOUT_OUTPUTS_YC :
                        case VOUT_OUTPUTS_CVBS :
                            Value |= DENC_CFG5_NYC;
                            break;
                        case VOUT_OUTPUTS_HD_RGB:
                        case VOUT_OUTPUTS_HD_YCBCR:
                            /* not used */
                            break;
                        default:
                            Error = ST_ERROR_BAD_PARAMETER;
                            break;
                    }
                }
                if (Error == ST_NO_ERROR)
                {
                    Error = STDENC_WriteReg8( Handle, DENC_CFG5, Value);
                    Error = ERROR(Error);
                }
                break;
            case 5 :   /* nothing to do, hardware configuration is fixed*/
                break;
            case 6 :   /* no break */
            case 7 :   /* no break */
            case 8 :   /* no break */
            case 9 :   /* no break */
            case 10 :   /* no break */
            case 11 :
                Error = STDENC_ReadReg8( Handle, DENC_CFG8, &Value);
                Error = ERROR(Error);
                if (Error == ST_NO_ERROR)
                {
                    Value &= DENC_CFG8_MASK_CONF;
                    switch (Type)
                    {
                        case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
                              switch ( OutputsConfiguration)
                              {
                                  case VOUT_OUTPUTS_RGB :
                                       Value |= DENC_CFG8_CONF_OUT_2;
                                       break;
                                  case VOUT_OUTPUTS_YUV :
                                       Value |= DENC_CFG8_CONF_OUT_1;
                                       break;
                                  case VOUT_OUTPUTS_YC :
                                       if (Dac==(STVOUT_DAC_4|STVOUT_DAC_5))
                                       {
                                             Value |= DENC_CFG8_CONF_OUT_0;
                                       }
                                       else /* dac 1 and 2 were selected*/
                                       {
                                             Value |= DENC_CFG8_CONF_OUT_2;
                                       }
                                      break;
                                 case VOUT_OUTPUTS_CVBS :
                                     if(Dac==STVOUT_DAC_6)
                                     {
                                            Value |= DENC_CFG8_CONF_OUT_0;
                                     }
                                     else /*dac 3 was selected*/
                                     {
                                            Value |= DENC_CFG8_CONF_OUT_2;
                                     }
                                     break;
                               case VOUT_OUTPUTS_HD_RGB:
                               case VOUT_OUTPUTS_HD_YCBCR:
                              /* not used */
                                     break;
                               default:
                                  Error = ST_ERROR_BAD_PARAMETER;
                                    break;
                              }
                            break;
                        default :
                              switch ( OutputsConfiguration)
                              {
                                  case VOUT_OUTPUTS_RGB :
                                       Value |= DENC_CFG8_CONF_OUT_2;
                                       break;
                                  case VOUT_OUTPUTS_YUV :
                                       Value |= DENC_CFG8_CONF_OUT_1;
                                       break;
                                  case VOUT_OUTPUTS_YC :
                                  case VOUT_OUTPUTS_CVBS :
                                     if ((Type == STVOUT_DEVICE_TYPE_7015) || (Type == STVOUT_DEVICE_TYPE_7020))
                                     {
                                            Value |= DENC_CFG8_CONF_OUT_0;
                                     }
                                     else /* STVOUT_DEVICE_TYPE_DENC, GX1... */
                                     {
                                            Value |= DENC_CFG8_CONF_OUT_2;
                                     }
                                     break;
                               case VOUT_OUTPUTS_HD_RGB:
                               case VOUT_OUTPUTS_HD_YCBCR:
                              /* not used */
                                     break;
                               default:
                                  Error = ST_ERROR_BAD_PARAMETER;
                                    break;
                              }
                             break;
                    }
                }
                if (Error == ST_NO_ERROR)
                {
                    Error = STDENC_WriteReg8( Handle, DENC_CFG8, Value);
                    Error = ERROR(Error);
                }
                break;
            case 12 : /* no break */
            case 13 :
                break;
            default:
                break;
        }
        STDENC_RegAccessUnlock( Handle);
    }
    return (ERROR(Error));
}

/**********************************************************7*********************
Name        : stvout_HalSetOption
Description :
Parameters  : Device, Option Parameters (pointers)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t stvout_HalSetOption( stvout_Device_t * Device_p, const STVOUT_OptionParams_t * OptionParams_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stvout_HalCheckOptionIsSupported(Device_p, OptionParams_p);
    if ( Error == ST_NO_ERROR)
    {
        switch (OptionParams_p->Option)
        {
            case STVOUT_OPTION_GENERAL_AUX_NOT_MAIN:
                Device_p->AuxNotMain = OptionParams_p->Enable;
                break;
            case STVOUT_OPTION_NOTCH_FILTER_ON_LUMA:
                Error = stvout_HalDencWrite8(Device_p, DENC_CFG12, (U8)~DENC_CFG12_MAIN_ENNOTCH,
                                            OptionParams_p->Enable ? DENC_CFG12_MAIN_ENNOTCH : 0);
                break;
            case STVOUT_OPTION_RGB_SATURATION:
                Error = stvout_HalDencWrite8(Device_p, DENC_CFG10, (U8)~DENC_CFG10_RGB_SAT_EN,
                                            OptionParams_p->Enable ? DENC_CFG10_RGB_SAT_EN : 0);
                break;
            case STVOUT_OPTION_IF_DELAY:
                Error = stvout_HalDencWrite8(Device_p, DENC_CFG11, (U8)~DENC_CFG11_MAIN_IF_DEL,
                                            OptionParams_p->Enable ? 0 : DENC_CFG11_MAIN_IF_DEL);
                break;
            default :
                Error = ST_ERROR_BAD_PARAMETER;
        }
    }
    return(Error);
} /* stvout_HalSetOption */

/*******************************************************************************
Name        : stvout_HalGetOption
Description : Gets the parameters of the chosen option
Parameters  : Device, Option Parameters (pointers)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t stvout_HalGetOption( stvout_Device_t * Device_p, STVOUT_OptionParams_t * const OptionParams_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 RegValue=0;

    Error = stvout_HalCheckOptionIsSupported(Device_p, OptionParams_p);
    if ( Error == ST_NO_ERROR)
    {
        switch (OptionParams_p->Option)
        {
            case STVOUT_OPTION_GENERAL_AUX_NOT_MAIN:
                OptionParams_p->Enable = Device_p->AuxNotMain;
                break;
            case STVOUT_OPTION_NOTCH_FILTER_ON_LUMA:
                Error = stvout_HalDencRead8(Device_p, DENC_CFG12, &RegValue );
                OptionParams_p->Enable = (RegValue & DENC_CFG12_MAIN_ENNOTCH) == DENC_CFG12_MAIN_ENNOTCH ;
                break;
            case STVOUT_OPTION_RGB_SATURATION:
                Error = stvout_HalDencRead8(Device_p, DENC_CFG10, &RegValue );
                OptionParams_p->Enable = (RegValue & DENC_CFG10_RGB_SAT_EN) == DENC_CFG10_RGB_SAT_EN ;
                break;
            case STVOUT_OPTION_IF_DELAY:
                Error = stvout_HalDencRead8(Device_p, DENC_CFG11, &RegValue );
                OptionParams_p->Enable = ((RegValue & DENC_CFG11_MAIN_IF_DEL) == 0) ;
                break;
            default :
                Error = ST_ERROR_BAD_PARAMETER;
        }
    }
    return(Error);
} /* stvout_HalSetOption */


/*******************************************************************************
Name        : stvout_HalCheckOptionIsSupported
Description : Checks if the chosen option is supported by the hardware
Parameters  : Device, Option Parameters (pointers)
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t stvout_HalCheckOptionIsSupported(stvout_Device_t * Device_p, const STVOUT_OptionParams_t * const OptionParams_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (  (Device_p->DencVersion < 12 )
       || ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629) && (OptionParams_p->Option == STVOUT_OPTION_GENERAL_AUX_NOT_MAIN)))
    {
        Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return(Error);
} /* stvout_HalCheckOptionIsSupported */

/*******************************************************************************
Name        : stvout_HalDencWrite8
Description : Writes an 8bits value to a denc register, allows an exclusive access
Parameters  : Device, Denc register, 8bits mask, 8bits value
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t stvout_HalDencWrite8(stvout_Device_t * Device_p, U32 Reg, U8 RegMask, U8 Value)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error;

    DencHnd = Device_p->DencHandle;
    Error = OKHandle( DencHnd);
    if (  Error == ST_NO_ERROR)
    {
        STDENC_RegAccessLock( DencHnd);
        Error = STDENC_MaskReg8( DencHnd, Reg, RegMask, Value);
        STDENC_RegAccessUnlock( DencHnd);
        Error = ERROR(Error);
    }
    return(Error);
} /* stvout_HalDencWrite8 */

/*******************************************************************************
Name        : stvout_HalDencRead8
Description : Reads an 8bits value to a denc register, allows an exclusive access
Parameters  : Device, Denc register, 8bits mask, 8bits value
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t stvout_HalDencRead8(stvout_Device_t * Device_p, U32 Reg, U8* Value_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error;

    DencHnd = Device_p->DencHandle;
    Error = OKHandle( DencHnd);
    if (  Error == ST_NO_ERROR)
    {
        STDENC_RegAccessLock( DencHnd);
        Error = STDENC_ReadReg8( DencHnd, Reg, Value_p);
        STDENC_RegAccessUnlock( DencHnd);
        Error = ERROR(Error);
    }
    return(Error);
} /* stvout_HalDencRead8 */

/*
------------------------------------------------------------------------------
Disable/Enable the output signals by writing useful Denc register
parameter Number : 1-6 -> Dac 1-6
------------------------------------------------------------------------------
*/

ST_ErrorCode_t stvout_HalDisableDac( stvout_Device_t *Device_p, U8 Dac)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error1 = ST_NO_ERROR;
    U32 Value, Address;

    if ( (Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629) && (Dac > 3 ) )
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    if ( (Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525) && (Dac > 4 ) )
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    if (Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020)
    {
        Address = (U32)Device_p->DeviceBaseAddress_p + (U32)Device_p->BaseAddress_p;
        Value = stvout_rd32( Address + DSPCFG_DAC);
        Value &= (U32)~DSPCFG_DAC_ENABLE_SD;     /* 7020 only ... */
        stvout_wr32( Address + DSPCFG_DAC, Value);
    }
    else if (Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)
    {
        Address = (U32)Device_p->DeviceBaseAddress_p + (U32)Device_p->BaseAddress_p;
        Value = stvout_rd32( Address + VOS_MISC_VDAC0_CTL);
        Value |= VOS_MISC_VDAC0_CTL_HIGH_IMPEDANCE;     /* 5528 and 5100 ... */
        stvout_wr32( Address + VOS_MISC_VDAC0_CTL, Value);

        Value = stvout_rd32( Address + VOS_MISC_VDAC1_CTL);
        Value |= VOS_MISC_VDAC1_CTL_HIGH_IMPEDANCE;     /* 5528 and 5100 ... */
        stvout_wr32( Address + VOS_MISC_VDAC1_CTL, Value);
    }
    else if (Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629)
    {
        Address = (U32)Device_p->DeviceBaseAddress_p + (U32)Device_p->BaseAddress_p;
        Value = stvout_rd8( Address + VDAC_CFG_ADR);
        Value |= VOS_MISC_VDAC_CTL_HIGH_IMPEDANCE;     /* FOR 4629*/
        stvout_wr8( Address + VDAC_CFG_ADR, Value);
    }
    else if (Device_p->DeviceType == STVOUT_DEVICE_TYPE_7710)
    {
            Address = (U32)Device_p->DeviceBaseAddress_p + (U32)Device_p->BaseAddress_p;
            Value = stvout_rd32( Address + DSPCFG_DAC);
            Value &= (U32)~DSPCFG_DAC_ENABLE_SD;
            stvout_wr32( Address + DSPCFG_DAC, Value);
    }
    DencHnd = Device_p->DencHandle;

    if ( (Error1 = OKHandle( DencHnd)) == ST_NO_ERROR)
    {
        if ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525) && (Dac == 4))
        {
            STDENC_RegAccessLock( DencHnd);
            Error1 = STDENC_OrReg8( DencHnd, DENC_CFG5, (BLKDAC >> (Dac+2)));
            STDENC_RegAccessUnlock( DencHnd);
        }
        else
        {
            STDENC_RegAccessLock( DencHnd);
            Error1 = STDENC_OrReg8( DencHnd, DENC_CFG5, (BLKDAC >> Dac));
            STDENC_RegAccessUnlock( DencHnd);
        }

    }
    return (ERROR(Error1));
}

ST_ErrorCode_t stvout_HalEnableDac( stvout_Unit_t *Unit_p, U8 Dac)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 Value, Address;

    if ( (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629) && (Dac > 3 ) )
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    if ( (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525) && (Dac > 4 ) )
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020)
    {
        Address = (U32)Unit_p->Device_p->DeviceBaseAddress_p + (U32)Unit_p->Device_p->BaseAddress_p;
        Value = stvout_rd32( Address + DSPCFG_DAC);
        Value |= DSPCFG_DAC_ENABLE_SD;      /* 7020 only ... */
        stvout_wr32( Address + DSPCFG_DAC, Value);
    }
    else if (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)
    {
        Address = (U32)Unit_p->Device_p->DeviceBaseAddress_p + (U32)Unit_p->Device_p->BaseAddress_p;
        Value = stvout_rd32( Address + VOS_MISC_VDAC0_CTL);
        Value &= (U32)~VOS_MISC_VDAC0_CTL_HIGH_IMPEDANCE;     /* 5528 and 5100 ... */
        stvout_wr32( Address + VOS_MISC_VDAC0_CTL, Value);

        Value = stvout_rd32( Address + VOS_MISC_VDAC1_CTL);
        Value &= (U32)~VOS_MISC_VDAC1_CTL_HIGH_IMPEDANCE;     /* 5528 and 5100 ... */
        stvout_wr32( Address + VOS_MISC_VDAC1_CTL, Value);
    }
    else if (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629)
    {
        Address = (U32)Unit_p->Device_p->DeviceBaseAddress_p + (U32)Unit_p->Device_p->BaseAddress_p;
        Value = stvout_rd8( Address + VDAC_CFG_ADR);
        Value &= (U8)~VOS_MISC_VDAC_CTL_HIGH_IMPEDANCE;     /* FOR 4629*/
        stvout_wr8( Address + VDAC_CFG_ADR, Value);
    }
    else if (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7710)
    {
            Address = (U32)Unit_p->Device_p->DeviceBaseAddress_p + (U32)Unit_p->Device_p->BaseAddress_p;
            Value = stvout_rd32( Address + DSPCFG_DAC);
            Value |= DSPCFG_DAC_ENABLE_SD;
            stvout_wr32( Address + DSPCFG_DAC, Value);
    }

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
    {
        if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525) && (Dac == 4))
        {
            STDENC_RegAccessLock( DencHnd);
            Error = STDENC_AndReg8( DencHnd, DENC_CFG5, (~(BLKDAC >> (Dac+2))));
            STDENC_RegAccessUnlock( DencHnd);
        }
        else
        {
            STDENC_RegAccessLock( DencHnd);
            Error = STDENC_AndReg8( DencHnd, DENC_CFG5, (~(BLKDAC >> Dac)));
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}

/*
------------------------------------------------------------------------------
programs inverted/non inverted on DAC
------------------------------------------------------------------------------
*/

ST_ErrorCode_t stvout_HalSetInvertedOutput( stvout_Unit_t *Unit_p, BOOL Inverted)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
    {
        STDENC_RegAccessLock( DencHnd);
        if ( Inverted )
        {
            Error = STDENC_OrReg8( DencHnd, DENC_CFG5, DENC_CFG5_DAC_INV);
        }
        else
        {
            Error = STDENC_AndReg8( DencHnd, DENC_CFG5, (U8)(~DENC_CFG5_DAC_INV));
        }
        STDENC_RegAccessUnlock( DencHnd);
    }
    return (ERROR(Error));
}

/*
 * ----------------------------------------------------
 * adjust level on the DACs Output
 * DAC 1 3 4 5 6 and C for DENC macrocell V6
 * DAC 1 2 3 4 5 6 and C for DENC macrocell V7 and later
 * --------------------------------------------------
 */

ST_ErrorCode_t stvout_HalWriteDAC6( stvout_Unit_t *Unit_p, U8 DACValue)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

#if defined (ST_5188)|| defined (ST_5525)|| defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
    UNUSED_PARAMETER(Unit_p);
    UNUSED_PARAMETER(DACValue);
    Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STDENC_Handle_t DencHnd;

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( Unit_p->Device_p->DencVersion > 5)
    {
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
           STDENC_RegAccessLock( DencHnd);
           #if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)\
            || defined(ST_5107)|| defined(ST_5162)
               Error = STDENC_MaskReg8( DencHnd, DENC_DAC6, DENC_DAC6_MASK_DAC6, (DACValue & ~DENC_DAC6_MASK_DAC6));
           #else
               Error = STDENC_MaskReg8( DencHnd, DENC_DAC6C, DENC_DAC6C_MASK_DAC6, ((DACValue << 4) & ~DENC_DAC6C_MASK_DAC6));
           #endif
           STDENC_RegAccessUnlock( DencHnd);
        }
    }
#endif /*5188-5525*/
    return (ERROR(Error));
}

ST_ErrorCode_t stvout_HalWriteDAC5( stvout_Unit_t *Unit_p, U8 DACValue)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
#if defined (ST_5188)|| defined (ST_5525)|| defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
    UNUSED_PARAMETER(Unit_p);
    UNUSED_PARAMETER(DACValue);
    Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STDENC_Handle_t DencHnd;

    DencHnd = Unit_p->Device_p->DencHandle;
    if ( Unit_p->Device_p->DencVersion > 5)
    {
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
            STDENC_RegAccessLock( DencHnd);
            Error = STDENC_MaskReg8( DencHnd, DENC_DAC45, DENC_DAC45_MASK_DAC5, (DACValue & ~DENC_DAC45_MASK_DAC5));
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
#endif
    return (ERROR(Error));
}

ST_ErrorCode_t stvout_HalWriteDAC4( stvout_Unit_t *Unit_p, U8 DACValue)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #if defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
        UNUSED_PARAMETER(Unit_p);
        UNUSED_PARAMETER(DACValue);
        Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    #else
        STDENC_Handle_t DencHnd;
        DencHnd = Unit_p->Device_p->DencHandle;

        if ( Unit_p->Device_p->DencVersion > 5)
        {
            if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
            {
                STDENC_RegAccessLock( DencHnd);
                #if defined(ST_5100)|| defined(ST_7710)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)
                    Error = STDENC_MaskReg8( DencHnd, DENC_DAC34, DENC_DAC34_MASK_DAC4, ((DACValue >> 2) & ~DENC_DAC34_MASK_DAC4));
                    Error = STDENC_MaskReg8( DencHnd, DENC_DAC45, DENC_DAC45_MASK_DAC4, ((DACValue << 6) & ~DENC_DAC45_MASK_DAC4));
                #elif defined(ST_5188)|| defined(ST_5525)
                    Error = STDENC_MaskReg8( DencHnd, DENC_DAC4, DENC_DAC4_MASK_DAC4, (DACValue & ~DENC_DAC4_MASK_DAC4));
                #elif !(defined(ST_5105)|| defined(ST_5107)||defined(ST_5162))
                    Error = STDENC_MaskReg8( DencHnd, DENC_DAC45, DENC_DAC45_MASK_DAC4, ((DACValue << 4) & ~DENC_DAC45_MASK_DAC4));
                #else /*(defined(ST_5105)|| defined(ST_5107))*/
                   UNUSED_PARAMETER(DACValue);
                #endif
                STDENC_RegAccessUnlock( DencHnd);
            }
        }
    #endif
    return (ERROR(Error));
}

ST_ErrorCode_t stvout_HalWriteDAC3( stvout_Unit_t *Unit_p, U8 DACValue)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( Unit_p->Device_p->DencVersion > 5)
    {
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
           STDENC_RegAccessLock( DencHnd);
           #if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)\
            || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)|| defined(ST_7111)|| defined (ST_7105)
               Error = STDENC_MaskReg8( DencHnd, DENC_DAC13, DENC_DAC13_MASK_DAC3, ((DACValue >> 4) & ~DENC_DAC13_MASK_DAC3));
               #if defined(ST_5188) || defined(ST_5525)
                  Error = STDENC_MaskReg8( DencHnd, DENC_DAC3, DENC_DAC3_MASK_DAC3, ((DACValue << 4) & ~DENC_DAC3_MASK_DAC3));
               #else
                  Error = STDENC_MaskReg8( DencHnd, DENC_DAC34, DENC_DAC34_MASK_DAC3, ((DACValue << 4) & ~DENC_DAC34_MASK_DAC3));
               #endif
           #else
               Error = STDENC_MaskReg8( DencHnd, DENC_DAC13, DENC_DAC13_MASK_DAC3, (DACValue & ~DENC_DAC13_MASK_DAC3));
           #endif
           STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}

ST_ErrorCode_t stvout_HalWriteDAC2( stvout_Unit_t *Unit_p, U8 DACValue)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
    {
        if ( Unit_p->Device_p->DencVersion > 5)
        {
            STDENC_RegAccessLock( DencHnd);

            #if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)\
             || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)|| defined(ST_7111)|| defined (ST_7105)
                Error = STDENC_MaskReg8( DencHnd, DENC_DAC2, DENC_DAC2_MASK_DAC2, (DACValue & ~DENC_DAC2_MASK_DAC2));
            #else
                Error = STDENC_MaskReg8( DencHnd, DENC_DAC2, DENC_DAC2_MASK_DAC2, ((DACValue << 4) & ~DENC_DAC2_MASK_DAC2));
            #endif

            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}

ST_ErrorCode_t stvout_HalWriteDAC1( stvout_Unit_t *Unit_p, U8 DACValue)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( Unit_p->Device_p->DencVersion > 5)
    {
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
            STDENC_RegAccessLock( DencHnd);
            #if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)\
             || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)|| defined(ST_7111)|| defined (ST_7105)
                Error = STDENC_MaskReg8( DencHnd, DENC_DAC13, DENC_DAC13_MASK_DAC1, ((DACValue << 2) & ~DENC_DAC13_MASK_DAC1));
            #else
                Error = STDENC_MaskReg8( DencHnd, DENC_DAC13, DENC_DAC13_MASK_DAC1, ((DACValue << 4) & ~DENC_DAC13_MASK_DAC1));
            #endif
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}

ST_ErrorCode_t stvout_HalWriteDACC( stvout_Unit_t *Unit_p, U8 DACValue)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( Unit_p->Device_p->DencVersion > 5)
    {
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
            STDENC_RegAccessLock( DencHnd);

            #if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)||defined(ST_7109)\
             || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)||defined(ST_7111)|| defined (ST_7105)
                Error = STDENC_MaskReg8( DencHnd, DENC_DACC, DENC_DACC_MASK_DACC, ((DACValue << 4) & ~DENC_DACC_MASK_DACC));
            #else
                Error = STDENC_MaskReg8( DencHnd, DENC_DAC6C, DENC_DAC6C_MASK_DACC, (DACValue & ~DENC_DAC6C_MASK_DACC));
            #endif
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}



/*----------------------------------------------------
 * Enable/Disable the Brightness/Contrast/Saturation
 * --------------------------------------------------*/
ST_ErrorCode_t stvout_HalEnableBCS( stvout_Unit_t *Unit_p, BOOL Enable)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 RegBit1=0, RegBit2=0, Value1=0, Value2=0;
    BOOL BCS_Enable422;

    if ( Unit_p->Device_p->DencVersion > 6)
    {
        if (Unit_p->Device_p->DencVersion < 12)
        {
            RegBit1 = DENC_DAC2_BCS_EN_444;
            RegBit2 = DENC_DAC2_BCS_EN_422;
            BCS_Enable422 = Enable;
            Value2 = (BCS_Enable422) ? RegBit2 : 0;
        }
        else
        {
            #if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)||defined(ST_7109)\
             || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)||defined(ST_5162)||defined(ST_7111)|| defined (ST_7105)
                RegBit1 = (Unit_p->Device_p->AuxNotMain) ? DENC_DACC_BCS_EN_AUX : DENC_DACC_BCS_EN_MAIN;
            #else
                RegBit1 = (Unit_p->Device_p->AuxNotMain) ? DENC_DAC2_BCS_EN_AUX : DENC_DAC2_BCS_EN_MAIN;
            #endif
            BCS_Enable422 = FALSE;
        }
        DencHnd = Unit_p->Device_p->DencHandle;

        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
            Value1 = (Enable) ? RegBit1 : 0;

            STDENC_RegAccessLock( DencHnd);
            #if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)\
             || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)||defined(ST_7111)|| defined (ST_7105)
            	Error = STDENC_MaskReg8( DencHnd, DENC_DACC, (U8)(~RegBit1), Value1);
            #else
               Error = STDENC_MaskReg8( DencHnd, DENC_DAC2, (U8)(~RegBit1), Value1);
            #endif
            if ((BCS_Enable422) && (OK(Error)))
            {
                Error = STDENC_MaskReg8( DencHnd, DENC_DAC2, (U8)(~RegBit2), Value2);
            }
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}


ST_ErrorCode_t stvout_HalMaxDynamic( stvout_Unit_t *Unit_p, BOOL MaxDyn)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 Reg1, RegBit1, Value1;
    U8 RegBit2=0, Value2=0;

    if ( Unit_p->Device_p->DencVersion > 6)
    {
        DencHnd = Unit_p->Device_p->DencHandle;
        if (Unit_p->Device_p->AuxNotMain)
        {
            Reg1    = DENC_CFG12;
            RegBit1 = DENC_CFG12_AUX_MAX_DYN;
        }
        else
        {
            Reg1    = DENC_CFG6;
            RegBit1 = DENC_CFG6_MAX_DYN;
        }
        if ( Unit_p->Device_p->DencVersion >= 12)
        {
            RegBit2 = DENC_CFG13_RGB_MAX_DYN;
            Value2 = (MaxDyn) ? RegBit2 : 0;
        }
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
            Value1 = (MaxDyn) ? RegBit1 : 0;
            STDENC_RegAccessLock( DencHnd);
            Error = STDENC_MaskReg8( DencHnd, Reg1, (U8)(~RegBit1), Value1);
            if (( Unit_p->Device_p->DencVersion >= 12)&& (OK(Error)))
            {
                Error = STDENC_MaskReg8( DencHnd, DENC_CFG13, (U8)(~RegBit2), Value2);
            }
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}

/*----------------------------------------------------
 * select and adjust Brightness, Contrast, Saturation
 * --------------------------------------------------*/
ST_ErrorCode_t stvout_HalSetBrightness( stvout_Unit_t *Unit_p, U8 Brightness)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( Unit_p->Device_p->DencVersion > 6)
    {
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
            STDENC_RegAccessLock( DencHnd);
            Error = STDENC_WriteReg8( DencHnd, DENC_BRIGHT, Brightness);
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}

ST_ErrorCode_t stvout_HalSetContrast( stvout_Unit_t *Unit_p, S8 Contrast)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( Unit_p->Device_p->DencVersion > 6)
    {
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
            STDENC_RegAccessLock( DencHnd);
            Error = STDENC_WriteReg8( DencHnd, DENC_CONTRA, Contrast);
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}

ST_ErrorCode_t stvout_HalSetSaturation( stvout_Unit_t *Unit_p, U8 Saturation)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( Unit_p->Device_p->DencVersion > 6)
    {
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
            STDENC_RegAccessLock( DencHnd);
            Error = STDENC_WriteReg8( DencHnd, DENC_SATURA, Saturation);
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}

/*------------------------------------------------------------------------------
 Loads DENC Chroma Filter Coefficients
 Param:     Device  DENC device to access
            Coeffs_p  array of 9 signed 16 bits coefficients
                    or NULL to use default filter coeffs
------------------------------------------------------------------------------*/
ST_ErrorCode_t stvout_HalSetChromaFilter( stvout_Unit_t *Unit_p, S16 *Coeffs_p)
{
    int i, sum = 0;
    U32 div;
    U8 Value;
    const U8 *Reg_p;
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (Unit_p->Device_p->AuxNotMain)
    {
        Reg_p = &AuxChromaFilterReg[0];
    }
    else
    {
        Reg_p = &MainChromaFilterReg[0];
    }
    DencHnd = Unit_p->Device_p->DencHandle;

    if ( Unit_p->Device_p->DencVersion > 9)
    {
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
            STDENC_RegAccessLock( DencHnd);
            if (Coeffs_p == NULL)     /* use default Chroma Coeffs */
            {
                Error = STDENC_AndReg8( DencHnd, Reg_p[0], (U8)(~DENC_CHRM0_FLT_S));
            }
            else
            {
                /* sum = 2*Coef[0] + 2*Coef[1] ... + 2*Coef[7] + 1*Coef[8] */
                for (i=0; i<8; i++)
                    sum += 2 * Coeffs_p[i];
                sum += Coeffs_p[8];

                switch (sum)
                {
                    case 4096:
                        div = 3;
                        break;
                    case 2048:
                        div = 2;
                        break;
                    case 1024:
                        div = 1;
                        break;
                    case 512:
                        div = 0;
                        break;
                    default:
                        div = 1;
    /*                  STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
                "Sum of chroma filter coeffs should be 4096, 2048, 1024 or 512 !"));*/
                        break;
                }
                Value  = ((Coeffs_p[0] + 16) & DENC_CHRM0_MASK) | DENC_CHRM0_FLT_S | (div << 5);
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, Reg_p[0], Value);
                Value  = ((Coeffs_p[8] >> 2) & 0x40) | ((Coeffs_p[1] + 32) & DENC_CHRM1_MASK);
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, Reg_p[1], Value);
                Value  = (Coeffs_p[2] + 64) & DENC_CHRM2_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, Reg_p[2], Value);
                Value  = (Coeffs_p[3] + 32) & DENC_CHRM3_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, Reg_p[3], Value);
                Value  = (Coeffs_p[4] + 32) & DENC_CHRM4_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, Reg_p[4], Value);
                Value  = (Coeffs_p[5] + 32) & DENC_CHRM5_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, Reg_p[5], Value);
                Value  = Coeffs_p[6] & DENC_CHRM6_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, Reg_p[6], Value);
                Value  = Coeffs_p[7] & DENC_CHRM7_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, Reg_p[7], Value);
                Value  = Coeffs_p[8] & DENC_CHRM8_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, Reg_p[8], Value);
            }
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}

/*------------------------------------------------------------------------------
 Loads DENC Luma Filter Coefficients
 Param:     Device  DENC device to access
            Coeffs_p  array of 10 signed 16 bits coefficients (Coeffs[8..9] must be positive values)
                    or NULL to use default filter coeffs
------------------------------------------------------------------------------*/
ST_ErrorCode_t stvout_HalSetLumaFilter( stvout_Unit_t *Unit_p, S16 *Coeffs_p)
{
    int i, sum = 0;
    U32 div;
    U8 Value;
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DencHnd = Unit_p->Device_p->DencHandle;

    if ( Unit_p->Device_p->DencVersion > 9)
    {
        if ( (Error = OKHandle( DencHnd)) == ST_NO_ERROR)
        {
            STDENC_RegAccessLock( DencHnd);
            if (Coeffs_p == NULL)     /* use default Chroma Coeffs */
            {
                Error = STDENC_AndReg8( DencHnd, DENC_CFG9, (U8)(~DENC_CFG9_FLT_YS));
            }
            else
            {
                /* sum = 2*Coef[0] + 2*Coef[1] ... + 2*Coef[7] + 1*Coef[8] */
                for (i=0; i<9; i++)
                    sum += 2 * Coeffs_p[i];
                sum += Coeffs_p[9];

                switch (sum)
                {
                    case 4096:
                        div = 3;
                        break;
                    case 2048:
                        div = 2;
                        break;
                    case 1024:
                        div = 1;
                        break;
                    case 512:
                        div = 0;
                        break;
                    default:
                        div = 1;
    /*                      STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "Sum of luma filter coeffs should be 4096, 2048, 1024 or 512 !"));*/
                        break;
                }

                /* set FLT_YS and DIV value */
                if (OK(Error)) Error = STDENC_MaskReg8( DencHnd, DENC_CFG9, DENC_CFG9_MASK_PLG_DIV, (DENC_CFG9_FLT_YS | (div << 1)));

                /* set filter coefficients */
                Value = ((Coeffs_p[8] >> 3) & 0x20) | (Coeffs_p[0] & DENC_LUMA0_MASK);
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, DENC_LUMA0, Value);
                Value = ((Coeffs_p[9] >> 2) & 0xC0) | (Coeffs_p[1] & DENC_LUMA1_MASK);
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, DENC_LUMA1, Value);
                Value = ((Coeffs_p[6] >> 1) & 0x80) | (Coeffs_p[2] & DENC_LUMA2_MASK);
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, DENC_LUMA2, Value);
                Value = ((Coeffs_p[7] >> 1) & 0x80) | (Coeffs_p[3] & DENC_LUMA3_MASK);
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, DENC_LUMA3, Value);
                Value = Coeffs_p[4] & DENC_LUMA4_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, DENC_LUMA4, Value);
                Value = Coeffs_p[5] & DENC_LUMA4_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, DENC_LUMA5, Value);
                Value = Coeffs_p[6] & DENC_LUMA4_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, DENC_LUMA6, Value);
                Value = Coeffs_p[7] & DENC_LUMA4_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, DENC_LUMA7, Value);
                Value = Coeffs_p[8] & DENC_LUMA4_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, DENC_LUMA8, Value);
                Value = Coeffs_p[9] & DENC_LUMA4_MASK;
                if (OK(Error)) Error = STDENC_WriteReg8( DencHnd, DENC_LUMA9, Value);
            }
            STDENC_RegAccessUnlock( DencHnd);
        }
    }
    return (ERROR(Error));
}

/* ************************************************************************************
 * Function stvout_FindCurrentConfigurationFromReg
 * Description : Finds the current configuration of a tridac from the REG_CFG13 value
 * ************************************************************************************/

 void stvout_FindCurrentConfigurationFromReg(U8 MaskedRegValue,
                                             STVOUT_DACOutput_t TriDACConfig[3],
                                             const STVOUT_DAC_CONF_t AuthorizedDacsConfiguration[],
                                             int AuthorizedDacsConfigurationSize)
{
    int i;

    /* Find the configuration of DACs */
    for(i=0;i<AuthorizedDacsConfigurationSize;i++)
    {
        if (AuthorizedDacsConfiguration[i].Configuration == MaskedRegValue)
        {
            memcpy(TriDACConfig, AuthorizedDacsConfiguration[i].AuthorizedConfiguration ,sizeof(STVOUT_DACOutput_t)*3);
            break;
        }
    }
}

 /*
--------------------------------------------------------------------------------
Update dacs configuration and Set Main or Aux input for STi5528
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t stvout_HalSetInputSource   ( stvout_Device_t *              Device_p,
                                            STVOUT_Source_t                Source
                                           )

 {
      ST_ErrorCode_t Error = ST_NO_ERROR;
      U8 Value =0;
      STVOUT_DACOutput_t TriDACConfig[3];

      if ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)||(Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13))
      {
            Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
            switch (Source)
            {
                case STVOUT_SOURCE_MAIN :

                    switch (Device_p->OutputType)
                        {
                            case STVOUT_OUTPUT_ANALOG_RGB :
                                if ((Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3)) == (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                                {
                                    Value &=DENC_CFG13_DAC123_MASK;
                                    Value|=AuthorizedDacs123Configuration[2].Configuration;
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                                }
                                #if !(defined(ST_5105)|| defined(ST_5107)||defined(ST_5162)) /* No RGB output on DACS 4,5 and 6 for STi5105/07 */
                                else if ((Device_p->DacSelect &(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6)) == (STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))
                                {
                                    Value &=DENC_CFG13_DAC456_MASK;
                                    Value|=AuthorizedDacs456Configuration[2].Configuration;
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                                }
                                #endif
                                else
                                {
                                    Error=ST_ERROR_BAD_PARAMETER;
                                }
                                break;

                            case STVOUT_OUTPUT_ANALOG_YUV :
                                if ((Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3)) == (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                                {
                                    Value &=DENC_CFG13_DAC123_MASK;
                                    Value|=AuthorizedDacs123Configuration[3].Configuration;
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                                }
                                else
                                {
                                    Error=ST_ERROR_BAD_PARAMETER;
                                }
                                break;
                            case STVOUT_OUTPUT_ANALOG_YC :
                                /* YC_MAIN on DACs 1 and 2 */
                                if ((Device_p->DacSelect & (STVOUT_DAC_1|STVOUT_DAC_2)) == (STVOUT_DAC_1|STVOUT_DAC_2))
                                {
                                    /* First of all see if CVBS was selected on DAC3*/
                                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                                                       TriDACConfig,
                                                                       AuthorizedDacs123Configuration,
                                                                       sizeof(AuthorizedDacs123Configuration)/sizeof(STVOUT_DAC_CONF_t));
                                    Value &=DENC_CFG13_DAC123_MASK;
                                    if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                                    {
                                        Value|=AuthorizedDacs123Configuration[0].Configuration;
                                    }
                                    else
                                    {
                                       Value|=AuthorizedDacs123Configuration[1].Configuration;
                                    }
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);

                                }
                                /* YC_MAIN on DACs 4&5 */
                                #if !(defined(ST_5105)|| defined(ST_5107)||defined(ST_5162)) /* No YC output on DACS 4, 5 for STi5105/07... */
                                else if ((Device_p->DacSelect & (STVOUT_DAC_4|STVOUT_DAC_5)) == (STVOUT_DAC_4|STVOUT_DAC_5))
                                {
                                    /* First of all see if CVBS was selected on DAC6*/
                                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC456_MASK))),
                                                                           TriDACConfig,
                                                                           AuthorizedDacs456Configuration,
                                                                           sizeof(AuthorizedDacs456Configuration)/sizeof(STVOUT_DAC_CONF_t));
                                    Value &=DENC_CFG13_DAC456_MASK;
                                    if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                                    {
                                        Value|=AuthorizedDacs456Configuration[7].Configuration;
                                    }
                                    else
                                    {
                                        Value|=AuthorizedDacs456Configuration[6].Configuration;
                                    }
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle, DENC_CFG13, Value);
                                }
                                #endif
                                else
                                {
                                    Error=ST_ERROR_BAD_PARAMETER;
                                }
                                break;
                            case STVOUT_OUTPUT_ANALOG_CVBS :
                                /* CVBS_MAIN on DAC1 */
                                if ((Device_p->DacSelect & (STVOUT_DAC_1)) == (STVOUT_DAC_1))
                                {
                                    /* First of all see which CVBS(AUX/MAIN) was selected on DAC3*/
                                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                                                           TriDACConfig,
                                                                           AuthorizedDacs123Configuration,
                                                                           sizeof(AuthorizedDacs123Configuration)/sizeof(STVOUT_DAC_CONF_t));
                                    Value &=DENC_CFG13_DAC123_MASK;
                                    if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                                    {
                                        Value|=AuthorizedDacs123Configuration[5].Configuration;
                                    }
                                    else
                                    {
                                        Value|=AuthorizedDacs123Configuration[4].Configuration;
                                    }
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                                }
                                /* CVBS_MAIN on DAC3 */
                                else if ((Device_p->DacSelect & (STVOUT_DAC_3)) == (STVOUT_DAC_3))
                                {
                                    /* First of all see what there is on DACs 1 and 2 */
                                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                                                           TriDACConfig,
                                                                           AuthorizedDacs123Configuration,
                                                                           sizeof(AuthorizedDacs123Configuration)/sizeof(STVOUT_DAC_CONF_t));
                                    Value &= DENC_CFG13_DAC123_MASK;
                                    if (TriDACConfig[0] == STVOUT_DAC_OUTPUT_YC_MAIN)
                                    {
                                        Value|=AuthorizedDacs123Configuration[0].Configuration;
                                    }
                                    else if (TriDACConfig[0] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                                    {
                                        Value|=AuthorizedDacs123Configuration[5].Configuration;
                                    }
                                    else
                                    {
                                        Value|=AuthorizedDacs123Configuration[6].Configuration;
                                    }
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);

                                }
                                /* No configuration is required for the CVBS output on DAC6... only for STi5105*/
                                /* CVBS_MAIN on DAC6 */
                                else if ((Device_p->DacSelect & (STVOUT_DAC_6)) == (STVOUT_DAC_6))
                                {
                                    /* First of all see what there is on DACs 4 and 5 */
                                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC456_MASK))),
                                                                           TriDACConfig,
                                                                           AuthorizedDacs456Configuration,
                                                                           sizeof(AuthorizedDacs456Configuration)/sizeof(STVOUT_DAC_CONF_t));
                                    Value &=DENC_CFG13_DAC456_MASK;
                                    if (TriDACConfig[0] == STVOUT_DAC_OUTPUT_YC_AUX)
                                    {
                                        Value|=AuthorizedDacs456Configuration[1].Configuration;
                                    }
                                    else if (TriDACConfig[0] == STVOUT_DAC_OUTPUT_CVBS_AUX)
                                    {
                                        Value|=AuthorizedDacs456Configuration[4].Configuration;
                                    }
                                    else
                                    {
                                        Value|=AuthorizedDacs456Configuration[7].Configuration;
                                    }
                                    #if !(defined(ST_5105)|| defined(ST_5107)||defined(ST_5162))
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                                    #endif
                                }
                                #if !(defined(ST_5105)|| defined(ST_5107)||defined(ST_5162))
                                /* CVBS_MAIN on DAC5 */
                                else if ((Device_p->DacSelect & (STVOUT_DAC_5)) == (STVOUT_DAC_5))
                                {
                                    /* First of all see which CVBS(AUX/MAIN) was selected on DAC6*/
                                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC456_MASK))),
                                                                           TriDACConfig,
                                                                           AuthorizedDacs456Configuration,
                                                                           sizeof(AuthorizedDacs456Configuration)/sizeof(STVOUT_DAC_CONF_t));

                                    Value &=DENC_CFG13_DAC456_MASK;
                                    if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                                    {
                                        Value|=AuthorizedDacs456Configuration[4].Configuration;
                                    }
                                    else
                                    {
                                        Value|=AuthorizedDacs456Configuration[5].Configuration;
                                    }
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                                }
                                #endif
                                else
                                {
                                    Error=ST_ERROR_BAD_PARAMETER;
                                }

                                break;
                            default :
                                    Error=ST_ERROR_BAD_PARAMETER;
                                break;
                        }

                    break;

                case STVOUT_SOURCE_AUX :
                    #if !(defined(ST_5105)|| defined(ST_5107)||defined(ST_5162))/* It's possible to have only main video on STi5105/07 */
                    switch (Device_p->OutputType)
                    {
                        case STVOUT_OUTPUT_ANALOG_RGB :
                            if ((Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))== (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                                {
                                    Value &=DENC_CFG13_DAC123_MASK;
                                    Value|=AuthorizedDacs123Configuration[2].Configuration;
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                                }
                            else if ((Device_p->DacSelect &(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))== (STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))
                                {
                                    Value &=DENC_CFG13_DAC456_MASK;
                                    Value|=AuthorizedDacs456Configuration[2].Configuration;
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);

                                }
                                else
                                {
                                    Error=ST_ERROR_BAD_PARAMETER;
                                }
                                break;

                        case STVOUT_OUTPUT_ANALOG_YUV :
                            if ((Device_p->DacSelect &(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6)) == (STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))
                                {
                                    Value &=DENC_CFG13_DAC456_MASK;
                                    Value|=AuthorizedDacs456Configuration[3].Configuration;
                                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                                }
                            else
                                {
                                    Error=ST_ERROR_BAD_PARAMETER;
                                }

                            break;
                        case STVOUT_OUTPUT_ANALOG_YC :
                            /* YC_AUX on DACs 1 and 2 */
                            if ((Device_p->DacSelect & (STVOUT_DAC_1|STVOUT_DAC_2)) == (STVOUT_DAC_1|STVOUT_DAC_2))
                            {
                                /* First of all see if CVBS was selected on DAC3*/
                                stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                                                        TriDACConfig,
                                                                        AuthorizedDacs123Configuration,
                                                                        sizeof(AuthorizedDacs123Configuration)/sizeof(STVOUT_DAC_CONF_t));
                                Value &=DENC_CFG13_DAC123_MASK;
                                if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                                {
                                    Value|=AuthorizedDacs123Configuration[6].Configuration;
                                }
                                else
                                {
                                    Value|=AuthorizedDacs123Configuration[7].Configuration;
                                }
                                if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                            }
                            /* YC_AUX on DACS 4&5 */
                            else if ((Device_p->DacSelect & (STVOUT_DAC_4|STVOUT_DAC_5)) == (STVOUT_DAC_4|STVOUT_DAC_5))
                            {
                                /* First of all see if CVBS was selected on DAC6*/
                                stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC456_MASK))),
                                                                        TriDACConfig,
                                                                        AuthorizedDacs456Configuration,
                                                                        sizeof(AuthorizedDacs456Configuration)/sizeof(STVOUT_DAC_CONF_t));
                                Value &=DENC_CFG13_DAC456_MASK;
                                if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                                {
                                    Value|=AuthorizedDacs456Configuration[1].Configuration;
                                }
                                else
                                {
                                    Value|=AuthorizedDacs456Configuration[0].Configuration;
                                }
                                if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle, DENC_CFG13, Value);
                            }
                            else
                            {
                                Error=ST_ERROR_BAD_PARAMETER;
                            }
                            break;
                        case STVOUT_OUTPUT_ANALOG_CVBS :
                            /* CVBS_AUX on DAC4 */
                            if ((Device_p->DacSelect & (STVOUT_DAC_4)) == (STVOUT_DAC_4))
                            {
                                /* First of all see which CVBS(AUX/MAIN) was selected on DAC6*/
                                stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC456_MASK))),
                                                                        TriDACConfig,
                                                                        AuthorizedDacs456Configuration,
                                                                        sizeof(AuthorizedDacs456Configuration)/sizeof(STVOUT_DAC_CONF_t));
                                Value &=DENC_CFG13_DAC456_MASK;
                                if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                                {
                                    Value|=AuthorizedDacs456Configuration[4].Configuration;
                                }
                                else
                                {
                                    Value|=AuthorizedDacs456Configuration[5].Configuration;
                                }
                                if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                            }
                            /* CVBS_AUX on DAC6 */
                            else if ((Device_p->DacSelect & (STVOUT_DAC_6)) == (STVOUT_DAC_6))
                            {
                                /* First of all see what there is on DACs 4 and 5 */
                                stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC456_MASK))),
                                                                        TriDACConfig,
                                                                        AuthorizedDacs456Configuration,
                                                                        sizeof(AuthorizedDacs456Configuration)/sizeof(STVOUT_DAC_CONF_t));
                                Value &=DENC_CFG13_DAC456_MASK;
                                if (TriDACConfig[0] == STVOUT_DAC_OUTPUT_YC_MAIN)
                                {
                                    Value|=AuthorizedDacs456Configuration[6].Configuration;
                                }
                                else if (TriDACConfig[0] == STVOUT_DAC_OUTPUT_CVBS_AUX)
                                {
                                    Value|=AuthorizedDacs456Configuration[5].Configuration;
                                }
                                else
                                {
                                    Value|=AuthorizedDacs456Configuration[0].Configuration;
                                }
                                if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);

                            }
                            /* CVBS_AUX on DAC3 */
                            else if ((Device_p->DacSelect & (STVOUT_DAC_3)) == (STVOUT_DAC_3))
                            {
                                /* First of all see what there is on DACs 1 and 2 */
                                stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                                                        TriDACConfig,
                                                                        AuthorizedDacs123Configuration,
                                                                        sizeof(AuthorizedDacs123Configuration)/sizeof(STVOUT_DAC_CONF_t));
                                Value &=DENC_CFG13_DAC123_MASK;
                                if (TriDACConfig[0] == STVOUT_DAC_OUTPUT_YC_AUX)
                                {
                                    Value|=AuthorizedDacs123Configuration[7].Configuration;
                                }
                                else if (TriDACConfig[0] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                                {
                                    Value|=AuthorizedDacs123Configuration[4].Configuration;
                                }
                                else
                                {
                                    Value|=AuthorizedDacs123Configuration[1].Configuration;
                                }
                                if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                            }
                            /* CVBS_AUX on DAC2 */
                            else if ((Device_p->DacSelect & (STVOUT_DAC_2)) == (STVOUT_DAC_2))
                            {
                                /* First of all see which CVBS(AUX/MAIN) was selected on DAC3*/
                                stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                                                        TriDACConfig,
                                                                        AuthorizedDacs123Configuration,
                                                                        sizeof(AuthorizedDacs123Configuration)/sizeof(STVOUT_DAC_CONF_t));

                                Value &=DENC_CFG13_DAC123_MASK;
                                if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                                {
                                    Value|=AuthorizedDacs123Configuration[5].Configuration;
                                }
                                else
                                {
                                    Value|=AuthorizedDacs123Configuration[4].Configuration;
                                }
                                if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                            }
                            else
                            {
                                Error=ST_ERROR_BAD_PARAMETER;
                            }
                            break;
                        default :
                                Error=ST_ERROR_BAD_PARAMETER;
                            break;
                        }
                #endif
                default :
                            break;
            }

     }
     else if (Device_p->DeviceType==STVOUT_DEVICE_TYPE_4629)
     {
        Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
        Value&=DENC_CFG13_DAC123_MASK;

        switch (Device_p->OutputType)
        {
            case STVOUT_OUTPUT_ANALOG_RGB :
                 if ((Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))== (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                 {
                    Value|=AuthorizedDacs123Configuration4629[1].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                 }
                 else
                 {
                    Error=ST_ERROR_BAD_PARAMETER;
                 }
                 break;

            case STVOUT_OUTPUT_ANALOG_YUV :
                 if ((Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))== (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                 {
                    Value|=AuthorizedDacs123Configuration4629[2].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                 }
                 else
                 {
                    Error=ST_ERROR_BAD_PARAMETER;
                 }

                break;

            case STVOUT_OUTPUT_ANALOG_YC :
                if((Device_p->DacSelect&(STVOUT_DAC_1|STVOUT_DAC_2))==(STVOUT_DAC_1|STVOUT_DAC_2))
                {
                    Value|=AuthorizedDacs123Configuration4629[0].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {

                    Error=ST_ERROR_BAD_PARAMETER;
                }
                break;

            case STVOUT_OUTPUT_ANALOG_CVBS :
                if((Device_p->DacSelect&(STVOUT_DAC_3))==(STVOUT_DAC_3))
                {
                    Value|=AuthorizedDacs123Configuration4629[0].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                   Error=ST_ERROR_BAD_PARAMETER;
                }
                break;

            default :
                  Error=ST_ERROR_BAD_PARAMETER;
                break;
        }

     }
     else if ((Device_p->DeviceType==STVOUT_DEVICE_TYPE_7710))
     {
        switch (Device_p->OutputType)
        {

             case STVOUT_OUTPUT_ANALOG_YUV :
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if ((Device_p->DacSelect&(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))==(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                {
                    Value&=DENC_CFG13_DAC123_MASK;
                    Value|= AuthorizedDacsConfiguration7710[9].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                    Error=ST_ERROR_BAD_PARAMETER;
                }
                break;

             case STVOUT_OUTPUT_ANALOG_RGB :
                /* Add read value register here */
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if ((Device_p->DacSelect&(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))==(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                {
                    Value&=DENC_CFG13_DAC123_MASK;
                    Value|=AuthorizedDacsConfiguration7710[8].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else if ((Device_p->DacSelect&(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))==(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))
                {
                    Value&=DENC_CFG13_DAC456_MASK;
                    Value|= AuthorizedDacsConfiguration7710[1].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }

                else
                {
                    Error=ST_ERROR_BAD_PARAMETER;
                }

                break;

             case STVOUT_OUTPUT_ANALOG_YC :
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if((Device_p->DacSelect&(STVOUT_DAC_1|STVOUT_DAC_2))==(STVOUT_DAC_1|STVOUT_DAC_2))
                {
                    /* First of all see if CVBS was selected on DAC3*/
                    /* Use of DAC123_CONF in DEN_CFG13 for this configuration */
                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                        TriDACConfig,
                                        AuthorizedDacsConfiguration7710,
                                        sizeof(AuthorizedDacsConfiguration7710)/sizeof(STVOUT_DAC_CONF_t));

                    Value &=DENC_CFG13_DAC123_MASK;

                    if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                    {
                        Value|=AuthorizedDacsConfiguration7710[6].Configuration;
                    }
                    else
                    {
                        Value|=AuthorizedDacsConfiguration7710[7].Configuration;
                    }
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }

                else if((Device_p->DacSelect&(STVOUT_DAC_4|STVOUT_DAC_5))==(STVOUT_DAC_4|STVOUT_DAC_5))
                {
                    /* First of all see if CVBS was selected on DAC6*/
                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC456_MASK))),
                                                            TriDACConfig,
                                                            AuthorizedDacsConfiguration7710,
                                                            sizeof(AuthorizedDacsConfiguration7710)/sizeof(STVOUT_DAC_CONF_t));
                   Value &=DENC_CFG13_DAC456_MASK;
                   if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                   {
                       Value|=AuthorizedDacsConfiguration7710[5].Configuration;
                   }
                   else
                   {
                       Value|=AuthorizedDacsConfiguration7710[4].Configuration;
                   }
                   if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                    Error=ST_ERROR_BAD_PARAMETER;
                }
                break;

            case STVOUT_OUTPUT_ANALOG_CVBS :
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if((Device_p->DacSelect&(STVOUT_DAC_3))==(STVOUT_DAC_3))
                {
                    /* First of all see if CVBS was selected on DAC1*/
                    /* Use of DAC123_CONF in DEN_CFG13 for this configuration */
                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                        TriDACConfig,
                                        AuthorizedDacsConfiguration7710,
                                        sizeof(AuthorizedDacsConfiguration7710)/sizeof(STVOUT_DAC_CONF_t));

                    Value &=DENC_CFG13_DAC123_MASK;

                    if (TriDACConfig[1] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                    {
                        Value|=AuthorizedDacsConfiguration7710[11].Configuration;
                    }
                    else
                    {
                        Value|=AuthorizedDacsConfiguration7710[6].Configuration;
                    }
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);

                }
                else if ((Device_p->DacSelect&(STVOUT_DAC_1))==(STVOUT_DAC_1))
                {
                    Value &=DENC_CFG13_DAC123_MASK;
                    Value|=AuthorizedDacsConfiguration7710[11].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else if ((Device_p->DacSelect&(STVOUT_DAC_5))==(STVOUT_DAC_5))
                {
                    Value &=DENC_CFG13_DAC456_MASK;
                    Value|=AuthorizedDacsConfiguration7710[2].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else if ((Device_p->DacSelect&(STVOUT_DAC_6))==(STVOUT_DAC_6))
                {
                    Value &=DENC_CFG13_DAC456_MASK;
                    Value|=AuthorizedDacsConfiguration7710[0].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                   Error=ST_ERROR_BAD_PARAMETER;
                }
                break;


            default :
                  Error=ST_ERROR_BAD_PARAMETER;
                break;
        }
     }
     else if ((Device_p->DeviceType==STVOUT_DEVICE_TYPE_7100))
     {
        /* Don't read CFG13 register here because crash with HDMI handle */
        switch (Device_p->OutputType)
        {
             case STVOUT_OUTPUT_ANALOG_YUV :
                /* Add read value register here */
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if ((Device_p->DacSelect&(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))==(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                {
                    Value&=DENC_CFG13_DAC123_MASK;
                    Value|=AuthorizedDacsConfiguration7100[9].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                    Error=ST_ERROR_BAD_PARAMETER;
                }

                break;
             case STVOUT_OUTPUT_ANALOG_RGB :
                /* Add read value register here */
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if ((Device_p->DacSelect&(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))==(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                {
                    Value&=DENC_CFG13_DAC123_MASK;
                    Value|=AuthorizedDacsConfiguration7100[8].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else if ((Device_p->DacSelect&(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))==(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))
                {
                    Value&=DENC_CFG13_DAC456_MASK;
                    Value|= AuthorizedDacsConfiguration7100[1].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                    Error=ST_ERROR_BAD_PARAMETER;
                }

                break;

             case STVOUT_OUTPUT_ANALOG_YC :

                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if((Device_p->DacSelect&(STVOUT_DAC_1|STVOUT_DAC_2))==(STVOUT_DAC_1|STVOUT_DAC_2))
                {
                    /* First of all see if CVBS was selected on DAC3*/
                    /* Use of DAC123_CONF in DEN_CFG13 for this configuration */
                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                        TriDACConfig,
                                        AuthorizedDacsConfiguration7100,
                                        sizeof(AuthorizedDacsConfiguration7100)/sizeof(STVOUT_DAC_CONF_t));

                    Value &=DENC_CFG13_DAC123_MASK;
                    if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                    {
                        Value|= AuthorizedDacsConfiguration7100[6].Configuration;
                    }
                    else
                    {
                        Value|= AuthorizedDacsConfiguration7100[7].Configuration;
                    }
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else if((Device_p->DacSelect&(STVOUT_DAC_4|STVOUT_DAC_5))==(STVOUT_DAC_4|STVOUT_DAC_5))
                {
                    /* First of all see if CVBS was selected on DAC6*/
                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC456_MASK))),
                                                            TriDACConfig,
                                                            AuthorizedDacsConfiguration7710,
                                                            sizeof(AuthorizedDacsConfiguration7710)/sizeof(STVOUT_DAC_CONF_t));
                   Value &=DENC_CFG13_DAC456_MASK;
                   if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                   {
                       Value|= AuthorizedDacsConfiguration7100[5].Configuration;
                   }
                   else
                   {
                       Value|= AuthorizedDacsConfiguration7100[4].Configuration;
                   }
                   if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                    Error = ST_ERROR_BAD_PARAMETER;
                }
                break;

            case STVOUT_OUTPUT_ANALOG_CVBS :
                /* STFAE - Add read value register here */
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if((Device_p->DacSelect&(STVOUT_DAC_3))==(STVOUT_DAC_3))
                {
                    /* First of all see if CVBS was selected on DAC1*/
                    /* Use of DAC123_CONF in DEN_CFG13 for this configuration */
                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                        TriDACConfig,
                                        AuthorizedDacsConfiguration7100,
                                        sizeof(AuthorizedDacsConfiguration7100)/sizeof(STVOUT_DAC_CONF_t));

                    Value &=DENC_CFG13_DAC123_MASK;

                    if (TriDACConfig[1] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                    {
                        Value|=AuthorizedDacsConfiguration7100[11].Configuration;
                    }
                    else
                    {
                        Value|=AuthorizedDacsConfiguration7100[6].Configuration;
                    }
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);

                }
                else if ((Device_p->DacSelect&(STVOUT_DAC_1))==(STVOUT_DAC_1))
                {
                    Value &=DENC_CFG13_DAC123_MASK;
                    Value|=AuthorizedDacsConfiguration7100[11].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else if ((Device_p->DacSelect&(STVOUT_DAC_6))==(STVOUT_DAC_6))
                {
                    Value &=DENC_CFG13_DAC456_MASK;
                    Value|=AuthorizedDacsConfiguration7100[5].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);

                }
                else if ((Device_p->DacSelect&(STVOUT_DAC_5))==(STVOUT_DAC_5))
                {
                    Value &=DENC_CFG13_DAC456_MASK;
                    Value|=AuthorizedDacsConfiguration7100[2].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                   Error=ST_ERROR_BAD_PARAMETER;
                }
                break;

            /* Add support of HD_ANALOG_YUV */
            case STVOUT_OUTPUT_HD_ANALOG_YUV :
                break;

            default :
                  Error=ST_ERROR_BAD_PARAMETER;
                break;
        }
     }
     else if ((Device_p->DeviceType==STVOUT_DEVICE_TYPE_7200))
     {       /*for cut 1 SD dacs are 1,2 and 3 dacs 4 and 5 are not used*/
        switch (Device_p->OutputType)
        {
             case STVOUT_OUTPUT_ANALOG_YUV :
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if ((Device_p->DacSelect&(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))==(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                {
                    Value&=DENC_CFG13_DAC123_MASK;
                    Value|= AuthorizedDacsConfiguration7200[3].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                    Error=ST_ERROR_BAD_PARAMETER;
                }
                break;
             case STVOUT_OUTPUT_ANALOG_RGB :
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if ((Device_p->DacSelect&(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))==(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                {
                    Value&=DENC_CFG13_DAC123_MASK;
                    Value|= AuthorizedDacsConfiguration7200[2].Configuration;
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                    Error=ST_ERROR_BAD_PARAMETER;
                }
                break;
             case STVOUT_OUTPUT_ANALOG_YC :
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if((Device_p->DacSelect&(STVOUT_DAC_1|STVOUT_DAC_2))==(STVOUT_DAC_1|STVOUT_DAC_2))
                {
                    /* First of all see if CVBS was selected on DAC3*/
                    /* Use of DAC123_CONF in DEN_CFG13 for this configuration */
                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                        TriDACConfig,
                                        AuthorizedDacsConfiguration7200,
                                        sizeof(AuthorizedDacsConfiguration7200)/sizeof(STVOUT_DAC_CONF_t));

                    Value &=DENC_CFG13_DAC123_MASK;
                    if (TriDACConfig[2] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                    {
                        Value|= AuthorizedDacsConfiguration7200[0].Configuration;
                    }
                    else
                    {
                        Value|= AuthorizedDacsConfiguration7200[1].Configuration;
                    }
                    if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                    Error = ST_ERROR_BAD_PARAMETER;
                }
                break;
             case STVOUT_OUTPUT_ANALOG_CVBS :
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                if((Device_p->DacSelect&(STVOUT_DAC_3))==(STVOUT_DAC_3))
                {
                    /* First of all see if CVBS was selected on DAC1*/
                    /* Use of DAC123_CONF in DEN_CFG13 for this configuration */
                    stvout_FindCurrentConfigurationFromReg((Value & (~(DENC_CFG13_DAC123_MASK))),
                                        TriDACConfig,
                                        AuthorizedDacsConfiguration7200,
                                        sizeof(AuthorizedDacsConfiguration7200)/sizeof(STVOUT_DAC_CONF_t));

                    Value &=DENC_CFG13_DAC123_MASK;

                    if (TriDACConfig[1] == STVOUT_DAC_OUTPUT_CVBS_MAIN)
                    {
                        Value|= AuthorizedDacsConfiguration7200[5].Configuration;
                    }
                    else
                    {
                        Value|= AuthorizedDacsConfiguration7200[6].Configuration;
                    }
                    if(OK(Error)) Error = STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);

                }
                else if ((Device_p->DacSelect&(STVOUT_DAC_1))==(STVOUT_DAC_1))
                {
                    Value &=DENC_CFG13_DAC123_MASK;
                    Value|= AuthorizedDacsConfiguration7200[5].Configuration;
                    if(OK(Error)) Error = STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                }
                else
                {
                   Error = ST_ERROR_BAD_PARAMETER;
                }
                break;
             default:
                Error = ST_ERROR_BAD_PARAMETER;
                break;
        }
     }
     else if ((Device_p->DeviceType==STVOUT_DEVICE_TYPE_5525))
     {
        Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
        switch (Source)
        {
            case STVOUT_SOURCE_MAIN :
                switch (Device_p->OutputType)
                {
                    case STVOUT_OUTPUT_ANALOG_RGB :
                        if ((Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3)) == (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                        {
                            Value &=DENC_CFG13_DAC123_MASK;
                            Value|=AuthorizedDacs123Configuration5525[1].Configuration;
                            if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                        }
                        else
                        {
                            Error=ST_ERROR_BAD_PARAMETER;
                        }
                        break;

                    case STVOUT_OUTPUT_ANALOG_YUV :
                        if ((Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3)) == (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                        {
                            Value &=DENC_CFG13_DAC123_MASK;
                            Value|=AuthorizedDacs123Configuration5525[2].Configuration;
                            if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                        }
                        else
                        {
                            Error=ST_ERROR_BAD_PARAMETER;
                        }
                        break;
                    case STVOUT_OUTPUT_ANALOG_YC :
                        /* YC_MAIN on DACs 1 and 2 */
                        if ((Device_p->DacSelect & (STVOUT_DAC_1|STVOUT_DAC_2)) == (STVOUT_DAC_1|STVOUT_DAC_2))
                        {
                            Value &=DENC_CFG13_DAC123_MASK;
                            Value|=AuthorizedDacs123Configuration5525[0].Configuration;
                            if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                        }
                        else
                        {
                            Error=ST_ERROR_BAD_PARAMETER;
                        }
                        break;
                    case STVOUT_OUTPUT_ANALOG_CVBS :
                        if ((Device_p->DacSelect & (STVOUT_DAC_3)) == (STVOUT_DAC_3))
                        {
                             Value &= DENC_CFG13_DAC123_MASK;
                             Value|=AuthorizedDacs123Configuration5525[0].Configuration;
                             if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_4)) == (STVOUT_DAC_4))
                        {
                            Value &=DENC_CFG13_DAC4_MASK;  /*to be checked*/
                            Value|=AuthorizedDacs4Configuration5525[0].Configuration;
                            if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                         }
                         else
                         {
                             Error=ST_ERROR_BAD_PARAMETER;
                         }
                         break;
                    default :
                         Error=ST_ERROR_BAD_PARAMETER;
                         break;
                }
            case STVOUT_SOURCE_AUX :
                #if defined (ST_5188)
                    Error=ST_ERROR_BAD_PARAMETER;
                #else
                    switch (Device_p->OutputType)
                    {
                        case STVOUT_OUTPUT_ANALOG_CVBS :
                            if ((Device_p->DacSelect & (STVOUT_DAC_4)) == (STVOUT_DAC_4))
                            {
                                Value &=DENC_CFG13_DAC4_MASK;  /*to be checked*/
                                Value|=AuthorizedDacs4Configuration5525[0].Configuration;
                                if(OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                            }
                            else
                            {
                                Error=ST_ERROR_BAD_PARAMETER;
                            }
                            break;
                        default :
                            Error=ST_ERROR_BAD_PARAMETER;
                            break;
                    }
                #endif

                break;
            default :
                break;
        }
     }

     if ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)||(Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13))
       {
            if ((Device_p->OutputType==STVOUT_OUTPUT_ANALOG_RGB)&&(Source==STVOUT_SOURCE_AUX))
            {
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                #if !(defined(ST_5105)|| defined(ST_5107)||defined(ST_5162))
                Value |=DENC_CFG13_AUX_NMAI_RGB;
                 /* It's possible to have only main video on STi5105 */
                if (OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
                #endif
            }
            else if ((Device_p->OutputType==STVOUT_OUTPUT_ANALOG_RGB)&&(Source==STVOUT_SOURCE_MAIN))
            {
                Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
                Value &= ~DENC_CFG13_AUX_NMAI_RGB;
                if (OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
            }
       }
       else if(Device_p->DeviceType==STVOUT_DEVICE_TYPE_4629)
       {
               Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
               Value &= ~DENC_CFG13_AUX_NMAI_RGB;
               if (OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
       }
       else if(Device_p->DeviceType==STVOUT_DEVICE_TYPE_5525)
       {
               Error=STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG13,&Value);
               Value &= ~DENC_CFG13_AUX_NMAI_RGB;
               if (OK(Error)) Error=STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG13,Value);
       }

     return(ERROR(Error));
 } /* end of stvout_HalSetInputSource */

/*
--------------------------------------------------------------------------------
Adjust Chroma Luma delay
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t stvout_HalAdjustChromaLumaDelay ( stvout_Device_t *              Device_p,
                                                 STVOUT_ChromaLumaDelay_t       ChrLumDelay
                                                )

{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8  Value = 0;

    switch (Device_p->OutputType)
    {
        case STVOUT_OUTPUT_ANALOG_YC :
            #if defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
                Error = STDENC_ReadReg8(Device_p->DencHandle,DENC_CHRDEL_EN,&Value);
                Value |= DENC_CHRDEL_DELAY_ENABLED ;
                if(OK(Error)) Error = STDENC_WriteReg8(Device_p->DencHandle,DENC_CHRDEL_EN,Value);
                switch (ChrLumDelay)
                {
                    case STVOUT_CHROMALUMA_DELAY_M2     :
                        Error = stvout_HalDencWrite8( Device_p , DENC_CHRDEL , (U8) DENC_CHRDEL_MASK_DELAY, DENC_CHRDEL_DELAY_M2);
                        break;
                    case STVOUT_CHROMALUMA_DELAY_M1_5   :
                        Error = stvout_HalDencWrite8( Device_p , DENC_CHRDEL , (U8) DENC_CHRDEL_MASK_DELAY, DENC_CHRDEL_DELAY_M1_5);
                        break;
                    case STVOUT_CHROMALUMA_DELAY_M1     :
                        Error = stvout_HalDencWrite8( Device_p , DENC_CHRDEL , (U8) DENC_CHRDEL_MASK_DELAY, DENC_CHRDEL_DELAY_M1);
                        break;
                    case STVOUT_CHROMALUMA_DELAY_M0_5   :
                        Error = stvout_HalDencWrite8( Device_p , DENC_CHRDEL , (U8) DENC_CHRDEL_MASK_DELAY, DENC_CHRDEL_DELAY_M0_5);
                        break;
                    case STVOUT_CHROMALUMA_DELAY_0      :
                        Error = stvout_HalDencWrite8( Device_p , DENC_CHRDEL , (U8) DENC_CHRDEL_MASK_DELAY, DENC_CHRDEL_DELAY_0);
                        break;
                    case STVOUT_CHROMALUMA_DELAY_0_5    :
                        Error = stvout_HalDencWrite8( Device_p , DENC_CHRDEL , (U8) DENC_CHRDEL_MASK_DELAY, DENC_CHRDEL_DELAY_0_5);
                        break;
                    case STVOUT_CHROMALUMA_DELAY_P1     :
                        Error = stvout_HalDencWrite8( Device_p , DENC_CHRDEL , (U8) DENC_CHRDEL_MASK_DELAY, DENC_CHRDEL_DELAY_P1);
                        break;
                    case STVOUT_CHROMALUMA_DELAY_P1_5   :
                        Error = stvout_HalDencWrite8( Device_p , DENC_CHRDEL , (U8) DENC_CHRDEL_MASK_DELAY, DENC_CHRDEL_DELAY_P1_5);
                        break;
                    case STVOUT_CHROMALUMA_DELAY_P2     :
                        Error = stvout_HalDencWrite8( Device_p , DENC_CHRDEL , (U8) DENC_CHRDEL_MASK_DELAY, DENC_CHRDEL_DELAY_P2);
                        break;
                    case STVOUT_CHROMALUMA_DELAY_P2_5   :
                        Error = stvout_HalDencWrite8( Device_p , DENC_CHRDEL , (U8) DENC_CHRDEL_MASK_DELAY, DENC_CHRDEL_DELAY_P2_5);
                        break;
                    default:
                        Error = ST_ERROR_BAD_PARAMETER;
                        break;
                }
                break;
            #endif /*No break if !(defined(ST_7109) || defined (ST_7200) || defined (ST_7111)|| defined (ST_7105))*/
        case STVOUT_OUTPUT_ANALOG_CVBS :
            Error = STDENC_ReadReg8(Device_p->DencHandle,DENC_CFG3,&Value);
            Value |= DENC_CFG3_DELAY_ENABLE ;
            if(OK(Error)) Error = STDENC_WriteReg8(Device_p->DencHandle,DENC_CFG3,Value);
            switch (ChrLumDelay)
            {
                case STVOUT_CHROMALUMA_DELAY_M2     :
                    Error = stvout_HalDencWrite8( Device_p , DENC_CFG9 , (U8) DENC_CFG9_MASK_DELAY, DENC_CFG9_DELAY_M2);
                    break;
                case STVOUT_CHROMALUMA_DELAY_M1_5   :
                    Error = stvout_HalDencWrite8( Device_p , DENC_CFG9 , (U8) DENC_CFG9_MASK_DELAY, DENC_CFG9_DELAY_M1_5);
                    break;
                case STVOUT_CHROMALUMA_DELAY_M1     :
                    Error = stvout_HalDencWrite8( Device_p , DENC_CFG9 , (U8) DENC_CFG9_MASK_DELAY, DENC_CFG9_DELAY_M1);
                    break;
                case STVOUT_CHROMALUMA_DELAY_M0_5   :
                    Error = stvout_HalDencWrite8( Device_p , DENC_CFG9 , (U8) DENC_CFG9_MASK_DELAY, DENC_CFG9_DELAY_M0_5);
                    break;
                case STVOUT_CHROMALUMA_DELAY_0      :
                    Error = stvout_HalDencWrite8( Device_p , DENC_CFG9 , (U8) DENC_CFG9_MASK_DELAY, DENC_CFG9_DELAY_0);
                    break;
                case STVOUT_CHROMALUMA_DELAY_0_5    :
                    Error = stvout_HalDencWrite8( Device_p , DENC_CFG9 , (U8) DENC_CFG9_MASK_DELAY, DENC_CFG9_DELAY_0_5);
                    break;
                case STVOUT_CHROMALUMA_DELAY_P1     :
                    Error = stvout_HalDencWrite8( Device_p , DENC_CFG9 , (U8) DENC_CFG9_MASK_DELAY, DENC_CFG9_DELAY_P1);
                    break;
                case STVOUT_CHROMALUMA_DELAY_P1_5   :
                    Error = stvout_HalDencWrite8( Device_p , DENC_CFG9 , (U8) DENC_CFG9_MASK_DELAY, DENC_CFG9_DELAY_P1_5);
                    break;
                case STVOUT_CHROMALUMA_DELAY_P2     :
                    Error = stvout_HalDencWrite8( Device_p , DENC_CFG9 , (U8) DENC_CFG9_MASK_DELAY, DENC_CFG9_DELAY_P2);
                    break;
                case STVOUT_CHROMALUMA_DELAY_P2_5   :
                    Error = stvout_HalDencWrite8( Device_p , DENC_CFG9 , (U8) DENC_CFG9_MASK_DELAY, DENC_CFG9_DELAY_P2_5);
                    break;
                default:
                    Error = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            break;
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return (Error);
}
/* end of file */
