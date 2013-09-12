/*******************************************************************************

File name   : simvtg.h

Description : header file of STVTG driver simulator source file.

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
07 Mar 2002        Created                                           HS
21 Jan 2004        Update for 60hz and Square pixel modes on 5528    AC
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __SIMVTG_H
#define __SIMVTG_H


/* Includes ----------------------------------------------------------------- */

#include "stdenc.h"
#include "testtool.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#if defined (ST_7200)
#if defined (ST_OSLINUX)
#define CKGB_BASE ClockMap.MappedBaseAddress
#else
#define CKGB_BASE CKG_B_BASE_ADDRESS
#endif
/* From TVOUT*/
#define TVO_GLUE_MAIN_OFFSET    0x400
#define TVO_GLUE_AUX_OFFSET     0x500
#define TVO_HDMI_OFFSET         0x6000

#define HDMI_GPOUT              0x20
#define TVO_MAIN_CLK_SEL        0x4
#define TVO_AUX_CLK_SEL         0x4
#define TVO_AUX_PADS            0x0

#define CKGB_OUT_MUX_CFG        0x048



typedef enum
{
    HD_FS = 0,
    SD_FS
} Fs_source_t;

typedef struct
{
    /* FS outputs */
    U32 Fs0_clk[5]; /*Fs0_Clock[0] is input frequency*/
    U32 Fs1_clk[5];
    U32 Fs2_clk[5];

    /* CKGB clocks */
    U32 pixel_hd0;
    U32 pixel_hd1;
    U32 pipe;
    U32 dmu0_pipe;
    U32 dmu1_pipe;
    U32 disp_pipe;
    U32 vdp_pipe;
    U32 usb48;

    U32 pixel_sd0;
    U32 tsg_sd0;
    U32 periph27;

    U32 pixel_sd1;
    U32 tsg_sd1;
    U32 ck216;
    U32 frc2;
    U32 ic177;

    U32 spare;
    U32 dss;
    U32 ic166;
    U32 rtc;
    U32 lpc;

    U32 hdmi_pll;
    U32 obelix_tmds;
    U32 obelix_ch34mod;
    U32 loc_hdtvo;
    U32 loc_sdtvo;
    U32 rem_sdtvo;
    U32 obelix_fvp;
    U32 obelix_main;
    U32 obelix_aux;
    U32 obelix_gdp;
    U32 obelix_vdp;
    U32 obelix_cap;
    U32 asterix_aux;

    U32 hdtvo_hdvid_dac;
    U32 hdtvo_pix_main;
    U32 hdtvo_tmds;
    U32 hdtvo_bch;
    U32 hdtvo_fdvo;

    U32 hdtvo_sdvid_dac;
    U32 hdtvo_pix_aux;
    U32 hdtvo_denc;

    U32 sdtvo_pix_main;
    U32 sdtvo_fvdo;
    U32 sdtvo_denc;

} CkgB_Clock_t;

typedef enum
{
	FS0_out1=1,
	FS0_out2,
	FS0_out3,
	FS0_out4,
	FS1_out1,
	FS1_out2,
	FS1_out3,
	FS1_out4,
	FS2_out1,
	FS2_out2,
	FS2_out3,
	FS2_out4
} FS_sel_t;

typedef enum
{
	CLK_NONE=0,
	FS0_CK1        ,
	FS0_CK2        ,
	FS0_CK3        ,
	FS0_CK4        ,
	FS1_CK1        ,
	FS1_CK2        ,
	FS1_CK3        ,
	FS1_CK4        ,
	FS2_CK1        ,
	FS2_CK2        ,
	FS2_CK3        ,
	FS2_CK4        ,
	CLK_HDMI_PLL   ,
	CLK_PIX_HD0    ,
	CLK_PIX_HD1    ,
	CLK_PIPE       ,
	CLK_PIX_SD0    ,
	CLK_PIX_SD1    ,

	CLK_PIX_OBELIX ,
	CLK_PIX_ASTERIX,

	CLK_LOC_HDTVO  ,
	CLK_LOC_SDTVO  ,
	CLK_REM_SDTVO  ,
	CLK_OBELIX_FVP ,
	CLK_OBELIX_MAIN,
	CLK_OBELIX_AUX ,
	CLK_OBELIX_GDP ,
	CLK_OBELIX_VDP ,
	CLK_OBELIX_CAP ,
	CLK_ASTERIX_AUX,
	CLK_TMDS       ,
	CLK_CH34MOD    ,

	CLK_USB48      ,
	CLK_216        ,
	CLK_FRC2       ,
	CLK_IC177      ,
	CLK_SPARE      ,
	CLK_DSS        ,
	CLK_IC166      ,
	CLK_RTC        ,
	CLK_LPC        ,
	CLK_HDTVO_PIX_MAIN,
	CLK_HDTVO_TMDS_MAIN,
	CLK_HDTVO_BCH_MAIN,
	CLK_HDTVO_FDVO_MAIN,
    CLK_HDTVO_PIX_AUX,
    CLK_HDTVO_DENC,
    CLK_SDTVO_PIX_MAIN,
    CLK_SDTVO_FDVO,
    CLK_SDTVO_DENC,
    SYSCLKINB
} clockName;

typedef struct
{
	const char name[20];
	U32 id;
	U16 sourceid;
	U16 sources[2];
	U16 divider;
	U16 div_allow;

} CkgB_ClockInstance_t;

#endif

/* Exported Types ----------------------------------------------------------- */

typedef enum VtgSimTimingMode_e
{
    VTGSIM_TIMING_MODE_SLAVE,
    VTGSIM_TIMING_MODE_480I60000_13514,    /* NTSC 60Hz */
    VTGSIM_TIMING_MODE_480I59940_13500,    /* NTSC, PAL M */
    VTGSIM_TIMING_MODE_480I60000_12285,    /* NTSC 60Hz square */
    VTGSIM_TIMING_MODE_480I59940_12273,    /* NTSC square, PAL M square */
    VTGSIM_TIMING_MODE_576I50000_13500,    /* PAL B,D,G,H,I,N, SECAM */
    VTGSIM_TIMING_MODE_576I50000_14750,    /* PAL B,D,G,H,I,N, SECAM square */
    VTGSIM_TIMING_MODE_COUNT /* must stay last */
} VtgSimTimingMode_t;

typedef enum VtgSimDeviceType_e
{
    VTGSIM_DEVICE_TYPE_0,  /* DENC only */
    VTGSIM_DEVICE_TYPE_1,  /* VTG stand alone cell, Video Front End (VFE): GX1, 5528*/
    VTGSIM_DEVICE_TYPE_2   /* VTG stand alone cell, Video Output Stage (VOS): 7015, 7020 */
} VtgSimDeviceType_t;

#if defined (ST_5508) || defined (ST_5518) || defined (ST_5510) || defined (ST_5512) || \
    defined (ST_5516) || ((defined (ST_5514) || defined (ST_5517)) && !defined(ST_7020))
#define VTGSIM_DEVICE_TYPE       VTGSIM_DEVICE_TYPE_0

#elif defined(ST_GX1) || defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_8010)\
   || defined(ST_5557)|| defined(ST_5525)|| defined(ST_5188)|| defined(ST_5107)|| defined(ST_7200)
#define VTGSIM_DEVICE_TYPE       VTGSIM_DEVICE_TYPE_1

#elif defined(ST_7015)|| defined(ST_7020)|| defined(ST_7710)|| defined(ST_7100)|| defined(ST_7109)
#define VTGSIM_DEVICE_TYPE       VTGSIM_DEVICE_TYPE_2

#else
#error Not defined yet
#endif

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
#if defined (ST_7200)
U32 CKGB_SetClockSource (int destclockid,int srcclockid);
U32 CKGB_SetClockDivider (int clockid, int divider);
BOOL  ST_Ckg_SetFSClock(FS_sel_t FS_sel, int freq);
boolean GetFSParameterFromFreq( int inputfreq, int outputfreq, int *md, int *pe, int *sdiv );
U32 CKGB_UpdateClkCfg(U32 cfg);
U32 CKGB_GetClockConfig(void);
#endif

void VtgSimInit( const STDENC_Handle_t    DencHnd,
                 const VtgSimDeviceType_t DeviceType);

BOOL VtgSimSetConfiguration(   const STDENC_Handle_t     DencHnd
                             , const VtgSimDeviceType_t  DeviceType
                             , const VtgSimTimingMode_t  TimingMode
                             , const U8                  VtgNb);

void VtgSimRstConfiguration(const VtgSimDeviceType_t  DeviceType);

BOOL VtgSimSetConfCmd( STTST_Parse_t *pars_p, char *result_sym_p );
BOOL VtgSimRstCmd( STTST_Parse_t *pars_p, char *result_sym_p );
BOOL VtgSim_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __SIMVTG_H */

/* End of simvtg.h */
