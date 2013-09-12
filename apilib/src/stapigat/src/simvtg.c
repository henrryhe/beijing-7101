/*******************************************************************************

File name   : simvtg.c

Description : STVTG driver simulator source file.
 * This file is intended to free test application from STVTG driver to prevent from
 * circular dependencies.
 * VTG features are basic.

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
21 Feb 2002        Created                                           HS
14 Oct 2002        Update for 7020 cut2                              HS
30 Oct 2002        New synthesiser parameters for STi7020 cut 2.0,   HS
 *                 2 * BOTTOM_BLANKING_SIZE, GENERIC_ADDRESSES.
12 May 2003        Add register command for init from testtool       HS
14 May 2003        Update for 5528                                   HS
21 Jan 2004        Update for espresso                               AC
31 Mar 2004        Update for STi5100                                MH+TA
10 Sep 2004        Update for ST40/OS21                              MH
03 dec 2004        Port to 5105 + 5301 + 8010, supposing same as 5100 TA+HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

#if defined (ST_7020_CUT1) && !defined (VTGSIM_EXTERNAL_DENC)
#define USE_ICS9161 /* needed for STi7020 cut 1 */
#endif


/* Includes ----------------------------------------------------------------- */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "testcfg.h"

#ifdef USE_VTGSIM
#include "stddefs.h"
#include "stdevice.h"
#include <math.h>

#include "stsys.h"
#include "stdenc.h"

#ifdef ST_OSLINUX
#include "compat.h"
#include "iocstapigat.h"
/*#else*/
#endif

#include "sttbx.h"
#include "testtool.h"
#include "simvtg.h"

/* work-arounds 7015 */
#if defined (ST_7015)
#define WA_GNBvd06791 /* following rule must be respected : DISP_CLK <= 3 x DENC_CLK and 3 x PIX_CLK */
#endif
/* 7020 : WA_GNBvd06791 : ADISP_CLK is set to 72MHz so always < 3*AUX_CLK for PAL/NTSC with square pixel variants */
/* Private Types ------------------------------------------------------------ */

typedef struct TimingModeParams_s
{
    U32 PixelsPerLine;
    U32 LinesByFrame;
    U32 PixelClock;
    U32 HSyncPulseWidth;
    U32 VSyncPulseWidth;

} TimingModeParams_t;


#if defined (ST_7015) || defined(ST_7020)
typedef enum Clock_e
{
    SYNTH3,
#ifdef WA_GNBvd06791
    SYNTH6,
#endif
    SYNTH7
} Clock_t;
#endif  /*#if ST_7015 || ST_7020 */

#if defined (ST_7710)|| defined(ST_7100)|| defined (ST_7109)
typedef enum Clock_e
{
    FSY1_1,                             /* SD pixel clock */
    FSY0_4                              /* HD pixel clock */
} Clock_t;
#endif  /*#if ST_7710 || ST_7100 || ST_7109*/
#if defined (ST_7200)
static U32 refclockB =30;
static CkgB_ClockInstance_t CkgB_Clocks[]=
{
 { ""                        ,CLK_NONE             , 0                , {0,0}                            , 0 , 0 },
 { "FS0_CK1"                 ,FS0_CK1              , 0                , {0,0}                            , 0 , 0 },
 { "FS0_CK2"                 ,FS0_CK2              , 0                , {0,0}                            , 0 , 0 },
 { "FS0_CK3"                 ,FS0_CK3              , 0                , {0,0}                            , 0 , 0 },
 { "FS0_CK4"                 ,FS0_CK4              , 0                , {0,0}                            , 0 , 0 },
 { "FS1_CK1"                 ,FS1_CK1              , 0                , {0,0}                            , 0 , 0 },
 { "FS1_CK2"                 ,FS1_CK2              , 0                , {0,0}                            , 0 , 0 },
 { "FS1_CK3"                 ,FS1_CK3              , 0                , {0,0}                            , 0 , 0 },
 { "FS1_CK4"                 ,FS1_CK4              , 0                , {0,0}                            , 0 , 0 },
 { "FS2_CK1"                 ,FS2_CK1              , 0                , {0,0}                            , 0 , 0 },
 { "FS2_CK2"                 ,FS2_CK2              , 0                , {0,0}                            , 0 , 0 },
 { "FS2_CK3"                 ,FS2_CK3              , 0                , {0,0}                            , 0 , 0 },
 { "FS2_CK4"                 ,FS2_CK4              , 0                , {0,0}                            , 0 , 0 },
 { "CLK_HDMI_PLL"            ,CLK_HDMI_PLL         , FS0_CK1          , {0,0}/*{FS0_CK1,SYSCLKINB}*/     , 0 , 0 },
 { "CLK_PIX_HD0"             ,CLK_PIX_HD0          , FS0_CK1          , {0,0}/*{FS0_CK1,SYSCLKINB}*/     , 0 , 0 },
 { "CLK_PIX_HD1"             ,CLK_PIX_HD1          , FS0_CK1          , {0,0}/*{FS0_CK2,SYSCLKINB}*/     , 0 , 0 },
 { "CLK_PIPE"                ,CLK_PIPE             , FS0_CK1          , {0,0}/*{FS0_CK3,SYSCLKINB}*/     , 0 , 0 },
 { "CLK_PIX_SD0"             ,CLK_PIX_SD0          , FS0_CK1          , {0,0}                            , 8 , 0 },
 { "CLK_PIX_SD1"             ,CLK_PIX_SD1          , FS0_CK2          , {0,0}                            , 8 , 0 },
 /* intermediate clocks */
 { "CLK_PIX_OBELIX"         ,CLK_PIX_OBELIX        , CLK_PIX_HD0      , {CLK_PIX_HD0,CLK_PIX_SD0}        , 0 , 0 },
 { "CLK_PIX_ASTERIX"        ,CLK_PIX_ASTERIX       , CLK_PIX_HD0      , {CLK_PIX_HD0,CLK_PIX_SD0}        , 0 , 0 },
 /* TVOUT Inputs */
 { "CLK_LOC_HDTVO"          ,CLK_LOC_HDTVO         , CLK_PIX_HD0      , {0,0}                            , 1 , 0 },
 { "CLK_LOC_SDTVO"          ,CLK_LOC_SDTVO         , CLK_PIX_HD0      , {CLK_PIX_HD0,CLK_PIX_SD0}        , 0 , 0 },
 { "CLK_REM_SDTVO"          ,CLK_REM_SDTVO         , CLK_PIX_SD0      , {CLK_PIX_SD0,CLK_PIX_SD1}        , 0 , 0 },
 { "CLK_OBELIX_FVP"         ,CLK_OBELIX_FVP        , CLK_PIX_HD0      , {0,0}                            , 1 , 1 },
 { "CLK_OBELIX_MAIN"        ,CLK_OBELIX_MAIN       , CLK_PIX_HD0      , {0,0}                            , 1 , 1 },
 { "CLK_OBELIX_AUX"         ,CLK_OBELIX_AUX        , CLK_PIX_OBELIX   , {0,0}                            , 1 , 1 },
 { "CLK_OBELIX_GDP"         ,CLK_OBELIX_GDP        , CLK_OBELIX_MAIN  , {CLK_OBELIX_MAIN,CLK_OBELIX_AUX} , 0 , 0 },
 { "CLK_OBELIX_VDP"         ,CLK_OBELIX_VDP        , CLK_OBELIX_MAIN  , {CLK_OBELIX_MAIN,CLK_OBELIX_AUX} , 0 , 0 },
 { "CLK_OBELIX_CAP"         ,CLK_OBELIX_CAP        , CLK_OBELIX_MAIN  , {CLK_OBELIX_MAIN,CLK_OBELIX_AUX} , 0 , 0 },
 { "CLK_ASTERIX_AUX"        ,CLK_ASTERIX_AUX       , CLK_PIX_ASTERIX  , {0,0}                            , 1 , 1 },
 { "CLK_TMDS"               ,CLK_TMDS              , CLK_PIX_HD0      , {0,0}                            , 1 , 1 },
 { "CLK_CH34MOD"            ,CLK_CH34MOD           , CLK_PIX_OBELIX   , {0,0}                            , 1 , 1 },
 /* constant clocks*/
 { "CLK_USB48"              ,CLK_USB48             , FS0_CK4          , {0,0}/*{FS0_CK4,SYSCLKINB}*/    , 0 , 0 },
 { "CLK_216"                ,CLK_216               , FS1_CK1          , {0,0}/*{FS1_CK1,FS1_CK2  }*/    , 0 , 0 },
 { "CLK_FRC2"               ,CLK_FRC2              , FS1_CK3          , {0,0}/*{FS1_CK3,SYSCLKINB}*/    , 0 , 0 },
 { "CLK_IC177"              ,CLK_IC177             , FS1_CK4          , {0,0}/*{FS1_CK4,SYSCLKINB}*/    , 0 , 0 },
 { "CLK_SPARE"              ,CLK_SPARE             , FS2_CK1          , {0,0}/*{FS2_CK1,SYSCLKINB}*/    , 0 , 0 },
 { "CLK_DSS"                ,CLK_DSS               , FS2_CK2          , {0,0}/*{FS2_CK2,SYSCLKINB}*/    , 0 , 0 },
 { "CLK_IC166"              ,CLK_IC166             , FS2_CK3          , {0,0}/*{FS2_CK3,SYSCLKINB}*/    , 0 , 0 },
 { "CLK_RTC"                ,CLK_RTC               , FS2_CK4          , {0,0}/*{FS2_CK4,SYSCLKINB}*/    , 512 , 0 },
 { "CLK_LPC"                ,CLK_LPC               , FS2_CK4          , {0,0}/*{FS2_CK4,SYSCLKINB}*/    , 512 , 0 },
	/* HDTVO output clocks */
 { "CLK_HDTVO_PIX_MAIN"     ,CLK_HDTVO_PIX_MAIN    , CLK_LOC_HDTVO    , {0,0}             						  , 1 , 1 },
 { "CLK_HDTVO_TMDS_MAIN"    ,CLK_HDTVO_TMDS_MAIN   , CLK_LOC_HDTVO    , {0,0}             						  , 1 , 1 },
 { "CLK_HDTVO_BCH_MAIN"     ,CLK_HDTVO_BCH_MAIN    , CLK_LOC_HDTVO    , {0,0}             						  , 1 , 1 },
 { "CLK_HDTVO_FDVO_MAIN"    ,CLK_HDTVO_FDVO_MAIN   , CLK_LOC_HDTVO    , {0,0}             						  , 1 , 1 },
 { "CLK_HDTVO_PIX_AUX"    	,CLK_HDTVO_PIX_AUX     , CLK_LOC_SDTVO    , {0,0}             						  , 1 , 1 },
 { "CLK_HDTVO_DENC"         ,CLK_HDTVO_DENC        , CLK_LOC_SDTVO    , {0,0}                                     , 1 , 1 },

 { "CLK_SDTVO_PIX_MAIN"    	,CLK_SDTVO_PIX_MAIN    , CLK_REM_SDTVO    , {0,0}             						  , 1 , 1 },
 { "CLK_SDTVO_FDVO"         ,CLK_SDTVO_FDVO        , CLK_REM_SDTVO    , {0,0}                                     , 1 , 1 },
 { "CLK_SDTVO_DENC"         ,CLK_SDTVO_DENC        , CLK_REM_SDTVO    , {0,0}                                     , 1 , 1 },

};
#endif /*7200*/

#if defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188) \
 || defined(ST_5525)|| defined(ST_5107)
typedef enum Clock_e
{
    QSYNTH1_CLK1,
    QSYNTH1_CLK2,
    QSYNTH2_CLK1,
    QSYNTH2_CLK2,
    QSYNTH2_CLK3,
    QSYNTH2_CLK4,
    SYNTH_CLK                           /* for ST_5100 */
 }Clock_t;
#endif /*#if ST_5528 || ST_5100 || ST_5105 || ST_5301 || ST_8010 */

#if defined(ST_7015)|| defined(ST_7020)|| defined(ST_5528)|| defined(ST_5100)|| defined(ST_8010)|| defined(ST_5105)\
 || defined(ST_5301)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)
typedef struct FreqSynthElements_s
{
    U32 Fout;
    U32 sdiv;
    U32 md;
    U32 pe;
} FreqSynthElements_t;
#endif /* #if ST_7015 || ST_7020 || ST_5528 || ST_5100 || ST_5105 || ST_5301 || ST_8010*/

#ifdef USE_ICS9161
/* THIS PART IS ONLY FOR STI7020 CUT 1 BECAUSE OF SYNTH 3 problem : USE ICS9161 */
typedef struct ICS9161_ClkFreq_s
{
	U32	UsrClk1;
	U32	UsrClk2;
	U32	UsrClk3;
	U32	PixClk;
	U32	RClk2;
	U32	PcmClk;
	U32	CdClk;
} ICS_ClkFreq_t;
#endif /* #ifdef USE_ICS9161 */

/* Private Constants -------------------------------------------------------- */

#if defined (ST_7015) || defined (ST_7020)
/* DDTS GNBvd09294 : active video must not end too close to next VSync  */
#define BOTTOM_BLANKING_SIZE 3  /* lines */
#else
#define BOTTOM_BLANKING_SIZE 0  /* lines */
#endif

#if defined (REMOVE_GENERIC_ADDRESSES) && !defined(USE_7020_GENERIC_ADDRESSES)
#if defined (ST_7015)
#define CKG_BASE_ADDRESS  (STI7015_BASE_ADDRESS + ST7015_CKG_OFFSET)
#define VTG1_BASE_ADDRESS (STI7015_BASE_ADDRESS + ST7015_VTG1_OFFSET)
#define VTG2_BASE_ADDRESS (STI7015_BASE_ADDRESS + ST7015_VTG2_OFFSET)
#elif defined (ST_7020)
#define CKG_BASE_ADDRESS  (STI7020_BASE_ADDRESS + ST7020_CKG_OFFSET)
#define VTG1_BASE_ADDRESS (STI7020_BASE_ADDRESS + ST7020_VTG1_OFFSET)
#define VTG2_BASE_ADDRESS (STI7020_BASE_ADDRESS + ST7020_VTG2_OFFSET)
#endif
#endif

#if defined (ST_7710)
#define CKG_BASE_ADDRESS  ST7710_CKG_BASE_ADDRESS
#define VTG1_BASE_ADDRESS ST7710_VTG1_BASE_ADDRESS
#define VTG2_IT_OFFSET    0x8
#define VTG2_BASE_ADDRESS ST7710_VTG2_BASE_ADDRESS
#define UNLK_KEY              0x5AA50000
#define CLK_OFF_PIXHD         0x00000008
#define CLK_OFF_PIXSD         0x00000200
#define CLK_ON_PIXHD          0xFFFFFFF7
#define CLK_ON_PIXSD          0xFFFFFDFF
#define FS_EN_PRG             0x00000001
#define FS_DIS_PRG            0xFFFFFFFE
#define FS_CLOCKGEN_CFG_0     0x000C0
#define PLL2_CONFIG_2         0x98
#define FS_CLOCKGEN_CFG_2     0xC8
#define FS_12_CONFIG_0        0x6C
#define FS_12_CONFIG_1        0x70
#define FS_04_CONFIG_0        0x0004C
#define FS_11_CONFIG_0        0x00064
#define FS_04_CONFIG_1        0x00050
#define FS_11_CONFIG_1        0x00068
#endif

#if defined (ST_7100)
#ifdef ST_OSLINUX
extern MapParams_t   OutputStageMap;
#define VOS_BASE_ADDRESS         OutputStageMap.MappedBaseAddress
#define VTG1_BASE_ADDRESS VOS_BASE_ADDRESS
#define VTG2_BASE_ADDRESS VOS_BASE_ADDRESS + 0x34

#else
#define VTG1_BASE_ADDRESS ST7100_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS ST7100_VTG2_BASE_ADDRESS
#endif   /** ST_OSLINUX **/
/*#define CKG_BASE_ADDRESS  ST7100_CKG_BASE_ADDRESS*/
#define VTG2_IT_OFFSET    0x8
#ifdef ST_OSLINUX
#define CKGB_BASE               ClockMap.MappedBaseAddress
#else
#define PERIPH_BASE             0xB8000000
#define CKGB_BASE               (PERIPH_BASE + 0x01000000)
#endif
#define CKGB_LOCK               0x010
#define CKGB_DISPLAY_CFG        0x0a4
#define CKGB_FS_SELECT          0x0a8
#define CKGB_FS0_MD1            0x018
#define CKGB_FS0_PE1            0x01c
#define CKGB_FS0_EN_PRG1        0x020
#define CKGB_FS0_SDIV1          0x024
#define CKGB_FS1_MD1            0x060
#define CKGB_FS1_PE1            0x064
#define CKGB_FS1_EN_PRG1        0x068
#define CKGB_FS1_SDIV1          0x06c
#endif

#if defined (ST_7109)

#ifdef ST_OSLINUX
extern MapParams_t OutputStageMap;
#undef VOS_BASE_ADDRESS
#define VOS_BASE_ADDRESS        OutputStageMap.MappedBaseAddress
#define VTG1_BASE_ADDRESS       VOS_BASE_ADDRESS
#define VTG2_BASE_ADDRESS       VOS_BASE_ADDRESS + 0x34

#else
#define VTG1_BASE_ADDRESS ST7109_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS ST7109_VTG2_BASE_ADDRESS
#endif   /** ST_OSLINUX **/

#ifndef CKGB_BASE
#define CKGB_BASE         ST7109_CKG_BASE_ADDRESS
#endif
#define VTG2_IT_OFFSET    0x8

#define CKGB_LOCK               0x010
#define CKGB_DISPLAY_CFG        0x0a4
#define CKGB_FS_SELECT          0x0a8
#define CKGB_FS0_MD1            0x018
#define CKGB_FS0_PE1            0x01c
#define CKGB_FS0_EN_PRG1        0x020
#define CKGB_FS0_SDIV1          0x024
#define CKGB_FS1_MD1            0x060
#define CKGB_FS1_PE1            0x064
#define CKGB_FS1_EN_PRG1        0x068
#define CKGB_FS1_SDIV1          0x06c
#endif

#if defined (ST_7200)
#define VTG_H_HD1        0x2c
#define VTG_TOP_V_VD1    0x30
#define VTG_BOT_V_VD1    0x34
#define VTG_TOP_V_HD1    0x38
#define VTG_BOT_V_HD1    0x3c
#define VTG_ITM          0x98
#define VTG_VTGMOD       0x24        /* VTG Mode */
#define VTG_CLKLN        0x28        /* Line length (Pixclk) */
#define VTG_HLFLN        0x40        /* Half-Lines per Field*/
#define VTG_DRST         0x48

#if defined (ST_OSLINUX)
#define VOS_HD_BASE_ADDRESS        OutputStageMap.MappedBaseAddress
#define VOS_SD_BASE_ADDRESS        OutputStageSDMap.MappedBaseAddress
#define VOS_HDMI_BASE_ADDRESS      HDMIMap.MappedBaseAddress

#undef VTG_1_BASE_ADDRESS
#undef VTG_2_BASE_ADDRESS
#undef VTG_3_BASE_ADDRESS
#define VTG_1_BASE_ADDRESS         VOS_HD_BASE_ADDRESS + 0x300
#define VTG_2_BASE_ADDRESS         VOS_HD_BASE_ADDRESS + 0x200
#define VTG_3_BASE_ADDRESS         VOS_SD_BASE_ADDRESS + 0x200
#else /*ST_OSLINUX*/
#define VOS_HD_BASE_ADDRESS        ST7200_HD_TVOUT_BASE_ADDRESS
#define VOS_SD_BASE_ADDRESS        ST7200_SD_TVOUT_BASE_ADDRESS
#define VOS_HDMI_BASE_ADDRESS      ST7200_HDMI_BASE_ADDRESS
#endif  /*ST_OSLINUX*/
#endif/*7200*/

#if defined (ST_5528)
#define CKG_BASE_ADDRESS  ST5528_CKG_BASE_ADDRESS
#define PWM_CLK_CTRL                        0x8c    /* PWM clock control */
#define PWM_CLK_STOP                        0x1     /* Stops PWMCLK */

#endif
/* DEN - Digital encoder registers - standard 8 bit calls     --------_----- */
#define DENC_CFG0   0x00  /* Configuration register 0                [7:0]   */
#define DENC_CFG4   0x04  /* Configuration register 4                [7:0]   */
#define DENC_CFG6   0x06  /* Configuration register 6                [7:0]   */

#define DENC_CFG0_MASK_STD     0x3F /* Mask for standard selected              */
#define DENC_CFG0_VSHS_SLV     0x28 /* VSYNC + HSYNC based slave mode(line l  )*/
#define DENC_CFG0_FRHS_SLV     0x18 /* Frame + HSYNC based slave mode(line l)  */
#define DENC_CFG0_ODHS_SLV     0x10 /* BnotREF + Href based slave mode(line 1) */
#define DENC_CFG0_MASTER       0x30 /* Master mode selected                    */
#define DENC_CFG0_HSYNC_POL    0x04 /* HSYNC positive pulse                    */


#define DENC_CFG4_MASK_SYIN    0x3F /* Mask for adjustment of incoming       */
                                    /* synchro signals                       */
#define DENC_CFG4_SYIN_P1      0x40 /* delay = +1 ckref                      */
#define DENC_CFG4_ALINE        0x08 /* Video active line duration control    */

#define DENC_CFG6_RST_SOFT     0x80 /* Denc soft reset                       */

#if defined (ST_7015) || defined(ST_7020)
#define DEFAULT_PIX_CLK         148500000
/*#elif defined (ST_5525)
#define DEFAULT_PIX_CLK         30000000*/
#else
#define DEFAULT_PIX_CLK         27000000
#endif
#define DEFAULT_DENC_CLK        27000000

#ifdef WA_GNBvd06791
#define MIN_PIXCLK_FOR_DISPCLK  27000000
#define DISP_CLOCK_FOR_PIXCLK24 70000000
#endif

#if defined (ST_7015) || defined(ST_7020)
#define VTG_HVDRIVE                         0x20        /* H/Vsync output enable */
#define VTG_CLKLN                           0x28        /* Line length (Pixclk) */
#define VTG_VOS_HDO                         0x2C        /* HSync start relative to Href (Pixclk) */
#define VTG_VOS_HDS                         0x30        /* Hsync end relative to Href (Pixclk) */
#define VTG_HLFLN                           0x34        /* Half-Lines per Field*/
#define VTG_VOS_VDO                         0x38        /* Vertical Drive Output start relative to Vref (half lines) */
#define VTG_VOS_VDS                         0x3C        /* Vertical Drive Output end relative to Vref (half lines) */
#define VTG_VTGMOD                          0x40        /* VTG Mode */
#define VTG_VTIMER                          0x44        /* Vsync Timer (Cycles) */
#define VTG_DRST                            0x48        /* VTG Reset */
#define VTG_ITM                             0x98        /* Interrupt Mask */

#define VTG_VTGMOD_SLAVEMODE_MASK           0x1f
#define VTG_VTGMOD_SLAVE_MODE_INTERNAL      0x0
#define VTG_VTGMOD_HVSYNC_CMD_MASK          0x3C0
#define VTG_HVDRIVE_VSYNC                   0x1
#define VTG_HVDRIVE_HSYNC                   0x2

#define CKG_SYN3_PAR                        0x4C        /* Frequency synthesizer 3 parameters       */
#define CKG_SYN6_PAR                        0x58        /* Frequency synthesizer parameters         */
#define CKG_SYN7_PAR                        0x5C        /* Frequency synthesizer 7 parameters       */
#define CKG_SYN3_REF                        0x70        /* Synthesizer 3 ref clock selection        */
#define CKG_SYN7_REF                        0x74        /* Synthesizer 7 denc pixel clock selection */

#define CKG_SYN_PAR_PE_MASK                 0x0000ffff  /* Fine selector mask               */
#define CKG_SYN_PAR_MD_MASK                 0x001f0000  /* Coarse selector mask             */
#define CKG_SYN_PAR_SDIV_MASK               0x00e00000  /* Output divider mask              */
#define CKG_SYN_PAR_NDIV_MASK               0x03000000  /* Input divider mask               */
#define CKG_SYN_PAR_SSL                     0x04000000  /* not Bypassed bit                 */
#define CKG_SYN_PAR_EN                      0x08000000  /* Enable bit                       */
#endif /* #if ST_7015 || ST_7020 */

#if defined (ST_7710) || defined(ST_7100) || defined (ST_7109)
#define VTG_HVDRIVE                         0x00        /* H/Vsync output enable */
#define VTG_CLKLN                           0x04        /* Line length (Pixclk) */
#define VTG_VOS_HDO                         0x08        /* HSync start relative to Href (Pixclk) */
#define VTG_VOS_HDS                         0x0c        /* Hsync end relative to Href (Pixclk) */
#define VTG_HLFLN                           0x10        /* Half-Lines per Field*/
#define VTG_VOS_VDO                         0x14        /* Vertical Drive Output start relative to Vref (half lines) */
#define VTG_VOS_VDS                         0x18        /* Vertical Drive Output end relative to Vref (half lines) */
#define VTG_VTGMOD                          0x1c        /* VTG Mode */
#define VTG_VTIMER                          0x20        /* Vsync Timer (Cycles) */
#define VTG_DRST                            0x24        /* VTG Reset */
#define VTG_ITM                             0x28        /* Interrupt Mask */
#define VTG_ITS                             0x2c        /* Interrupt Status */
#define VTG_STA                             0x30        /* Status register */
#define VTG_HVDRIVE_VSYNC                   0x1
#define VTG_HVDRIVE_HSYNC                   0x2
#define VTG_MOD_FI                          0x0         /* Enable Force interleaved MODE */
#define VTG_RG1                             0x28
#define VTG_RG2                             0x2c
#endif /* #if ST_7710 || ST_7100 || ST_7109*/

#if defined(ST_GX1) || defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010) \
 || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)
#define VTG_VFE_MOD                         0x24        /* VTG Mode */
#define VTG_VFE_CLKLN                       0x28        /* Line length (Pixclk) */
#define VTG_VFE_H_HD                        0x2C        /* HSYNC signal rising and falling edge (Pixclk) */
#define VTG_VFE_TOP_V_VD                    0x30        /* VSYNC signal rising and falling edge, Top field (lines) */
#define VTG_VFE_BOT_V_VD                    0x34        /* VSYNC signal rising and falling edge, Bottom field (lines)*/
#define VTG_VFE_TOP_V_HD                    0x38        /* VSYNC signal rising and falling edge, Top field (Pixclk) */
#define VTG_VFE_BOT_V_HD                    0x3C        /* VSYNC signal rising and falling edge, Bottom field (Pixclk)*/
#define VTG_VFE_HLFLN                       0x40        /* Half-Lines per Field*/
#define VTG_VFE_SRST                        0x48        /* VTG Soft Reset */
#define VTG_ITM                             0x98

#define VTG_VFE_MOD_MASK                    0x0000000B
#define VTG_VFE_H_HDO_MASK                  0x00001FFF
#define VTG_VFE_TOP_V_VD0_MASK              0x000007FF
#define VTG_VFE_TOP_V_VDS_MASK              0x07FF0000
#define VTG_VFE_BOT_V_VD0_MASK              0x000007FF
#define VTG_VFE_BOT_V_HD0_MASK              0x000007FF
#define VTG_VFE_ITM                         0x98        /* Interrupt Mask */
#define VTG_VFE_ITS                         0x9C        /* Interrupt Status */
#define VTG_VFE_STA                         0xA0        /* Status register */
 /* masks */
#define VTG_VFE_IT_VSB                      0x00000008         /* 5100 */
#define VTG_VFE_IT_VST                      0x00000010         /* 5100 */

#define VTG_VFE_MOD_MODE_MASTER             0x00000000
#define VTG_VFE_MOD_DISABLE                 0x00000200  /* 5528 , 5100 */
#define VTG_TOP_V_VD1                       0x00010107  /*VSYNC Top Synchro for 5528 */
#define VTG_BOT_V_VD1                       0x00000000 /*VSYNC Bot Synchro for 5528 */
#define VTG_TOP_V_HD1                       0x00000000 /*HSYNC Top Synchro for 5528 */
#define VTG_BOT_V_HD1                       0x00000000 /*HSYNC Bot Synchro for 5528 */

#define CKG_SYN1_CLK                        0x30     /*For general control*/
#define CKG_SYN1_CLK2                       0x38     /*For USBCLK control*/

#define CKG_SYN2_CLK                        0x50      /* For general control*/
#define CKG_SYN2_CLK1                       0x54      /* For ST20CLK control*/
#define CKG_SYN2_CLK2                       0x58      /* For HDDCLK control*/
#define CKG_SYN2_CLK3                       0x5C      /* For SCCLK control */
#define CKG_SYN2_CLK4                       0x60      /* For VIDPIX_2XCLK control */
#define CKG_SYN2_CLK4_RST                   0xF5D5B1E /*Square pixel configuration Reset*/


#define PIX_CLK_CTRL                        0x84    /*Pixel clock control*/
#define PIX_CLK_CTRL1                       0x00    /*pixel clock (2X) is connected to SYSACLKIN*/
#define PIX_CLK_CTRL2                       0x01    /*pixel clock (2X) is connected to SYSBCLKIN*/
#define PIX_CLK_CTRL3                       0x02    /*pixel clock (2X) is connected to VIDINCLKIN*/
#define PIX_CLK_CTRL4                       0x03    /*pixel clock (2X) is connected to QSYNTH2_CLK4*/
#define PIX_CLK_RST                         0x00    /*pixel clock control reset*/

#define CKG_LOCK_REG                        0x90    /* register write lock to prevent inopportune SW access */
#define CKG_LOCK                            0xc1a0  /* lock clock registers acces */
#define CKG_UNLOCK                          0xc0de  /* lock clock registers acces */

#define CKG_SYN2_PE_MASK                 0x0000ffff  /* Fine selector mask               */
#define CKG_SYN2_MD_MASK                 0x001f0000  /* Coarse selector mask             */
#define CKG_SYN2_SDIV_MASK               0x00e00000  /* Output divider mask              */
#define CKG_SYN_RST_MASK                 0x07000000  /* 3 bits from bit24->bit26 mask    */

#define CKG_CLK_STOP                     0x78000000   /*Clock stoped*/

#endif /* GX1 || 5528 */

#if defined (ST_5100)
#define CKG_BASE_ADDRESS  ST5100_CKG_BASE_ADDRESS
#elif defined (ST_5105)
#define CKG_BASE_ADDRESS  ST5105_CKG_BASE_ADDRESS
#elif defined (ST_5107)
#define CKG_BASE_ADDRESS  ST5107_CKG_BASE_ADDRESS
#elif defined (ST_5188)
#define CKG_BASE_ADDRESS  ST5188_CKG_BASE_ADDRESS
#elif defined (ST_5301)
#define CKG_BASE_ADDRESS  ST5301_CKG_BASE_ADDRESS
#elif defined (ST_8010)
#define CKG_BASE_ADDRESS  ST8010_CKG_BASE_ADDRESS
#elif defined (ST_5525)
#define CKG_BASE_ADDRESS  ST5525_CKG_BASE_ADDRESS

#endif

#if defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188)|| defined(ST_5107)
    #define CKG_PIX_CLK_SETUP0            (CKG_BASE_ADDRESS + 0x014)
    #define CKG_PIX_CLK_SETUP1            (CKG_BASE_ADDRESS + 0x018)
    #define DCO_MODE_CFG                  (CKG_BASE_ADDRESS + 0x170)
    #define RGST_LK_CFG                   (CKG_BASE_ADDRESS + 0x300)
    #define RGST_LK_CFG_LK                 0x100
    #define RGST_LK_CFG_UNLK               0xff
    #define DCO_MODE_CFG_PRG               0x1
    #define PIX_CLK_SETUP0_RESETN          0x1
    #define PIX_CLK_SETUP0_SELOUT          0x1
    #define PIX_CLK_SETUP0_ENPRG           0x1
    #define PIX_CLK_SETUP0_RST            0x0EBF
    #define PIX_CLK_SETUP1_RST            0x0000
    #define PIX_CLK_SETUP0_MD_MASK         0x1F
    #define PIX_CLK_SETUP0_ENPRG_MASK      0x20
    #define PIX_CLK_SETUP0_SDIV_MASK       0x1C0
    #define PIX_CLK_SETUP0_SELOUT_MASK     0x200
    #define PIX_CLK_SETUP0_RESETN_MASK     0x400
#elif defined (ST_5525)
    #define CKG_PIX_CLK_SETUP0            (CKG_BASE_ADDRESS + 0x350)
    #define CKG_PIX_CLK_SETUP1            (CKG_BASE_ADDRESS + 0x354)
    #define DCO_MODE_CFG                  (CKG_BASE_ADDRESS + 0x170)
    #define DCO_MODE_CFG_PRG               0x1
    #define PIX_CLK_SETUP0_SELOUT          0x1
    #define PIX_CLK_SETUP0_ENPRG           0x1
    #define PIX_CLK_SETUP0_RST             0x0EF1
    #define PIX_CLK_SETUP1_RST             0x1C72
    #define PIX_CLK_SETUP0_MD_MASK         0x1F
    #define PIX_CLK_SETUP0_ENPRG_MASK      0x20
    #define PIX_CLK_SETUP0_SDIV_MASK       0x1C0
    #define PIX_CLK_SETUP0_SELOUT_MASK     0x200
#endif

#define VTG_MAX_MODE  7

#ifdef USE_ICS9161
/* THIS PART IS ONLY FOR STI7020 CUT 1 BECAUSE OF SYNTH 3 problem : USE ICS9161 */

/* Default clock frequencies */
#define DEFAULT_USRCLK1  27000000
#define DEFAULT_USRCLK2 108000000
#define DEFAULT_USRCLK3  27000000
#define DEFAULT_PIXCLK   74250000
#define DEFAULT_RCLK2    27000000
#define DEFAULT_PCMCLK   27000000
#define DEFAULT_CDCLK    80000000
/* EPLD registers for ICS programmation */
#if defined(mb295) || defined(mb295b) || defined(mb295c)
  #define ICS_CTRL          0x70400000
  #define ICS_DATA          0x70404000

  #define ICS_CTRL_CLKOE1   0x10
  #define ICS_CTRL_CLKOE2   0x20
  #define ICS_CTRL_CLKOE3   0x40
  #define ICS_CTRL_CLKCS    0x80
#elif defined(mb290) || defined(mb290c)
  #define ICS_CTRL          0x61000000
  #define ICS_DATA          0x61100000

  #define ICS_CTRL_CLKOE1   0x01
  #define ICS_CTRL_CLKCS    0x02
#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif

#define STSYS_WriteIcsReg8(reg, value) STSYS_WriteRegDev8((reg), (value))

/* ICS Address locations */
#define ICS_REG0              0
#define ICS_REG1              1
#define ICS_REG2              2
#define ICS_MREG              3
#define ICS_PWRDWN            4
#define ICS_CNTLREG           6

#endif /* #ifdef USE_ICS9161 */

/* Private variables ------------------------------------------------------ */

#if defined(ST_7015)|| defined(ST_7020)|| defined(ST_GX1) || defined(ST_5528)|| defined(ST_5100)|| defined(ST_7710) \
 || defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_7100)|| defined(ST_7109)|| defined(ST_5188) \
 || defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)
static VtgSimTimingMode_t LastVtg1TimingMode = VTG_MAX_MODE; /* invalid mode */
#if defined (ST_7015) || defined(ST_7020)|| defined(ST_7710) || defined (ST_7100)|| defined (ST_7109)|| defined (ST_5525)\
 || defined(ST_7200)
static VtgSimTimingMode_t LastVtg2TimingMode = VTG_MAX_MODE; /* invalid mode */
#endif
#if defined(ST_7200)
static VtgSimTimingMode_t LastVtg3TimingMode = VTG_MAX_MODE; /* invalid mode */
#endif
#endif

#if defined(ST_7015)|| defined(ST_7020)|| defined(ST_7710)|| defined(ST_7100)|| defined(ST_7109)
static BOOL FirstInitDone = FALSE;
static U32  LastPixClk;
static U32  LastAuxClk;
#endif /* #if ST_7015 || ST_7020 */


#if defined (ST_7015)
static const FreqSynthElements_t FreqSynthElements[] = {   /* Freq synth configuration depending on frequency */
    { 24545454, 0x3, 0x11, 0x3333},   /* 2 x 12272727 */
    { 24570000, 0x3, 0x11, 0x3573},   /* 2 x 12285000 */
    { 27000000, 0x3, 0x0f, 0x0000},   /* also 2 x 13500000 */
    { 27027000, 0x2, 0x1f, 0x0417},   /* 2 x 13513500 */
    { 29500000, 0x2, 0x1d, 0x5b1e},   /* 2 x 14750000 */
    { 70000000, 0x1, 0x18, 0x283a},   /* closest <= 3 x 24545454 and 3 x 24570000 and 3 x 24576000 */
    { 81000000, 0x1, 0x15, 0x5555},   /* general case disp clock */
    {148500000, 0x0, 0x17, 0x5d17}
};
#elif defined (ST_7020)
static const FreqSynthElements_t FreqSynthElements[] = {   /* Freq synth configuration depending on frequency */
    { 24545454, 0x3, 0x4, 0x4ccd},   /* 2 x 12272727 */
    { 24570000, 0x3, 0x4, 0x4d5d},   /* 2 x 12285000 */
    { 27000000, 0x2, 0x7, 0x0000},   /* also 2 x 13500000 */
    { 27027000, 0x2, 0x7, 0x0106},   /* 2 x 13513500 */
    { 29500000, 0x2, 0x7, 0x56c8},   /* 2 x 14750000 */
    { 70000000, 0x1, 0x6, 0x6a0f},   /* closest <= 3 x 24545454 and 3 x 24570000 and 3 x 24576000 */
    { 81000000, 0x1, 0x5, 0x5555},   /* general case disp clock */
    {148500000, 0x0, 0x5, 0x1746}
};

#elif defined (ST_5528)
static const FreqSynthElements_t FreqSynthElements[] = {   /* Freq synth configuration depending on frequency */
    { 24545454, 0x3, 0x11,0x3334},   /* 2 x 12272727 */
    { 24570000, 0x3, 0x11,0x3334},   /* 2 x 12285000 :This Configuration must be verified*/
    { 27000000, 0x2, 0x7, 0x0000},   /* also 2 x 13500000 :This Configuration must be verified*/
    { 27027000, 0x2, 0x7, 0x0106},   /* 2 x 13513500 :This Configuration must be verified*/
    { 29500000, 0x2, 0x1d, 0x5b1f},   /* 2 x 14750000 */
    { 70000000, 0x1, 0x6, 0x6a0f},   /* closest <= 3 x 24545454 and 3 x 24570000 and 3 x 24576000 :This Configuration must be verified!*/
    { 81000000, 0x1, 0x5, 0x5555},   /* general case disp clock :This Configuration must be verified*/
    {148500000, 0x0, 0x5, 0x1746}    /*  This Configuration must be verified! */
};
#endif /* #if ST_7015 || ST_7020 || ST_5528*/

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)
static TimingModeParams_t ModeParamsTable[] = /* index of this table should be VtgSimTimingMode_t */
    {
    {   0,    0,         0,  0, 0 },     /* VTGSIM_TIMING_MODE_SLAVE,           */
    { 858,  525,  13513500, 65, 6 },     /* VTGSIM_TIMING_MODE_480I60000_13514, */   /* NTSC 60Hz */
    { 858,  525,  13500000, 65, 6 },     /* VTGSIM_TIMING_MODE_480I59940_13500, */   /* NTSC, PAL M */
    { 780,  525,  12285000, 59, 6 },     /* VTGSIM_TIMING_MODE_480I60000_12285, */   /* NTSC 60Hz square */
    { 780,  525,  12272727, 59, 6 },     /* VTGSIM_TIMING_MODE_480I59940_12273, */   /* NTSC square, PAL M square */
    { 864,  625,  13500000, 65, 6 },     /* VTGSIM_TIMING_MODE_576I50000_13500, */   /* PAL B,D,G,H,I,N, SECAM */
    { 944,  625,  14750000, 71, 6 }      /* VTGSIM_TIMING_MODE_576I50000_14750, */   /* PAL B,D,G,H,I,N, SECAM square */
    };
/* #if ST_7015 || ST_7020 || ST_GX1  */

#elif  defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188)|| defined(ST_5525)\
    || defined(ST_5107)
static TimingModeParams_t ModeParamsTable[] = /* index of this table should be VtgSimTimingMode_t */
    {
    {   0,    0,         0,  0,  0  },     /* VTGSIM_TIMING_MODE_SLAVE,           */
    { 858,  525,  13513500, 65, 263 },     /* VTGSIM_TIMING_MODE_480I60000_13514, */   /* NTSC 60Hz */
    { 858,  525,  13500000, 65, 263 },     /* VTGSIM_TIMING_MODE_480I59940_13500, */   /* NTSC, PAL M */
    { 780,  525,  12285000, 59, 263 },     /* VTGSIM_TIMING_MODE_480I60000_12285, */   /* NTSC 60Hz square */
    { 780,  525,  12272727, 59, 263 },       /* VTGSIM_TIMING_MODE_480I59940_12273, */   /* NTSC square, PAL M square */
    { 864,  625,  13500000, 65, 313 },      /* VTGSIM_TIMING_MODE_576I50000_13500, */   /* PAL B,D,G,H,I,N, SECAM */
    { 944,  625,  14750000, 71, 313 }      /* VTGSIM_TIMING_MODE_576I50000_14750, */   /* PAL B,D,G,H,I,N, SECAM square */
    };
/* #if ST_5100 || ST_5105 || ST_5301 || ST_8010 */

#elif defined (ST_5528)
static TimingModeParams_t ModeParamsTable[] = /* index of this table should be VtgSimTimingMode_t */
    {
    {   0,    0,         0,  0, 0},          /* VTGSIM_TIMING_MODE_SLAVE,           */
    { 858,  525,  13513500, 8388613, 0},     /* VTGSIM_TIMING_MODE_480I60000_13514, */   /* NTSC 60Hz */
    { 858,  525,  13500000, 8388613, 0},     /* VTGSIM_TIMING_MODE_480I59940_13500, */   /* NTSC, PAL M */
    { 780,  525,  12285000, 8388613, 0},     /* VTGSIM_TIMING_MODE_480I60000_12285, */   /* NTSC 60Hz square */
    { 780,  525,  12272727, 8388613, 0},     /* VTGSIM_TIMING_MODE_480I59940_12273, */   /* NTSC square, PAL M square */
    { 864,  625,  13500000, 8388613, 50},    /* VTGSIM_TIMING_MODE_576I50000_13500, */   /* PAL B,D,G,H,I,N, SECAM */
    { 944,  625,  14750000, 8388613, 50}     /* VTGSIM_TIMING_MODE_576I50000_14750, */   /* PAL B,D,G,H,I,N, SECAM square */
    };

#elif  defined(ST_7710) || defined(ST_7100) || defined (ST_7109)|| defined(ST_7200)
static TimingModeParams_t ModeParamsTable[] = /* index of this table should be VtgSimTimingMode_t */
    {
    {   0,    0,         0,  0, 0},          /* VTGSIM_TIMING_MODE_SLAVE,           */
    { 858,  525,  13513500, 65, 6},     /* VTGSIM_TIMING_MODE_480I60000_13514, */   /* NTSC 60Hz */
    { 858,  525,  13500000, 65, 6},     /* VTGSIM_TIMING_MODE_480I59940_13500, */   /* NTSC, PAL M (NTSCCC)*/
    { 780,  525,  12285000, 65, 6},     /* VTGSIM_TIMING_MODE_480I60000_12285, */   /* NTSC 60Hz square */
    { 780,  525,  12272727, 65, 6},     /* VTGSIM_TIMING_MODE_480I59940_12273, */   /* NTSC square, PAL M square */
    { 864,  625,  13500000, 65, 6},    /* VTGSIM_TIMING_MODE_576I50000_13500, */   /* PAL B,D,G,H,I,N, SECAM (PALLLL) */
    { 944,  625,  14750000, 65, 6}     /* VTGSIM_TIMING_MODE_576I50000_14750, */   /* PAL B,D,G,H,I,N, SECAM square */
    };
#endif /* #if ST_7710 || ST_7100 || ST_7109*/

static const char ModeName[VTG_MAX_MODE][23] = {
    "MODE_SLAVE"            ,
    "MODE_480I60000_13514"  , /* NTSC 60Hz */
    "MODE_480I59940_13500"  , /* NTSC, PAL M */
    "MODE_480I60000_12285"  , /* NTSC 60Hz square */
    "MODE_480I59940_12273"  , /* NTSC square, PAL M square */
    "MODE_576I50000_13500"  , /* PAL B,D,G,H,I,N, SECAM */
    "MODE_576I50000_14750"  , /* PAL B,D,G,H,I,N, SECAM square */
    };

#ifdef USE_ICS9161
/* THIS PART IS ONLY FOR STI7020 CUT 1 BECAUSE OF SYNTH 3 problem : USE ICS9161 */

#if defined(mb295) || defined(mb295b) || defined(mb295c)
#define    ICS_FREF 14318180
#elif defined(mb290) || defined(mb290c)
#define    ICS_FREF 27000000
#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif
static ICS_ClkFreq_t ICS_ClkFreq;

static ICS_ClkFreqFirstInitDone = FALSE;
#endif /* #ifdef USE_ICS9161 */

/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */
#define vtgsim_Write32(a, v)     STSYS_WriteRegDev32LE((a), (v))
#define vtgsim_Read32(a)         STSYS_ReadRegDev32LE((a))
#define vtgsim_Write8(a, v)      STSYS_WriteRegDev8((a), (v))
#define vtgsim_Read8(a)          STSYS_ReadRegDev8((a))

/* Private Function prototypes ---------------------------------------------- */
#if defined(ST_7015)|| defined(ST_7020)|| defined(ST_5528)|| defined(ST_5100)|| defined(ST_7710)|| defined(ST_8010)\
 || defined(ST_5105)|| defined(ST_5301)|| defined(ST_7100)|| defined(ST_7109)|| defined(ST_5188)|| defined(ST_5525)\
 || defined(ST_5107)
static void ConfigSynth(const Clock_t Synth, const U32 OutputClockFreq);
static void SetPixelClock(const VtgSimTimingMode_t TimingMode, const U8 VtgNb);
#endif /* #if ST_7015 || ST_7020 || ST_5528*/

#if defined (ST_7015) || defined(ST_7020) || defined(ST_5528)
static void GetSynthParam(const U32 Fout, U32 * const ndiv, U32 * const sdiv, U32 * const md, U32 * const pe);
#endif

#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_8010)||defined(ST_5188)||defined(ST_5525)||defined(ST_5107)
#if !defined(ST_OSLINUX)
static BOOL ComputeFrequencySynthesizerParameters(const U32 Fin , const U32 Fout, U32 * const ndiv1,
                                                    U32 * const sdiv1,U32 * const md1, U32 * const pe1);
#endif
#endif
#if defined (ST_7100) || defined (ST_7109)
#if !defined(ST_OSLINUX)
BOOL ST_Ckg_Init(STTST_Parse_t *pars_p, char *result_sym_p);
BOOL  ST_Ckg_SetFSClock(Clock_t FS_sel, int freq);
 BOOL ST_Ckg_Term(STTST_Parse_t *pars_p, char *result_sym_p);
#endif
#endif
#if defined (ST_7200)
BOOL ST_Ckg_Init(STTST_Parse_t *pars_p, char *result_sym_p);
#endif
BOOL VtgSimInitCmd( STTST_Parse_t *pars_p, char *result_sym_p );

#ifdef USE_ICS9161
/* THIS PART IS ONLY FOR STI7020 CUT 1 BECAUSE OF SYNTH 3 problem : USE ICS9161 */

static void Unlock( void );
static void Startbit( void );
static void Progbit(U8 bit0, U8 bit1, U8 bit2, U8 bit3);
static void Stopbit( void );
static void ConfigPLL(S32 pll[4], S32 addr[4]);
static U32 ComputeICSSetup(U32 Frequency);
static U32 ComputeActualFrequency(U32 RegValue);
static void ICS9161SetPixClkFrequency(U32 Frequency);
#endif /* #ifdef USE_ICS9161 */


/* Functions ---------------------------------------------------------------- */

#ifdef USE_ICS9161
/* THIS PART IS ONLY FOR STI7020 CUT 1 BECAUSE OF SYNTH 3 problem : USE ICS9161 */

static void Unlock()
{
  S32 i;

  STSYS_WriteIcsReg8(ICS_DATA, 0x0F);
  STSYS_WriteIcsReg8(ICS_CTRL, 0x0);

  for(i=0; i<5; i++)
  {
    STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKCS); /* Rising edge of CS */
    STSYS_WriteIcsReg8(ICS_CTRL, 0x0);            /* Falling edge of CS */
  }
}

static void Startbit()
{
  STSYS_WriteIcsReg8(ICS_DATA, 0x0);            /* Start bit has data = 0 */
  STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKCS); /* Rising edge of CS */
  STSYS_WriteIcsReg8(ICS_CTRL, 0x0);            /* Falling edge of CS */
  STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKCS); /* Rising edge of CS */
}

static void Progbit(U8 bit0, U8 bit1, U8 bit2, U8 bit3)
{
  U8 workdata;

  workdata = (U8)((bit0 | (bit1<<1) | (bit2<<2) | (bit3<<3)) & 0xF );

  STSYS_WriteIcsReg8(ICS_DATA, ~workdata);      /* Complement on falling */
  STSYS_WriteIcsReg8(ICS_CTRL, 0x0);            /* Falling edge of CS */
  STSYS_WriteIcsReg8(ICS_DATA, workdata);       /* True data on rising */
  STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKCS); /* Rising edge of CS */
}

static void Stopbit()
{
  STSYS_WriteIcsReg8(ICS_DATA, 0x0F);
  STSYS_WriteIcsReg8(ICS_CTRL, 0x0);            /* Falling edge of CS */
  STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKCS); /* Rising edge of CS */
}

static void ConfigPLL(S32 pll[4], S32 addr[4])
{
  S32 i;

  /* Initialise ClkControl bus into known state */
  STSYS_WriteIcsReg8(ICS_DATA, 0x0F);
  STSYS_WriteIcsReg8(ICS_CTRL, 0x0);

  /* Start programming sequence */
  Unlock();
  Startbit();
  for (i=0; i<21; i++) /* Output data bits - LSB first */
  {
    Progbit( ((pll[0]>>i)&1), ((pll[1]>>i)&1), ((pll[2]>>i)&1), ((pll[3]>>i)&1));
  }
  for (i=0; i<3; i++) /* Output address bits - LSB first */
  {
    Progbit( ((addr[0]>>i)&1), ((addr[1]>>i)&1), ((addr[2]>>i)&1), ((addr[3]>>i)&1));
  }
  Stopbit();

} /* end of ConfigPLL() */

static void ICS9161_InitializeExternalClocks(ICS_ClkFreq_t ICS_ClkFreq)
{
#if defined(mb295) || defined(mb295b) || defined(mb295c)
                      /* N.C.        USRCLK2             PIXCLK          PCMCLK  */
  S32 vcodata[4];
  S32 vcoaddr[4]  = {ICS_REG2,  	ICS_REG2,	 		ICS_REG2,  		ICS_REG2};

                     /* CD Clk       USRCLK1             USRCLK3         RCLK2     */
  S32 mregdata[4];
  S32 mregaddr[4] = {ICS_MREG,  	ICS_MREG,	 		ICS_MREG,  		ICS_MREG};

	vcodata[0] = ComputeICSSetup(27000000);
	vcodata[1] = ICS_ClkFreq.UsrClk2;
	vcodata[2] = ICS_ClkFreq.PixClk;
	vcodata[3] = ICS_ClkFreq.PcmClk;

	mregdata[0] = ICS_ClkFreq.CdClk;
	mregdata[1] = ICS_ClkFreq.UsrClk1;
	mregdata[2] = ICS_ClkFreq.UsrClk3;
	mregdata[3] = ICS_ClkFreq.RClk2;

  ConfigPLL(vcodata, vcoaddr);
  ConfigPLL(mregdata, mregaddr);

  STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKOE1   /* Rising edge of CS */
                             | ICS_CTRL_CLKOE2
                             | ICS_CTRL_CLKOE3
                             | ICS_CTRL_CLKCS);
  STSYS_WriteIcsReg8(ICS_DATA, 0x0F);

#elif defined(mb290) || defined(mb290c)

  								/* PIXCLK				N.C 			    N.C				    N.C 	*/
  S32 vcodata[4];
  S32 vcoaddr[4]  = {ICS_REG2,  	ICS_REG2,	 		ICS_REG2,  		ICS_REG2};

  								/* USRCLK2 			N.C 			    N.C				    N.C 	*/
  S32 mregdata[4];
  S32 mregaddr[4] = {ICS_MREG,  	ICS_MREG,	 		ICS_MREG,  		ICS_MREG};

	vcodata[0] = ICS_ClkFreq.PixClk;
	vcodata[1] = ComputeICSSetup(27000000);
	vcodata[2] = ComputeICSSetup(27000000);
	vcodata[3] = ComputeICSSetup(27000000);
	mregdata[0] = ICS_ClkFreq.UsrClk2;
	mregdata[1] = ComputeICSSetup(27000000);
	mregdata[2] = ComputeICSSetup(27000000);
	mregdata[3] = ComputeICSSetup(27000000);

  ConfigPLL(vcodata, vcoaddr);
  ConfigPLL(mregdata, mregaddr);

  STSYS_WriteIcsReg8(ICS_CTRL, ICS_CTRL_CLKOE1   /* Rising edge of CS */
                             | ICS_CTRL_CLKCS);
  STSYS_WriteIcsReg8(ICS_DATA, 0x0F);

#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif
} /* end of ICS9161_InitializeExternalClocks() */

static U32 ComputeICSSetup(U32 Frequency)
{
    U32 Fvco, DivOut, R, I, M, N, RealFvco, FvcoError, CurrM, CurrN, CurrRealFvco, CurrFvcoError, RegValue;

    DivOut = 1;
    R = 0;
    Fvco = Frequency * DivOut;
    while ((Fvco < 50000000) && (DivOut <= 128))
    {
        DivOut *= 2;
        R++;
        Fvco = Frequency*DivOut;
    }
    if(R == 8)
    {
        return(0);
    }

    if(Fvco <= 51000000)
        I = 0;
    else if(Fvco <= 53200000)
        I = 1;
    else if(Fvco <= 58500000)
        I = 2;
    else if(Fvco <= 60700000)
        I = 3;
    else if(Fvco <= 64400000)
        I = 4;
    else if(Fvco <= 66800000)
        I = 5;
    else if(Fvco <= 73500000)
        I = 6;
    else if(Fvco <= 75600000)
        I = 7;
    else if(Fvco <= 80900000)
        I = 8;
    else if(Fvco <= 83200000)
        I = 9;
    else if(Fvco <= 91500000)
        I = 10;
    else if(Fvco <= 100000000)
        I = 11;
    else
        I = 12;

    FvcoError = 0xFFFFFFFF;
    for(CurrM=3 ; CurrM<=129 ; CurrM++)
    {
        CurrN = (Fvco * CurrM)/(2 * ICS_FREF);

        if((CurrN >= 4) && (CurrN <= 130))
        {
            CurrRealFvco = (2 * ICS_FREF * CurrN) / CurrM;
            if(CurrRealFvco < Fvco)
                CurrFvcoError = Fvco - CurrRealFvco;
            else
                CurrFvcoError = CurrRealFvco - Fvco;
            if(CurrFvcoError < FvcoError)
            {
                M = CurrM;
                N = CurrN;
                RealFvco = CurrRealFvco;
                FvcoError = CurrFvcoError;
            }
        }

        CurrN++;

        if((CurrN >= 4) && (CurrN <= 130))
        {
            CurrRealFvco = (2 * ICS_FREF * CurrN) / CurrM;
            if(CurrRealFvco < Fvco)
                CurrFvcoError = Fvco - CurrRealFvco;
            else
                CurrFvcoError = CurrRealFvco - Fvco;
            if(CurrFvcoError < FvcoError)
            {
                M = CurrM;
                N = CurrN;
                RealFvco = CurrRealFvco;
                FvcoError = CurrFvcoError;
            }
        }
    }
    for(CurrN=4 ; CurrN<=130 ; CurrN++)
    {
        CurrM = (2 * ICS_FREF * CurrN)/ Fvco;

        if((CurrM >= 3) && (CurrN <= 129))
        {
            CurrRealFvco = (2 * ICS_FREF * CurrN) / CurrM;
            if(CurrRealFvco < Fvco)
                CurrFvcoError = Fvco - CurrRealFvco;
            else
                CurrFvcoError = CurrRealFvco - Fvco;
            if(CurrFvcoError < FvcoError)
            {
                M = CurrM;
                N = CurrN;
                RealFvco = CurrRealFvco;
                FvcoError = CurrFvcoError;
            }
        }

        CurrM++;

        if((CurrM >= 3) && (CurrN <= 129))
        {
            CurrRealFvco = (2 * ICS_FREF * CurrN) / CurrM;
            if(CurrRealFvco < Fvco)
                CurrFvcoError = Fvco - CurrRealFvco;
            else
                CurrFvcoError = CurrRealFvco - Fvco;
            if(CurrFvcoError < FvcoError)
            {
                M = CurrM;
                N = CurrN;
                RealFvco = CurrRealFvco;
                FvcoError = CurrFvcoError;
            }
        }
    }
    RegValue = (M - 2) |
               (R << 7) |
               ((N - 3) << 10) |
               (I << 17);
    return(RegValue);
} /* end of ComputeICSSetup() */

static U32 ComputeActualFrequency(U32 RegValue)
{
    U32 M, N, R, OutDiv, Fout;

    M = (RegValue & 0x7F) + 2;
    R = (RegValue >> 7) & 0x7;
    N = ((RegValue >> 10) & 0x7F) + 3;

    OutDiv = 1 << R;
    Fout = (2 * N * ICS_FREF)/(M*OutDiv);
    return(Fout);
} /* end of ComputeActualFrequency() */

static void ICS9161SetPixClkFrequency(U32 Frequency)
{
    U32 ActualFrequency;

    if (!ICS_ClkFreqFirstInitDone)
    {
#if defined(mb295) || defined(mb295b) || defined(mb295c)
        ICS_ClkFreq.UsrClk1     = ComputeICSSetup(DEFAULT_USRCLK1);
        ICS_ClkFreq.UsrClk2     = ComputeICSSetup(DEFAULT_USRCLK2);
        ICS_ClkFreq.UsrClk3     = ComputeICSSetup(DEFAULT_USRCLK3);
        ICS_ClkFreq.RClk2       = ComputeICSSetup(DEFAULT_RCLK2);
        ICS_ClkFreq.PcmClk      = ComputeICSSetup(DEFAULT_PCMCLK);
        ICS_ClkFreq.CdClk       = ComputeICSSetup(DEFAULT_CDCLK);
#elif defined(mb290) || defined(mb290c)
        ICS_ClkFreq.UsrClk2     = ComputeICSSetup(DEFAULT_USRCLK2);
#else
  #error ERROR:invalid DVD_PLATFORM defined
#endif
        ICS_ClkFreqFirstInitDone = TRUE;
    }
    ICS_ClkFreq.PixClk = ComputeICSSetup(Frequency);
    ActualFrequency = ComputeActualFrequency((U32)ICS_ClkFreq.PixClk);
	ICS9161_InitializeExternalClocks(ICS_ClkFreq);
} /* end of ICS9161SetPixClkFrequency() */


#endif /* #ifdef USE_ICS9161 */

/*-----------------9/13/2004 10:55AM----------------
 * Program clock for 7710
 * --------------------------------------------------*/
#if defined (ST_7710)

void Stop_PLL_Rej(void){
  /* stop clock by setting PLL_CONFIG_2.poff to 1 */
  STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS+PLL2_CONFIG_2,0x5AA50006);
}

void Change_Freq_PixHD_CLK(U32 sdiv, U32 md, U32 pe){
  U32 data;
  /* Stop clock */
  data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data | 0x00000008 ;   /* clk_pixhd is switched off  */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS,data);
  /* Change parameters and set en_prog to '0' */
  data = STSYS_ReadRegDev32LE(FS_04_CONFIG_1+CKG_BASE_ADDRESS);
  /* pe */
  data = (U32)pe & 0x0000FFFF ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_04_CONFIG_1+CKG_BASE_ADDRESS,data);
  data = STSYS_ReadRegDev32LE(FS_04_CONFIG_0+CKG_BASE_ADDRESS);
  /*  md,sdiv,sel_out=1  ,reset_n=1  ,en_prog=0 */
  data = 0x00000006 ; /* sel_out=1  ,reset_n=1 , en_prog=0  */
  data = ( (U32)(((U32)md<<6)|((U32)sdiv<<3)) & 0x0000FFF8 ) | data ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_04_CONFIG_0+CKG_BASE_ADDRESS,data);
  /* en_prog = 1 */
  data = data | 0x00000001 ;
  STSYS_WriteRegDev32LE(FS_04_CONFIG_0+CKG_BASE_ADDRESS,data);
  /* en_prog = 0 */
  data = data & 0xFFFFFFFE ;
  STSYS_WriteRegDev32LE(FS_04_CONFIG_0+CKG_BASE_ADDRESS,data);
  /* Enable clock */
  data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data & 0xFFFFFFF7 ;   /* clk_pixhd is switched on   */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS,data);
}

/* PLL2 : rejection PLL for HDMI analog block  */
void Start_PLL_Rej(void){
 /*  U32 data;*/
  /* start PLL1 by setting PLL_CONFIG_2.poff to 0  fbenable=0 rstn=1 */
  STSYS_WriteRegDev32LE(PLL2_CONFIG_2+CKG_BASE_ADDRESS,0x5AA50004);
}

void Change_Freq_Disp_CLK(U32 sdiv, U32 md, U32 pe){
  U32 data;
  /* Stop clock */
  data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data | 0x00000200 ;   /* clk_dispclk is switched off  */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS,data);
  /* Change parameters and set en_prog to '0' */
  data = STSYS_ReadRegDev32LE(FS_11_CONFIG_1+CKG_BASE_ADDRESS);
  /* pe */
  data = (U32)pe & 0x0000FFFF ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_11_CONFIG_1+CKG_BASE_ADDRESS,data);
  data = STSYS_ReadRegDev32LE(FS_11_CONFIG_0+CKG_BASE_ADDRESS);
  /*  md,sdiv,sel_out=1  ,reset_n=1  ,en_prog=0 */
  data = 0x00000006 ; /* sel_out=1  ,reset_n=1 , en_prog=0  */
  data = ( (U32)(((U32)md<<6)|((U32)sdiv<<3)) & 0x0000FFF8 ) | data ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_11_CONFIG_0+CKG_BASE_ADDRESS,data);
  /* en_prog = 1 */
  data = data | 0x00000001 ;
  STSYS_WriteRegDev32LE(FS_11_CONFIG_0+CKG_BASE_ADDRESS,data);
  /* en_prog = 0 */
  data = data & 0xFFFFFFFE ;
  STSYS_WriteRegDev32LE(FS_11_CONFIG_0+CKG_BASE_ADDRESS,data);
  /* Enable clock */
  data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data & 0xFFFFFDFF ;   /* clk_pixsd is switched on   */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS,data);
}

void Change_Freq_Pipe_CLK(U32 sdiv, U32 md, U32 pe){
  U32 data;
  /* Stop clock */
  data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data | 0x00000800 ;   /* clk_pipe    is switched off  */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS,data);
  /* Change parameters and set en_prog to '0' */
  data = STSYS_ReadRegDev32LE(FS_12_CONFIG_1+CKG_BASE_ADDRESS);
  /* pe */
  data = (U32)pe & 0x0000FFFF ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_12_CONFIG_1+CKG_BASE_ADDRESS,data);
  data = STSYS_ReadRegDev32LE(FS_12_CONFIG_0+CKG_BASE_ADDRESS);
  /*  md,sdiv,sel_out=1  ,reset_n=1  ,en_prog=0 */
  data =  0x00000006 ; /* sel_out=1  ,reset_n=1 , en_prog=0  */
  data = ( (U32)(((U32)md<<6)|((U32)sdiv<<3)) & 0x0000FFF8 ) | data ;
  data = data | 0x5AA50000 ;   /* key for registers access */
  STSYS_WriteRegDev32LE(FS_12_CONFIG_0+CKG_BASE_ADDRESS,data);
  /* en_prog = 1 */
  data = data | 0x00000001 ;
  STSYS_WriteRegDev32LE(FS_12_CONFIG_0+CKG_BASE_ADDRESS,data);
  /* en_prog = 0 */
  data = data & 0xFFFFFFFE ;
  STSYS_WriteRegDev32LE(FS_12_CONFIG_0+CKG_BASE_ADDRESS,data);
  /* Enable clock */
  data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS);
  data = data | 0x5AA50000 ;   /* key for registers access */
  data = data & 0xFFFFF7FF ;   /* clk_pipe  is switched on   */
  STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_0+CKG_BASE_ADDRESS,data);
}

#endif /* ST_7710 */

/*-----------------9/13/2004 10:55AM----------------
 * End programming clock for STi7710
 * --------------------------------------------------*/


#if defined(ST_7015)|| defined(ST_7020)|| defined(ST_5528)|| defined(ST_5100)|| defined(ST_7710)|| defined(ST_8010)\
 || defined(ST_5105)|| defined(ST_5301)|| defined(ST_7100)|| defined(ST_7109)|| defined(ST_5188)|| defined(ST_5525) \
 || defined(ST_5107)
/*****************************************************************************
Name        : ConfigSynth
Description : Configures a synthesizer
Parameters  : Synth           : Addressed synthesizer
              OutputClockFreq : Required output clock frequency
Assumptions :
Limitations :
Returns     : None
*****************************************************************************/
static void ConfigSynth(const Clock_t Synth, const U32 OutputClockFreq)
{
    #if !(defined(ST_7710)||defined(ST_7100)|| defined(ST_7109))
        U32 par,ndiv, sdiv, md, pe;
    #elif defined(ST_7710)
        U32 data;
    #endif
    #if !defined(ST_5100) && !defined(ST_7710) && !defined(ST_5105) && !defined(ST_5301) && !defined(ST_8010) \
     && !defined(ST_5188) && !defined(ST_5525) && !defined(ST_5107) && !defined(ST_7100) && !defined(ST_7109)
        U8 CkgSynPar =0;
    #endif

#if defined (ST_7015) || defined(ST_7020)
 U8 CkgSynRef =0;
 U32 en, ssl;

    switch (Synth)
    {
        case SYNTH3:
            CkgSynPar = CKG_SYN3_PAR;
            CkgSynRef = CKG_SYN3_REF;
            break;
#ifdef WA_GNBvd06791
        case SYNTH6:
            CkgSynPar = CKG_SYN6_PAR;
            break;
#endif
        case SYNTH7:
            CkgSynPar = CKG_SYN7_PAR;
            CkgSynRef = CKG_SYN7_REF;
            break;
        default:
            /* checked cannot be possible */
            break;
    }

    /* Read current parameters */
    par = STSYS_ReadRegDev32LE(CKG_BASE_ADDRESS + CkgSynPar);
    pe   =  par & CKG_SYN_PAR_PE_MASK;
    md   = (par & CKG_SYN_PAR_MD_MASK)   >> 16;
    sdiv = (par & CKG_SYN_PAR_SDIV_MASK) >> 21;
    ndiv = (par & CKG_SYN_PAR_NDIV_MASK) >> 24;
    en   = (par & CKG_SYN_PAR_EN)        >> 27;
    ssl  = (par & CKG_SYN_PAR_SSL)       >> 26;

    /* Select synthesizer */
    ssl = 1;
    /* Enable synthesizer */
    en = 1;

    /* Reference clock is clock reference 1 (RCLK1) */
    if ( (Synth == SYNTH3) || (Synth == SYNTH7))
    {
        STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CkgSynRef, 0);
    }

    /* Get synthesizer parameters */
    GetSynthParam(OutputClockFreq, &ndiv, &sdiv, &md, &pe);

    /* Add synthesizer parameters to register */
    par = (en  << 27)
        | (ssl << 26)
        | (ndiv << 24)
        | (sdiv << 21)
        | (md << 16)
        |  pe;

    /* Refresh parameters */
    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CkgSynPar, par);

#elif defined (ST_5528)
  /* unlock access to clock registers  */
  STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKG_LOCK_REG, CKG_UNLOCK);

  switch (Synth)
    {
        case QSYNTH1_CLK1 : /* no break */
        case QSYNTH1_CLK2 : /* no break */
        case QSYNTH2_CLK1 : /* no break */
        case QSYNTH2_CLK2 : /* no break */
        case QSYNTH2_CLK3 :
            break;
        case QSYNTH2_CLK4 :
             CkgSynPar = CKG_SYN2_CLK4;
             /* pixel clock (2x) is connected to QSYNTH2_clk4*/
             STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + PIX_CLK_CTRL, PIX_CLK_CTRL4);

        default :
            break;
    }

    /* Read current parameters For 5528*/
    par = STSYS_ReadRegDev32LE(CKG_BASE_ADDRESS + CKG_SYN2_CLK4);
    par &= ~CKG_CLK_STOP;

    pe   =  par & CKG_SYN2_PE_MASK;
    md   = (par & CKG_SYN2_MD_MASK)   >> 16;
    sdiv = (par & CKG_SYN2_SDIV_MASK) >> 21;
    ndiv = (par & CKG_SYN_RST_MASK)   >> 24;
   /* Get synthesizer parameters For 5528*/
    GetSynthParam(OutputClockFreq,&ndiv, &sdiv, &md, &pe);

    /* Add synthesizer parameters to register For 5528 */
    par = (ndiv << 24)
        | (sdiv << 21)
        | (md << 16)
        |  pe;

    /* Refresh parameters for qsynth2 on 5528*/
    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CkgSynPar, par);
    /* lock access to clock registers  */
    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKG_LOCK_REG, CKG_LOCK);


#elif defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188)|| defined(ST_5525) \
   || defined(ST_5107)
#if !defined(ST_OSLINUX)
if (Synth == SYNTH_CLK)                 /* for STi5100 */
 {
    #if !defined (ST_5525)
    /** Unlock clock generator register **/
      STSYS_WriteRegDev32LE(RGST_LK_CFG, RGST_LK_CFG_UNLK);
      STSYS_WriteRegDev32LE(RGST_LK_CFG, 0x0);
    #endif
    /** dco regsiter **/
    STSYS_WriteRegDev32LE(DCO_MODE_CFG, DCO_MODE_CFG_PRG);

    /* Get synthesizer parameters  */
    ComputeFrequencySynthesizerParameters(DEFAULT_PIX_CLK, OutputClockFreq , &ndiv , &sdiv , &md , &pe);

    /* Add synthesizer parameters to register CLK_SETUP0 */
    par = (0xe00 |(sdiv)<<6 | md |(0x1<<5) |(0x1<<9) );
    STSYS_WriteRegDev32LE(CKG_PIX_CLK_SETUP0, par );

    /* Add synthesizer parameters to register CLK_SETUP1 */
    STSYS_WriteRegDev32LE(CKG_PIX_CLK_SETUP1, pe);
    #if !defined (ST_5525)
    /* Lock clock generator register */
    STSYS_WriteRegDev32LE(RGST_LK_CFG, RGST_LK_CFG_UNLK);
    #endif
 }
#endif

#elif defined(ST_7710)
     UNUSED_PARAMETER(OutputClockFreq);
switch (Synth)
    {
        case FSY1_1 : /* SD pixel clock */
             Stop_PLL_Rej();
             Change_Freq_PixHD_CLK( 0x01, 0x0F, 0x0000);
             Start_PLL_Rej();
             /* FS11 = 27Mhz */
             Change_Freq_Disp_CLK(  0x03, 0x0F, 0x0000);
             /* clk_disp_hd = FS04 / 8 = 13.5 (from HD) */
             /* => data[1:0] = 11 */
             /* clk_comp = FS04 / 8 = 13.5 (from HD)  */
             /* => data[4] = 0 */
             /* clk_pixel_sd =  FS04 / 4 = 27 (from HD) */
             /* => data[5] =  1 */
             /* clk_hdmi = FS04 /4 = 27  (from HD)  */
             /* => data[2] =  1 */
             /* clk_656   = FS04 /4 = 27 */
             /* => data[3] = 1 */

             data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_2+CKG_BASE_ADDRESS);

             data = data & 0xFFC0 ;
             data = data | 0x2F   ; /* 10_1111 */
             data = data | 0x5AA50000 ;
             STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKG_BASE_ADDRESS,data);

            break;
        case FSY0_4 : /* HD pixel clock */
          Stop_PLL_Rej();
          Change_Freq_PixHD_CLK( 0x0, 0x17, 0x5D17);
          Start_PLL_Rej();
		 /* FS11 = 27Mhz */
		 /*
		  Real output frequency =  27000000 Hz
		  NDIV[1:0]  -> ndiv(hex)    =	0         SELF27     =	  1
		    MD[4:0]  -> md(hex)      =	F
		   PE[15:0]  -> pe(hex)      =	0
		   SDIV[2:0]  -> sdiv(hex)    =	3
		 */
         Change_Freq_Disp_CLK(  0x03, 0x0F, 0x0000);
		 /* clk_disp_hd = FS04 /2 = 74.25 */
		 /* => data[1:0] = 01 */
		 /* clk_comp = FS04 /2 = 74.25  */
		 /* => data[4] = 0 */
		 /* clk_pixel_sd =  FS11 = 27 (from SD )*/
		 /* => data[5] = 0 */
		 /* clk_hdmi = FS04 /2 = 74.25  */
		 /* => data[2] = 0 */
		 /* clk_656   = not used */
		 /* => data[3] = dont care */
         data = STSYS_ReadRegDev32LE(FS_CLOCKGEN_CFG_2+CKG_BASE_ADDRESS);
		 data = data & 0xFFC0 ;
		 data = data | 0x01   ;
		 data = data | 0x5AA50000 ;
         STSYS_WriteRegDev32LE(FS_CLOCKGEN_CFG_2+CKG_BASE_ADDRESS,data);
		 /* to program the clock_pipe = 148.5 MHz */
         Change_Freq_Pipe_CLK( 0x00, 0x17, 0x5D17);

             break;
        default :
            break;
    }
#elif defined (ST_7100) || defined (ST_7109)
    UNUSED_PARAMETER(OutputClockFreq);
    switch (Synth)
    {
        case FSY1_1 : /* SD pixel clock */
            /*ST_Ckg_SetFSClock( FSY1_1, 27000000);*/
            break;
        case FSY0_4 : /* HD pixel clock */
             /*ST_Ckg_SetFSClock( FSY0_4, 148500000);*/
             break;

        default :
            break;
    }
#endif
} /* End of ConfigSynth() function */

#if !(defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_7100)\
    ||defined(ST_7109)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107))
/*****************************************************************************
Name        : GetSynthParam
Description : Get synthesizer parameters from table
Parameters  : Fout:  required output frequency
              ndiv / sdiv / md / pe: synthesizer parameter bits
Assumptions : required output frequency is known in FreqSynthElements table
Limitations :
Returns     : none
*****************************************************************************/
static void GetSynthParam(const U32 Fout, U32 * const ndiv, U32 * const sdiv, U32 * const md, U32 * const pe)
{
    U8 k;
    U32 ipe1 = 0;
    U32 sp   = 0;
    U32 md2  = 0;

    for(k=0; k<sizeof(FreqSynthElements)/sizeof(FreqSynthElements_t); k++)
    {
        if (FreqSynthElements[k].Fout == Fout)
        {
            sp = FreqSynthElements[k].sdiv;
            md2 = FreqSynthElements[k].md;
            ipe1 = FreqSynthElements[k].pe;
            break;
        }
    }
#if defined (ST_7015)|| defined (ST_7020)
    *ndiv = (1    & 0x3);
#endif
    *sdiv = (sp   & 0x7);
    *md   = (md2  & 0x1f);
    *pe   = (ipe1 & 0xffff);
} /* End of GetSynthParam() function */

#endif

/******************************************************************************
Name        : SetPixelClock
Description : Set Pixel Clock
Parameters  : TimingMode : timing mode to program
Assumptions : Parameters are OK
Limitations :
Returns     : None
*****************************************************************************/
static void SetPixelClock(const VtgSimTimingMode_t TimingMode, const U8 VtgNb)
{
    U32 PixClk;

    /* Warning : on STi7015/20 and on 5528, VTG input clocks are half programmed pixel clocks */
    /* so multiplicate by 2 programmed pixel clocks to get right VTG clocks */
    PixClk = ModeParamsTable[TimingMode].PixelClock * 2;

#if defined (ST_7020)|| defined(ST_7015)
    /* Configure synthesizers */
    if (VtgNb == 1)
    {
        if (PixClk == LastPixClk) /* avoid useless re-programming */
        {
            return;
        }
        LastPixClk = PixClk;
#ifdef USE_ICS9161
        ICS9161SetPixClkFrequency(PixClk);
#else
        ConfigSynth(SYNTH3, PixClk);
#endif
    }
    else
    {
        if (PixClk == LastAuxClk) /* avoid useless re-programming */
        {
            return;
        }
        LastAuxClk = PixClk;
        ConfigSynth(SYNTH7, PixClk);
    }
#ifdef WA_GNBvd06791
    if (PixClk < MIN_PIXCLK_FOR_DISPCLK)
    {
        ConfigSynth(SYNTH6, DISP_CLOCK_FOR_PIXCLK24);
    }
    else
    {
        ConfigSynth(SYNTH6, (MIN_PIXCLK_FOR_DISPCLK * 3));
    }
#endif  /* WA_GNBvd06791 */

#elif defined (ST_7710)|| defined(ST_7100) || defined (ST_7109)
    /* Configure synthesizers */
    if (VtgNb == 1)
    {
        if (PixClk == LastAuxClk) /* avoid useless re-programming */
        {
            return;
        }

        LastAuxClk = PixClk;
        ConfigSynth(FSY0_4, PixClk);
    }
    else
    {
        if (PixClk == LastPixClk ) /* avoid useless re-programming */
        {
            return;
        }

        LastPixClk = PixClk;
        ConfigSynth(FSY1_1, PixClk);
    }
#elif defined (ST_5528)
     ConfigSynth(QSYNTH2_CLK4,PixClk);
#elif defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188)|| defined(ST_5525)\
   || defined(ST_5107)
   ConfigSynth(SYNTH_CLK,PixClk);
#endif

} /* End of SetPixelClock() function */
#endif /* ST_7015 || ST_7020 || ST_5528 || ST_7710 || ST_7100 */


/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

/*******************************************************************************
Name        : VtgSimInit
Description : Initialization of VTG cells
Parameters  :
Assumptions : parameters are OK
Limitations :
Returns     : none
*******************************************************************************/
void VtgSimInit( const STDENC_Handle_t    DencHnd,
                 const VtgSimDeviceType_t DeviceType)
{
    U8 OrMaskCfg0 =0 ,OrMaskCfg4 = 0 ,AndMaskCfg0 =0;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    if ( (DeviceType == VTGSIM_DEVICE_TYPE_1)
         || (DeviceType == VTGSIM_DEVICE_TYPE_2))
    {
      /* set HSync polarity positive */
      OrMaskCfg0 = DENC_CFG0_HSYNC_POL;
      OrMaskCfg4 = DENC_CFG4_SYIN_P1;
    }

    AndMaskCfg0 |= ~DENC_CFG0_HSYNC_POL;
    STDENC_RegAccessLock(DencHnd);
    ErrorCode = STDENC_MaskReg8(DencHnd, DENC_CFG0, AndMaskCfg0, OrMaskCfg0);
    if (ErrorCode == ST_NO_ERROR) ErrorCode = STDENC_MaskReg8(DencHnd, DENC_CFG4, DENC_CFG4_MASK_SYIN, OrMaskCfg4);
    if (ErrorCode == ST_NO_ERROR) ErrorCode = STDENC_OrReg8(DencHnd, DENC_CFG6, DENC_CFG6_RST_SOFT); /* DENC software reset */
    STDENC_RegAccessUnlock(DencHnd);

#if defined (ST_OS21) && defined (ST_5528)
    if ( !FirstInitDone)
    {
        STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKG_LOCK_REG,CKG_UNLOCK);
        STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + PWM_CLK_CTRL, ~PWM_CLK_STOP);
        STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKG_LOCK_REG, CKG_LOCK);
    }
#endif

#if defined (ST_7015) || defined(ST_7020)
    /* First time: initialize clock generator */
    if ( !FirstInitDone)
    {
        /* Configure synthesizers
         * Association :
         * SYNTH3 -> PIX_CLK
         * SYNTH7 -> DENC_CLK
         */

#ifdef USE_ICS9161
        ICS9161SetPixClkFrequency(DEFAULT_PIX_CLK);
#else
        ConfigSynth(SYNTH3, DEFAULT_PIX_CLK);
#endif
#ifdef WA_GNBvd06791
        ConfigSynth(SYNTH6, (MIN_PIXCLK_FOR_DISPCLK * 3));
#endif /* WA_GNBvd06791 */
        ConfigSynth(SYNTH7, DEFAULT_DENC_CLK);

        LastPixClk = DEFAULT_PIX_CLK;
        LastAuxClk = DEFAULT_DENC_CLK;

        FirstInitDone = TRUE;
    }
#endif /* ST_7015 || ST_7020*/

#if defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
    if ( !FirstInitDone)
    {
        ConfigSynth(FSY0_4, DEFAULT_PIX_CLK);
        ConfigSynth(FSY1_1, DEFAULT_DENC_CLK);

        LastPixClk = DEFAULT_PIX_CLK;
        LastAuxClk = DEFAULT_DENC_CLK;
        FirstInitDone = TRUE;
    }
#endif /* ST_7710 || ST_7100 || ST_7109 */



} /* end of VtgSimInit() */

/*******************************************************************************
Name        : VtgSimSetConfiguration
Description : Basic configuration of timing modes of VTG cells
Parameters  : DencHnd : Handle to open STDENC device
 *            TimingMode : PAL, NTSC, ...
Assumptions : all parameters are OK
Limitations : numerous compared to STVTG driver !!!
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
BOOL VtgSimSetConfiguration(   const STDENC_Handle_t     DencHnd
                             , const VtgSimDeviceType_t  DeviceType
                             , const VtgSimTimingMode_t  TimingMode
                             , const U8                  VtgNb)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U8 OrMaskCfg0, OrMaskCfg4;
    U32 VtgBaseAddress;
#if defined(ST_7015)|| defined(ST_7020)|| defined(ST_GX1) || defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105)\
 || defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)
    U32 VtgMod;
#endif /* ST_7015 || ST_7020 || ST_GX1 || ST_5528 */
#if defined (ST_7015) || defined(ST_7020)
    U32 VSyncRising, VSyncFalling;
    S32 SignedVSyncFallingEdgePosition;
#endif /* ST_7015 || ST_7020 */
#if defined(ST_GX1) || defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010) \
 || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)
    U32 Vdo, Hdo, VHdo;
#endif /* GX1 || 5528 ||5100*/



    if (TimingMode >= VTGSIM_TIMING_MODE_COUNT) return (TRUE);

    if (DeviceType == VTGSIM_DEVICE_TYPE_0) /* not 7015/7020/GX1/5528, where DENC is always slave */
    {
        /* DENC configuration */
        ErrorCode = STDENC_CheckHandle(DencHnd);
        if (ErrorCode != ST_NO_ERROR) return (TRUE);


        /* default:  HSync and Vsync polarity negative.  */
        OrMaskCfg0 = 0;
        OrMaskCfg4 = DENC_CFG4_SYIN_P1;
        if (TimingMode == VTGSIM_TIMING_MODE_SLAVE)
        {
#if defined (mb376) || defined (espresso) /* Assume external denc is an STi4629 */
            /* Frame + HSYNC based slave mode(line l)  */
            OrMaskCfg0 = DENC_CFG0_ODHS_SLV |DENC_CFG0_HSYNC_POL;
            OrMaskCfg4 = DENC_CFG4_ALINE;
#else
            /* Slave H + V, of VSync */
            OrMaskCfg0 |= DENC_CFG0_VSHS_SLV;
            /* let default: no output synchro signal, no free run */

            /* DENC slave: mode currently used only with STV0119 slave, on mb295 board */
            /* in this case, remove Synchro in delay. */
            OrMaskCfg4 = 0;
#endif  /* mb376 || espresso*/
        }
        else /* Mode is a master one  */
        {
            #if defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188)||defined(ST_5525)\
             || defined(ST_5107)
            OrMaskCfg0 = DENC_CFG0_FRHS_SLV;
            OrMaskCfg4 = DENC_CFG4_ALINE;
            #endif
            OrMaskCfg0 |= DENC_CFG0_MASTER;
        } /* end of if (Mode == VTGSIM_TIMING_MODE_SLAVE) */
        STDENC_RegAccessLock(DencHnd);
        ErrorCode = STDENC_MaskReg8(DencHnd, DENC_CFG0, (U8)~DENC_CFG0_MASK_STD, OrMaskCfg0);
       if (ErrorCode == ST_NO_ERROR) ErrorCode = STDENC_MaskReg8(DencHnd, DENC_CFG4, DENC_CFG4_MASK_SYIN, OrMaskCfg4);
       if (ErrorCode == ST_NO_ERROR) ErrorCode = STDENC_OrReg8(DencHnd, DENC_CFG6, DENC_CFG6_RST_SOFT); /* DENC software reset */

        STDENC_RegAccessUnlock(DencHnd);
    }

    /* VTG cell (not DENC one) configuration */

#if defined (ST_7015) || defined(ST_7020)
    if (DeviceType == VTGSIM_DEVICE_TYPE_2)
    {
        switch (VtgNb)
        {
            case 1 :
                if (TimingMode == LastVtg1TimingMode) /* avoid useless re-programming */
                {
                    return(FALSE);
                }
                LastVtg1TimingMode = TimingMode;
                VtgBaseAddress = (U32)VTG1_BASE_ADDRESS;
                break;
            case 2 :
                if (TimingMode == LastVtg2TimingMode) /* avoid useless re-programming */
                {
                    return(FALSE);
                }
                LastVtg2TimingMode = TimingMode;
                VtgBaseAddress = (U32)VTG2_BASE_ADDRESS;

                break;
            default :
                return(TRUE);
                break;
        }

        /* so we are on a STi7015/20, so program STi7015/20 VTG !*/
        VtgMod =  vtgsim_Read32((VtgBaseAddress + VTG_VTGMOD))
                & (U32)~VTG_VTGMOD_SLAVEMODE_MASK
                & (U32)~VTG_VTGMOD_HVSYNC_CMD_MASK;

        vtgsim_Write32((VtgBaseAddress + VTG_HVDRIVE), VTG_HVDRIVE_VSYNC | VTG_HVDRIVE_HSYNC);
        VtgMod |= VTG_VTGMOD_SLAVE_MODE_INTERNAL;
        vtgsim_Write32((VtgBaseAddress + VTG_CLKLN), ModeParamsTable[TimingMode].PixelsPerLine);
        vtgsim_Write32((VtgBaseAddress + VTG_HLFLN), ModeParamsTable[TimingMode].LinesByFrame);

        /* DDTS GNBvd09294 : active video must not end too close to next VSync */
        VSyncRising  = ModeParamsTable[TimingMode].LinesByFrame - (2 * BOTTOM_BLANKING_SIZE);
        SignedVSyncFallingEdgePosition = ModeParamsTable[TimingMode].VSyncPulseWidth - (2 * BOTTOM_BLANKING_SIZE);
        if (SignedVSyncFallingEdgePosition < 0)
        {
            SignedVSyncFallingEdgePosition += ModeParamsTable[TimingMode].LinesByFrame;
        }
        VSyncFalling = (U32)SignedVSyncFallingEdgePosition;

        vtgsim_Write32((VtgBaseAddress + VTG_VOS_HDO), ModeParamsTable[TimingMode].HSyncPulseWidth);
        vtgsim_Write32((VtgBaseAddress + VTG_VOS_HDS), 0);
        vtgsim_Write32((VtgBaseAddress + VTG_VOS_VDO), VSyncFalling);
        vtgsim_Write32((VtgBaseAddress + VTG_VOS_VDS), VSyncRising);

        vtgsim_Write32((VtgBaseAddress + VTG_VTGMOD), VtgMod);
        vtgsim_Write32((VtgBaseAddress + VTG_ITM), 0x18);
        vtgsim_Write32((VtgBaseAddress + VTG_VTIMER), 0);
        vtgsim_Write32((VtgBaseAddress + VTG_DRST), 1);
   }
#elif defined(ST_7710)|| defined (ST_7100) ||defined (ST_7109)
    if (DeviceType == VTGSIM_DEVICE_TYPE_2)
    {
        switch (VtgNb)
        {
            case 1 :
                if (TimingMode == LastVtg1TimingMode) /* avoid useless re-programming */
                {
                    return(FALSE);
                }
                LastVtg1TimingMode = TimingMode;
                VtgBaseAddress = (U32)VTG1_BASE_ADDRESS;
                break;
            case 2 :
                if (TimingMode == LastVtg2TimingMode) /* avoid useless re-programming */
                {
                    return(FALSE);
                }
                LastVtg2TimingMode = TimingMode;
                VtgBaseAddress = (U32)VTG2_BASE_ADDRESS;
                break;
            default :
                return(TRUE);
                break;
        }
            #if defined (ST_7710)
               #ifdef  STI7710_CUT2x
                vtgsim_Write32((VtgBaseAddress  + VTG_HVDRIVE), 0x0);
               #else
                vtgsim_Write32((VtgBaseAddress  + VTG_HVDRIVE),0x10000);
               #endif
                vtgsim_Write32((VtgBaseAddress  + VTG_CLKLN), ModeParamsTable[TimingMode].PixelsPerLine);

                vtgsim_Write32((VtgBaseAddress  + VTG_HLFLN),ModeParamsTable[TimingMode].LinesByFrame);

                vtgsim_Write32((VtgBaseAddress  + VTG_VOS_HDO), ModeParamsTable[TimingMode].HSyncPulseWidth);
                vtgsim_Write32((VtgBaseAddress  + VTG_VOS_HDS), 0x0);
                vtgsim_Write32((VtgBaseAddress  + VTG_VOS_VDO), ModeParamsTable[TimingMode].VSyncPulseWidth);
                vtgsim_Write32((VtgBaseAddress  + VTG_VOS_VDS), 0x0);

                vtgsim_Write32((VtgBaseAddress  + VTG_VTGMOD), VTG_MOD_FI);
                if (VtgNb == 1)
                {
                    vtgsim_Write32((VtgBaseAddress + VTG_ITM), 0x6);
                }
                else
                {
                        vtgsim_Write32((VtgBaseAddress + VTG_ITM + VTG2_IT_OFFSET), 0x6);
                }

                vtgsim_Write32((VtgBaseAddress  + VTG_VTIMER), 0x0);
                vtgsim_Write32((VtgBaseAddress  + VTG_DRST), 0x1);
             #elif defined (ST_7100)||defined (ST_7109)

                if (TimingMode == VTGSIM_TIMING_MODE_576I50000_13500) /*PAL BDGHI*/
                {
                    vtgsim_Write32((VtgBaseAddress  + VTG_HVDRIVE), 0x0);

                    vtgsim_Write32((VtgBaseAddress  + VTG_CLKLN), 0x360);

                    vtgsim_Write32((VtgBaseAddress  + VTG_HLFLN),0x271);

                    vtgsim_Write32((VtgBaseAddress  + VTG_VOS_HDO), 0x41);
                    vtgsim_Write32((VtgBaseAddress  + VTG_VOS_HDS), 0x0);
                    vtgsim_Write32((VtgBaseAddress  + VTG_VOS_VDO), 0x06);
                    vtgsim_Write32((VtgBaseAddress  + VTG_VOS_VDS), 0x0);

                    vtgsim_Write32((VtgBaseAddress  + VTG_VTGMOD), 0x3);
                    if (VtgNb == 1)
                    {
                        vtgsim_Write32((VtgBaseAddress + VTG_ITM), 0x6);
                    }
                    else
                    {
                        /*vtgsim_Write32((VtgBaseAddress + VTG_ITM + VTG2_IT_OFFSET), 0x6);*/
                        vtgsim_Write32((VtgBaseAddress +(U32)0x64), 0x6);
                        vtgsim_Write32((VtgBaseAddress + VTG_RG1 ), 0xD7);
                        vtgsim_Write32((VtgBaseAddress + VTG_RG2 ), 0x284);
                    }

                    vtgsim_Write32((VtgBaseAddress  + VTG_VTIMER), 0x0);
                    vtgsim_Write32((VtgBaseAddress  + VTG_DRST), 0x1);
                }
                else if (TimingMode == VTGSIM_TIMING_MODE_480I59940_13500) /*NTSC*/
                {
                    vtgsim_Write32((VtgBaseAddress  + VTG_HVDRIVE), 0x0);

                    vtgsim_Write32((VtgBaseAddress  + VTG_CLKLN), 0x35A);

                    vtgsim_Write32((VtgBaseAddress  + VTG_HLFLN),0x20D);

                    vtgsim_Write32((VtgBaseAddress  + VTG_VOS_HDO), 0x41);
                    vtgsim_Write32((VtgBaseAddress  + VTG_VOS_HDS), 0x0);
                    vtgsim_Write32((VtgBaseAddress  + VTG_VOS_VDO), 0x06);
                    vtgsim_Write32((VtgBaseAddress  + VTG_VOS_VDS), 0x0);

                    vtgsim_Write32((VtgBaseAddress  + VTG_VTGMOD), 0x3);
                    if (VtgNb == 1)
                    {
                        vtgsim_Write32((VtgBaseAddress + VTG_ITM), 0x6);
                    }
                    else
                    {
                        /*vtgsim_Write32((VtgBaseAddress + VTG_ITM + VTG2_IT_OFFSET), 0x6);*/
                        vtgsim_Write32((VtgBaseAddress +(U32) 0x64), 0x6);
                        vtgsim_Write32((VtgBaseAddress + VTG_RG1 ), 0xD7);
                        vtgsim_Write32((VtgBaseAddress + VTG_RG2 ), 0x284);

                    }

                    vtgsim_Write32((VtgBaseAddress  + VTG_VTIMER), 0x0);
                    vtgsim_Write32((VtgBaseAddress  + VTG_DRST), 0x1);
                }
             #endif
    }

#endif /* ST_7710 || ST_7100 || ST_7109*/


#if defined (ST_7015) || defined(ST_7020) || defined(ST_7710)|| defined (ST_7100) ||defined (ST_7109)
            SetPixelClock(TimingMode, VtgNb);
#endif

#if defined(ST_GX1) || defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010) \
 || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)

    if (DeviceType == VTGSIM_DEVICE_TYPE_1)
    {
#if defined (ST_5525)
      switch (VtgNb)
        {
            case 1 :
                if (TimingMode == LastVtg1TimingMode) /* avoid useless re-programming */
                {
                    return(FALSE);
                }
                LastVtg1TimingMode = TimingMode;
                VtgBaseAddress = (U32)VTG_0_BASE_ADDRESS;
                break;
            case 2 :
                if (TimingMode == LastVtg2TimingMode) /* avoid useless re-programming */
                {
                    return(FALSE);
                }
                LastVtg2TimingMode = TimingMode;
                VtgBaseAddress = (U32)VTG_1_BASE_ADDRESS;
                break;
            default :
                return(TRUE);
                break;
        }
#elif  defined(ST_7200)
        switch (VtgNb)
        {
            case 1 :
                if (TimingMode == LastVtg1TimingMode) /* avoid useless re-programming */
                {
                    return(FALSE);
                }
                LastVtg1TimingMode = TimingMode;
                VtgBaseAddress = (U32)VTG_1_BASE_ADDRESS;
                break;
            case 2 :
                if (TimingMode == LastVtg2TimingMode) /* avoid useless re-programming */
                {
                    return(FALSE);
                }
                LastVtg2TimingMode = TimingMode;
                VtgBaseAddress = (U32)VTG_2_BASE_ADDRESS;
                break;
            case 3 :
                if (TimingMode == LastVtg3TimingMode) /* avoid useless re-programming */
                {
                    return(FALSE);
                }
                LastVtg3TimingMode = TimingMode;
                VtgBaseAddress = (U32)VTG_3_BASE_ADDRESS;
                break;
            default :
                return(TRUE);
                break;
        }

#else
        if (TimingMode == LastVtg1TimingMode) /* avoid useless re-programming */
        {
            return(FALSE);
        }
        LastVtg1TimingMode = TimingMode;
        VtgBaseAddress = VTG_BASE_ADDRESS ;
#endif
        #if defined(ST_5100)|| defined(ST_5528)|| defined(ST_5105)|| defined(ST_5301) || defined(ST_8010)|| defined(ST_5188) \
         || defined(ST_5525)|| defined(ST_5107)
               /** reset clcok system if clock was changed **/
               VtgSimRstConfiguration(DeviceType);
        #endif

  #if !defined(ST_7200)
    VtgMod =  vtgsim_Read32(((U32)VtgBaseAddress + VTG_VFE_MOD)) & (U32)~VTG_VFE_MOD_MASK;

     /* Set mode as master */
     VtgMod |= VTG_VFE_MOD_MODE_MASTER;
  #endif
#if defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188) \
 || defined(ST_5525)|| defined(ST_5107)
        VtgMod &= (U32)~VTG_VFE_MOD_DISABLE;
#endif
#if defined (ST_7200)
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_CLKLN), ModeParamsTable[TimingMode].PixelsPerLine);
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_HLFLN), ModeParamsTable[TimingMode].LinesByFrame);

        vtgsim_Write32(((U32)VtgBaseAddress + VTG_H_HD1), 0x20001);
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_TOP_V_VD1), 0x10001);
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_BOT_V_VD1), 0);
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_TOP_V_HD1), 0x20000);
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_BOT_V_HD1),0x1b00b1);
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_ITM)     , 0x18);
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_VTGMOD) , 0);
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_DRST) , 0);
#else
        /* set CLKLN to twice PixelsPerLine */
         vtgsim_Write32(((U32)VtgBaseAddress + VTG_VFE_CLKLN), 2*(ModeParamsTable[TimingMode].PixelsPerLine));

        vtgsim_Write32( ((U32)VtgBaseAddress + VTG_VFE_HLFLN), ModeParamsTable[TimingMode].LinesByFrame);
        Hdo = ModeParamsTable[TimingMode].HSyncPulseWidth;
        VHdo = Hdo + ModeParamsTable[TimingMode].PixelsPerLine;

#if defined (ST_5528)
        Vdo = ModeParamsTable[TimingMode].VSyncPulseWidth + VTG_TOP_V_VD1;
        vtgsim_Write32( ((U32)VtgBaseAddress  + VTG_VFE_H_HD)    , Hdo);
        vtgsim_Write32( ((U32)VtgBaseAddress  + VTG_VFE_TOP_V_VD), Vdo);
        vtgsim_Write32( ((U32)VtgBaseAddress  + VTG_VFE_BOT_V_VD), (VTG_BOT_V_VD1 & VTG_VFE_BOT_V_VD0_MASK));
        vtgsim_Write32( ((U32)VtgBaseAddress  + VTG_VFE_TOP_V_HD), VTG_TOP_V_HD1);
        vtgsim_Write32( ((U32)VtgBaseAddress  + VTG_VFE_BOT_V_HD), (VTG_BOT_V_HD1 & VTG_VFE_BOT_V_HD0_MASK));
        vtgsim_Write32( ((U32)VtgBaseAddress  + VTG_VFE_MOD) , VtgMod);
        vtgsim_Write32( ((U32)VtgBaseAddress  + VTG_ITM)     , 0x18);
        vtgsim_Write32( ((U32)VtgBaseAddress  + VTG_VFE_SRST), 1);

#else

        Vdo = ModeParamsTable[TimingMode].VSyncPulseWidth;
        vtgsim_Write32( ((U32)VtgBaseAddress + VTG_VFE_H_HD)    , Hdo & VTG_VFE_H_HDO_MASK);
        vtgsim_Write32( ((U32)VtgBaseAddress + VTG_VFE_TOP_V_VD), (Vdo  & VTG_VFE_TOP_V_VD0_MASK | (1<<16)  & VTG_VFE_TOP_V_VDS_MASK));
        vtgsim_Write32( ((U32)VtgBaseAddress + VTG_VFE_BOT_V_VD), Vdo & VTG_VFE_BOT_V_VD0_MASK );
        vtgsim_Write32( ((U32)VtgBaseAddress + VTG_VFE_TOP_V_HD), 0);
        #if !(defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_8010)|| defined (ST_5188)\
           || defined (ST_5525) || defined (ST_5107))
            vtgsim_Write32( ((U32)VtgBaseAddress + VTG_VFE_BOT_V_HD), (VHdo & VTG_VFE_BOT_V_HD0_MASK));
        #endif
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_VFE_MOD) , VtgMod);
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_ITM)     , 0x18);
        vtgsim_Write32(((U32)VtgBaseAddress + VTG_VFE_SRST), 1);
#endif
#endif /*7200*/
#if defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188) \
 || defined(ST_5525)|| defined(ST_5107)
if ((TimingMode==VTGSIM_TIMING_MODE_480I59940_12273)||(TimingMode==VTGSIM_TIMING_MODE_576I50000_14750)||
             (TimingMode==VTGSIM_TIMING_MODE_480I60000_12285))

        {
           SetPixelClock(TimingMode, VtgNb);
        }

 #endif


    }
#endif /* GX1 || 5528 */

    return (ErrorCode != ST_NO_ERROR);
} /* end of VtgSimSetConfiguration() */

/*******************************************************************************
Name        : VtgSimRstConfiguration
Description : Reset square pixel configuration of timing modes
Parameters  : DencHnd : Handle to open STDENC device
 *            TimingMode : PAL, NTSC, ...
Assumptions : all parameters are OK
Limitations :
Returns     : none
*******************************************************************************/
void VtgSimRstConfiguration(const VtgSimDeviceType_t  DeviceType)
{
   switch (DeviceType)
   {

    case VTGSIM_DEVICE_TYPE_1 :
        #if defined(ST_5528)
            STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKG_LOCK_REG, CKG_UNLOCK);
            vtgsim_Write32(((U32)CKG_BASE_ADDRESS + CKG_SYN2_CLK4),CKG_SYN2_CLK4_RST);
            vtgsim_Write32 (((U32)CKG_BASE_ADDRESS + PIX_CLK_CTRL), PIX_CLK_RST);
            STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKG_LOCK_REG, CKG_LOCK);
        #elif defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188)|| defined(ST_5525) \
           || defined(ST_5107)
            #if !defined(ST_OSLINUX)
            vtgsim_Write32(CKG_PIX_CLK_SETUP0, PIX_CLK_SETUP0_RST);
            vtgsim_Write32(CKG_PIX_CLK_SETUP1, PIX_CLK_SETUP1_RST);
            vtgsim_Write32(DCO_MODE_CFG, ~DCO_MODE_CFG_PRG);
            #endif
       #endif
       break;
    case VTGSIM_DEVICE_TYPE_0 : /* no break */
    case VTGSIM_DEVICE_TYPE_2 :
        break;
    default :
        break;
   }
}

#if defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)|| defined(ST_5188)|| defined(ST_5525)\
 || defined(ST_5107)
#if !defined(ST_OSLINUX)
/*------------------------------------------------------------------------------
 Name    : ComputeFrequencySynthesizerParameters
 Purpose : Computes synthesizer parameters from input/output frequencies
 In      : InputFrequency   : input reference frequency (27 MHz)
           OutputFrequency  : required output frequency
 Out     : *ndiv, *sdiv, *md and *pe : synthesizer parameters
           returns false if no error occurs, otherwise true
 Note    : -
------------------------------------------------------------------------------*/
static BOOL ComputeFrequencySynthesizerParameters(const U32 InputFrequency,
                                                  const U32 OutputFrequency,
                                                     U32 * const ndiv1, U32 * const sdiv1,
                                                     U32 * const md1, U32 * const pe1)
{
  double      OutFrequency, InFrequency, foutr, fout2, fr,Tdl, Tr, Tdif,
              resol, diff;
  S32         k, m, n , s ,ipe1, ipe2, nd1, nd2,Ntap, sp, np, md2;

  OutFrequency = (double)OutputFrequency;
  InFrequency  = (double)InputFrequency;

  /* k = number of bits for compute ipe digital bus */
  k = 16;
  if (!((InFrequency == 13500000) || (InFrequency == 27000000) || (InFrequency == 54000000)) )

  {
    STTBX_Report((severity_info,
                   "Input frequency not allowed for this synthesizer !!\n"));
    STTBX_Report((severity_info,
    "Please enter an input frequency equal to 13.5MHz, 27MHz or 54MHz ...\n"));
    return(TRUE);
  }

  if ( ( (OutFrequency <= 843750) || (OutFrequency >= 216000000) ) &&
      (OutFrequency != 0) )
  {
    STTBX_Report((severity_info,
                   "Output frequency not allowed for this synthetizer !!\n"));
    STTBX_Report((severity_info,
                "Please enter frequency in interval ]843.75 kHz , 216 MHz[\n"));
    return(TRUE);
  }

    /* Ntap = Number of taps at interpolators exit */
    Ntap = 32;

    /* m = division rank of internal divider */
    m = 16;

    /* Divider N rank */
    n = (S32)(InFrequency / 13500000);

    /* fr = vco ring frequency */
    fr = (16*InFrequency / n);
    Tr = (1 / fr);
    Tdl = (1 / (Ntap*fr));

    /* divider s computation */
    if (OutFrequency >= 108000000)
    {
      s = 2;
      sp = 0;
    }
    else if (OutFrequency >= 54000000)
		{
      s = 4;
      sp = 1;
		}
    else if (OutFrequency >= 27000000)
		{
      s = 8;
      sp = 2;
		}
    else if (OutFrequency >= 13500000)
		{
      s = 16;
      sp = 3;
		}
    else if (OutFrequency >= 6750000)
		{
      s = 32;
      sp = 4;
		}
    else if (OutFrequency >= 3375000)
		{
      s = 64;
      sp = 5;
		}
    else if (OutFrequency >= 1687500)
		{
      s = 128;
      sp = 6;
		}
    else if (OutFrequency >= 843750)
		{
      s = 256;
      sp = 7;
		}

    Tdif = (1/fr) - (1/(s*OutFrequency));
    nd1 = (S32)floor(Tdif/Tdl);
    nd2 = nd1 + 1;
    ipe1 = (S32)(pow(2, (double)(k-1.0))*(Tdif - (double)nd1*Tdl)/Tdl);
    ipe2 = ipe1 - (S32)pow(2, (double)(k-1.0));
    foutr = OutFrequency;
    OutFrequency  = pow(2, (double)(k-1.0)) /
              (s*((double)ipe1*(Tr-(double)nd2*Tdl)-ipe2*(Tr-(double)nd1*Tdl)));
    fout2 = pow(2, (double)(k-1.0))/(s*(((double)ipe1+1.0)*
                (Tr-(double)nd2*Tdl)-((double)ipe2+1.0)*(Tr-(double)nd1*Tdl)));
    diff  = OutFrequency - foutr;
    resol = (fout2/OutFrequency -1)*1000000;

    switch (n)
    {
      case 1 :
        np = 0;
        break;
      case 2 :
        np = 1;
        break;
      case 4 :
        np = 2;
        break;
    }
     md2 = -nd2;
    *ndiv1 = (np & 0x3);
    *sdiv1 = (sp & 0x7);
    *md1   = (md2 & 0x1f);
    *pe1   = (ipe1 & 0xffff);

  return(FALSE);
}
#endif  /* LINUX */
#endif /* ST_5100 - ST_5105 - ST_5301 - ST_8010 */

/***  Programming clock registers for STi7100 ***/
#if defined (ST_7100) || defined (ST_7109)
#if !defined(ST_OSLINUX)
BOOL ST_Ckg_Init(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetOk = FALSE, Error;

    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

    /* Set FS0_out0 in HD mode */
    Error = ST_Ckg_SetFSClock( FSY0_4, 148500000);

    /* Set FS1_out0 in SD mode */
    Error = ST_Ckg_SetFSClock( FSY1_1, 27000000);

	/* hdmi clk at 74.25, pix_hdmi at 74.25, disp_hd at 74.25, 656 do not care, disp_sd at 13.5, pix_sd do not care */
    STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );

    STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_DISPLAY_CFG, 0x0 );

	/* for SD */
	STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS_SELECT, 0x0 );

    STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );
	return (RetOk);
}

 BOOL ST_Ckg_Term(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetOk = FALSE;

    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);
    STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );

	return (RetOk);
}

/* ========================================================================
   Name:        GetFSParameterFromFreq
   Description: Freq to Parameters

   ======================================================================== */
static boolean GetFSParameterFromFreq( int freq, int *md, int *pe, int *sdiv )
{
	double fr, Tr, Td1, Tx, fmx, nd1, nd2, Tdif;
    int NTAP, msdiv, mfmx, ndiv;

	NTAP = 32;
	mfmx = 0;

	ndiv = 1.0;

	fr = 240000000.0;
	Tr = 1.0 / fr;
	Td1 = 1.0 / (NTAP * fr);
	msdiv = 0;

	/* Recherche de SDIV */
	while (! ((mfmx >= 240000000) && (mfmx <= 480000000)) && (msdiv < 7))
	{
  		msdiv = msdiv + 1;
  		mfmx = pow(2,msdiv) * freq;
	}

	*sdiv = msdiv - 1;
	fmx = mfmx / 1000000.0;
	if ((fmx < 240) || (fmx > 480))
	{
	    return (TRUE);
	}

	Tx = 1 / (fmx * 1000000.0);

	Tdif = Tr - Tx;

	/* Recherche de MD */
	nd1 = floor((32.0 * (mfmx - fr) / mfmx));
	nd2 = nd1 + 1.0;

	*md = 32.0 - nd2;

	/* Recherche de PE */
	*pe = ceil((32.0 * (mfmx - fr) / mfmx - nd1) * 32768.0);

    return (FALSE);
}

/* ========================================================================
   Name:        ST_Ckg_SetFSClock
   Description: Set FS clock

   ======================================================================== */
BOOL  ST_Ckg_SetFSClock(Clock_t FS_sel, int freq)
{
	BOOL RetError = FALSE;

    int md, pe, sdiv;

    if ( (GetFSParameterFromFreq( freq, &md, &pe, &sdiv )) == TRUE )
	{
		RetError = TRUE;
	}
	else
	{
        if ( FS_sel == FSY1_1 )
		{
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_MD1, md );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_PE1, pe );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_SDIV1, sdiv );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_EN_PRG1, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );
/*          STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_MD1, 0x11 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_PE1,0x1cfe  );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_EN_PRG1, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_SDIV1, 0x1 );   */


		}

        if ( FS_sel == FSY0_4 )
		{
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_MD1, md );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_PE1, pe );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_SDIV1, sdiv );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_EN_PRG1, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );
/*          STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_MD1, 0x11 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_PE1, 0xc72 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_EN_PRG1, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_SDIV1, 0x3 );   */


		}
	}

	return (RetError);
}

#endif /** ST_OSLINUX **/
#endif /* ST_7100 | 7109 */
#if defined (ST_7200)
/* ========================================================================
   Name:        CKGB_GetClockConfig
   Description:

   ======================================================================== */

U32 CKGB_GetClockConfig(void)
{
    U32 ckg_glue_cfg,data;

	ckg_glue_cfg=0;

    data= STSYS_ReadRegDev32LE(VOS_HD_BASE_ADDRESS + TVO_GLUE_AUX_OFFSET + TVO_AUX_PADS);
    data= (data>>15)&0xFFFF;
	ckg_glue_cfg |= (data<<8);

    data= STSYS_ReadRegDev32LE(/*ST7200_HD_TVOUT_BASE_ADDRESS + TVO_HDMI_OFFSET*/VOS_HDMI_BASE_ADDRESS + HDMI_GPOUT);
    data= (data>>8)&0xFF;
	ckg_glue_cfg |= (data);

    return ckg_glue_cfg;

}
/* ========================================================================
   Name:        CKGB_UpdateClkCfg
   Description:

   ======================================================================== */


U32 CKGB_UpdateClkCfg(U32 cfg)
{
     U32 data;

    data= STSYS_ReadRegDev32LE(VOS_HD_BASE_ADDRESS + TVO_GLUE_AUX_OFFSET + TVO_AUX_PADS);
    data = data & (0x00007FFF);
	data = data | (((cfg & 0x1FFFF00)>>8)<<15);
    STSYS_WriteRegDev32LE( VOS_HD_BASE_ADDRESS + TVO_GLUE_AUX_OFFSET + TVO_AUX_PADS , data );

    data= STSYS_ReadRegDev32LE(VOS_HDMI_BASE_ADDRESS  + HDMI_GPOUT );
    data = data & 0xFFFF00FF ;
    data = data | ((cfg & 0xFF)<< 8);
    STSYS_WriteRegDev32LE(VOS_HDMI_BASE_ADDRESS + HDMI_GPOUT, data );
	return data;

}
/* ========================================================================
   Name:        GetFSParameterFromFreq
   Description: Freq to Parameters

   ======================================================================== */
boolean GetFSParameterFromFreq( int inputfreq, int outputfreq, int *md, int *pe, int *sdiv )
{
   double fr, Tr, Td1, Tx, fmx, nd1, nd2, Tdif;
    int NTAP;
    int msdiv, mfmx, ndiv;

	NTAP = 32;
	mfmx = 0;

	ndiv = 1.0;

	fr = inputfreq * 8000000.0;
	Tr = 1.0 / fr;
	Td1 = 1.0 / (NTAP * fr);
	msdiv = 0;

	/* Recherche de SDIV */
	while (! ((mfmx >= (inputfreq*8000000)) && (mfmx <= (inputfreq*16000000))) && (msdiv < 7))
	{
  		msdiv = msdiv + 1;
  		mfmx = pow(2,msdiv) * outputfreq;
	}

	*sdiv = msdiv - 1;
	fmx = mfmx / 1000000.0;
	if ((fmx < (8*inputfreq)) || (fmx > (16*inputfreq)))
	{
	    return (TRUE);
	}

	Tx = 1 / (fmx * 1000000.0);

	Tdif = Tr - Tx;

	/* Recherche de MD */
	nd1 = floor((32.0 * (mfmx - fr) / mfmx));
	nd2 = nd1 + 1.0;

	*md = 32.0 - nd2;

	/* Recherche de PE */
	*pe = ceil((32.0 * (mfmx - fr) / mfmx - nd1) * 32768.0);

/*    STTBX_Print(( "Programmation for QSYNTH C090_4FS216_25 with fout= %d Hz and fin= 30 MHz\n", freq));
    STTBX_Print(( "      MD[4:0]               ->       md      = 0x%x\n", *md));
    STTBX_Print(( "      PE[15:0]              ->       pe      = 0x%x\n", *pe));
    STTBX_Print(( "      SDIV[2:0]             ->       sdiv    = 0x%x\n", *sdiv));*/

    return (FALSE);
}
/* ========================================================================
   Name:        ST_Ckg_SetFSClock
   Description: Set FS clock

   ======================================================================== */
BOOL  ST_Ckg_SetFSClock(FS_sel_t FS_sel, int freq)
{
    U32 data;
	boolean RetError = FALSE;
	int md, pe, sdiv;

	if ( (FS_sel == FS0_out1) || (FS_sel == FS0_out2)|| (FS_sel == FS0_out3) || (FS_sel == FS0_out4) )
	{
		if ( (GetFSParameterFromFreq( refclockB, freq, &md, &pe, &sdiv )) == TRUE )
		{
			return TRUE;
		}
	}
	if ( (FS_sel == FS1_out1) || (FS_sel == FS1_out2) || (FS_sel == FS1_out3) ||(FS_sel == FS1_out4))
	{
		if ( (GetFSParameterFromFreq( refclockB, freq, &md, &pe, &sdiv )) == TRUE )
		{
			return TRUE;
		}
	}
	if ((FS_sel == FS2_out1) || (FS_sel == FS2_out2)|| (FS_sel == FS2_out3)||(FS_sel == FS2_out4))
	{
		if ( (GetFSParameterFromFreq( refclockB, freq, &md, &pe, &sdiv )) == TRUE )
		{
			return TRUE;
		}
	}

	data  = (0x1<<25);
	data |= (pe&0xffff) | ((md&0x1f)<<16) | ((sdiv&0x7)<<22) | (0x1<<21);


/*	STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );*/
	STSYS_WriteRegDev32LE( CKGB_BASE + 0x8 + 4*FS_sel,data);
/*	STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );*/

    return (RetError);

}
/* ========================================================================
   Name:        CKGB_SetClockDivider
   Description:

   ======================================================================== */


U32 CKGB_SetClockDivider (int clockid, int divider)
{
    U32 ckg_cfg,divid=0,hdtvo_cfg,hdtvo_auxcfg,sdtvo_cfg;

	ckg_cfg= CKGB_GetClockConfig();

    hdtvo_cfg= STSYS_ReadRegDev32LE( VOS_HD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_MAIN_CLK_SEL );
    hdtvo_auxcfg= STSYS_ReadRegDev32LE( VOS_HD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_AUX_CLK_SEL );
    sdtvo_cfg= STSYS_ReadRegDev32LE( VOS_SD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_MAIN_CLK_SEL );


	if(CkgB_Clocks[clockid].div_allow != 1 )
	{
        STTBX_Print(("CKGB_SetClockDivider: cannot set divider on clock %s \n",CkgB_Clocks[clockid].name));
        return(FALSE);
	}
	if(divider != 1 && divider !=2 && divider !=4 && divider !=8   )
	{
        STTBX_Print(("CKGB_SetClockDivider: Bad divider value :%d \n",divider));
        return(FALSE);
	}
	else
	{
		if (divider==1)
            {divid =0x0;}
		else if (divider==2)
            {divid=0x1;}
		else if (divider==4)
            {divid=0x2;}
		else if (divider==8)
            {divid=0x3;}
	}

	switch (clockid)
	{
		case CLK_OBELIX_AUX:
            ckg_cfg = (ckg_cfg & 0xfffff9ff )| divid<<9;
			CKGB_UpdateClkCfg(ckg_cfg) ;
			CkgB_Clocks[CLK_OBELIX_AUX].divider = divider;
			break;
		case CLK_OBELIX_FVP:
		case CLK_OBELIX_MAIN:
            ckg_cfg = (ckg_cfg & 0xfffffe7f) | divid<<7;
			CKGB_UpdateClkCfg(ckg_cfg) ;
			CkgB_Clocks[CLK_OBELIX_FVP].divider = divider;
			CkgB_Clocks[CLK_OBELIX_MAIN].divider = divider;
			break;
		case CLK_ASTERIX_AUX:
            ckg_cfg = (ckg_cfg & 0xffffe7ff) | divid<<11;
			CKGB_UpdateClkCfg(ckg_cfg) ;
			CkgB_Clocks[CLK_ASTERIX_AUX].divider = divider;
			break;
		case CLK_TMDS:
            ckg_cfg = (ckg_cfg & 0xffff9fff) | divid<<13;
			CKGB_UpdateClkCfg(ckg_cfg) ;
			CkgB_Clocks[CLK_TMDS].divider = divider;
			break;
		case CLK_CH34MOD:
            ckg_cfg = (ckg_cfg & 0xfffe7fff) | divid<<15;
			CKGB_UpdateClkCfg(ckg_cfg) ;
			CkgB_Clocks[CLK_CH34MOD].divider = divider;
			break;
		case CLK_HDTVO_PIX_MAIN:
            hdtvo_cfg = (hdtvo_cfg & ( ~(U32)(0x3))) | divid;
            STSYS_WriteRegDev32LE( VOS_HD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_MAIN_CLK_SEL , hdtvo_cfg );
			break;

		case CLK_HDTVO_TMDS_MAIN:
            hdtvo_cfg = (hdtvo_cfg & ( ~(U32)(0x3<<2))) | divid<<2;
            STSYS_WriteRegDev32LE( VOS_HD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_MAIN_CLK_SEL , hdtvo_cfg );
			break;

		case CLK_HDTVO_BCH_MAIN:
            hdtvo_cfg = (hdtvo_cfg & ( ~(U32)(0x3<<4))) | divid<<4;
            STSYS_WriteRegDev32LE( VOS_HD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_MAIN_CLK_SEL , hdtvo_cfg );
			break;

		case CLK_HDTVO_FDVO_MAIN:
            hdtvo_cfg = (hdtvo_cfg & ( ~(U32)(0x3<<6))) | divid<<6;
            STSYS_WriteRegDev32LE( VOS_HD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_MAIN_CLK_SEL , hdtvo_cfg );
			break;

		case CLK_HDTVO_PIX_AUX:
            hdtvo_auxcfg = (hdtvo_auxcfg & 0xfffffffc) | divid;
            STSYS_WriteRegDev32LE( VOS_HD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_AUX_CLK_SEL , hdtvo_auxcfg );
			break;

		case CLK_HDTVO_DENC:
            hdtvo_auxcfg = (hdtvo_auxcfg & 0xfffffff3) | (divid<<2);
            STSYS_WriteRegDev32LE( VOS_HD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_AUX_CLK_SEL , hdtvo_auxcfg );
			break;

		case CLK_SDTVO_PIX_MAIN:
            sdtvo_cfg = (sdtvo_cfg & 0xfffffffc) | (divid);
            STSYS_WriteRegDev32LE( VOS_SD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_MAIN_CLK_SEL , sdtvo_cfg );
			break;

        case CLK_SDTVO_DENC:
            sdtvo_cfg = (sdtvo_cfg & 0xffffffe3) | (divid << 2);
            STSYS_WriteRegDev32LE( VOS_SD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + TVO_MAIN_CLK_SEL , sdtvo_cfg );

			break;
		case CLK_SDTVO_FDVO:
            sdtvo_cfg = (sdtvo_cfg & 0xffffff9f) | (divid<<5);
            STSYS_WriteRegDev32LE(VOS_SD_BASE_ADDRESS  + TVO_GLUE_MAIN_OFFSET + TVO_MAIN_CLK_SEL , sdtvo_cfg );
			break;
        default:
            break;
    }

    return(TRUE);
}
/* ========================================================================
   Name:        CKGB_SetClockSource
   Description:

   ======================================================================== */
U32 CKGB_SetClockSource (int destclockid,int srcclockid)
{
    U32 ckg_cfg;

	if(CkgB_Clocks[destclockid].sources[0] != srcclockid &&  CkgB_Clocks[destclockid].sources[1] != srcclockid)
	{
        STTBX_Print(("CKGB_SetClockSource: %s cannot be set as %s source \n",CkgB_Clocks[srcclockid].name,CkgB_Clocks[destclockid].name));
        return (FALSE);
	}

	ckg_cfg= CKGB_GetClockConfig();

	/* change source mux */
	switch (destclockid)
	{
		case CLK_PIX_OBELIX:
			if (srcclockid == CLK_PIX_HD0)
		 {
				ckg_cfg |= (0x1<<1);
			}
			else
			{
			  ckg_cfg &= ~(U32)(0x1<<1);
			}
			break;
		case CLK_PIX_ASTERIX:
			if (srcclockid == CLK_PIX_HD0)
			{
				ckg_cfg |= 0x1;
			}
			else
			{
			  ckg_cfg &= ~(U32)0x1;
			}
			break;
	  case CLK_OBELIX_GDP:
			if (srcclockid == CLK_OBELIX_AUX)
			{
				ckg_cfg |= (0x1<<2);
			}
			else
			{
			  ckg_cfg &= ~(U32)(0x1<<2);
			}
			break;
	  case CLK_OBELIX_VDP:
			if (srcclockid == CLK_OBELIX_AUX)
			{
				ckg_cfg |= (0x1<<3);
			}
			else
			{
			  ckg_cfg &= ~(U32)(0x1<<3);
			}
			break;
	  case CLK_OBELIX_CAP:
			if (srcclockid == CLK_OBELIX_AUX)
			{
				ckg_cfg |= (0x1<<4);
			}
			else
			{
			  ckg_cfg &= ~(U32)(0x1<<4);
			}
			break;
		case CLK_LOC_SDTVO:
			if (srcclockid == CLK_PIX_HD0)
			{
				ckg_cfg |= (0x1<<5);
			}
			else
			{
			  ckg_cfg &= ~(U32)(0x1<<5);
			}
			break;
		case CLK_REM_SDTVO:
			if (srcclockid == CLK_PIX_HD0)
			{
				ckg_cfg |= (0x1<<6);
			}
			else
			{
			  ckg_cfg &= ~(U32)(0x1<<6);
			}
			break;
	}

	CKGB_UpdateClkCfg(ckg_cfg) ;

    CkgB_Clocks[destclockid].sourceid = srcclockid;
    return(TRUE);
}
/* ========================================================================
   Name:        ST_Ckg_Init
   Description: CKG Initialize function for HD main and SD aux

   ======================================================================== */
BOOL ST_Ckg_Init(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetOk = FALSE;


    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

	/*switch to PLL input for CKGB*/
    STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_OUT_MUX_CFG,0);
    /* set clock for MAIN HD and AUX SD*/
    printf ("Primary mode\n");fflush(stdout);
    printf ("------------\n");fflush(stdout);
    ST_Ckg_SetFSClock(FS0_out1,148500000);
    ST_Ckg_SetFSClock(FS1_out1,216000000);
    ST_Ckg_SetFSClock(FS1_out2,216000000);


    CKGB_SetClockDivider(CLK_OBELIX_MAIN,2);
    CKGB_SetClockDivider(CLK_HDTVO_PIX_MAIN,2);
    CKGB_SetClockDivider(CLK_TMDS,2);
    CKGB_SetClockDivider(CLK_HDTVO_TMDS_MAIN,2);
    CKGB_SetClockDivider(CLK_HDTVO_FDVO_MAIN,2);

    CKGB_SetClockSource(CLK_LOC_SDTVO,CLK_PIX_SD0);

    CKGB_SetClockSource(CLK_PIX_OBELIX,CLK_PIX_SD0);

    CKGB_SetClockDivider(CLK_OBELIX_AUX,2);
    CKGB_SetClockSource(CLK_OBELIX_GDP,CLK_OBELIX_AUX);
    CKGB_SetClockSource(CLK_OBELIX_VDP,CLK_OBELIX_AUX);
    CKGB_SetClockSource(CLK_OBELIX_CAP,CLK_OBELIX_AUX);

    CKGB_SetClockSource(CLK_PIX_ASTERIX,CLK_PIX_SD0);
    CKGB_SetClockDivider(CLK_ASTERIX_AUX,2);

    CKGB_SetClockDivider(CLK_HDTVO_PIX_AUX,2);
    CKGB_SetClockDivider(CLK_HDTVO_DENC,1);

    CKGB_SetClockSource(CLK_REM_SDTVO,CLK_PIX_SD1);
    CKGB_SetClockDivider(CLK_SDTVO_PIX_MAIN,2);
    CKGB_SetClockDivider(CLK_SDTVO_DENC,1);
    CKGB_SetClockDivider(CLK_SDTVO_FDVO,1);

    /* set OBELIX_AUX : pending debug*/
    STSYS_WriteRegDev32LE((VOS_HDMI_BASE_ADDRESS + 0x20),0x9c00);
    STSYS_WriteRegDev32LE(VOS_HD_BASE_ADDRESS + TVO_GLUE_AUX_OFFSET , 0x150000);


    /* SET DENC dividers : pending debug of code */
    STSYS_WriteRegDev32LE(VOS_HD_BASE_ADDRESS + TVO_GLUE_AUX_OFFSET + 0x4 , 0x1);
    STSYS_WriteRegDev32LE(VOS_HD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + 0x4 , 0x45);

    /*select TVO GLUE_main to select HSYNC1/VSYNC1 for HD formatter, HSYNC2/VSYNC2 for DVO and HSYNC3/VSYNC3 for HDMI*/
    STSYS_WriteRegDev32LE(VOS_HD_BASE_ADDRESS + TVO_GLUE_MAIN_OFFSET + 0x8 , 0xF6);

    return (RetOk);
}

#endif /*7200*/
/*-------------------------------------------------------------------------
 * Function : VtgSimSetConfCmd
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL VtgSimSetConfCmd( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_Handle_t     DENCHndl;
    VtgSimDeviceType_t  DeviceType;
    VtgSimTimingMode_t  TimingMode;
    char *ptr=NULL, TrvStr[80], IsStr[80];
    U32 Var;
    U32 VtgNb;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DENCHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Denc Handle\n");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceType);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Denc Device Type\n");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 2, (S32*)&VtgNb);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Vtg number: 1 or 2\n");
        return(TRUE);
    }

    STTST_GetItem(pars_p, "MODE_576I50000_13500", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }


    for(Var=0; Var < VTGSIM_TIMING_MODE_COUNT; Var++)
    {
        if((strcmp(ModeName[Var], TrvStr)==0)|| \
           strncmp(ModeName[Var], TrvStr+1, strlen(ModeName[Var]))==0)
            break;
    }
    if(Var == VTGSIM_TIMING_MODE_COUNT)
    {
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if (RetErr)
        {
            Var = (U32)strtoul(TrvStr, &ptr, 10);
        }
    }

    if ((Var > VTGSIM_TIMING_MODE_COUNT) || (TrvStr == ptr))
    {
        STTST_TagCurrentLine( pars_p, "expected TimingMode, from 0 to 6\n"); /* see VtgSimTimingMode_t definition */
        return(TRUE);
    }
    TimingMode = (VtgSimTimingMode_t)Var;

    RetErr = VtgSimSetConfiguration(DENCHndl, DeviceType, TimingMode,VtgNb);

    if (!RetErr)
    {
        STTBX_Print(( "VtgSimSetConfiguration(%d, %d, %d, %s): ok\n", DENCHndl, DeviceType, VtgNb, ModeName[TimingMode]));
    }
    else
    {
        STTBX_Print(( "VtgSimSetConfiguration(%d, %d, %d, %s): **** FAILED ****\n", DENCHndl, DeviceType, VtgNb, ModeName[TimingMode]));
    }
    return(RetErr);
} /* end of VtgSimSetConfCmd */

/*-------------------------------------------------------------------------
 * Function : VtgSimInitCmd
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL VtgSimInitCmd( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_Handle_t     DENCHndl;
    VtgSimDeviceType_t  DeviceType;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DENCHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Denc Handle\n");
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceType);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected VTGSim Device Type\n");
        return(TRUE);
    }
    VtgSimInit( DENCHndl, DeviceType);
    STTBX_Print(( "VtgSimInit(%d, %d): ok\n", DENCHndl, DeviceType));
    return(FALSE);

} /* end of VtgSimInitCmd */

/*-------------------------------------------------------------------------
 * Function : VtgSim_RegisterCmd
 *            Definition of register command
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL VtgSim_RegisterCmd(void)
{
    BOOL RetErr = FALSE;

    RetErr =  STTST_RegisterCommand( "VTGSim_SetConf", VtgSimSetConfCmd, "<Denc handle><0|1(5528)|2(7020)><Vtg #><Mode> VTG configuration by simulation (without STVTG)" );
    RetErr |= STTST_RegisterCommand( "VTGSim_Init", VtgSimInitCmd, "VTG simulator init (without STVTG)" );
#if defined (ST_7100) || defined (ST_7109)
#if !defined(ST_OSLINUX)
    RetErr |= STTST_RegisterCommand( "ckg_init", ST_Ckg_Init, " ST_Ckg_Init " );
    RetErr |= STTST_RegisterCommand( "ckg_term", ST_Ckg_Term, " ST_Ckg_Term " );
#endif /** ST_OSLINUX **/
#endif
#if defined (ST_7200)
 RetErr |= STTST_RegisterCommand( "ckg_init", ST_Ckg_Init, " ST_Ckg_Init " );
#endif
    if ( RetErr )
    {
        printf( "VtgSim_RegisterCmd() \t: failed !\n");
    }
    else
    {
        printf( "VtgSim_RegisterCmd() \t: ok\n");
    }
    return(RetErr ? FALSE : TRUE);
} /* end of VtgSim_RegisterCmd() */

#endif /* USE_VTGSIM */

/* End of simvtg.c */

