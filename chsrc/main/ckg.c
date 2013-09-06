/****************************************************************************

File Name   : ckg.c

Description : Sets the file system clock for CKG registers,
			  Get CKG clocks frequencies ,set FS clock,
			  TEST TC RAM for PTI Patch

Copyright (C) ST Microelectronics 2005

References  :
****************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stcommon.h"
#include "stdenc.h"
#include "stdevice.h"
#include "stsys.h"
#include "sttbx.h"


#include "ckg_reg.h"


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables -------------------------------------------------------- */
#if (STCLKRV_EXT_CLK_MHZ == 30)
   static U32 refclock=30;
#elif (STCLKRV_EXT_CLK_MHZ == 27)
   static U32 refclock=27;
#elif (STCLKRV_EXT_CLK_MHZ == 0x00)
   static U32 refclock=30;
#endif
/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
boolean TSTCkg_Read_tt ( void );

/* Functions ---------------------------------------------------------------- */

/*------------------------------------------------------------------------------
* Function    : Main_TC_IRam_Test
* Description : TEST TC RAM for PTI Patch
* Input       : None
* Output      : None
* Return      : FALSE if Ok
* --------------------------------------------------------------------------- */
   
boolean Main_TC_IRam_Test(void)
{
    int i,j;

    unsigned int a,d;
    unsigned int Start_address = 0;
    unsigned int Size_address = 2048;
    BOOL Error=FALSE;
    U32 RandomData[Start_address + Size_address];

    U32 * TC_Ram = (U32 *)0xB923C000;

    /* Fill TC Ram with Random Data */
    for  (i=0;i<Size_address;i++)
    {
        RandomData[i]=rand();
        TC_Ram[i] = RandomData[i];
    }

    /* Check 1 times the TC RAM */
    for (j=0;j<10000 && Error==FALSE;j++)
    {
        for  (i=0;i<Size_address && Error==FALSE;i++)
        {
            a=TC_Ram[i];
            d=RandomData[i];
            if (a != d)
            {
                Error=TRUE;
            }
        }
    }
    STTBX_Print(( "j = %lu, i = %lu\n", j, i));
    return(Error);
}

/*------------------------------------------------------------------------------
* Function    : ST_Ckg_Init
* Description : CKG Initialize function for HD main and SD aux
* Input       : none
* Output      : none
* Return      : TRUE if success
* --------------------------------------------------------------------------- */
BOOL ST_Ckg_Init(void)
{
    BOOL RetOk = TRUE;
    BOOL  IntError;

    /* Set FS1_out2 for Preproc */
    IntError = ST_Ckg_SetFSClock(FS1_out2, 150000000);
    if ( IntError == TRUE)
    {
        printf( "Error in setting FS1_out2\n");
        RetOk = FALSE;
    }

	#if 0
		/* Set FS1_out1 for Pipe+Ic_150, with clock below 133 MHz */
		IntError = ST_Ckg_SetFSClock(FS1_out1, 128000000);
		if ( IntError == TRUE)
		{
			printf( "Error in setting FS1_out1\n");
			RetOk = FALSE;
		}

		/* Set mode */
		STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );

		/* hdmi clk at 74.25, pix_hdmi at 74.25, disp_hd at 74.25, 656 do not care, disp_sd at 13.5, pix_sd do not care */
		#ifdef  TESTAPP_HD
			STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_DISPLAY_CFG, 0x00 );
		#elif  TESTAPP_DUAL
			STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_DISPLAY_CFG, 0x00 );
		#elif TESTAPP_SD
			STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_DISPLAY_CFG, 0x469 );
		#else
			STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_DISPLAY_CFG, 0x00 );
		#endif

		/* for SD */
		STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS_SELECT, 0x2 );

		STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );
	#endif

    return (RetOk);
}

/*------------------------------------------------------------------------------
* Function    : ST_CkgA_GetClock
* Description : Get CKGA clocks frequencies function
* Input       : CkgA_Clock_t *Clock
* Output      : None
* Return      : Always 0
* --------------------------------------------------------------------------- */
int ST_CkgA_GetClock(CkgA_Clock_t *Clock)
{
    U32 data;
    int mdiv, ndiv, pdiv;
    int pll1frq, infrq, clkfrq = 0;

    /* PLL1 clocks */
    data = STSYS_ReadRegDev32LE( CKGA_BASE + PLL1_CTRL );
    mdiv = (data&0xFF);
    ndiv=((data>>8)&0xFF);
    pdiv =((data>>16)&0x7);
    #if defined(TESTAPP_COCOREF)
		pll1frq = (((2*30*ndiv)/mdiv) / (1 << pdiv))*1000000;
    #else
		pll1frq = (((2*27*ndiv)/mdiv) / (1 << pdiv))*1000000;
    #endif

    infrq = pll1frq / 2;

    /* SH4 clock */
    data = STSYS_ReadRegDev32LE( CKGA_BASE + PLL1_CLK1_CTRL );
    data = data & 0x7;
    switch (data)
    {
		case 0 :
			clkfrq = infrq;
			break;
		case 1 :
			clkfrq = infrq / 2;
			break;
		case 2 :
			clkfrq = infrq / 3;
			break;
		case 3 :
			clkfrq = infrq / 4;
			break;
		case 4 :
			clkfrq = infrq / 6;
			break;
		case 5 :
			clkfrq = infrq / 8;
			break;
		case 6 :
			clkfrq = infrq;
			break;
		case 7 :
			clkfrq = infrq;
			break;
		default :
			break;
    }
    Clock->sh4_clk = clkfrq;

    /* SH4_INTERCO clock */
    data = STSYS_ReadRegDev32LE( CKGA_BASE + PLL1_CLK2_CTRL );
    data = data & 0x7;
    switch (data)
    {
		case 0 :
			clkfrq = infrq;
			break;
		case 1 :
			clkfrq = infrq / 2;
			break;
		case 2 :
			clkfrq = infrq / 3;
			break;
		case 3 :
			clkfrq = infrq / 4;
			break;
		case 4 :
			clkfrq = infrq / 6;
			break;
		case 5 :
			clkfrq = infrq / 8;
			break;
		case 6 :
			clkfrq = infrq;
			break;
		case 7 :
			clkfrq = infrq;
			break;
		default :
			break;
    }
    Clock->sh4_ic_clk = clkfrq;

    /* SH4_PERIPHERAL clock */
    data = STSYS_ReadRegDev32LE( CKGA_BASE + PLL1_CLK3_CTRL );
    data = data & 0x7;
    switch (data)
    {
		case 0 :
			clkfrq = infrq;
			break;
		case 1 :
			clkfrq = infrq / 2;
			break;
		case 2 :
			clkfrq = infrq / 3;
			break;
		case 3 :
			clkfrq = infrq / 4;
			break;
		case 4 :
			clkfrq = infrq / 6;
			break;
		case 5 :
			clkfrq = infrq / 8;
			break;
		case 6 :
			clkfrq = infrq;
			break;
		case 7 :
			clkfrq = infrq;
			break;
		default :
			break;
    }
    Clock->sh4_per_clk = clkfrq;

    /* SLIM and MPEG2 clocks */
    data = STSYS_ReadRegDev32LE( CKGA_BASE + PLL1_CLK4_CTRL );
    data = data & 0x7;
    switch (data)
    {
		case 0 :
			clkfrq = infrq;
			break;
		case 1 :
			clkfrq = infrq / 2;
			break;
		case 2 :
			clkfrq = infrq / 3;
			break;
		case 3 :
			clkfrq = infrq / 4;
			break;
		case 4 :
			clkfrq = infrq / 6;
			break;
		case 5 :
			clkfrq = infrq / 8;
			break;
		case 6 :
			clkfrq = infrq;
			break;
		case 7 :
			clkfrq = infrq;
			break;
		default :
			break;
    }
    Clock->slim_clk = clkfrq;
    Clock->mpeg2_clk = clkfrq;


    /* PLL2 clocks */
    data = STSYS_ReadRegDev32LE( CKGA_BASE + PLL2_CTRL );
    mdiv = (data&0xFF);
    ndiv=((data>>8)&0xFF);
    pdiv =((data>>16)&0x7);
    #if defined(TESTAPP_COCOREF)
		pll1frq = (((2*30*ndiv)/mdiv) / (1 << pdiv))*1000000;
    #else
		pll1frq = (((2*27*ndiv)/mdiv) / (1 << pdiv))*1000000;
    #endif

    /* LX Delta Phi clock */
    Clock->lx_dh_clk = pll1frq;

    /* LX Audio clock */
    Clock->lx_aud_clk = pll1frq;

    /* LMI2X sys clock */
    Clock->lmi2x_sys_clk = pll1frq;

    /* LMI2X vid clock */
    Clock->lmi2x_vid_clk = pll1frq;

    /* STBUS clock */
    Clock->ic_clk = pll1frq / 2;

    /* EMI clock */
    Clock->icdiv2_emi_clk = pll1frq / 4;

    return 0;
}

/*------------------------------------------------------------------------------
* Function    : ST_CkgB_GetClock
* Description : Get CKGB clocks frequencies function
* Input       : CkgB_Clock_t *Clock
* Output      : None
* Return      : Always 0
* --------------------------------------------------------------------------- */
int ST_CkgB_GetClock(CkgB_Clock_t *Clock)
{
    U32 data, mux, fs_sel;
    int mddiv, pediv, sdiv;
    U32 Fs0_clk1, Fs0_clk2, Fs0_clk3, Fs1_clk1, Fs1_clk2, Fs1_clk3, Fs1_clk4;

    /* FS0 clock 1 */
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS0_PE1 );
    pediv = (data&0xffff);
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS0_MD1 );
    mddiv =(data&0x1f);
    mddiv = ( mddiv - 32 );
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS0_SDIV1 );
    sdiv =(data&0x7);
    sdiv = pow(2,(sdiv+1));
    Fs0_clk1 = ((pow(2,15)*(refclock*8))/(sdiv*((pediv*(1.0+mddiv/32.0))-((pediv-pow(2,15))*(1.0+(mddiv+1.0)/32.0)))))*1000000;

    /* FS0 clock 2 */
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS0_PE2 );
    pediv = (data&0xffff);
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS0_MD2 );
    mddiv =(data&0x1f);
    mddiv = ( mddiv - 32 );
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS0_SDIV2 );
    sdiv =(data&0x7);
    sdiv = pow(2,(sdiv+1));
    Fs0_clk2 = ((pow(2,15)*(refclock*8))/(sdiv*((pediv*(1.0+mddiv/32.0))-((pediv-pow(2,15))*(1.0+(mddiv+1.0)/32.0)))))*1000000;

    /* FS0 clock 3 */
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS0_PE3 );
    pediv = (data&0xffff);
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS0_MD3 );
    mddiv =(data&0x1f);
    mddiv = ( mddiv - 32 );
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS0_SDIV3 );
    sdiv =(data&0x7);
    sdiv = pow(2,(sdiv+1));
    Fs0_clk3 = ((pow(2,15)*(refclock*8))/(sdiv*((pediv*(1.0+mddiv/32.0))-((pediv-pow(2,15))*(1.0+(mddiv+1.0)/32.0)))))*1000000;

    /* FS1 clock 1 */
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_PE1 );
    pediv = (data&0xffff);
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_MD1 );
    mddiv =(data&0x1f);
    mddiv = ( mddiv - 32 );
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_SDIV1 );
    sdiv =(data&0x7);
    sdiv = pow(2,(sdiv+1));
    Fs1_clk1 = ((pow(2,15)*(refclock*8))/(sdiv*((pediv*(1.0+mddiv/32.0))-((pediv-pow(2,15))*(1.0+(mddiv+1.0)/32.0)))))*1000000;

    /* FS1 clock 2 */
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_PE2 );
    pediv = (data&0xffff);
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_MD2 );
    mddiv =(data&0x1f);
    mddiv = ( mddiv - 32 );
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_SDIV2 );
    sdiv =(data&0x7);
    sdiv = pow(2,(sdiv+1));
    Fs1_clk2 = ((pow(2,15)*(refclock*8))/(sdiv*((pediv*(1.0+mddiv/32.0))-((pediv-pow(2,15))*(1.0+(mddiv+1.0)/32.0)))))*1000000;

    /* FS1 clock 3 */
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_PE3 );
    pediv = (data&0xffff);
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_MD3 );
    mddiv =(data&0x1f);
    mddiv = ( mddiv - 32 );
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_SDIV3 );
    sdiv =(data&0x7);
    sdiv = pow(2,(sdiv+1));
    Fs1_clk3 = ((pow(2,15)*(refclock*8))/(sdiv*((pediv*(1.0+mddiv/32.0))-((pediv-pow(2,15))*(1.0+(mddiv+1.0)/32.0)))))*1000000;

    /* FS1 clock 4 */
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_PE4 );
    pediv = (data&0xffff);
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_MD4 );
    mddiv =(data&0x1f);
    mddiv = ( mddiv - 32 );
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS1_SDIV4 );
    sdiv =(data&0x7);
    sdiv = pow(2,(sdiv+1));
    Fs1_clk4 = ((pow(2,15)*(refclock*8))/(sdiv*((pediv*(1.0+mddiv/32.0))-((pediv-pow(2,15))*(1.0+(mddiv+1.0)/32.0)))))*1000000;

    /* hdmi_dll_bch_clk */
    Clock->hdmi_dll_bch_clk = Fs0_clk1;

    /* Read mux */
    data = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_DISPLAY_CFG );

    /* hdmi_tmds_clk */
    mux = data & 0x3;
    switch (mux)
    {
		case 0 :
			Clock->hdmi_tmds_clk = Fs0_clk1 / 2;
			break;
		case 1 :
			Clock->hdmi_tmds_clk = Fs0_clk1 / 4;
			break;
		case 2 :
			Clock->hdmi_tmds_clk = Fs0_clk1 / 8;
			break;
		case 3 :
			Clock->hdmi_tmds_clk = Fs0_clk1 / 2;
			break;
		default :
			break;
    }

    /* hdmi_pix_clk */
    mux = ((data>>2) & 0x3);
    switch (mux)
    {
		case 0 :
			Clock->hdmi_pix_clk = Fs0_clk1 / 2;
			break;
		case 1 :
			Clock->hdmi_pix_clk = Fs0_clk1 / 4;
			break;
		case 2 :
			Clock->hdmi_pix_clk = Fs0_clk1 / 8;
			break;
		case 3 :
			Clock->hdmi_pix_clk = Fs0_clk1 / 2;
			break;
		default :
			break;
    }

    /* pipe_clk */
    Clock->pipe_clk = Fs1_clk2;

    /* pix_hd_clk */
    Clock->pix_hd_clk = Fs0_clk1;

    /* disp_hd_clk */
    mux = ((data>>4) & 0x3);
    switch (mux)
    {
		case 0 :
			Clock->disp_hd_clk = Fs0_clk1 / 2;
			break;
		case 1 :
			Clock->disp_hd_clk = Fs0_clk1 / 4;
			break;
		case 2 :
			Clock->disp_hd_clk = Fs0_clk1 / 8;
			break;
		case 3 :
			Clock->disp_hd_clk = Fs0_clk1 / 2;
			break;
		default :
			break;
    }

    /* ccir656_clk */
    mux = ((data>>6) & 0x3);
    switch (mux)
    {
		case 0 :
			Clock->ccir656_clk = Fs0_clk1 / 2;
			break;
		case 1 :
			Clock->ccir656_clk = Fs0_clk1 / 4;
			break;
		case 2 :
			Clock->ccir656_clk = Fs0_clk1 / 8;
			break;
		case 3 :
			Clock->ccir656_clk = Fs0_clk1 / 2;
			break;
		default :
			break;
    }

    /* disp_sd_clk */
    mux = ((data>>8) & 0x3);
    switch (mux)
    {
		case 0 :
			Clock->disp_sd_clk = Fs1_clk1 / 2;
			break;
		case 1 :
			Clock->disp_sd_clk = Fs1_clk1 / 4;
			break;
		case 2 :
			Clock->disp_sd_clk = Fs1_clk1 / 8;
			break;
		case 3 :
			Clock->disp_sd_clk = Fs1_clk1 / 2;
			break;
		default :
			break;
    }

    /* preproc_clk */
    Clock->preproc_clk = Fs1_clk3;

    /* rtc_clk */
    Clock->rtc_clk = 32768;

    /* Read fs_sel mux */
    fs_sel = STSYS_ReadRegDev32LE( CKGB_BASE + CKGB_FS_SELECT );

    /* pix_sd_clk */
    if ((fs_sel&0x2) == 2)
    {
        /* source is FS1 */
        Clock->pix_sd_src = SD_FS;
        Clock->pix_sd_clk = Fs1_clk1;
    }
    else
    {
        /* source is FS0 */
        Clock->pix_sd_src = HD_FS;
        mux = ((data>>10) & 0x3);
        switch (mux)
        {
			case 0 :
				Clock->pix_sd_clk = Fs0_clk1 / 2;
				break;
			case 1 :
				Clock->pix_sd_clk = Fs0_clk1 / 4;
				break;
			case 2 :
				Clock->pix_sd_clk = Fs0_clk1 / 8;
				break;
			case 3 :
				Clock->pix_sd_clk = Fs0_clk1 / 2;
				break;
			default :
				break;
        }
    }

    /* gdp2_clk */
    if ((fs_sel&0x1) == 1)
    {
        /* source is FS1 */
        Clock->gdp2_src = SD_FS;
        Clock->gdp2_clk = Clock->disp_sd_clk;
    }
    else
    {
        /* source is FS0 */
        Clock->gdp2_src = HD_FS;
        Clock->gdp2_clk = Clock->disp_hd_clk;
    }

    return 0;
}

/*------------------------------------------------------------------------------
* Function    : ST_CkgC_GetClock
* Description : Get CKGC clocks frequencies function
* Input       : CkgC_Clock_t *Clock
* Output      : None
* Return      : Always 0
* --------------------------------------------------------------------------- */
int ST_CkgC_GetClock(CkgC_Clock_t *Clock)
{
    U32 data;
    int mddiv, pediv, sdiv;
    U32 Fs2_clk1, Fs2_clk2, Fs2_clk3;

    /* FS2 clock 1 */
    data = STSYS_ReadRegDev32LE( CKGC_BASE + CKGC_FS0_PE1 );
    pediv = (data&0xffff);
    data = STSYS_ReadRegDev32LE( CKGC_BASE + CKGC_FS0_MD1 );
    mddiv =(data&0x1f);
    mddiv = ( mddiv - 32 );
    data = STSYS_ReadRegDev32LE( CKGC_BASE + CKGC_FS0_SDIV1 );
    sdiv =(data&0x7);
    sdiv = pow(2,(sdiv+1));
    Fs2_clk1 = ((pow(2,15)*(refclock*8))/(sdiv*((pediv*(1.0+mddiv/32.0))-((pediv-pow(2,15))*(1.0+(mddiv+1.0)/32.0)))))*1000000;

    /* FS2 clock 2 */
    data = STSYS_ReadRegDev32LE( CKGC_BASE + CKGC_FS0_PE2 );
    pediv = (data&0xffff);
    data = STSYS_ReadRegDev32LE( CKGC_BASE + CKGC_FS0_MD2 );
    mddiv =(data&0x1f);
    mddiv = ( mddiv - 32 );
    data = STSYS_ReadRegDev32LE( CKGC_BASE + CKGC_FS0_SDIV2 );
    sdiv =(data&0x7);
    sdiv = pow(2,(sdiv+1));
    Fs2_clk2 = ((pow(2,15)*(refclock*8))/(sdiv*((pediv*(1.0+mddiv/32.0))-((pediv-pow(2,15))*(1.0+(mddiv+1.0)/32.0)))))*1000000;

    /* FS2 clock 3 */
    data = STSYS_ReadRegDev32LE( CKGC_BASE + CKGC_FS0_PE3 );
    pediv = (data&0xffff);
    data = STSYS_ReadRegDev32LE( CKGC_BASE + CKGC_FS0_MD3 );
    mddiv =(data&0x1f);
    mddiv = ( mddiv - 32 );
    data = STSYS_ReadRegDev32LE( CKGC_BASE + CKGC_FS0_SDIV3 );
    sdiv =(data&0x7);
    sdiv = pow(2,(sdiv+1));
    Fs2_clk3 = ((pow(2,15)*(refclock*8))/(sdiv*((pediv*(1.0+mddiv/32.0))-((pediv-pow(2,15))*(1.0+(mddiv+1.0)/32.0)))))*1000000;

    Clock->pcm0_clk = Fs2_clk1;
    Clock->pcm1_clk = Fs2_clk2;
    Clock->spdif_clk = Fs2_clk3;

    return 0;
}

/*------------------------------------------------------------------------------
* Function    : TSTCkg_Read_tt
* Description : Dump CKG registers
* Input       : None
* Output      : None
* Return      : Always FALSE
* --------------------------------------------------------------------------- */
boolean TSTCkg_Read_tt ( void )
{
    CkgA_Clock_t clockA_detect;
    CkgB_Clock_t clockB_detect;
    CkgC_Clock_t clockC_detect;

    ST_CkgA_GetClock(&clockA_detect);
    ST_CkgB_GetClock(&clockB_detect);
    ST_CkgC_GetClock(&clockC_detect);

    printf( "Clock gen A frequencies :\n");
    printf( "=========================\n");
    printf( "SH4_clk = %d Hz\n", clockA_detect.sh4_clk);
    printf( "SH4_IC_clk = %d Hz\n", clockA_detect.sh4_ic_clk);
    printf( "SLIM_clk = %d Hz\n", clockA_detect.slim_clk);
    printf( "MPEG2_clk = %d Hz\n", clockA_detect.mpeg2_clk);
    printf( "LX_DH_clk = %d Hz\n", clockA_detect.lx_dh_clk);
    printf( "LX_AUD_clk = %d Hz\n", clockA_detect.lx_aud_clk);
    printf( "LMI2X_SYS_clk = %d Hz\n", clockA_detect.lmi2x_sys_clk);
    printf( "LMI2X_VID_clk = %d Hz\n", clockA_detect.lmi2x_vid_clk);
    printf( "IC_clk = %d Hz\n", clockA_detect.ic_clk);
    printf( "EMI_clk = %d Hz\n", clockA_detect.icdiv2_emi_clk);

    printf( "\nClock gen B frequencies :\n");
    printf( "===========================\n");
    printf( "HDMI_DLL_clk = %d Hz\n", clockB_detect.hdmi_dll_bch_clk);
    printf( "HDMI_TMDS_clk = %d Hz\n", clockB_detect.hdmi_tmds_clk);
    printf( "HDMI_PIX_clk = %d Hz\n", clockB_detect.hdmi_pix_clk);
    printf( "PIX_HD_clk = %d Hz\n", clockB_detect.pix_hd_clk);
    printf( "DISP_HD_clk = %d Hz\n", clockB_detect.disp_hd_clk);
    printf( "PIX_SD_clk = %d Hz\n", clockB_detect.pix_sd_clk);
    printf( "DISP_SD_clk = %d Hz\n", clockB_detect.disp_sd_clk);
    printf( "CCIR656_clk = %d Hz\n", clockB_detect.ccir656_clk);
    printf( "GDP2_clk = %d Hz\n", clockB_detect.gdp2_clk);
    printf( "PIPE_clk = %d Hz\n", clockB_detect.pipe_clk);
    printf( "PreProc_clk = %d Hz\n", clockB_detect.preproc_clk);

    if ( clockB_detect.pix_sd_src == HD_FS)
    {
        printf( "Source of pix_sd is FS0 (HD)\n");
    }
    else
    {
        printf( "Source of pix_sd is FS1 (SD)\n");
    }
    if ( clockB_detect.gdp2_src == HD_FS)
    {
        printf( "Source of gdp2 is FS0 (HD)\n");
    }
    else
    {
        printf( "Source of gdp2 is FS1 (SD)\n");
    }

    printf( "\nClock gen C frequencies :\n");
    printf( "===========================\n");
    printf( "PCM0_clk = %d Hz\n", clockC_detect.pcm0_clk);
    printf( "PCM1_clk = %d Hz\n", clockC_detect.pcm1_clk);
    printf( "PCM2_clk = %d Hz\n", clockC_detect.spdif_clk);

    return (FALSE);
}

/*------------------------------------------------------------------------------
* Function    : GetFSParameterFromFreq
* Description : Freq to Parameters
* Input       : freq, *md, *pe, *sdiv
* Output      : None
* Return      : FALSE if success
* --------------------------------------------------------------------------- */

static boolean GetFSParameterFromFreq( int freq, int *md, int *pe, int *sdiv )
{
    double fr, Tr, Td1, Tx, fmx, nd1, nd2, Tdif;
    int NTAP, msdiv, mfmx, ndiv;

    NTAP = 32;
    mfmx = 0;

    ndiv = 1.0;

    fr = refclock * 8000000.0;
    Tr = 1.0 / fr;
    Td1 = 1.0 / (NTAP * fr);
    msdiv = 0;

    /* Recherche de SDIV */
    while (! ((mfmx >= (refclock*8000000)) && (mfmx <= (refclock*16000000))) && (msdiv < 7))
    {
        msdiv = msdiv + 1;
        mfmx = pow(2,msdiv) * freq;
    }

    *sdiv = msdiv - 1;
    fmx = mfmx / 1000000.0;
    if ((fmx < (8*refclock)) || (fmx > (16*refclock)))
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

    /*    printf( "Programmation for QSYNTH C090_4FS216_25 with fout= %d Hz and fin= 30 MHz\n", freq);
        printf( "      MD[4:0]               ->       md      = 0x%x\n", *md);
        printf( "      PE[15:0]              ->       pe      = 0x%x\n", *pe);
        printf( "      SDIV[2:0]             ->       sdiv    = 0x%x\n", *sdiv);*/

    return (FALSE);
}


/*------------------------------------------------------------------------------
* Function    : ST_Ckg_SetFSClock
* Description : Set FS clock
* Input       : FS_sel, freq
* Output      : None
* Return      : FALSE if sucess, TRUE if wrong parameters.
* --------------------------------------------------------------------------- */

BOOL  ST_Ckg_SetFSClock(FS_sel_t FS_sel, int freq)
{
    BOOL   RetError = FALSE;
    int md, pe, sdiv;

    if ((GetFSParameterFromFreq( freq, &md, &pe, &sdiv )) == TRUE )
    {
        RetError = TRUE;
    }
    else
    {
        if ( FS_sel == FS0_out0 )
        {
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_MD1, md );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_PE1, pe );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_SDIV1, sdiv );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_EN_PRG1, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );

        }
        if ( FS_sel == FS0_out1 )
        {
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_MD2, md );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_PE2, pe );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_SDIV2, sdiv );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_EN_PRG2, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );
        }
        if ( FS_sel == FS0_out2 )
        {
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_MD3, md );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_PE3, pe );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_SDIV3, sdiv );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS0_EN_PRG3, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );
        }

        if ( FS_sel == FS1_out0 )
        {
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_MD1, md );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_PE1, pe );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_SDIV1, sdiv );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_EN_PRG1, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );
        }
        if ( FS_sel == FS1_out1 )
        {
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_MD2, md );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_PE2, pe );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_SDIV2, sdiv );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_EN_PRG2, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );

/*            printf("PTI CLOCK CONFIGURATION\n");

            while(PTI_OK==TRUE)
            {
                PTI_OK=Main_TC_IRam_Test();
                if(PTI_OK==TRUE)
                {
                    STSYS_WriteRegDev32LE((0xb9000000 + 0x10) ,0xc0de);*/  /* unlock clock Gen B register */
                    /*pe = rand();
                    STSYS_WriteRegDev32LE((0xb9000000 + 0x74) ,pe);
                    pe = 0;
                    STSYS_WriteRegDev32LE((0xb9000000 + 0x74) ,pe);
                    STSYS_WriteRegDev32LE((0xb9000000 + 0x10) ,0xc1a0);*/  /* lock clock Gen B register */
/*                }
            }*/

        }

        if ( FS_sel == FS1_out2 )
        {
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_MD3, md );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_PE3, pe );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_SDIV3, sdiv );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_EN_PRG3, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );
        }
        if ( FS_sel == FS1_out3 )
        {
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc0de );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_MD4, md );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_PE4, pe );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_SDIV4, sdiv );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_FS1_EN_PRG4, 0x1 );
            STSYS_WriteRegDev32LE( CKGB_BASE + CKGB_LOCK, 0xc1a0 );
        }
        if ( FS_sel == FS2_out0 )
        {
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_MD1, md );
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_PE1, pe );
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_SDIV1, sdiv );
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_EN_PRG1, 0x1 );
        }
        if ( FS_sel == FS2_out1 )
        {
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_MD2, md );
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_PE2, pe );
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_SDIV2, sdiv );
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_EN_PRG2, 0x1 );
        }
        if ( FS_sel == FS2_out2 )
        {
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_MD3, md );
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_PE3, pe );
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_SDIV3, sdiv );
            STSYS_WriteRegDev32LE( CKGC_BASE + CKGC_FS0_EN_PRG3, 0x1 );
        }
    }

    return (RetError);
} /* ST_Ckg_SetFSClock() */

#ifdef TESTTOOL_SUPPORT
 /* ----------------------------------------------------------------------
 * Function    : TT_ReadCkg
 * Description : Testtool macro for reading ClkGen A B C
 * Input       : *pars_p, *result_sym_p
 * Output      : None
 * Return      : FALSE if success, else TRUE
 * ----------------------------------------------------------------------*/
BOOL TT_ReadCkg(parse_t *pars_p, char *result_sym_p)
{
    BOOL        RetErr;

    RetErr = TSTCkg_Read_tt();

    return FALSE;
} /* TT_ReadCkg() */

 /* ----------------------------------------------------------------------
 * Function    : CKG_RegisterCommands
 * Description : Register testtool macros' commands
 * Input       : None
 * Output      : None
 * Return      : None
 * ----------------------------------------------------------------------*/
void CKG_RegisterCommands(void)
{

    STTST_RegisterCommand("ckg_read", TT_ReadCkg, "Reads ClkGen A, B and C");

} /* CKG_RegisterCommands() */
#endif /* TESTTOOL_SUPPORT */
