/****************************************************************************

File Name   : initfuncs.c

Description : Initialization functions, used incase of Boot from Flash.

Copyright (C) 2003, ST Microelectronics

 ****************************************************************************/
 /* Includes ----------------------------------------------------------- */
 
#include <initfuncs.h>
#include "stsys.h"
#include "reg.h"

/* Private Types ------------------------------------------------------ */

/* Private Constants -------------------------------------------------- */

/* Private Variables -------------------------------------------------- */

/* Private Macros ----------------------------------------------------- */

/* Private Function prototypes ---------------------------------------- */

/* Functions ---------------------------------------------------------- */

static void clock_gen (void);
#pragma ST_nolink(clock_gen)
#if 0
static void delay(int);
#pragma ST_nolink(delay)
#endif


#if !defined(SETTINGS_BY_ST40)/*Setting for testapp16*/
	static void Simudelay(int);
	#pragma ST_nolink(Simudelay)
	static void EMIpokes5528(void);
	#pragma ST_nolink(EMIpokes5528)
	static void ConfigureSysConf(void);
	#pragma ST_nolink(ConfigureSysConf)
	static void ConfigureLmiDDR(void);
	#pragma ST_nolink(ConfigureLmiDDR)
#endif

static void PLL1_STmode_ChangeFreq(void); 	
#pragma ST_nolink(PLL1_STmode_ChangeFreq)
static void PLL1_STmode_ChangeRatio(void);
#pragma ST_nolink(PLL1_STmode_ChangeRatio)
static void PLL2_STmode_ChangeFreq(void);
#pragma ST_nolink(PLL2_STmode_ChangeFreq)
static void QSYNTH1_PCI_Start_and_ChangeFreq(void);
#pragma ST_nolink(QSYNTH1_PCI_Start_and_ChangeFreq)
static void QSYNTH1_USB_IRDA_Start_and_ChangeFreq(void);
#pragma ST_nolink(QSYNTH1_USB_IRDA_Start_and_ChangeFreq)
static void QSYNTH2_ST20_Start_and_ChangeFreq(void);
#pragma ST_nolink(QSYNTH2_ST20_Start_and_ChangeFreq)
static void QSYNTH2_HDD_Start_and_ChangeFreq(void);
#pragma ST_nolink(QSYNTH2_HDD_Start_and_ChangeFreq)
static void QSYNTH2_SMARTCARD_Start_and_ChangeFreq(void);
#pragma ST_nolink(QSYNTH2_SMARTCARD_Start_and_ChangeFreq)
static void QSYNTH2_PIXEL_Start_and_ChangeFreq(void);
#pragma ST_nolink(QSYNTH2_PIXEL_Start_and_ChangeFreq)
static void Mux_and_Other_Dividers(void);
#pragma ST_nolink(Mux_and_Other_Dividers)
static void Interconnect_Init(void);
#pragma ST_nolink(Interconnect_Init)

/* Functions -------------------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : PostPokeLoopCallback
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
void PostPokeLoopCallback(void)
{
}

/*-------------------------------------------------------------------------
 * Function : PrePokeLoopCallback
 * Input    : None
 * Output   :
 * Return   : None
 * Comment  : Function automatically called after the poke loop
 * ----------------------------------------------------------------------*/
void PrePokeLoopCallback(void)
{
    clock_gen();
#if !defined(SETTINGS_BY_ST40)/*Setting for testapp16*/
    EMIpokes5528(); 
    ConfigureSysConf();
    ConfigureLmiDDR();
	
#endif
	Interconnect_Init();
}
#if 0
/*-------------------------------------------------------------------------
 * Function : delay
 * Input    : value
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void delay (int value)
{
    volatile int i;
    for (i=0; i<value ; i++);
}
#endif
/*-------------------------------------------------------------------------
 * Function : clock_gen
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void clock_gen (void)
{
	
	PLL1_STmode_ChangeFreq(); 
	PLL1_STmode_ChangeRatio();
	PLL2_STmode_ChangeFreq();
	QSYNTH1_PCI_Start_and_ChangeFreq();
	QSYNTH1_USB_IRDA_Start_and_ChangeFreq();
	QSYNTH2_ST20_Start_and_ChangeFreq();
	QSYNTH2_HDD_Start_and_ChangeFreq();
	QSYNTH2_SMARTCARD_Start_and_ChangeFreq();
	QSYNTH2_PIXEL_Start_and_ChangeFreq();
	Mux_and_Other_Dividers();	
	
}

#if !defined(SETTINGS_BY_ST40)/*Setting for testapp16*/

/*-------------------------------------------------------------------------
 * Function : Simudelay
 * Input    : delay
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void Simudelay(int delay)
{
    volatile int j,dummy;
    
    for ( j = 0 ; j < delay ; j ++ )
       dummy = STSYS_ReadRegDev32LE(0x007C);
}


/*-------------------------------------------------------------------------
 * Function : ConfigureSysConf
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void ConfigureSysConf(void)
{
    #if defined(mb376)
	    /* Take Lmi out from RESET */
	    STSYS_WriteRegDev8(0x1916205C,0x000000C0);
	    /* Program COC Register */
	    STSYS_WriteRegDev32LE(0x19162060,0x0000000D);
	    /* Program DLL Register */
	    STSYS_WriteRegDev32LE(0x19162064,0x00000000);
    #else
        STSYS_WriteRegDev16LE(0x19162058,0x00000810);
	    STSYS_WriteRegDev8(0x1916205C,0x000000FF);
	    STSYS_WriteRegDev32LE(0x19162060,0x00000005);
	    STSYS_WriteRegDev32LE(0x19162064,0x00000000);
	#endif
    
	
}
	
/*-------------------------------------------------------------------------
 * Function : ConfigureLmiDDR
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void ConfigureLmiDDR(void)
{
    #if defined(mb376)
        /* STI5528_LMI_MEM_INTERFACE_MODE */
        STSYS_WriteRegDev32LE(0x0f000008,0x05100243);
        /* STI5528_LMI_SDRAM_TIMING */
        STSYS_WriteRegDev32LE(0x0f000018,0x00EF5C5A);
        /*STI5528_LMI_SDRAM_ROW_ATTRIBUTE0*/
        STSYS_WriteRegDev32LE(0x0f000030,0x0c001A00);
        /*STI5528_LMI_SDRAM_ROW_ATTRIBUTE1*/
        STSYS_WriteRegDev32LE(0x0f000038,0x0c001A00);
        Simudelay(200);
    
        /*STI5528_LMI_SDRAM_CONTROL*/
        STSYS_WriteRegDev32LE(0x0f000010,0x00000001); /* Issue NOP */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000003); /* CKE High */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000001); /* CKE High */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000002); /* Issue Precharge */
    
        /* STI5528_LMI_SDRAM_ROW_MODE0 */
        STSYS_WriteRegDev32LE((0x0f800000+(0x0000400<<3)),0x00000000);
        STSYS_WriteRegDev32LE((0x0f800000+(0x0000163<<3)),0x00000000);
        Simudelay(200);
        STSYS_WriteRegDev32LE(0x0f000010,0x00000002); /* Issue PAL */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000004); /* Issue CBR */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000004); /* Issue CBR */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000004); /* Issue CBR */
   
        STSYS_WriteRegDev32LE((0x0f800000+(0x0000063<<3)),0x00000000);
        STSYS_WriteRegDev32LE(0x0f000010,0x00000000); /* Issue SCR */
    #else
        /* STI5528_LMI_MEM_INTERFACE_MODE */
        STSYS_WriteRegDev32LE(0x0f000008,0x01000243);
        /* STI5528_LMI_SDRAM_TIMING */
        STSYS_WriteRegDev32LE(0x0f000018,0x00C7B45);
        /*STI5528_LMI_SDRAM_ROW_ATTRIBUTE0*/
        STSYS_WriteRegDev32LE(0x0f000030,0x08001900);
        /*STI5528_LMI_SDRAM_ROW_ATTRIBUTE1*/
        STSYS_WriteRegDev32LE(0x0f000038,0x0c001900);
        Simudelay(1);
    
        /*STI5528_LMI_SDRAM_CONTROL*/
        STSYS_WriteRegDev32LE(0x0f000010,0x00000001); /* Issue NOP */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000003); /* CKE High */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000001); /* CKE High */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000002); /* Issue Precharge */
    
        /* STI5528_LMI_SDRAM_ROW_MODE0 */
        STSYS_WriteRegDev32LE((0x0f800000+(0x0000400<<3)),0x00000000);
        STSYS_WriteRegDev32LE((0x0f900000+(0x0000400<<3)),0x00000000);
        STSYS_WriteRegDev32LE((0x0f800000+(0x0000133<<3)),0x00000000);
        STSYS_WriteRegDev32LE((0x0f900000+(0x0000133<<3)),0x00000000);
        Simudelay(1);
        STSYS_WriteRegDev32LE(0x0f000010,0x00000002); /* Issue PAL */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000004); /* Issue CBR */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000004); /* Issue CBR */
        STSYS_WriteRegDev32LE(0x0f000010,0x00000004); /* Issue CBR */
   
        STSYS_WriteRegDev32LE((0x0f800000+(0x0000133<<3)),0x00000000);
        STSYS_WriteRegDev32LE((0x0f900000+(0x0000133<<3)),0x00000000);
        STSYS_WriteRegDev32LE(0x0f000010,0x00000000); /* Issue SCR */
    #endif
}

/*-------------------------------------------------------------------------
 * Function : EMIpokes5528
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
void EMIpokes5528(void) 
{
    #if defined(mb376)
        /* Lock all control registers */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK_ENABLE ,0x00000006); /* Number of EMI Banks to be enabled*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK0_BASE ,0x00000000); /* EMI Bank0 base*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK1_BASE ,0x00000004); /* EMI Bank1 base*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK2_BASE ,0x00000008); /* EMI Bank2 base*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK3_BASE ,0x0000000C); /* EMI Bank3 base*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK4_BASE ,0x00000014); /* EMI Bank4 base*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK5_BASE ,0x0000001C); /* EMI Bank5 base*/
    
        /* Bank 0 configured for 16 Mbytes ST M58LW064D flash */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK0_DATA0 ,0x001016e9); /* EMI Bank0 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK0_DATA1 ,0x9d200000); /* EMI Bank0 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK0_DATA2 ,0x9d220000); /* EMI Bank0 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK0_DATA3 ,0x00000000); /* EMI Bank0 DATA3*/
    
        /* Bank 1 configured for 16 Mbytes ST M58LW064D flash */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK1_DATA0 ,0x001016e9); /* EMI Bank1 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK1_DATA1 ,0x9d200000); /* EMI Bank1 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK1_DATA2 ,0x9d220000); /* EMI Bank1 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK1_DATA3 ,0x00000000); /* EMI Bank1 DATA3*/
    
    
        /* Bank 2 configured for EPLD/4629 */   
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK2_DATA0 ,0x002046f9); /* EMI Bank2 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK2_DATA1 ,0xa5a00000); /* EMI Bank2 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK2_DATA2 ,0xa5a00000); /* EMI Bank2 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK2_DATA3 ,0x00000000); /* EMI Bank2 DATA3*/
    
        /* Bank 3 configured for STEM Module */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK3_DATA0 ,0x001016e9); /* EMI Bank3 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK3_DATA1 ,0x9d200000); /* EMI Bank3 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK3_DATA2 ,0x9d220000); /* EMI Bank3 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK3_DATA3 ,0x00000000); /* EMI Bank3 DATA3*/
    
        /* Bank 4 configured for STEM Module */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK4_DATA0 ,0x001016e9); /* EMI Bank4 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK4_DATA1,0x9d200000); /* EMI Bank4 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK4_DATA2,0x9d220000); /* EMI Bank4 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK4_DATA3,0x00000000); /* EMI Bank4 DATA3*/
    
        /* Bank 5 configured for 32 MB SDRAM */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK5_DATA0,0x0400044a); /* EMI Bank5 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK5_DATA1,0x007ffff0); /* EMI Bank5 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK5_DATA2,0x00000010); /* EMI Bank5 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK5_DATA3,0x00005fba); /* EMI Bank5 DATA3*/
    
        /* Others */
    
        STSYS_WriteRegDev32LE (STI5528_EMI_GEN_CFG ,0x00000108);   /* EMI4.GENCFG, shift for B2 */
        STSYS_WriteRegDev8(STI5528_EMI_FLASH_CLK_SEL ,0x00000002);   /* EMI4.FLASHCLKSEL */
        STSYS_WriteRegDev8 (STI5528_EMI_SDRAM_CLK_SEL ,0x00000000);   /* EMI4.SDRAMCLKSEL */
        STSYS_WriteRegDev8 (STI5528_EMI_CLK_ENABLE ,0x00000001);   /* EMI4.CLKENABLE */
        STSYS_WriteRegDev32LE (STI5528_EMI_REFRESH_INT ,0x000003e8);   /* EMI4.REFRESHINIT */
        STSYS_WriteRegDev32LE (STI5528_EMI_SDRAM_MODE_REG ,0x00000032);   /* EMI4.SDRAMODEREG */
        STSYS_WriteRegDev8 (STI5528_EMI_SDRAM_INIT ,0x00000001);   /* EMI4.SDRAMODEREG */
    #else
    
        STSYS_WriteRegDev32LE(STI5528_EMI_LOCK ,0x00000000); /* Number of EMI Banks to be enabled*/
        STSYS_WriteRegDev32LE(STI5528_EMI_STATUS_LOCK ,0x00000000); /* Number of EMI Banks to be enabled*/
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK_ENABLE ,0x00000006); /* Number of EMI Banks to be enabled*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK0_BASE ,0x00000000); /* EMI Bank0 base*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK1_BASE ,0x00000004); /* EMI Bank1 base*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK2_BASE ,0x00000008); /* EMI Bank2 base*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK3_BASE ,0x00000009); /* EMI Bank3 base*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK4_BASE ,0x0000000A); /* EMI Bank4 base*/
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK5_BASE ,0x0000000E); /* EMI Bank5 base*/
    
        /* Bank 0 configured for 16 Mbytes ST M58LW064D flash */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK0_DATA0 ,0x001016e9); /* EMI Bank0 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK0_DATA1 ,0x9d200000); /* EMI Bank0 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK0_DATA2 ,0x9d220000); /* EMI Bank0 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK0_DATA3 ,0x00000000); /* EMI Bank0 DATA3*/
    
        /* Bank 1 configured for 16 Mbytes ST M58LW064D flash */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK1_DATA0 ,0x001016e9); /* EMI Bank1 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK1_DATA1 ,0x9d200000); /* EMI Bank1 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK1_DATA2 ,0x9d220000); /* EMI Bank1 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK1_DATA3 ,0x00000000); /* EMI Bank1 DATA3*/
    
        /* Bank 2 configured for EPLD/4629 */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK2_DATA0 ,0x001016e9); /* EMI Bank2 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK2_DATA1 ,0x9d200000); /* EMI Bank2 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK2_DATA2 ,0x9d220000); /* EMI Bank2 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK2_DATA3 ,0x00000000); /* EMI Bank2 DATA3*/
    
        /* Bank 3 configured for STEM Module */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK3_DATA0 ,0x002046f9); /* EMI Bank3 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK3_DATA1 ,0xa5a00000); /* EMI Bank3 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK3_DATA2 ,0xa5a20000); /* EMI Bank3 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK3_DATA3 ,0x00000000); /* EMI Bank3 DATA3*/
    
        /* Bank 4 configured for STEM Module */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK4_DATA0 ,0x001016e9); /* EMI Bank4 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK4_DATA1,0x9d200000); /* EMI Bank4 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK4_DATA2,0x9d220000); /* EMI Bank4 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK4_DATA3,0x00000000); /* EMI Bank4 DATA3*/
    
        /* Bank 5 configured for 32 MB SDRAM */
    
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK5_DATA0,0x001086f1); /* EMI Bank5 DATA0 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK5_DATA1,0x84001100); /* EMI Bank5 DATA1 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK5_DATA2,0x84001100); /* EMI Bank5 DATA2 */
        STSYS_WriteRegDev32LE(STI5528_EMI_BANK5_DATA3,0x00000000); /* EMI Bank5 DATA3*/
    
        /* Others */
    
        STSYS_WriteRegDev8 (STI5528_EMI_CLK_ENABLE ,0x00000001);   /* EMI4.CLKENABLE */
        STSYS_WriteRegDev8(STI5528_EMI_FLASH_CLK_SEL ,0x00000000);   /* EMI4.FLASHCLKSEL */
        STSYS_WriteRegDev8 (STI5528_EMI_SDRAM_CLK_SEL ,0x00000000);   /* EMI4.SDRAMCLKSEL */
        STSYS_WriteRegDev32LE (STI5528_EMI_GEN_CFG ,0x00000208);   /* EMI4.GENCFG, shift for B2 */
    #endif
}


#endif
/*-------------------------------------------------------------------------
 * Function : PLL1_STmode_ChangeFreq
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void PLL1_STmode_ChangeFreq(void)
{
    #if defined(mb376)
        volatile unsigned int mdiv=0x1a,ndiv=0xc7,pdiv=0x0,setup85=0xe,setup30=0x8,fref=0,data=0;
    #else
        volatile unsigned int mdiv=0x1a,ndiv=0xc7,pdiv=0x0,setup85=0xe,setup30=0x8,fref=0,data=0;
    #endif
                                             
        
    mdiv = (mdiv&0xff);
    ndiv = (ndiv&0xff);
    pdiv = (pdiv&0x7);
    setup85 = (setup85 & 0xf);
    setup30 = (setup30 & 0xf);
    fref = 27;

    data = mdiv;
    data = data|(ndiv << 8);
    data = data|(pdiv << 16);
    data = data|(setup30 << 19);
    data = data|(setup85 << 24);

    STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC0DE); /* Unlock clock_gen registers */
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CTRL2,0x07); /* PLL1_CTRL2 */
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CTRL2,0x0F);
    STSYS_WriteRegDev8(STI5528_CKG_CPG_BYPASS,0x01);
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CTRL2,0x0A);
    STSYS_WriteRegDev32LE(STI5528_CKG_PLL1_CTRL,data);   /* PLL1_CTRL */ 
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CTRL2,0x0B);
    data = STSYS_ReadRegDev32LE(STI5528_CKG_PLL1_CTRL);
    while( (data & 0x80000000) == 0)
    {
    	data = STSYS_ReadRegDev32LE(STI5528_CKG_PLL1_CTRL);
    }
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CTRL2,0x0F);
       
    STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC1A0); /* Lock Configuration registers */
} 	

/*-------------------------------------------------------------------------
 * Function : PLL1_STmode_ChangeRatio
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void PLL1_STmode_ChangeRatio(void)
{
    volatile unsigned int clk1=0x00,clk2=0x03,clk3=0x02,clk4=0x02,clk5=0x02,clk6=0x03;
	    
    /*PLL1_STmode_ChangeRatio */
    STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC0DE);
    STSYS_WriteRegDev8(STI5528_CKG_CPG_BYPASS,0x01);
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CLK1_CTRL,clk1);
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CLK2_CTRL,clk2);
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CLK3_CTRL,clk3);
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CLK4_CTRL,clk4);
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CLK5_CTRL,clk5);
    STSYS_WriteRegDev8(STI5528_CKG_PLL1_CLK6_CTRL,clk6);
    STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC1A0); /* Lock Configuration registers */
}

/*-------------------------------------------------------------------------
 * Function : PLL2_STmode_ChangeFreq
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/   	
static void PLL2_STmode_ChangeFreq(void)
{
    #if defined(mb376)
        volatile unsigned int mdiv=0x1a,ndiv=0x63,pdiv=0x0,setup85=0xe,setup30=0x4,fref=0,data=0;
    #else
        volatile unsigned int mdiv=0x1a,ndiv=0xa5,pdiv=0x0,setup85=0xe,setup30=0x8,fref=0,data=0;
    #endif
        
    mdiv = (mdiv&0xff);
    ndiv = (ndiv&0xff);
    pdiv = (pdiv&0x7);
    setup85 = (setup85 & 0xf);
    setup30 = (setup30 & 0xf);
    fref = 27;

    data = mdiv;
    data = data|(ndiv << 8);
    data = data|(pdiv << 16);
    data = data|(setup30 << 19);
    data = data|(setup85 << 24);

    STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC0DE); /* Unlock clock_gen registers */
    data = STSYS_ReadRegDev32LE(STI5528_CKG_PLL2_CTRL);
    data = (data & 0x3fffffff);
    STSYS_WriteRegDev32LE(STI5528_CKG_PLL2_CTRL,data);
    data = (data|0x20000000);
    STSYS_WriteRegDev32LE(STI5528_CKG_PLL2_CTRL,data);

    data = (mdiv|0x30000000);
    data = data|(ndiv << 8);
    data = data|(pdiv << 16);
    data = data|(setup30 << 19);
    data = data|(setup85 << 24);
    STSYS_WriteRegDev32LE(STI5528_CKG_PLL2_CTRL,data);  

    data = (data&0x5fffffff);
    STSYS_WriteRegDev32LE(STI5528_CKG_PLL2_CTRL,data);
  
    data = STSYS_ReadRegDev32LE(STI5528_CKG_PLL2_CTRL);
    while ( (data & 0x80000000) == 0x0) 
    {
        data = STSYS_ReadRegDev32LE(STI5528_CKG_PLL2_CTRL);
    }

    data = (data|0x40000000);
    STSYS_WriteRegDev32LE(STI5528_CKG_PLL2_CTRL,data);
    STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC1A0);
}

/*-------------------------------------------------------------------------
 * Function : QSYNTH1_PCI_Start_and_ChangeFreq
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/   	
static void QSYNTH1_PCI_Start_and_ChangeFreq(void)
{
	unsigned int sdiv = 0x02, md = 0x1A, pe =0x68BA, data = 0x00;
	
	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC0DE); /* Unlock clock_gen registers */
 	data = STSYS_ReadRegDev32LE(STI5528_CKG_QSYNTH1_CLK1_CTRL);
 	data = (data & 0x00ffffff);
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK1_CTRL,(data|0x0f000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK1_CTRL,(data|0x0b000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK1_CTRL,(data|0x07000000));
 	sdiv = (sdiv&0x7);
  	md = (md&0x1f);
  	pe = (pe&0xffff);
  	data = pe;
  	data = data|(md << 16);
  	data = data|(sdiv << 21);
  	data = data&0xffffff;
  	
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK1_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK1_CTRL,(data|0x0e000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK1_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK1_CTRL,(data|0x07000000));
  	STSYS_WriteRegDev8(STI5528_CKG_QSYNTH1_CTRL,0x04);
  	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC1A0);
}
  
/*-------------------------------------------------------------------------
 * Function : QSYNTH1_USB_IRDA_Start_and_ChangeFreq
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/   	
static void QSYNTH1_USB_IRDA_Start_and_ChangeFreq(void)
{
	unsigned int sdiv = 0x02, md = 0x11, pe =0x00, data = 0x00;
	
	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC0DE); /* Unlock clock_gen registers */
	
	data = STSYS_ReadRegDev32LE(STI5528_CKG_QSYNTH1_CLK2_CTRL);
 	data = (data & 0x00ffffff);
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK2_CTRL,(data|0x0f000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK2_CTRL,(data|0x0b000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK2_CTRL,(data|0x07000000));
 	sdiv = (sdiv&0x7);
  	md = (md&0x1f);
  	pe = (pe&0xffff);
  	data = pe;
  	data = data|(md << 16);
  	data = data|(sdiv << 21);
  	data = data&0xffffff;
  	
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK2_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK2_CTRL,(data|0x0e000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK2_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH1_CLK2_CTRL,(data|0x07000000));
  	STSYS_WriteRegDev8(STI5528_IRDA_CLK_CTRL,0x00);
  	STSYS_WriteRegDev8(STI5528_CKG_QSYNTH1_CTRL,0x04);
  	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC1A0);
}
  
/*-------------------------------------------------------------------------
 * Function : QSYNTH2_ST20_Start_and_ChangeFreq
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/   	
static void QSYNTH2_ST20_Start_and_ChangeFreq(void)
{
	unsigned int sdiv = 0x00, md = 0x14, pe =0x1722, data = 0x00;
	
	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC0DE); /* Unlock clock_gen registers */
	
	data = STSYS_ReadRegDev32LE(STI5528_CKG_QSYNTH2_CLK1_CTRL);
 	data = (data & 0x00ffffff);
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK1_CTRL,(data|0x0f000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK1_CTRL,(data|0x0b000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK1_CTRL,(data|0x07000000));
 	sdiv = (sdiv&0x7);
  	md = (md&0x1f);
  	pe = (pe&0xffff);
  	data = pe;
  	data = data|(md << 16);
  	data = data|(sdiv << 21);
  	data = data&0xffffff;
  	
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK1_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK1_CTRL,(data|0x0e000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK1_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK1_CTRL,(data|0x07000000));
  	STSYS_WriteRegDev8(STI5528_CKG_QSYNTH1_CTRL,0x04);
  	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC1A0);
}

/*-------------------------------------------------------------------------
 * Function : QSYNTH2_HDD_Start_and_ChangeFreq
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/   	  
static void QSYNTH2_HDD_Start_and_ChangeFreq(void)
{
	unsigned int sdiv = 0x00, md = 0x19, pe =0x1ed, data = 0x00;
	
	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC0DE); /* Unlock clock_gen registers */
	
	data = STSYS_ReadRegDev32LE(STI5528_CKG_QSYNTH2_CLK2_CTRL);
 	data = (data & 0x00ffffff);
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK2_CTRL,(data|0x0f000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK2_CTRL,(data|0x0b000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK2_CTRL,(data|0x07000000));
 	sdiv = (sdiv&0x7);
  	md = (md&0x1f);
  	pe = (pe&0xffff);
  	data = pe;
  	data = data|(md << 16);
  	data = data|(sdiv << 21);
  	data = (data & 0xffffff);
  	
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK2_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK2_CTRL,(data|0x0e000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK2_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK2_CTRL,(data|0x07000000));
  	STSYS_WriteRegDev8(STI5528_CKG_QSYNTH2_CTRL,0x04);
  	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC1A0);
}

/*-------------------------------------------------------------------------
 * Function : QSYNTH2_SMARTCARD_Start_and_ChangeFreq
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/   	    
static void QSYNTH2_SMARTCARD_Start_and_ChangeFreq(void)
{
	unsigned int sdiv = 0x02, md = 0x1f, pe =0x00, data = 0x00;
	
	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC0DE); /* Unlock clock_gen registers */
	
	data = STSYS_ReadRegDev32LE(STI5528_CKG_QSYNTH2_CLK3_CTRL);
 	data = (data & 0x00ffffff);
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK3_CTRL,(data|0x0f000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK3_CTRL,(data|0x0b000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK3_CTRL,(data|0x07000000));
 	sdiv = (sdiv&0x7);
  	md = (md&0x1f);
  	pe = (pe&0xffff);
  	data = pe;
  	data = data|(md << 16);
  	data = data|(sdiv << 21);
  	data = data & 0xffffff;
  	
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK3_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK3_CTRL,(data|0x0e000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK3_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK3_CTRL,(data|0x07000000));
  	STSYS_WriteRegDev8(STI5528_CKG_QSYNTH2_CTRL,0x04);
  	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC1A0);
}

/*-------------------------------------------------------------------------
 * Function : QSYNTH2_PIXEL_Start_and_ChangeFreq
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/   	      
static void QSYNTH2_PIXEL_Start_and_ChangeFreq(void)
{
	unsigned int sdiv = 0x02, md = 0x1d, pe =0x5b1e, data = 0x00;
	
	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC0DE); /* Unlock clock_gen registers */
	
	data = STSYS_ReadRegDev32LE(STI5528_CKG_QSYNTH2_CLK4_CTRL);
 	data = (data & 0x00ffffff);
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK4_CTRL,(data|0x0f000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK4_CTRL,(data|0x0b000000));
 	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK4_CTRL,(data|0x07000000));
 	sdiv = (sdiv&0x7);
  	md = (md&0x1f);
  	pe = (pe&0xffff);
  	data = pe;
  	data = data|(md << 16);
  	data = data|(sdiv << 21);
  	data = data&0xffffff;
  	
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK4_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK4_CTRL,(data|0x0e000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK4_CTRL,(data|0x0f000000));
  	STSYS_WriteRegDev32LE(STI5528_CKG_QSYNTH2_CLK4_CTRL,(data|0x07000000));
  	STSYS_WriteRegDev8(STI5528_CKG_QSYNTH2_CTRL,0x04);
  	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC1A0);
}

/*-------------------------------------------------------------------------
 * Function : Mux_and_Other_Dividers
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/   	        
static void Mux_and_Other_Dividers(void)
{
    unsigned int data = 0x00 ;
	STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC0DE); /* Unlock clock_gen registers */
	STSYS_WriteRegDev8(STI5528_CKG_ST20_CLK_CTRL,0x00);
	
	data = STSYS_ReadRegDev8(STI5528_CKG_PIX_CLK_CTRL);
	data = (data|0x4);
	STSYS_WriteRegDev8(STI5528_CKG_PIX_CLK_CTRL,data);
	data = (data&0xfffffffc);
    data = (data&0xfffffffb);
    STSYS_WriteRegDev8(STI5528_CKG_PIX_CLK_CTRL,(data|0x4));
    STSYS_WriteRegDev8(STI5528_CKG_PIX_CLK_CTRL,data);
    data = STSYS_ReadRegDev8(STI5528_CKG_DVP_CLK_CTRL);
	STSYS_WriteRegDev8(STI5528_CKG_DVP_CLK_CTRL,(data|0x2));
	data = (data&0xfffffffe);
    data=(data&0xfffffffd);
    STSYS_WriteRegDev8(STI5528_CKG_DVP_CLK_CTRL,(data|0x2));
    STSYS_WriteRegDev8(STI5528_CKG_DVP_CLK_CTRL,data);
    STSYS_WriteRegDev16LE(STI5528_CKG_LP_CLK_CTRL,0xB4B);
    STSYS_WriteRegDev8(STI5528_CKG_PWM_CLK_CTRL,0x00);
    STSYS_WriteRegDev8(STI5528_CKG_AUD_CLK_REF_CTRL,0x00);
    STSYS_WriteRegDev16LE(STI5528_CKG_ST20_TICK_CTRL,0x1B);
    STSYS_WriteRegDev16LE(STI5528_CKG_AUDBIT_CLK_CTRL,0x01);
    STSYS_WriteRegDev8(STI5528_CKG_CKOUT_CTRL,0x00);
    STSYS_WriteRegDev16LE(STI5528_CKG_LOCK,0xC1A0);
}



/*-------------------------------------------------------------------------
 * Function : Interconnect_Init
 * Input    : None
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/   	        
static void Interconnect_Init(void)
{

    STSYS_WriteRegDev32LE(STI5528_GDP0_DISP_PRIORITY, 0x00000007)
    STSYS_WriteRegDev32LE(STI5528_GDP0_LL_PRIORITY,0x00000003)
    STSYS_WriteRegDev32LE(STI5528_GDP1_DISP_PRIORITY,0x00000006)
    STSYS_WriteRegDev32LE(STI5528_GDP1_LL_PRIORITY,0x00000002)
    STSYS_WriteRegDev32LE(STI5528_GDP2_DISP_PRIORITY,0x00000005)
    STSYS_WriteRegDev32LE(STI5528_GDP2_LL_PRIORITY, 0x00000001)
    STSYS_WriteRegDev32LE(STI5528_GDP3_DISP_PRIORITY,0x00000004)
    STSYS_WriteRegDev32LE(STI5528_DISP0_IC_LUMA_PRIORITY,0x00000005)
    STSYS_WriteRegDev32LE(STI5528_DISP0_IC_CHROMA_PRIORITY,0x00000003)
    STSYS_WriteRegDev32LE(STI5528_DISP1_IC_LUMA_PRIORITY,0x00000004)
    STSYS_WriteRegDev32LE(STI5528_DISP1_IC_CHROMA_PRIORITY, 0x00000002)
    STSYS_WriteRegDev32LE(STI5528_DVP_IC_ANC_PRIORITY,0x00000001)
    STSYS_WriteRegDev32LE(STI5528_N04_EMI_PRIORITY,0x00000003)
    STSYS_WriteRegDev32LE(STI5528_N04_LMI_PRIORITY,0x00000005)
    STSYS_WriteRegDev32LE(STI5528_N05_EMI_PRIORITY,0x00000004)
    STSYS_WriteRegDev32LE(STI5528_N05_LMI_PRIORITY,0x00000006)
    STSYS_WriteRegDev32LE(STI5528_N06_PRIORITY,0x00000002)
    STSYS_WriteRegDev32LE(STI5528_N07_PRIORITY,0x00000001)
    STSYS_WriteRegDev32LE(STI5528_CPUPLUG_IC_PRIORITY,0x00000004)
    STSYS_WriteRegDev32LE(STI5528_ST20_IC_ID_PRIORITY,0x00000003)
    STSYS_WriteRegDev32LE(STI5528_N02_PRIORITY,0x00000006)
    STSYS_WriteRegDev32LE(STI5528_N03_PRIORITY,0x00000002)
    STSYS_WriteRegDev32LE(STI5528_N08_LMI_PRIORITY,0x00000005)
    STSYS_WriteRegDev32LE(STI5528_N09_PRIORITY,0x00000001)
    STSYS_WriteRegDev32LE(STI5528_CPUPLUG_IC_LATENCY,0x00000000)
    STSYS_WriteRegDev32LE(STI5528_ST20_IC_ID_LATENCY,0x00000000)
    STSYS_WriteRegDev32LE(STI5528_N03_LATENCY, 0x00000000)
    STSYS_WriteRegDev32LE(STI5528_N08_LMI_LATENCY,0x00000000)
    STSYS_WriteRegDev32LE(STI5528_N09_LATENCY,0x00000000)
}
