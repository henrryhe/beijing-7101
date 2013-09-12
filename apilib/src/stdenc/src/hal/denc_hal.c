/**********************************************************************************

File Name   : denc_hal.c

Description : DENC Hardware Abstraction Layer module

COPYRIGHT (C) STMicroelectronics 2003

Date               Modification                                             Name
----               ------------                                             ----
06 Jun 2000        Created                                                  JG
16 Oct 2000        multi-init ..., hal for STV0119 : function               JG
 *                 renamed for Sti55xx, Sti70xx
15 Nov 2000        improve error tracability                                HSdLM
06 Feb 2001        Reorganize HAL for providing mutual exclusion on         HSdLM
 *                 registers access
07 Mar 2001        Add NTSC M-J, NTSC M-4.43 and PAL Nc features            HSdLM
 *                 Mode NTSC is replaced by NTSCM
 *                 Add square pixel, non-interlaced and NTSC 60Hz options.
 *                 Set 7.5IRE blanking setup for PAL N,M and NTSC M
 *                 Enable trap filter for color sub-carrier frequency
 *                 IDFS configuration (color sub-carrier frequency)
 *                 Remove enabling CGMS & WSS (STVBI driver task)
 *                 Remove useless code (read-write registers not modified)
 *                 Fix error : REG7 exists for pour CellId >=6 and not 5
21 Mar 2001        Fix error handling bit DENC_CFG5_SEL_INC                 HSdLM
 *                 At init, set reset values in registers.
22 Jun 2001        Add parenthesis around macro argument                    HSdLM
 *                 Exported symbols => stdenc_...
 *                 Reorganize code to make it clear after new options
 *                 setting. Set HSync positive pulse for 7015.
12 Jul 2001        Fix issue : Option.Pal.Interlaced used in NTSC config.   HSdLM
 *                 Non-Interlaced bit not programmed to 0 in SECAM.
02 Aou 2001        Remove enabling cyclic subcarrier phase reset.           HSdLM
 *                 (that fixes DDTS GNBvd06616). Set 7.5IRE blanking
 *                 setup on YUV. Work around for SECAM autotest on 7015
 *                 (DDTS GNBvd07052)
30 Aou 2001        Remove dependency upon STI2C if not needed, to support   HSdLM
 *                 ST40GX1
16 Apr 2002        Add support for STi7020                                  HSdLM
30 Oct 2002        Fix SD quality issue: enable oscillator reset            HSdLM
27 Feb 2004        Add support for STi5100                                  MH
***********************************************************************************/

 /* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stdevice.h"
/*#if !defined (ST_OSLINUX)*/
#include "sttbx.h"
/*#endif*/

#include "denc_reg.h"
#include "stdenc.h"
#include "denc_drv.h"
#include "denc_hal.h"
/* Private Types ------------------------------------------------------------ */

typedef struct
{
    U8 Cfg0;
    U8 Cfg1;
    U8 Cfg2;
    U8 Cfg3;
    U8 Cfg4;
    U8 Cfg5;
    U8 Cfg6;
    U8 Cfg7;
    U8 Cfg9;
    U8 Cfg10;
    U8 Cfg11;
    U8 Cfg12;
    U8 Idfs0;
    U8 Idfs1;
    U8 Idfs2;
    BOOL ProgIdfs;
} Configuration_t;

/* Private Constants -------------------------------------------------------- */
enum
{
    NTSCM_443,          /* ref clk 27000kHz,    color subcarrier frequency 4433618.75 Hz*/
    NTSCM_443_SQ,       /* ref clk 24545.45kHz, color subcarrier frequency 4433618.75 Hz*/
    NTSCM_60Hz,         /* ref clk 27027kHz,    color subcarrier frequency 3579545.2  Hz*/
    NTSCM_60Hz_SQ,      /* ref clk 24057kHz,    color subcarrier frequency 3579545.2  Hz*/
    NTSCM_443_60Hz,     /* ref clk 27027kHz,    color subcarrier frequency 4433618.75 Hz*/
    NTSCM_443_60Hz_SQ,   /* ref clk 24057kHz,    color subcarrier frequency 4433618.75 Hz*/
    PALN_SQ
};

/* Private Variables (static)------------------------------------------------ */
#if defined(ST_5528)||defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_5188)||defined(ST_5525)||defined(ST_5107)\
  ||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)||defined(ST_7200)||defined(ST_5162)

static U32 Idfs[] = {   0x2A0850, /* NTSCM_443         */ /*yxl 2007-09-06 modify from 0x2A098B to 0x2A0898 to improve chrom subcarrier freq offset*/
                        0x26798C, /* NTSCM_443_SQ      */
                        0x21F07C, /* NTSCM_60Hz        */
                        0x255554, /* NTSCM_60Hz_SQ     */
                        0x21E6F0, /* NTSCM_443_60Hz    */
                        0x254AD4,  /* NTSCM_443_60Hz_SQ */
                        0x1F15C0  /* PALN_SQ */
                    };
#else
static U32 Idfs[] = {   0x2A098B, /* NTSCM_443         */
                        0x2E3DB2, /* NTSCM_443_SQ      */
                        0x21E7CE, /* NTSCM_60Hz        */
                        0x254BC9, /* NTSCM_60Hz_SQ     */
                        0x29FECB, /* NTSCM_443_60Hz    */
                        0x2E31DF  /* NTSCM_443_60Hz_SQ */
                    };
#endif

/* DENC registers are 'Delay on luma path with reference to chroma path on 4:2:2 inputs'
 * At API level, choosen value is 'Chroma Delay', so delays must be inverted, that's why
 * table starts with positive delay toward negative delays */
static U8 ChromaDelayV3To9[] = {    DENC_CFG3_DELAY_P2,
                                    DENC_CFG3_DELAY_P1,
                                    DENC_CFG3_DELAY_0,
                                    DENC_CFG3_DELAY_M1,
                                    DENC_CFG3_DELAY_M2
                               };

/* Delay on chroma path with reference to luma path on S-VHS and CVBS */
static U8 ChromaDelayV10More[] = {  DENC_CFG9_DELAY_M2,
                                    DENC_CFG9_DELAY_M1_5,
                                    DENC_CFG9_DELAY_M1,
                                    DENC_CFG9_DELAY_M0_5,
                                    DENC_CFG9_DELAY_0,
                                    DENC_CFG9_DELAY_0_5,
                                    DENC_CFG9_DELAY_P1,
                                    DENC_CFG9_DELAY_P1_5,
                                    DENC_CFG9_DELAY_P2,
                                    DENC_CFG9_DELAY_P2_5
                                 };

/* Delay on AUX chroma path with reference to AUX luma path on S-VHS and CVBS */
static U8 ChromaDelayAux[] = {      DENC_CFG11_AUX_DEL_M2,
                                    DENC_CFG11_AUX_DEL_M1_5,
                                    DENC_CFG11_AUX_DEL_M1,
                                    DENC_CFG11_AUX_DEL_M0_5,
                                    DENC_CFG11_AUX_DEL_0,
                                    DENC_CFG11_AUX_DEL_0_5,
                                    DENC_CFG11_AUX_DEL_P1,
                                    DENC_CFG11_AUX_DEL_P1_5,
                                    DENC_CFG11_AUX_DEL_P2,
                                    DENC_CFG11_AUX_DEL_P2_5
                                 };

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define UPDATE_REG(reg, mask, condition) ((condition) ? ((reg) |= (mask)) : ((reg) &= ~(mask)))
#define OK(ErrorCode) ((ErrorCode) == ST_NO_ERROR)
#define IDFS2(val)    ((U8)(((val) & 0xFF0000)>>16))
#define IDFS1(val)    ((U8)(((val) & 0xFF00)>>8))
#define IDFS0(val)    ((U8)((val)  & 0xFF))

/* Private Function prototypes ---------------------------------------------- */
static void ConfigPAL(DENCDevice_t * const Device_p, Configuration_t * const Conf_p);
static void ConfigNTSC(DENCDevice_t * const Device_p, Configuration_t * const Conf_p);
static void ConfigSECAM(DENCDevice_t * const Device_p, Configuration_t * const Conf_p);

/* Functions ---------------------------------------------------------------- */


/* Private functions (static) ----------------------------------------------- */

/*******************************************************************************
Name        : ConfigPAL
Description : Configure PAL modes
Parameters  : IN  : Device_p : hardware target identification
              IN/OUT : Conf_p : configuration values to update
Assumptions : Device_p and Conf_p are not NULL.
 *            Device_p->EncodingMode.Mode is OK.
Limitations :
Returns     : none
*******************************************************************************/
static void ConfigPAL(DENCDevice_t * const Device_p, Configuration_t * const Conf_p)
{
    /* common PAL B,D,G,H,I,M,N,Nc*/

    Conf_p->Cfg2 &= ~DENC_CFG2_SEL_RST;
    Conf_p->Cfg2 |= (DENC_CFG2_ENA_RST | DENC_CFG2_ENA_BURST | DENC_CFG2_RST_2F);
    UPDATE_REG(Conf_p->Cfg2, DENC_CFG2_NO_INTER,  !Device_p->EncodingMode.Option.Pal.Interlaced);
    UPDATE_REG(Conf_p->Cfg3, DENC_CFG3_ENA_TRFLT, Device_p->LumaTrapFilter);

    if (Device_p->DencVersion < 10)
    {
        Conf_p->Cfg3 &= DENC_CFG3_MASK_DELAY;
        Conf_p->Cfg3 |= ChromaDelayV3To9[(Device_p->ChromaDelay-STDENC_MIN_CHROMA_DELAY)/STDENC_STEP_CHROMA_DELAY_V10_LESS];
    }
    else /* Id >= 10 */
    {
        Conf_p->Cfg3 |= DENC_CFG3_DELAY_ENABLE;
        Conf_p->Cfg9 &= DENC_CFG9_MASK_DELAY;
/* *********************************** *
 * conpensate 1 pixel for 4:4:4 mode   *
 * compensate 1.5 pixel for 4:2:2 mode *
 * *********************************** */

#if defined DENC_MAIN_COMPONSATE_2PIXEL/*DENC_COMPENSATION_ENABLED*/
        Conf_p->Cfg9  |= DENC_CFG9_DELAY_M2;  /*add -2 pixel delay for the main (supporting only 4:4:4 mode)*/
#elif defined DENC_MAIN_COMPONSATE_1_5PIXEL
        Conf_p->Cfg9  |= DENC_CFG9_DELAY_M1_5;  /*add -2 pixel delay for the main (supporting only 4:4:4 mode)*/
#elif defined DENC_MAIN_COMPONSATE_1PIXEL
        Conf_p->Cfg9  |= DENC_CFG9_DELAY_M1;  /*add -1 pixel delay for the main (supporting only 4:4:4 mode)*/
#elif defined DENC_MAIN_COMPONSATE_0_5PIXEL
        Conf_p->Cfg9  |= DENC_CFG9_DELAY_M0_5;  /*add -0.5 pixel delay for the main (supporting only 4:4:4 mode)*/
#else   /*no composation needed -> set default value*/
        Conf_p->Cfg9 |= ChromaDelayV10More[(Device_p->ChromaDelay-STDENC_MIN_CHROMA_DELAY)/STDENC_STEP_CHROMA_DELAY_V10_MORE];
#endif /*DENC_COMPENSATION_2PIXEL*/
    }
    if (Device_p->IsAuxEncoderOn)
    {
        Conf_p->Cfg12 |= DENC_CFG12_AUX_DEL_EN;
        UPDATE_REG(Conf_p->Cfg12, DENC_CFG12_AUX_ENTRAP, Device_p->LumaTrapFilterAux);
        Conf_p->Cfg11 &= DENC_CFG11_AUX_MASK_DEL;

#if defined DENC_AUX_COMPONSATE_2PIXEL/*DENC_AUX_COMPENSATION_ENABLED*/
        Conf_p->Cfg11  |= DENC_CFG11_AUX_DEL_M2;  /*add -2 pixel delay for the aux (supporting only 4:2:2 mode)*/
#elif defined DENC_AUX_COMPONSATE_1_5PIXEL
        Conf_p->Cfg11  |= DENC_CFG11_AUX_DEL_M1_5;  /*add -1.5 pixel delay for the aux (supporting only 4:2:2 mode)*/
#elif defined DENC_AUX_COMPONSATE_1PIXEL
        Conf_p->Cfg11  |= DENC_CFG11_AUX_DEL_M1;  /*add -1 pixel delay for the aux (supporting only 4:2:2 mode)*/
#elif defined DENC_AUX_COMPONSATE_0_5PIXEL
        Conf_p->Cfg11  |= DENC_CFG11_AUX_DEL_M0_5;  /*add -0.5 pixel delay for the aux (supporting only 4:2:2 mode)*/
#else   /*no composation needed -> set default value*/
        Conf_p->Cfg11 |= ChromaDelayAux[(Device_p->ChromaDelay-STDENC_MIN_CHROMA_DELAY)/STDENC_STEP_CHROMA_DELAY_V10_MORE];
#endif
    }

    if (Device_p->DencVersion >= 6)
    {
        Conf_p->Cfg7 &= ~DENC_CFG7_SECAM;
        UPDATE_REG(Conf_p->Cfg7, DENC_CFG7_SQ_PIX, Device_p->EncodingMode.Option.Pal.SquarePixel);
    }

    #if defined(ST_5100)|| defined(ST_5528)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188) \
     || defined(ST_5525)|| defined(ST_5107)|| defined(ST_7100)|| defined(ST_7109)|| defined(ST_7200)|| defined(ST_5162)
            if (Device_p->EncodingMode.Option.Pal.SquarePixel)
            {
                if (Device_p->DencVersion >= 12)
                {
                    /* for mode PAL, IDFS registers always need to be programmed */
                    Conf_p->Cfg5 |= DENC_CFG5_SEL_INC;
                    Conf_p->ProgIdfs = TRUE;
                }
            }
    #endif


    /* specific */
    switch( Device_p->EncodingMode.Mode)
    {
        case STDENC_MODE_PALBDGHI :
            Conf_p->Cfg0 |= DENC_CFG0_PAL_BDGHI;
            Conf_p->Cfg1 |= DENC_CFG1_FLT_19;
            Conf_p->Cfg3 |= DENC_CFG3_PAL_TRFLT;
            if (Device_p->DencVersion >= 12)
            {
                Conf_p->Cfg10 |= DENC_CFG10_AUX_FLT_19;
                Conf_p->Cfg0  |= DENC_CFG0_HSYNC_POL;
                if (Device_p->EncodingMode.Option.Pal.SquarePixel)
                 {
                    Conf_p->Idfs2 = IDFS2(Idfs[NTSCM_443_SQ]) ;
                    Conf_p->Idfs1 = IDFS1(Idfs[NTSCM_443_SQ]) ;
                    Conf_p->Idfs0 = IDFS0(Idfs[NTSCM_443_SQ]) ;
                 }

            }
            break;
        case STDENC_MODE_PALN  :
            Conf_p->Cfg0 |= DENC_CFG0_PAL_N;
            Conf_p->Cfg1 |= DENC_CFG1_FLT_16;
            Conf_p->Cfg3 |= DENC_CFG3_PAL_TRFLT;

            if (Device_p->DencVersion >= 12)
            {
                Conf_p->Cfg10 |= DENC_CFG10_AUX_FLT_16;
                 if (Device_p->EncodingMode.Option.Pal.SquarePixel)
                 {
                    Conf_p->Idfs2 = IDFS2(Idfs[PALN_SQ]) ;
                    Conf_p->Idfs1 = IDFS1(Idfs[PALN_SQ]) ;
                    Conf_p->Idfs0 = IDFS0(Idfs[PALN_SQ]) ;
                 }

            }
            break;
        case STDENC_MODE_PALN_C:
            Conf_p->Cfg0 |= DENC_CFG0_PAL_N;
            Conf_p->Cfg1 |= DENC_CFG1_FLT_16;
            Conf_p->Cfg3 &= ~DENC_CFG3_PAL_TRFLT;
             if (Device_p->DencVersion >= 12)
            {
                Conf_p->Cfg10 |= DENC_CFG10_AUX_FLT_16;
                 if (Device_p->EncodingMode.Option.Pal.SquarePixel)
                 {
                    Conf_p->Idfs2 = IDFS2(Idfs[PALN_SQ]) ;
                    Conf_p->Idfs1 = IDFS1(Idfs[PALN_SQ]) ;
                    Conf_p->Idfs0 = IDFS0(Idfs[PALN_SQ]) ;
                 }

            }
            break;

        case STDENC_MODE_PALM :
            Conf_p->Cfg0 |= DENC_CFG0_PAL_M;
            Conf_p->Cfg1 |= DENC_CFG1_FLT_16;
            Conf_p->Cfg3 &= ~DENC_CFG3_PAL_TRFLT;
            if (Device_p->DencVersion >= 12)
            {
                Conf_p->Cfg10 |= DENC_CFG10_AUX_FLT_16;

                 if (Device_p->EncodingMode.Option.Pal.SquarePixel)
                 {
                    Conf_p->Idfs2 = IDFS2(Idfs[NTSCM_443_60Hz_SQ]) ;
                    Conf_p->Idfs1 = IDFS1(Idfs[NTSCM_443_60Hz_SQ]) ;
                    Conf_p->Idfs0 = IDFS0(Idfs[NTSCM_443_60Hz_SQ]) ;
                 }

            }
            break;
        default :
            /* see assumptions */
            break;
    } /* end of switch( Device_p->EncodingMode.Mode) */

} /* end of ConfigPAL() */

/*******************************************************************************
Name        : ConfigNTSC
Description : Configure NTSC modes
Parameters  : IN  : Device_p : hardware target identification
              IN/OUT : Conf_p : configuration values to update
Assumptions : Device_p and Conf_p are not NULL.
 *            Device_p->EncodingMode.Mode is OK.
Limitations :
Returns     : none
*******************************************************************************/
static void ConfigNTSC(DENCDevice_t * const Device_p, Configuration_t * const Conf_p)
{
    /* common NTSC M, M Japan, M 443 */

    Conf_p->Cfg0 |= DENC_CFG0_NTSC_M;
    Conf_p->Cfg1 |= DENC_CFG1_FLT_16;
    Conf_p->Cfg2 &= ~DENC_CFG2_RST_2F;
    Conf_p->Cfg2 |= DENC_CFG2_ENA_BURST;
    UPDATE_REG(Conf_p->Cfg2, DENC_CFG2_NO_INTER,  !Device_p->EncodingMode.Option.Ntsc.Interlaced);
    UPDATE_REG(Conf_p->Cfg3, DENC_CFG3_ENA_TRFLT, Device_p->LumaTrapFilter);

    if (Device_p->DencVersion < 10)
    {
        Conf_p->Cfg3 &= DENC_CFG3_MASK_DELAY;
        Conf_p->Cfg3 |= ChromaDelayV3To9[(Device_p->ChromaDelay-STDENC_MIN_CHROMA_DELAY)/STDENC_STEP_CHROMA_DELAY_V10_LESS];
    }
    else /* Id >= 10 */
    {
        Conf_p->Cfg3 |= DENC_CFG3_DELAY_ENABLE;
        Conf_p->Cfg9 &= DENC_CFG9_MASK_DELAY;

#if defined DENC_MAIN_COMPONSATE_2PIXEL/*DENC_COMPENSATION_ENABLED  ** WA_GNBvd31390*/
        Conf_p->Cfg9  |= DENC_CFG9_DELAY_M2;  /*add -2 pixel delay for the main (supporting only 4:4:4 mode)*/
#elif defined DENC_MAIN_COMPONSATE_1_5PIXEL
        Conf_p->Cfg9  |= DENC_CFG9_DELAY_M1_5;  /*add -2 pixel delay for the main (supporting only 4:4:4 mode)*/
#elif defined DENC_MAIN_COMPONSATE_1PIXEL
        Conf_p->Cfg9  |= DENC_CFG9_DELAY_M1;  /*add -1 pixel delay for the main (supporting only 4:4:4 mode)*/
#elif defined DENC_MAIN_COMPONSATE_0_5PIXEL
        Conf_p->Cfg9  |= DENC_CFG9_DELAY_M0_5;  /*add -0.5 pixel delay for the main (supporting only 4:4:4 mode)*/
#else   /*no composation needed -> set default value*/
        Conf_p->Cfg9 |= ChromaDelayV10More[(Device_p->ChromaDelay-STDENC_MIN_CHROMA_DELAY)/STDENC_STEP_CHROMA_DELAY_V10_MORE];
#endif
    }
    if (Device_p->IsAuxEncoderOn)
    {
        Conf_p->Cfg12 |= DENC_CFG12_AUX_DEL_EN;
        UPDATE_REG(Conf_p->Cfg12, DENC_CFG12_AUX_ENTRAP, Device_p->LumaTrapFilterAux);
        Conf_p->Cfg11 &= DENC_CFG11_AUX_MASK_DEL;

#if defined DENC_AUX_COMPONSATE_2PIXEL/*DENC_COMPENSATION_AUX_ENABLED*/ /*WA_GNBvd31390*/
        Conf_p->Cfg11 |= DENC_CFG11_AUX_DEL_M2;  /*add -2 pixel delay for the aux (supporting only 4:2:2 mode)*/
#elif defined DENC_AUX_COMPONSATE_1_5PIXEL
        Conf_p->Cfg11 |= DENC_CFG11_AUX_DEL_M1_5;  /*add -1.5 pixel delay for the aux (supporting only 4:2:2 mode)*/
#elif defined DENC_AUX_COMPONSATE_1PIXEL
        Conf_p->Cfg11 |= DENC_CFG11_AUX_DEL_M1;  /*add -1 pixel delay for the aux (supporting only 4:2:2 mode)*/
#elif defined DENC_AUX_COMPONSATE_0_5PIXEL
        Conf_p->Cfg11 |= DENC_CFG11_AUX_DEL_M0_5;  /*add -0.5 pixel delay for the aux (supporting only 4:2:2 mode)*/
#else
        Conf_p->Cfg11 |= ChromaDelayAux[(Device_p->ChromaDelay-STDENC_MIN_CHROMA_DELAY)/STDENC_STEP_CHROMA_DELAY_V10_MORE];
#endif
    }


    if (Device_p->DencVersion >= 6)
    {
        Conf_p->Cfg7 &= ~DENC_CFG7_SECAM;
        UPDATE_REG(Conf_p->Cfg7, DENC_CFG7_SQ_PIX, Device_p->EncodingMode.Option.Ntsc.SquarePixel);
    }

    if (Device_p->IsAuxEncoderOn)
    {
        Conf_p->Cfg10 |= DENC_CFG10_AUX_FLT_16;
    }


     /* specific */
    switch( Device_p->EncodingMode.Mode)
    {
        case STDENC_MODE_NTSCM : /* no break */
        case STDENC_MODE_NTSCM_J :
            Conf_p->Cfg3 &= ~DENC_CFG3_PAL_TRFLT;
            if (Device_p->EncodingMode.Option.Ntsc.FieldRate60Hz)
            {
                Conf_p->Cfg2 &= ~DENC_CFG2_ENA_RST;
                if (Device_p->DencVersion <=6)
                {
                    /* IDFS registers need to be programmed */
                    Conf_p->Cfg2 |= DENC_CFG2_SEL_RST;
                    Conf_p->ProgIdfs = TRUE;
                }
                if (Device_p->DencVersion >= 7)
                {
                    /* IDFS registers need to be programmed */
                    Conf_p->Cfg5 |= DENC_CFG5_SEL_INC;
                    Conf_p->ProgIdfs = TRUE;
                }
                if ((Device_p->DencVersion >= 6) && (Device_p->EncodingMode.Option.Ntsc.SquarePixel))
                {
                    Conf_p->Idfs2 = IDFS2(Idfs[NTSCM_60Hz_SQ]) ;
                    Conf_p->Idfs1 = IDFS1(Idfs[NTSCM_60Hz_SQ]) ;
                    Conf_p->Idfs0 = IDFS0(Idfs[NTSCM_60Hz_SQ]) ;
                }
                else /* not square pixel */
                {
                    Conf_p->Idfs2 = IDFS2(Idfs[NTSCM_60Hz]) ;
                    Conf_p->Idfs1 = IDFS1(Idfs[NTSCM_60Hz]) ;
                    Conf_p->Idfs0 = IDFS0(Idfs[NTSCM_60Hz]) ;

                }
            }
            else
            {
                Conf_p->Cfg2 |= DENC_CFG2_ENA_RST;
            }
            break;
        case STDENC_MODE_NTSCM_443 :
            Conf_p->Cfg2 &= ~DENC_CFG2_ENA_RST;
            if (Device_p->DencVersion <=6)
            {
                /* for mode NTSCM_443, IDFS registers always need to be programmed */
                Conf_p->Cfg2 |= DENC_CFG2_SEL_RST;
                Conf_p->ProgIdfs = TRUE;
            }
            Conf_p->Cfg3 |= DENC_CFG3_PAL_TRFLT;
            if (Device_p->DencVersion >= 7)
            {
                /* for mode NTSCM_443, IDFS registers always need to be programmed */
                Conf_p->Cfg5 |= DENC_CFG5_SEL_INC;
                Conf_p->ProgIdfs = TRUE;
            }
            /* REG 10 11 12 */
            if ((Device_p->DencVersion >= 6) && (Device_p->EncodingMode.Option.Ntsc.SquarePixel))
            {
                if (Device_p->EncodingMode.Option.Ntsc.FieldRate60Hz)
                {
                    Conf_p->Idfs2 = IDFS2(Idfs[NTSCM_443_60Hz_SQ]) ;
                    Conf_p->Idfs1 = IDFS1(Idfs[NTSCM_443_60Hz_SQ]) ;
                    Conf_p->Idfs0 = IDFS0(Idfs[NTSCM_443_60Hz_SQ]) ;
                }
                else
                {
                    Conf_p->Idfs2 = IDFS2(Idfs[NTSCM_443_SQ]) ;
                    Conf_p->Idfs1 = IDFS1(Idfs[NTSCM_443_SQ]) ;
                    Conf_p->Idfs0 = IDFS0(Idfs[NTSCM_443_SQ]) ;
                }
            }
            else /* not square pixel */
            {
                if (Device_p->EncodingMode.Option.Ntsc.FieldRate60Hz)
                {
                    Conf_p->Idfs2 = IDFS2(Idfs[NTSCM_443_60Hz]) ;
                    Conf_p->Idfs1 = IDFS1(Idfs[NTSCM_443_60Hz]) ;
                    Conf_p->Idfs0 = IDFS0(Idfs[NTSCM_443_60Hz]) ;
                }
                else
                {
                    Conf_p->Idfs2 = IDFS2(Idfs[NTSCM_443]) ;
                    Conf_p->Idfs1 = IDFS1(Idfs[NTSCM_443]) ;
                    Conf_p->Idfs0 = IDFS0(Idfs[NTSCM_443]) ;
                }


            }
            break;
        default :
            /* see assumptions */
            break;
    } /* end of switch( Device_p->EncodingMode.Mode) */

} /* end of ConfigNTSC() */


/*******************************************************************************
Name        : ConfigSECAM
Description : Configure SECAM modes
Parameters  : IN  : Device_p : hardware target identification
              IN/OUT : Conf_p : configuration values to update
Assumptions : Device_p and Conf_p are not NULL.
 *            Device_p->EncodingMode.Mode is OK.
 *            Device_p->DencVersion >= 6
Limitations :
Returns     : none
*******************************************************************************/
static void ConfigSECAM(DENCDevice_t * const Device_p, Configuration_t * const Conf_p)
{
     /* specific */
    Conf_p->Cfg7 |= DENC_CFG7_SECAM;
    Conf_p->Cfg1 |= DENC_CFG1_FLT_19;
    Conf_p->Cfg2 &= ~(DENC_CFG2_SEL_RST | DENC_CFG2_NO_INTER);
    Conf_p->Cfg2 |= (DENC_CFG2_ENA_BURST | DENC_CFG2_RST_2F);
    Conf_p->Cfg3 |= DENC_CFG3_ENA_TRFLT | DENC_CFG3_PAL_TRFLT;
    UPDATE_REG(Conf_p->Cfg7, DENC_CFG7_SQ_PIX, Device_p->SECAMSquarePixel);

    if (Device_p->DencVersion < 10)
    {
        Conf_p->Cfg3 &= DENC_CFG3_MASK_DELAY;
        Conf_p->Cfg3 |= ChromaDelayV3To9[(Device_p->ChromaDelay-STDENC_MIN_CHROMA_DELAY)/STDENC_STEP_CHROMA_DELAY_V10_LESS];
    }
    else /* Id >= 10 */
    {
        Conf_p->Cfg3 |= DENC_CFG3_DELAY_ENABLE;
        Conf_p->Cfg9 &= DENC_CFG9_MASK_DELAY;
        Conf_p->Cfg9 |= ChromaDelayV10More[(Device_p->ChromaDelay-STDENC_MIN_CHROMA_DELAY)/STDENC_STEP_CHROMA_DELAY_V10_MORE];
    }
    switch( Device_p->EncodingMode.Mode)
    {
        case STDENC_MODE_SECAM:
            if (Device_p->IsAuxEncoderOn)
            {
               Conf_p->Cfg10 &= ~DENC_CFG10_SECAM_IN;
            }
            break;
        case STDENC_MODE_SECAM_AUX:
            Conf_p->Cfg10 |= DENC_CFG10_SECAM_IN;
            Conf_p->Cfg10 |= DENC_CFG10_AUX_FLT_19;
            Conf_p->Cfg12 |= (DENC_CFG12_AUX_DEL_EN | DENC_CFG12_AUX_ENTRAP);
            Conf_p->Cfg11 &= DENC_CFG11_AUX_MASK_DEL;
            Conf_p->Cfg11 |= ChromaDelayAux[(Device_p->ChromaDelay-STDENC_MIN_CHROMA_DELAY)/STDENC_STEP_CHROMA_DELAY_V10_MORE];
            break;
        default :
            /* see assumptions */
            break;
    } /* end of switch( Device_p->EncodingMode.Mode) */

} /* end of ConfigSECAM() */


/* Public functions ---------------------------------------------------------- */

#ifdef STDENC_I2C
/*******************************************************************************
Name        : stdenc_HALInitI2CConnection
Description : Open a connection to I2C driver
Parameters  : IN  : Device_p : hardware target identification
              IN : I2CAccess_p : containing I2C access needed information
Assumptions : Device_p and I2CAccess_p are not NULL.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t stdenc_HALInitI2CConnection(  DENCDevice_t * const Device_p
                                         , const STDENC_I2CAccess_t * const I2CAccess_p
                                        )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STI2C_Handle_t I2CHandle;

    ErrorCode = STI2C_Open( I2CAccess_p->DeviceName, &(I2CAccess_p->OpenParams), &I2CHandle );
    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = STDENC_ERROR_I2C;
    }
    else
    {
        Device_p->I2CHandle = I2CHandle;
    }
    return (ErrorCode);
}

/*******************************************************************************
Name        : stdenc_HALTermI2CConnection
Description : Close connection to I2C driver
Parameters  : IN  : Device_p : hardware target identification
Assumptions : parameter is OK
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t stdenc_HALTermI2CConnection( DENCDevice_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode;

    ErrorCode = STI2C_Close( Device_p->I2CHandle );
    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = STDENC_ERROR_I2C;
    }
    return (ErrorCode);
}
#endif /* #ifdef STDENC_I2C */

/*******************************************************************************
Name        : stdenc_HALSetOn
Description : Set DENC on on 7015/7020 chip
Parameters  : IN  : Device_p : hardware target identification
Assumptions : parameter is not NULL. Function called only for STDENC_DEVICE_TYPE_7015/20
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t stdenc_HALSetOn(DENCDevice_t * const Device_p)
{
    U8 Cfg;
    ST_ErrorCode_t ErrorCode;

    HALDENC_RegAccessLock(Device_p);
    ErrorCode = HALDENC_ReadReg8( Device_p, DENC_CFG_7015, &Cfg);
    if (ErrorCode == ST_NO_ERROR)
    {
        Cfg |= DENC_CFG_7015_ON;
        ErrorCode = HALDENC_WriteReg8( Device_p, DENC_CFG_7015, Cfg);
    }
    HALDENC_RegAccessUnlock(Device_p);
    return (ErrorCode);
} /* End of stdenc_HALSetOn() function */

/*******************************************************************************
Name        : stdenc_HALSetOff
Description : Set DENC off on 7015 chip
Parameters  : IN  : Device_p : hardware target identification
Assumptions : parameter is not NULL. Function called only for STDENC_DEVICE_TYPE_7015/20
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t stdenc_HALSetOff(DENCDevice_t * const Device_p)
{
    U8 Cfg;
    ST_ErrorCode_t ErrorCode;

    HALDENC_RegAccessLock(Device_p);
    ErrorCode = HALDENC_ReadReg8( Device_p, DENC_CFG_7015, &Cfg);
    if (ErrorCode == ST_NO_ERROR)
    {
        Cfg &= ~DENC_CFG_7015_ON;
        ErrorCode = HALDENC_WriteReg8( Device_p, DENC_CFG_7015, Cfg);
    }
    HALDENC_RegAccessUnlock(Device_p);
    return (ErrorCode);
} /* End of stdenc_HALSetOff() function */


/*******************************************************************************
Name        : stdenc_HALVersion
Description : get DENC version
Parameters  : IN  : Device_p : hardware target identification
Assumptions : parameter is not NULL
Limitations : V3 & V5->V12 DENC cell versions
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t stdenc_HALVersion(DENCDevice_t * const Device_p)
{
    ST_ErrorCode_t Ec = ST_NO_ERROR;
 #if defined(ST_7109)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)||defined(ST_5162)
    Device_p->DencVersion = (HALDENC_Version_t)12;
 #else
    U8 IdCell, Iscid;

    HALDENC_RegAccessLock(Device_p);
    Ec = HALDENC_ReadReg8(  Device_p, DENC_ISCID, &IdCell);
    if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_ISCID, ~IdCell);
    if (OK(Ec)) Ec = HALDENC_ReadReg8(  Device_p, DENC_ISCID, &Iscid);
    if (OK(Ec) && (IdCell != Iscid))    /* Version >=6, else if (IdCell == Iscid) => Version 3 or 5 */
    {
        Ec = HALDENC_WriteReg8( Device_p, DENC_ISCID, IdCell);
        if (OK(Ec)) Ec = HALDENC_ReadReg8(  Device_p, DENC_CID,  &IdCell);
    }
    HALDENC_RegAccessUnlock(Device_p);

    if (IdCell == 119) /* special case of Stv0119 chip */
    {
        Device_p->DencVersion = (HALDENC_Version_t)3;
    }
    else
    {
        Device_p->DencVersion = (HALDENC_Version_t)((IdCell & 0xF0) >> 4);
    }
 #endif
    return (Ec);
} /* end of stdenc_HALVersion */


/*******************************************************************************
Name        : stdenc_HALSetEncodingMode
Description : Set encoding mode
Parameters  : IN  : Device_p : hardware target identification
Assumptions : all parameters should be OK.
Limitations : V3 & V5->V12 DENC cell versions
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t stdenc_HALSetEncodingMode(DENCDevice_t * const Device_p)
{
    Configuration_t Conf;
    HALDENC_Version_t Id;
    ST_ErrorCode_t Ec = ST_NO_ERROR;
    HALDENC_RegAccessLock(Device_p);
    Id = Device_p->DencVersion;

    /* read configuration registers */
    Ec = HALDENC_ReadReg8( Device_p, DENC_CFG0, &Conf.Cfg0);

    if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_CFG1, &Conf.Cfg1);
    if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_CFG2, &Conf.Cfg2);
    if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_CFG3, &Conf.Cfg3);
    if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_CFG5, &Conf.Cfg5);
    if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_CFG6, &Conf.Cfg6);
	if (OK(Ec) && (Id >= 6))
    {
        Ec = HALDENC_ReadReg8( Device_p, DENC_CFG7, &Conf.Cfg7);
    }
    if (OK(Ec) && (Id >= 12))
    {
        Ec = HALDENC_ReadReg8( Device_p, DENC_CFG10, &Conf.Cfg10);
        if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_CFG11, &Conf.Cfg11);
        if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_CFG12, &Conf.Cfg12);
#if 1/*#ifdef DENC_MAIN_COMPENSATION_ENABLED*//*WA_GNBvd31390*//*yxl 2007-09-06 modify*/
        if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_CFG9, &Conf.Cfg9);
#endif
    }
    if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_IDFS2, &Conf.Idfs2);
    if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_IDFS1, &Conf.Idfs1);
    if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_IDFS0, &Conf.Idfs0);

    /* If reading register sequence failed leave now */
    if (!OK(Ec))
    {
        HALDENC_RegAccessUnlock(Device_p);
        return (Ec);
    }

    /* Common masking for all encoding modes */
    Conf.Cfg0 &= DENC_CFG0_MASK_STD;
    Conf.Cfg1 &= DENC_CFG1_MASK_FLT;
    UPDATE_REG(Conf.Cfg1, DENC_CFG1_SETUP, Device_p->BlackLevelPedestal);
    if ( (Id >= 6) && (Id <= 11))
    {
        UPDATE_REG(Conf.Cfg2, DENC_CFG2_SEL_444, Device_p->YCbCr444Input);
        UPDATE_REG(Conf.Cfg7, DENC_CFG7_SETUP_YUV, Device_p->BlackLevelPedestal);
    }
    if (Id >= 7)
    {
        Conf.Cfg5 &= ~DENC_CFG5_SEL_INC; /* default: use IDFS hard-wired values */
    }
    if (Id >= 12)
    {
        Conf.Cfg10 &= DENC_CFG10_AUX_MSK_FLT;
        UPDATE_REG(Conf.Cfg7, DENC_CFG7_SETUP_AUX, Device_p->BlackLevelPedestalAux);
    }
    Conf.ProgIdfs = FALSE;

    /***** intialisation / encoding mode *****/
    /*  cases PAL BDGHI, PAL M, PAL N, PAL Nc, NTSCM, NTSCM_J, NTSCM_443, SECAM.
        case 'NONE' for initialisation */
    switch( Device_p->EncodingMode.Mode)
    {
        case STDENC_MODE_PALBDGHI : /* no break */
        case STDENC_MODE_PALN :     /* no break */
        case STDENC_MODE_PALN_C :   /* no break */
        case STDENC_MODE_PALM :
            ConfigPAL(Device_p, &Conf);
            break;
        case STDENC_MODE_NTSCM :    /* no break */
        case STDENC_MODE_NTSCM_J :  /* no break */
        case STDENC_MODE_NTSCM_443 :
            ConfigNTSC(Device_p, &Conf);
            break;
        case STDENC_MODE_SECAM : /* so Id >=6, checked before */
        case STDENC_MODE_SECAM_AUX:
            ConfigSECAM(Device_p, &Conf);
            break;
        case STDENC_MODE_NONE :
            Conf.Cfg0 = DENC_CFG0_RESET_VALUE & ~DENC_CFG0_ODD_POL;   /* V falling edge active */

            if ( (Device_p->DeviceType == STDENC_DEVICE_TYPE_7015) || (Device_p->DeviceType == STDENC_DEVICE_TYPE_7020) )
            {
                Conf.Cfg0 |= DENC_CFG0_HSYNC_POL; /* H rising edge active */
            }
#if defined(ST_5528)|| defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)\
 || defined(ST_7109)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)
            else if ((Device_p->DeviceType == STDENC_DEVICE_TYPE_4629)
                        || (Device_p->DeviceType == STDENC_DEVICE_TYPE_V13))
            {
                Conf.Cfg0 |= DENC_CFG0_HSYNC_POL;
                Conf.Cfg0 |= DENC_CFG0_ODD_POL;
            }
            else if ((Device_p->DeviceType == STDENC_DEVICE_TYPE_DENC))

            {
               Conf.Cfg0 |= DENC_CFG0_HSYNC_POL;
            }
#endif
            Conf.Cfg1 = DENC_CFG1_RESET_VALUE;
            Conf.Cfg2 = DENC_CFG2_RESET_VALUE;
            Conf.Cfg3 = DENC_CFG3_RESET_VALUE;
            Conf.Cfg5 = DENC_CFG5_RESET_VALUE;
            Conf.Cfg6 = DENC_CFG6_RESET_VALUE;
            if (Id >= 6)
            {
                 Conf.Cfg7 = DENC_CFG7_RESET_VALUE;
            }

            /* REG_3 */
            if (Device_p->IsExternal)
            {
                Conf.Cfg3 |= DENC_CFG3_VAL_422_CK_MUX;
                Ec = HALDENC_WriteReg8( Device_p, DENC_CFG3, Conf.Cfg3);
            }

            /* REG_4 */
#if defined(ST_5528)|| defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)\
 || defined(ST_7109)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)
            if ((Device_p->DeviceType == STDENC_DEVICE_TYPE_4629)||(Device_p->DeviceType == STDENC_DEVICE_TYPE_DENC)||(Device_p->DeviceType == STDENC_DEVICE_TYPE_V13))
            {
                Conf.Cfg4 = (DENC_CFG4_RESET_VALUE & DENC_CFG4_MASK_SYOUT) | (DENC_CFG4_SYIN_0 | DENC_CFG4_SYOUT_0 | DENC_CFG4_ALINE);
            }
#else
            if ((Device_p->DeviceType == STDENC_DEVICE_TYPE_7015) || (Device_p->DeviceType == STDENC_DEVICE_TYPE_7020) )
            {
                    Conf.Cfg4 = (DENC_CFG4_RESET_VALUE & DENC_CFG4_MASK_SYOUT) | (DENC_CFG4_SYIN_P1 | DENC_CFG4_SYOUT_0 | DENC_CFG4_ALINE);
            }
            else
            {
                    Conf.Cfg4 = (DENC_CFG4_RESET_VALUE & DENC_CFG4_MASK_SYOUT) | (DENC_CFG4_SYIN_P1 | DENC_CFG4_SYOUT_P1 | DENC_CFG4_ALINE);
            }
#endif

            if (OK(Ec))
            {
                Ec = HALDENC_WriteReg8( Device_p, DENC_CFG4, Conf.Cfg4);
            }

            /* REG_8 */
            if (OK(Ec) && (Id >= 7))
            {
                Ec = HALDENC_WriteReg8( Device_p, DENC_CFG8, DENC_CFG8_RESET_VALUE);
                if (OK(Ec) && Device_p->IsExternal)
                    Ec = HALDENC_WriteReg8( Device_p, DENC_CFG8, DENC_CFG8_RESET_VALUE|DENC_CFG8_VAL_422_MUX);
            }
            /* REG_9 */
            if (OK(Ec) && (Id >= 10))
            {
                Conf.Cfg9 = DENC_CFG9_RESET_VALUE;
                if ( (Device_p->DeviceType == STDENC_DEVICE_TYPE_GX1) || (Device_p->DeviceType == STDENC_DEVICE_TYPE_7020))
                {
                    Conf.Cfg9 |= DENC_CFG9_444_CVBS;
                }
                if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG9, Conf.Cfg9);
            }
            /* REG_10, 11, 12, 13 */
            if (OK(Ec) && (Id >= 12))
            {
                Conf.Cfg10 = DENC_CFG10_RESET_VALUE;
                Conf.Cfg11 = DENC_CFG11_RESET_VALUE;
                Conf.Cfg12 = DENC_CFG12_RESET_VALUE;
                #if defined(ST_7710)||defined(ST_7100)||defined(ST_7109)||defined(ST_5188)||defined(ST_5525)||defined(ST_5107)||defined(ST_7200)||defined(ST_5162)
                   Ec = HALDENC_WriteReg8( Device_p, DENC_CFG13, DENC_CFG13_CVBS_MAIN);
                #else
                   Ec = HALDENC_WriteReg8( Device_p, DENC_CFG13, DENC_CFG13_RESET_VALUE);
                #endif

            }
#if 1 /*yxl 2007-09-06 add*/
		Conf.Cfg6 |= DENC_CFG6_RST_SOFT;
	/*	Conf.Cfg5 |= DENC_CFG5_SEL_INC;
		Conf.ProgIdfs = TRUE;
              Conf.Idfs2 = IDFS2(Idfs[NTSCM_443]) ;
              Conf.Idfs1 = IDFS1(Idfs[NTSCM_443]) ;
              Conf.Idfs0 = IDFS0(Idfs[NTSCM_443]) ;*/
#endif		
            break;
        default :
            /* should not be possible, checked before */
            STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "HALDENC_SetEncodingMode : should never display this !"));
            Ec = ST_ERROR_BAD_PARAMETER;
            break;
    }

    /* Setting common values for all encoding modes */

    /* REG_6 */
#if 0 /*yxl 2007-09-06 del*/
    Conf.Cfg6 |= DENC_CFG6_RST_SOFT;
#endif
    if (OK(Ec) && (Id >= 12))
    {
        Ec = HALDENC_WriteReg8( Device_p, DENC_CFG6, Conf.Cfg6);
    }
    /* write configuration registers */
    if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG0, Conf.Cfg0);
    if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG1, Conf.Cfg1);
    if (OK(Ec) && (Id >= 6))
    {
            Ec = HALDENC_WriteReg8( Device_p, DENC_CFG7, Conf.Cfg7);
    }
    if (OK(Ec) && (Id >= 12))
    {
        Ec = HALDENC_WriteReg8( Device_p, DENC_CFG10, Conf.Cfg10);
        if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG11, Conf.Cfg11);
        if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG12, Conf.Cfg12);
#if 1/*#ifdef DENC_MAIN_COMPENSATION_ENABLED*//*WA_GNBvd31390*//*yxl 2007-09-06 modify*/
        /*write the new values of DENC_CFG3 and DENC_CFG9*/
        if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG3, Conf.Cfg3);
        if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG9, Conf.Cfg9);
#endif
    }
    if (Conf.ProgIdfs) /* write IDFS values only if needed */
    {
        if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_IDFS2, Conf.Idfs2);
        if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_IDFS1, Conf.Idfs1);
        if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_IDFS0, Conf.Idfs0);
    }
    /* Cfg2: must be written after IDFS and before Cfg6 (reset) if v<=6 */
    if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG2, Conf.Cfg2);
    /* Cfg5: must be written after IDFS and before Cfg6 (reset) if v>=7 */
    if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG5, Conf.Cfg5);
    /* Cfg6: reset after 'bit SECAM' selected in CFG7, and selrst (cfg2) or selrst_inc (cfg5) */
    if (Device_p->DeviceType == STDENC_DEVICE_TYPE_V13)
    {
       Ec = HALDENC_WriteReg8( Device_p, DENC_CFG6, Conf.Cfg6);
    }
    if (OK(Ec)&& (Id < 12))
    {
        Ec = HALDENC_WriteReg8( Device_p, DENC_CFG6, Conf.Cfg6);
    }

    HALDENC_RegAccessUnlock(Device_p);
    if (OK(Ec))
    {
        /* keep good value for restoring config after a colorbar pattern */
        Device_p->RegCfg0 = Conf.Cfg0;
    }
    return (Ec);
} /* end of stdenc_HALSetEncodingMode */


/*******************************************************************************
Name        : stdenc_HALSetColorBarPattern
Description : Autotest = Color Bar Pattern
Parameters  : IN  : Device_p : hardware target identification
Assumptions : all parameters should be OK.
Limitations : V3 & V5->V12 DENC cell versions
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t stdenc_HALSetColorBarPattern( DENCDevice_t * const Device_p)
{
    U8             Cfg0,Cfg4,Cfg6;
    ST_ErrorCode_t Ec;

    HALDENC_RegAccessLock(Device_p);
    Ec = HALDENC_ReadReg8( Device_p, DENC_CFG0, &Cfg0);
    /* strange behaviour with colorbar pattern on v>=10 / SECAM : DENC_CFG4_ALINE must be off
     * DDTS GNBvd07052 */
    if ((OK(Ec)) && ((Cfg0 & DENC_CFG0_COL_BAR) != DENC_CFG0_COL_BAR))
    {
        if (    (Device_p->DencVersion >= 10)
             && (    (Device_p->EncodingMode.Mode == STDENC_MODE_SECAM)
                  || (Device_p->EncodingMode.Mode == STDENC_MODE_SECAM_AUX)))
        {
            Ec = HALDENC_ReadReg8( Device_p, DENC_CFG4, &Cfg4);
            if (OK(Ec))
            {
                Cfg4 &= (U8)~DENC_CFG4_ALINE;
                Ec = HALDENC_WriteReg8( Device_p, DENC_CFG4, Cfg4);
            }
        }
        if (OK(Ec))
        {
            /* save DENC_CFG0 value before modification, if colorbar not already set */
            Device_p->RegCfg0 = Cfg0;
            Cfg0 |= DENC_CFG0_COL_BAR;
            Ec = HALDENC_ReadReg8( Device_p, DENC_CFG6, &Cfg6);
            Cfg6 |= DENC_CFG6_RST_SOFT;
            if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG0, Cfg0);
            if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG6, Cfg6);
        }
    }
	HALDENC_RegAccessUnlock(Device_p);

    return (Ec);
} /* end of stdenc_HALSetColorBarPattern */


/*******************************************************************************
Name        : stdenc_HALRestoreConfiguration0
Description : Restore configuration
Parameters  : IN  : Device_p : hardware target identification
Assumptions : all parameters should be OK.
Limitations : V3 & V5->V12 DENC cell versions
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t stdenc_HALRestoreConfiguration0( DENCDevice_t * const Device_p)
{
    U8             Cfg4,Cfg6;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    HALDENC_RegAccessLock(Device_p);
    /* strange behaviour with colorbar pattern on v>=10 / SECAM : DENC_CFG4_ALINE must be off
     * so restore it now. DDTS GNBvd07052 */
        if (    (Device_p->DencVersion >= 10)
             && (    (Device_p->EncodingMode.Mode == STDENC_MODE_SECAM)
                  || (Device_p->EncodingMode.Mode == STDENC_MODE_SECAM_AUX)))
    {
        Ec = HALDENC_ReadReg8( Device_p, DENC_CFG4, &Cfg4);
        if (OK(Ec))
        {
            Cfg4 |= DENC_CFG4_ALINE;
            Ec = HALDENC_WriteReg8( Device_p, DENC_CFG4, Cfg4);
        }
    }
    /* restore saved value of DENC_CFG0 in HALDENC_SetColorBarPattern */
    if (OK(Ec)) Ec = HALDENC_ReadReg8( Device_p, DENC_CFG6, &Cfg6);
    if (OK(Ec))
    {
        Cfg6 |= DENC_CFG6_RST_SOFT;
        Ec = HALDENC_WriteReg8( Device_p, DENC_CFG0, Device_p->RegCfg0);
    }
    if (OK(Ec)) Ec = HALDENC_WriteReg8( Device_p, DENC_CFG6, Cfg6);
    HALDENC_RegAccessUnlock(Device_p);
    return (Ec);
} /* end of stdenc_HALRestoreConfiguration0 */

/*******************************************************************************
Name        : stdenc_HALSetDencId
Description : Set DENC Id (MAIN or AUX denc)
Parameters  : IN  : Device_p : hardware target identification
Assumptions : all parameters should be OK.
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stdenc_HALSetDencId( DENCDevice_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    #if defined (ST_7200)
        U32 offset = 0xffff;
    #endif

    #if defined (ST_5525)
           if((U32)(Device_p->BaseAddress_p)== DENC_0_BASE_ADDRESS)
           {
                    Device_p->DencId = STDENC_DENC_ID_1;
           }
           else if((U32)(Device_p->BaseAddress_p)== DENC_1_BASE_ADDRESS)
           {
                   Device_p->DencId = STDENC_DENC_ID_2;
           }
           else
           {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
           }

    #elif defined (ST_7200)
            offset &= (U32)(Device_p->BaseAddress_p);
            if(offset == 0xC000)
           {
                    Device_p->DencId = STDENC_DENC_ID_1;
           }
           else if(offset == 0xF000)
           {
                   Device_p->DencId = STDENC_DENC_ID_2;
           }
           else
           {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
           }
    #else
       Device_p->DencId = STDENC_DENC_ID_1;
    #endif

   return (ErrorCode);

}

/* ----------------------------- End of file ------------------------------ */
