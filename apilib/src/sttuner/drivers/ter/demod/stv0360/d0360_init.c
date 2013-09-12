#ifdef ST_OSLINUX
   #include "stos.h"
#else

 /*C libs */
#include "string.h"
#endif

#include "360_init.h"

#ifdef HOST_PC
#include "gen_types.h"
#endif

#include "360_map.h"

/*********To ADD Array consisting addresses of STV0360*****************/
U8 Def360Addr[STV360_NBREGS]={
/*R_ID*/             0x00, 
/*R_I2CRPT*/         0x01, 
/*R_TOPCTRL*/        0x02 ,
/*R_IOCFG0*/         0x03, 
/*R_DAC0R*/          0x04, 

/*R_IOCFG1*/         0x05, 
/*R_DAC1R*/          0x06, 
/*R_IOCFG2*/         0x07, 
/*R_PWMFR*/          0x08,
/*R_STATUS*/         0x09,

/*R_AUX_CLK*/        0x0a,
/*R_FREESYS1*/       0x0b,
/*R_FREESYS2*/       0x0c,
/*R_FREESYS3*/       0x0d,
/*R_AGC2MAX*/        0x10, 

/*R_AGC2MIN*/        0x11, 
/*R_AGC1MAX*/        0x12, 
/*R_AGC1MIN*/        0x13, 
/*R_AGCR*/           0x14, 
/*R_AGC2TH*/         0x15, 

/*R_AGC12C3*/        0x16,
/*R_AGCCTRL1*/       0x17, 
/*R_AGCCTRL2*/       0x18,
/*R_AGC1VAL1*/       0x19,
/*R_AGC1VAL2*/       0x1a,

/*R_AGC2VAL1*/       0x1b, 
/*R_AGC2VAL2*/       0x1c, 
/*R_AGC2PGA*/        0x1d, 
/*R_OVF_RATE1*/      0x1e, 
/*R_OVF_RATE2*/      0x1f, 

/*R_GAIN_SRC1*/      0x20, 
/*R_GAIN_SRC2*/      0x21,
/*R_INC_DEROT1*/     0x22,
/*R_INC_DEROT2*/     0x23,
/*R_FREESTFE_1*/     0x26,

/*R_SYR_THR*/        0x27,
/*R_INR*/            0x28, 
/*R_EN_PROCESS*/     0x29, 
/*R_SDI_SMOOTHER*/   0x2A, 
/*R_FE_LOOP_OPEN*/   0x2B, 

/*R_EPQ*/            0x31, 
/*R_EPQ2*/           0x32,
/*R_COR_CTL*/        0x80,
/*R_COR_STAT*/       0x81,
/*R_COR_INTEN*/      0x82,

/*R_COR_INTSTAT*/    0x83,
/*R_COR_MODEGUARD*/  0x84,
/*R_AGC_CTL*/        0x85,
/*R_AGC_MANUAL1*/    0x86,
/*R_AGC_MANUAL2*/    0x87,

/*R_AGC_TARGET*/     0x88,
/*R_AGC_GAIN1*/      0x89,
/*R_AGC_GAIN2*/      0x8a,
/*R_ITB_CTL*/        0x8b,
/*R_ITB_FREQ1*/      0x8c,

/*R_ITB_FREQ2*/      0x8d,
/*R_CAS_CTL*/        0x8e,
/*R_CAS_FREQ*/       0x8f,
/*R_CAS_DAGCGAIN*/   0x90,
/*R_SYR_CTL*/        0x91,

/*R_SYR_STAT*/       0x92,
/*R_SYR_NC01*/       0x93,
/*R_SYR_NC02*/       0x94,
/*R_SYR_OFFSET1*/    0x95,
/*R_SYR_OFFSET2*/    0x96,

/*R_FFT_CTL*/        0x97,
/*R_SCR_CTL*/        0x98,
/*R_PPM_CTL1*/       0x99,
/*R_TRL_CTL*/        0x9a,
/*R_TRL_NOMRATE1*/   0x9b,

/*R_TRL_NOMRATE2*/   0x9c,
/*R_TRL_TIME1*/      0x9d,
/*R_TRL_TIME2*/      0x9e,
/*R_CRL_CTL*/        0x9f,
/*R_CRL_FREQ1*/      0xa0,

/*R_CRL_FREQ2*/      0xa1,
/*R_CRL_FREQ3*/      0xa2,
/*R_CHC_CTL1*/       0xa3,
/*R_CHC_SNR*/        0xa4,
/*R_BDI_CTL*/        0xa5,

/*R_DMP_CTL*/        0xa6,
/*R_TPS_RCVD1*/      0xa7,
/*R_TPS_RCVD2*/      0xa8,
/*R_TPS_RCVD3*/      0xa9,
/*R_TPS_RCVD4*/      0xaa,

/*R_TPS_CELLID*/     0xab,
/*R_TPS_FREE2*/      0xac,
/*R_TPS_SET1*/       0xad,
/*R_TPS_SET2*/       0xae,
/*R_TPS_SET3*/       0xaf,

/*R_TPS_CTL*/        0xb0,
/*R_CTL_FFTOSNUM*/   0xb1,
/*R_CAR_DISP_SEL*/   0xb2,
/*R_MSC_REV*/        0xb3,
/*R_PIR_CTL*/        0xb4,

/*R_SNR_CARRIER1*/   0xb5,
/*R_SNR_CARRIER2*/   0xb6,
/*R_PPM_CPAMP*/      0xb7,
/*R_TSM_AP0*/        0xb8,
/*R_TSM_AP1*/        0xb9,

/*R_TSM_AP2*/        0xba,
/*R_TSM_AP3*/        0xbb,
/*R_TSM_AP4*/        0xbc,
/*R_TSM_AP5*/        0xbd,
/*R_TSM_AP6*/        0xbe,

/*R_TSM_AP7*/        0xbf,
/*R_CONSTMODE*/      0xcb,
/*R_CONSTCARR1*/     0xcc,
/*R_CONSTCARR2*/     0xcd,
/*R_ICONSTEL*/       0xce,

/*R_QCONSTEL*/       0xcf,
/*R_AGC1RF*/         0xD4,
/*R_EN_RF_AGC1*/     0xD5,
/*R_FECM*/           0x40,
/*R_VTH0*/           0x41,

/*R_VTH1*/           0x42,
/*R_VTH2*/           0x43,
/*R_VTH3*/           0x44,
/*R_VTH4*/           0x45,
/*R_VTH5*/           0x46,

/*R_FREEVIT*/        0x47,
/*R_VITPROG*/        0x49,
/*R_PR*/             0x4a,
/*R_VSEARCH*/        0x4b,
/*R_RS*/             0x4c,

/*R_RSOUT*/          0x4d,
/*R_ERRCTRL1*/       0x4e,
/*R_ERRCNTM1*/       0x4f,
/*R_ERRCNTL1*/       0x50,
/*R_ERRCTRL2*/       0x51,

/*R_ERRCNTM2*/       0x52,
/*R_ERRCNTL2*/       0x53,
/*R_ERRCTRL3*/       0x56,
/*R_ERRCNTM3*/       0x57,
/*R_ERRCNTL3*/       0x58,

/*R_DILSTK1*/        0x59,
/*R_DILSTK0*/        0x5A,
/*R_DILBWSTK1*/      0x5B,
/*R_DILBWST0*/       0x5C,
/*R_LNBRX*/          0x5D,

/*R_RSTC*/           0x5E,
/*R_VIT_BIST*/       0x5F,
/*R_FREEDRS*/        0x54,
/*R_VERROR*/         0x55,
/*R_TSTRES*/         0xc0,

/*R_ANACTRL*/        0xc1,
/*R_TSTBUS*/         0xc2,
/*R_TSTCK*/          0xc3,
/*R_TSTI2C*/         0xc4,
/*R_TSTRAM1*/        0xc5,

/*R_TSTRATE*/        0xc6,
/*R_SELOUT*/         0xc7,
/*R_FORCEIN*/        0xc8,
/*R_TSTFIFO*/        0xc9,
/*R_TSTRS*/          0xca,

/*R_TSTBISTRES0*/    0xd0,
/*R_TSTBISTRES1*/    0xd1,
/*R_TSTBISTRES2*/    0xd2,
/*R_TSTBISTRES3*/    0xd3

	        };/** end of array**/

STCHIP_Handle_t STV0360_Init(STV0360_InitParams_t *InitParams)
{
    STCHIP_Handle_t hChip;
    U8             *DefVal;
    U8 * Addr;
    
    #ifndef STTUNER_REG_INIT_OLD_METHOD 
    
    U8 fieldpattern[STV360_NBREGS]= {0x00,0x75,0x55,0x50,0x00,0x50,0x00,0x17,0x0F,0x57,
                                     0x59,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,
                                     0x54,0x77,0x67,0x00,0x0F,0x00,0x0F,0x3F,0x0F,0x00,
                                     0x50,0x00,0x00,0x00,0x05,0x20,0x00,0x01,0x40,0x02,
                                     0x00,0x00,0x2F,0x50,0x55,0x2A,0x0B,0x12,0x00,0x0F,
                                     0x00,0x00,0x10,0x01,0x00,0x3F,0x58,0x00,0x00,0x7A,
                                     0x14,0x00,0x3F,0x00,0x3F,0x05,0x02,0x32,0x47,0x00,
                                     0x00,0x00,0x00,0x47,0x00,0x00,0x7F,0x15,0x00,0x02,
                                     0x01,0x53,0x73,0x77,0x33,0x00,0x00,0x03,0x73,0x77,
                                     0x05,0x00,0x0F,0x00,0x01,0x00,0x60,0x00,0x00,0x00,
                                     0x00,0x00,0x00,0x00,0x00,0x00,0x1B,0x00,0x1F,0x00,
                                     0x00,0x00,0x7F,0x0A,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
                                     0x00,0x3C,0x55,0x4C,0x55,0x10,0x4B,0x00,0x00,0x4B,
                                     0x00,0x00,0x4B,0x00,0x00,0x00,0x00,0x00,0x00,0x40,
                                     0x55,0x47,0x00,0x00,0x55,0x55,0x47,0x75,0x68,0x5C,
                                     0x68,0x55,0x55,0x05,0x4D,0x55,0x55,0x55,0x02	};
	
	U8 Rindex = 0;
	U16 Findex = 0;
	U8 fieldwidth = 1;
	U8 bitvalue = 0;
	U8 temppatternstore = 0;
	S8 bitposition = 7;
	
	Addr = Def360Addr;
	InitParams->Chip->NbRegs   = STV360_NBREGS;
    InitParams->Chip->NbFields = STV360_NBFIELDS;
    InitParams->Chip->ChipMode = STCHIP_MODE_SUBADR_8;

if(InitParams->NbDefVal == STV360_NBREGS)
{
        hChip = ChipOpen(InitParams->Chip);

        DefVal = InitParams->DefVal;

if(hChip != NULL)
{
	for (Rindex=0;Rindex<STV360_NBREGS;Rindex++)
	{
		ChipAddReg(hChip,Rindex,"\0",*Addr++ ,*DefVal++,STCHIP_ACCESS_WR);
		temppatternstore=fieldpattern[Rindex];
		bitvalue = 0;
		for (bitposition=7;bitposition>=0;bitposition--)
		{
				temppatternstore = temppatternstore<<1;
				if (((temppatternstore & 0x80/*1000 0000*/)==bitvalue) && (bitposition!=0))
				{
					fieldwidth++;
				}
				else
				{
					ChipAddField(hChip,Rindex,Findex,"\0",bitposition,fieldwidth,CHIP_UNSIGNED);
					/*STTBX_Print(("\n\nChipAddField(hChip,Rindex%d,Findex%d,nullstring,(bitposition)%d,fieldwidth%d,CHIP_UNSIGNED)\n\n",Rindex,Findex,bitposition,fieldwidth));*/
					fieldwidth=1;
					Findex++;
					bitvalue = ((bitvalue==0) ? (bitvalue=0x80) : (bitvalue=0));
				}
		}/*end for loop on bitposition*/
	}/*end for loop on Rindex*/
}

ChipAddField(hChip,R_SYR_CTL,LONG_ECHO,"LONG_ECHO",3,4,CHIP_SIGNED);
ChipAddField(hChip,R_ICONSTEL,ICONSTEL,"\0",0,8,CHIP_SIGNED);
ChipAddField(hChip,R_QCONSTEL,QCONSTEL,"\0",0,8,CHIP_SIGNED);
ChipApplyDefaultValues(hChip);
}
else
{
   return (STCHIP_Handle_t) NULL;
}
    
   #else

    /*
    **   REGISTER CONFIGURATION
    **     ----------------------
    */


	/* fill elements of external chip data structure */
	InitParams->Chip->NbRegs   = STV360_NBREGS;
	InitParams->Chip->NbFields = STV360_NBFIELDS;
	InitParams->Chip->ChipMode = STCHIP_MODE_SUBADR_8;



    if(InitParams->NbDefVal != STV360_NBREGS) return (STCHIP_Handle_t) NULL;

    hChip = ChipOpen(InitParams->Chip);

    DefVal = InitParams->DefVal; 


	if(hChip != NULL)
	
	{
	


/*  REGISTER INITIALISATION */
            /*  ID  00000000 */ 
            ChipAddReg(hChip,R_ID,"ID",0x00,*DefVal++/*0x21*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ID,IDENTIFICATIONREGISTER,"IDENTIFICATIONREGISTER",0,8,CHIP_UNSIGNED);

            /*  I2CRPT  0111010R  01110101*/
            ChipAddReg(hChip,R_I2CRPT,"I2CRPT",0x01,*DefVal++/*0x27*/,STCHIP_ACCESS_WR); /* with no bug on I2Crpt 0xA2 */
            ChipAddField(hChip,R_I2CRPT,I2CT_ON,"I2CT_ON",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_I2CRPT,ENARPT_LEVEL,"ENARPT_LEVEL",4,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_I2CRPT,SCLT_DELAY,"SCLT_DELAY",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_I2CRPT,SCLT_NOD,"SCLT_NOD",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_I2CRPT,STOP_ENABLE,"STOP_ENABLE",1,1,CHIP_UNSIGNED);

            /*  TOPCTRL 01010101 */
            ChipAddReg(hChip,R_TOPCTRL,"TOPCTRL",0x02,*DefVal++/*0x2*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TOPCTRL,STDBY,"STDBY",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TOPCTRL,STDBY_FEC,"STDBY_FEC",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TOPCTRL,STDBY_CORE,"STDBY_CORE",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TOPCTRL,DIR_CLK,"DIR_CLK",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TOPCTRL,TS_DIS,"TS_DIS",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TOPCTRL,TQFP80,"TQFP80",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TOPCTRL,BYPASS_PGA,"BYPASS_PGA",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TOPCTRL,ENA_27,"ENA_27",0,1,CHIP_UNSIGNED);

            /*  IOCFG0   01010000 */
            ChipAddReg(hChip,R_IOCFG0,"IOCFG0",0x03,*DefVal++/*0x40*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_IOCFG0,OP0_SD,"OP0_SD",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_IOCFG0,OP0_VAL,"OP0_VAL",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_IOCFG0,OP0_OD,"OP0_OD",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_IOCFG0,OP0_INV,"OP0_INV",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_IOCFG0,OP0_DACVALUE_HI,"OP0_DACVALUE_HI",0,4,CHIP_UNSIGNED);

            /*  DAC0R  00000000 */
            ChipAddReg(hChip,R_DAC0R,"DAC0R",0x04,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_DAC0R,OP0_DACVALUE_LO,"OP0_DACVALUE_LO",0,8,CHIP_UNSIGNED);

            /*  IOCFG1  R1010000 01010000 */
            ChipAddReg(hChip,R_IOCFG1,"IOCFG1",0x05,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            /*ChipAddField(hChip,R_IOCFG1,OP1_SD,"OP1_SD",7,1,CHIP_UNSIGNED);*/
            ChipAddField(hChip,R_IOCFG1,IP0,"IP0",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_IOCFG1,OP1_OD,"OP1_OD",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_IOCFG1,OP1_INV,"OP1_INV",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_IOCFG1,OP1_DACVALUE_HI,"OP1_DACVALUE_HI",0,4,CHIP_UNSIGNED);

            /*  DAC1R   00000000*/
            ChipAddReg(hChip,R_DAC1R,"DAC1R",0x06,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_DAC1R,OP1_DACVALUE_LO,"OP1_DACVALUE_LO",0,8,CHIP_UNSIGNED);

            /*  IOCFG2  00010111 */
            ChipAddReg(hChip,R_IOCFG2,"IOCFG2",0x07,*DefVal++/*0x80*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_IOCFG2,OP2_LOCK_CONF,"OP2_LOCK_CONF",5,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_IOCFG2,OP2_OD,"OP2_OD",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_IOCFG2,OP2_VAL,"OP2_VAL",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_IOCFG2,OP1_LOCK_CONF,"OP1_LOCK_CONF",0,3,CHIP_UNSIGNED);

            /*  PWMFR    00001111 */
            ChipAddReg(hChip,R_PWMFR,"PWMFR",0x08,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_PWMFR,OP0_FREQ,"OP0_FREQ",4,4,CHIP_UNSIGNED);
            ChipAddField(hChip,R_PWMFR,OP1_FREQ,"OP1_FREQ",0,4,CHIP_UNSIGNED);

            /*  STATUS  01010111 */
            ChipAddReg(hChip,R_STATUS,"STATUS",0x09,*DefVal++/*0xf9*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_STATUS,TPS_LOCK,"TPS_LOCK",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_STATUS,SYR_LOCK,"SYR_LOCK",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_STATUS,AGC_LOCK,"AGC_LOCK",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_STATUS,PRF,"PRF",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_STATUS,LK,"LK",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_STATUS,PR,"PR",0,3,CHIP_UNSIGNED);

            /*  AUX_CLK 01011001 */
            ChipAddReg(hChip,R_AUX_CLK,"AUX_CLK",0x0a,*DefVal++/*0x1c*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AUX_CLK,AUXFEC12B,"AUXFEC12B",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AUX_CLK,AUXFEC_ENA,"AUXFEC_ENA",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AUX_CLK,DIS_CKX4,"DIS_CKX4",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AUX_CLK,CKSEL,"CKSEL",3,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AUX_CLK,CKDIV_PROG,"CKDIV_PROG",1,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AUX_CLK,AUXCLK_ENA,"AUXCLK_ENA",0,1,CHIP_UNSIGNED);

            /*  FREESYS1    RRRRRRRR 00000000 */
            ChipAddReg(hChip,R_FREESYS1,"FREESYS1",0x0b,*DefVal++/*0x0*/,STCHIP_ACCESS_WR);

            /*  FREESYS2   RRRRRRRR 00000000 */
            ChipAddReg(hChip,R_FREESYS2,"FREESYS2",0x0c,*DefVal++/*0x0*/,STCHIP_ACCESS_WR);

            /*  FREESYS3   RRRRRRRR 00000000 */
            ChipAddReg(hChip,R_FREESYS3,"FREESYS3",0x0d,*DefVal++/*0x0*/,STCHIP_ACCESS_WR);

            /*  AGC2MAX  00000000*/
            ChipAddReg(hChip,R_AGC2MAX,"AGC2MAX",0x10,*DefVal++/*0x8c*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC2MAX,AGC2MAX,"AGC2MAX",0,8,CHIP_UNSIGNED);

            /*  AGC2MIN  00000000*/
            ChipAddReg(hChip,R_AGC2MIN,"AGC2MIN",0x11,*DefVal++/*0x28*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC2MIN,AGC2MIN,"AGC2MIN",0,8,CHIP_UNSIGNED);

            /*  AGC1MAX 00000000*/
            ChipAddReg(hChip,R_AGC1MAX,"AGC1MAX",0x12,*DefVal++/*0xFF*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC1MAX,AGC1MAX,"AGC1MAX",0,8,CHIP_UNSIGNED);

            /*  AGC1MIN 00000000*/
            ChipAddReg(hChip,R_AGC1MIN,"AGC1MIN",0x13,*DefVal++/*0x6B*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC1MIN,AGC1MIN,"AGC1MIN",0,8,CHIP_UNSIGNED);

            /*  AGCR    00011000 */
            ChipAddReg(hChip,R_AGCR,"AGCR",0x14,*DefVal++/*0xBC*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGCR,RATIO_A,"RATIO_A",5,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGCR,RATIO_B,"RATIO_B",3,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGCR,RATIO_C,"RATIO_C",0,3,CHIP_UNSIGNED);



            /*  AGC2TH  00000000 */
            ChipAddReg(hChip,R_AGC2TH,"AGC2TH",0x15,*DefVal++/*0x0c*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC2TH,AGC2_THRES,"AGC2_THRES",0,8,CHIP_UNSIGNED);

            /*  AGC12C3 01010100*/
            ChipAddReg(hChip,R_AGC12C3,"AGC12C3",0x16,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC12C3,AGC1_IV,"AGC1_IV",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC12C3,AGC1_OD,"AGC1_OD",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC12C3,AGC1_LOAD,"AGC1_LOAD",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC12C3,AGC2_IV,"AGC2_IV",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC12C3,AGC2_OD,"AGC2_OD",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC12C3,AGC2_LOAD,"AGC2_LOAD",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC12C3,AGC12_MODE,"AGC12_MODE",0,2,CHIP_UNSIGNED);

            /*  AGCCTRL1  0x0RRR0111 0x01110111  */
            ChipAddReg(hChip,R_AGCCTRL1,"AGCCTRL1",0x17,*DefVal++/*0x85*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGCCTRL1,DAGC_ON,"DAGC_ON",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGCCTRL1,AGC1_MODE,"AGC1_MODE",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGCCTRL1,AGC2_MODE,"AGC2_MODE",0,3,CHIP_UNSIGNED);

            /*  AGCCTRL2    R1100111 01100111 */
            ChipAddReg(hChip,R_AGCCTRL2,"AGCCTRL2",0x18,*DefVal++/*0x1f*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGCCTRL2,FRZ2_CTRL,"FRZ2_CTRL",5,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGCCTRL2,FRZ1_CTRL,"FRZ1_CTRL",3,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGCCTRL2,TIME_CST,"TIME_CST",0,3,CHIP_UNSIGNED);

            /*  AGC1VAL1    00000000 */
            ChipAddReg(hChip,R_AGC1VAL1,"AGC1VAL1",0x19,*DefVal++/*0xFF*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC1VAL1,AGC1_VAL_LO,"AGC1_VAL_LO",0,8,CHIP_UNSIGNED);

            /*  AGC1VAL2    RRRR1111 00001111 */
            ChipAddReg(hChip,R_AGC1VAL2,"AGC1VAL2",0x1a,*DefVal++/*0xF*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC1VAL2,AGC1_VAL_HI,"AGC1_VAL_HI",0,4,CHIP_UNSIGNED);

            /*  AGC2VAL1   00000000 */
            ChipAddReg(hChip,R_AGC2VAL1,"AGC2VAL1",0x1b,*DefVal++/*0xFF*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC2VAL1,AGC2_VAL_LO,"AGC2_VAL_LO",0,8,CHIP_UNSIGNED);

            /*  AGC2VAL2    RRRR1111 00001111*/
            ChipAddReg(hChip,R_AGC2VAL2,"AGC2VAL2",0x1c,*DefVal++/*0x0F*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC2VAL2,AGC2_VAL_HI,"AGC2_VAL_HI",0,4,CHIP_UNSIGNED);

            /*  AGC2PGA RR111111 00111111 */
            ChipAddReg(hChip,R_AGC2PGA,"AGC2PGA",0x1d,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC2PGA,UAGC2PGA,"UAGC2PGA",0,6,CHIP_UNSIGNED);

            /*  OVF_RATE1   RRRR1111 00001111 */
            ChipAddReg(hChip,R_OVF_RATE1,"OVF_RATE1",0x1e,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_OVF_RATE1,OVF_RATE_HI,"OVF_RATE_HI",0,4,CHIP_UNSIGNED);

            /*  OVF_RATE2   00000000*/
            ChipAddReg(hChip,R_OVF_RATE2,"OVF_RATE2",0x1f,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_OVF_RATE2,OVF_RATE_LO,"OVF_RATE_LO",0,8,CHIP_UNSIGNED);

            /*  GAIN_SRC1   010R0000 01010000*/
            /*ChipAddReg(hChip,R_GAIN_SRC1,"GAIN_SRC1",0x20,0xCF,STCHIP_ACCESS_WR);gbgb*/
            ChipAddReg(hChip,R_GAIN_SRC1,"GAIN_SRC1",0x20,*DefVal++/*0xCA*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_GAIN_SRC1,INV_SPECTR,"INV_SPECTR",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_GAIN_SRC1,IQ_INVERT,"IQ_INVERT",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_GAIN_SRC1,INR_BYPASS,"INR_BYPASS",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_GAIN_SRC1,GAIN_SRC_HI,"GAIN_SRC_HI",0,4,CHIP_UNSIGNED);  /**/

            /*  GAIN_SRC2   00000000*/
            /*ChipAddReg(hChip,R_GAIN_SRC2,"GAIN_SRC2",0x21,0x3C,STCHIP_ACCESS_WR);gbgb*/
            ChipAddReg(hChip,R_GAIN_SRC2,"GAIN_SRC2",0x21,*DefVal++/*0x41*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_GAIN_SRC2,GAIN_SRC_LO,"GAIN_SRC_LO",0,8,CHIP_UNSIGNED);

            /*  INC_DEROT1  00000000 */
            /*ChipAddReg(hChip,R_INC_DEROT1,"INC_DEROT1",0x22,0x3F,STCHIP_ACCESS_WR); 24MHz*/
            ChipAddReg(hChip,R_INC_DEROT1,"INC_DEROT1",0x22,*DefVal++/*0x55*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_INC_DEROT1,INC_DEROT_HI,"INC_DEROT_HI",0,8,CHIP_UNSIGNED);

            /*  INC_DEROT2  00000000*/
            /*ChipAddReg(hChip,R_INC_DEROT2,"INC_DEROT2",0x23,0xF7,STCHIP_ACCESS_WR); 24MHz*/
            ChipAddReg(hChip,R_INC_DEROT2,"INC_DEROT2",0x23,*DefVal++/*0x48*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_INC_DEROT2,INC_DEROT_LO,"INC_DEROT_LO",0,8,CHIP_UNSIGNED);

            /*  INC_CLKENA1 */
            /*
            ChipAddReg(hChip,R_INC_CLKENA1,"INC_CLKENA1",0x24,0x00,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_INC_CLKENA1,INC_CLKENA_HI,"INC_CLKENA_HI",0,8,CHIP_UNSIGNED);
            */
            /*  INC_CLKENA2 */
            /*
            ChipAddReg(hChip,R_INC_CLKENA2,"INC_CLKENA2",0x25,0x00,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_INC_CLKENA2,INC_CLKENA_LO,"INC_CLKENA_LO",0,8,CHIP_UNSIGNED);
            */
            /*  FREESTFE_1  RRRRR101 00000101 */
            ChipAddReg(hChip,R_FREESTFE_1,"FREESTFE_1",0x26,*DefVal++/*0x03*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_FREESTFE_1,AVERAGE_ON,"AVERAGE_ON",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FREESTFE_1,DC_ADJ,"DC_ADJ",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FREESTFE_1,SEL_LSB,"SEL_LSB",0,1,CHIP_UNSIGNED);

            /*  SYR_THR  00R00000 00100000 */
            ChipAddReg(hChip,R_SYR_THR,"SYR_THR",0x27,*DefVal++/*0x1C*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SYR_THR,SEL_SRCOUT,"SEL_SRCOUT",6,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SYR_THR,SEL_SYRTHR,"SEL_SYRTHR",0,5,CHIP_UNSIGNED);

/* cut 2.0 registers */

            /*  INR  00000000 */
            ChipAddReg(hChip,R_INR,"INR",0x28,*DefVal++/*0xFF*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_INR,INR,"INR",0,8,CHIP_UNSIGNED);

            /*  EN_PROCESS  RRRRRRR1 00000001 */
            ChipAddReg(hChip,R_EN_PROCESS,"EN_PROCESS",0x29,*DefVal++/*0x1*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_EN_PROCESS,ENAB,"ENAB",0,1,CHIP_UNSIGNED);


            /*  SDI_SMOOTHER  0R000000 01000000*/
            ChipAddReg(hChip,R_SDI_SMOOTHER,"SDI_SMOOTHER",0x2A,*DefVal++/*0xFF*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SDI_SMOOTHER,DIS_SMOOTH,"DIS_SMOOTH",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SDI_SMOOTHER,SDI_INC_SMOOTHER,"SDI_INC_SMOOTHER",0,6,CHIP_UNSIGNED);


            /*  FE_LOOP_OPEN  RRRRRR10 00000010 */
            ChipAddReg(hChip,R_FE_LOOP_OPEN,"FE_LOOP_OPEN",0x2B,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_FE_LOOP_OPEN,TRL_LOOP_OP,"TRL_LOOP_OP",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FE_LOOP_OPEN,CRL_LOOP_OP,"CRL_LOOP_OP",0,1,CHIP_UNSIGNED);

            /*  EPQ  0x00000000 */
            ChipAddReg(hChip,R_EPQ,"EPQ",0x31,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_EPQ,EPQ,"EPQ",0,8,CHIP_UNSIGNED);

            /*  EPQ2  0x00000000*/
            ChipAddReg(hChip,R_EPQ2,"EPQ2",0x32,*DefVal++/*0x15*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_EPQ2,EPQ2,"EPQ2",0,8,CHIP_UNSIGNED);

/*end */

            /*  COR_CTL RR101111 00101111 */
            ChipAddReg(hChip,R_COR_CTL,"COR_CTL",0x80,*DefVal++/*0x20*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_COR_CTL,CORE_ACTIVE,"CORE_ACTIVE",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_CTL,HOLD,"HOLD",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_CTL,CORE_STATE_CTL,"CORE_STATE_CTL",0,4,CHIP_UNSIGNED);

            /*  COR_STAT    R1010000 01010000 */
            ChipAddReg(hChip,R_COR_STAT,"COR_STAT",0x81,*DefVal++/*0xf6*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_COR_STAT,TPS_LOCKED,"TPS_LOCKED",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_STAT,SYR_LOCKED_COR,"SYR_LOCKED_COR",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_STAT,AGC_LOCKED_STAT,"AGC_LOCKED_STAT",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_STAT,CORE_STATE_STAT,"CORE_STATE_STAT",0,4,CHIP_UNSIGNED);

            /*  COR_INTEN   0R010101 01010101 */
            ChipAddReg(hChip,R_COR_INTEN,"COR_INTEN",0x82,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_COR_INTEN,INTEN,"INTEN",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTEN,INTEN_SYR,"INTEN_SYR",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTEN,INTEN_FFT,"INTEN_FFT",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTEN,INTEN_AGC,"INTEN_AGC",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTEN,INTEN_TPS1,"INTEN_TPS1",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTEN,INTEN_TPS2,"INTEN_TPS2",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTEN,INTEN_TPS3,"INTEN_TPS3",0,1,CHIP_UNSIGNED);

            /*  COR_INTSTAT  RR101010 00101010 */
            ChipAddReg(hChip,R_COR_INTSTAT,"COR_INTSTAT",0x83,*DefVal++/*0x3F*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_COR_INTSTAT,INTSTAT_SYR,"INTSTAT_SYR",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTSTAT,INTSTAT_FFT,"INTSTAT_FFT",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTSTAT,INTSAT_AGC,"INTSAT_AGC",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTSTAT,INTSTAT_TPS1,"INTSTAT_TPS1",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTSTAT,INTSTAT_TPS2,"INTSTAT_TPS2",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_INTSTAT,INTSTAT_TPS3,"INTSTAT_TPS3",0,1,CHIP_UNSIGNED);

            /*  COR_MODEGUARD   RRRR1011 00001011 */
            ChipAddReg(hChip,R_COR_MODEGUARD,"COR_MODEGUARD",0x84,*DefVal++/*0x3*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_COR_MODEGUARD,FORCE,"FORCE",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_MODEGUARD,MODE,"MODE",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_COR_MODEGUARD,GUARD,"GUARD",0,2,CHIP_UNSIGNED);

            /*  AGC_CTL 00010010 */
            ChipAddReg(hChip,R_AGC_CTL,"AGC_CTL",0x85,*DefVal++/*0x18*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC_CTL,DELAY_STARTUP,"DELAY_STARTUP",5,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC_CTL,AGC_LAST,"AGC_LAST",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC_CTL,AGC_GAIN,"AGC_GAIN",2,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC_CTL,AGC_NEG,"AGC_NEG",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC_CTL,AGC_SET,"AGC_SET",0,1,CHIP_UNSIGNED);

            /*  AGC_MANUAL1  00000000 */
            ChipAddReg(hChip,R_AGC_MANUAL1,"AGC_MANUAL1",0x86,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC_MANUAL1,AGC_VAL_LO,"AGC_VAL_LO",0,8,CHIP_UNSIGNED);

            /*  AGC_MANUAL2 RRRR1111 00001111 */
            ChipAddReg(hChip,R_AGC_MANUAL2,"AGC_MANUAL2",0x87,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC_MANUAL2,AGC_VAL_HI,"AGC_VAL_HI",0,4,CHIP_UNSIGNED);

            /*  AGC_TARGET  00000000 */
            ChipAddReg(hChip,R_AGC_TARGET,"AGC_TARGET",0x88,*DefVal++/*0x28*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC_TARGET,AGC_TARGET,"AGC_TARGET",0,8,CHIP_UNSIGNED);

            /*  AGC_GAIN1   00000000 */
            ChipAddReg(hChip,R_AGC_GAIN1,"AGC_GAIN1",0x89,*DefVal++/*0xFF*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC_GAIN1,AGC_GAIN_LO,"AGC_GAIN_LO",0,8,CHIP_UNSIGNED);

            /*  AGC_GAIN2   RRR10000 00010000 */
            ChipAddReg(hChip,R_AGC_GAIN2,"AGC_GAIN2",0x8a,*DefVal++/*0x17*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC_GAIN2,AGC_LOCKED_GAIN2,"AGC_LOCKED_GAIN2",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_AGC_GAIN2,AGC_GAIN_HI,"AGC_GAIN_HI",0,4,CHIP_UNSIGNED);

            /*  ITB_CTL RRRRRRR1 00000001 */
            ChipAddReg(hChip,R_ITB_CTL,"ITB_CTL",0x8b,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ITB_CTL,ITB_INVERT,"ITB_INVERT",0,1,CHIP_UNSIGNED);

            /*  ITB_FREQ1   00000000 */
            ChipAddReg(hChip,R_ITB_FREQ1,"ITB_FREQ1",0x8c,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ITB_FREQ1,ITB_FREQ_LO,"ITB_FREQ_LO",0,8,CHIP_UNSIGNED);

            /*  ITB_FREQ2   RR111111 00111111*/
            ChipAddReg(hChip,R_ITB_FREQ2,"ITB_FREQ2",0x8d,*DefVal++/*0x0*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ITB_FREQ2,ITB_FREQ_HI,"ITB_FREQ_HI",0,6,CHIP_UNSIGNED);

            /*  CAS_CTL 01011000 */
            ChipAddReg(hChip,R_CAS_CTL,"CAS_CTL",0x8e,*DefVal++/*0x40*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CAS_CTL,CCS_ENABLE,"CCS_ENABLE",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CAS_CTL,ACS_DISABLE,"ACS_DISABLE",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CAS_CTL,DAGC_DIS,"DAGC_DIS",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CAS_CTL,DAGC_GAIN,"DAGC_GAIN",3,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CAS_CTL,CCSMU,"CCSMU",0,3,CHIP_UNSIGNED);

            /*  CAS_FREQ    00000000 */
            ChipAddReg(hChip,R_CAS_FREQ,"CAS_FREQ",0x8f,*DefVal++/*0xB3*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CAS_FREQ,CCS_FREQ,"CCS_FREQ",0,8,CHIP_UNSIGNED);

            /*  CAS_DAGCGAIN  00000000  */
            ChipAddReg(hChip,R_CAS_DAGCGAIN,"CAS_DAGCGAIN",0x90,*DefVal++/*0x10*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CAS_DAGCGAIN,CAS_DAGC_GAIN,"CAS_DAGC_GAIN",0,8,CHIP_UNSIGNED);

            /*  SYR_CTL 01111010 */
            ChipAddReg(hChip,R_SYR_CTL,"SYR_CTL",0x91,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            /* added due to design */
        
            ChipAddField(hChip,R_SYR_CTL,SICTHENABLE,"SICTHENABLE",7,1,CHIP_UNSIGNED);
            /* end added part */
            ChipAddField(hChip,R_SYR_CTL,LONG_ECHO,"LONG_ECHO",3,4,CHIP_SIGNED);
            ChipAddField(hChip,R_SYR_CTL,AUTO_LE_EN,"AUTO_LE_EN",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SYR_CTL,SYR_BYPASS,"SYR_BYPASS",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SYR_CTL,SYR_TR_DIS,"SYR_TR_DIS",0,1,CHIP_UNSIGNED);

            /*  SYR_STAT    RRR1R100 00010100 */
            ChipAddReg(hChip,R_SYR_STAT,"SYR_STAT",0x92,*DefVal++/*0x13*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SYR_STAT,SYR_LOCKED_STAT,"SYR_LOCKED_STAT",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SYR_STAT,SYR_MODE,"SYR_MODE",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SYR_STAT,SYR_GUARD,"SYR_GUARD",0,2,CHIP_UNSIGNED);

            /*  SYR_NCO1    00000000*/
            ChipAddReg(hChip,R_SYR_NCO1,"SYR_NCO1",0x93,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SYR_NCO1,SYR_NCO_LO,"SYR_NCO_LO",0,8,CHIP_UNSIGNED);

            /*  SYR_NCO2    RR111111 00111111 */
            ChipAddReg(hChip,R_SYR_NCO2,"SYR_NCO2",0x94,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SYR_NCO2,SYR_NCO_HI,"SYR_NCO_HI",0,6,CHIP_UNSIGNED);

            /*  SYR_OFFSET1 00000000 */
            ChipAddReg(hChip,R_SYR_OFFSET1,"SYR_OFFSET1",0x95,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SYR_OFFSET1,SYR_OFFSET_LO,"SYR_OFFSET_LO",0,8,CHIP_UNSIGNED);

            /*  SYR_OFFSET2 RR111111 00111111 */
            ChipAddReg(hChip,R_SYR_OFFSET2,"SYR_OFFSET2",0x96,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SYR_OFFSET2,SYR_OFFSET_HI,"SYR_OFFSET_HI",0,6,CHIP_UNSIGNED);

            /*  FFT_CTL 0xRRRRR101 0x00000101 */
            ChipAddReg(hChip,R_FFT_CTL,"FFT_CTL",0x97,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_FFT_CTL,FFT_TRIGGER,"FFT_TRIGGER",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FFT_CTL,FFT_MANUAL,"FFT_MANUAL",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FFT_CTL,IFFT_MODE,"IFFT_MODE",0,1,CHIP_UNSIGNED);

            /*  SCR_CTL RRRRRR10 00000010 */
            ChipAddReg(hChip,R_SCR_CTL,"SCR_CTL",0x98,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SCR_CTL,SCR_CPDIS,"SCR_CPDIS",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SCR_CTL,SCR_DIS,"SCR_DIS",0,1,CHIP_UNSIGNED);

            /*  PPM_CTL1    RR11RR10 00110010 */
            ChipAddReg(hChip,R_PPM_CTL1,"PPM_CTL1",0x99,*DefVal++/*0x30*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_PPM_CTL1,PPM_MAXFREQ,"PPM_MAXFREQ",4,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_PPM_CTL1,PPM_SCATDIS,"PPM_SCATDIS",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_PPM_CTL1,PPM_BYP,"PPM_BYP",0,1,CHIP_UNSIGNED);

            /*  TRL_CTL 0R000111 01000111 */
            /*ChipAddReg(hChip,R_TRL_CTL,"TRL_CTL",0x9a,0x14,STCHIP_ACCESS_WR);gbgb*/
            ChipAddReg(hChip,R_TRL_CTL,"TRL_CTL",0x9a,*DefVal++/*0x94*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TRL_CTL,TRL_NOMRATE_LSB,"TRL_NOMRATE_LSB",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TRL_CTL,TRL_TR_GAIN,"TRL_TR_GAIN",3,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TRL_CTL,TRL_LOOPGAIN,"TRL_LOOPGAIN",0,3,CHIP_UNSIGNED);

            /*  TRL_NOMRATE1    00000000 */
            /*ChipAddReg(hChip,R_TRL_NOMRATE1,"TRL_NOMRATE1",0x9b,0x7E,STCHIP_ACCESS_WR);gbgb*/
            ChipAddReg(hChip,R_TRL_NOMRATE1,"TRL_NOMRATE1",0x9b,*DefVal++/*0x4D*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TRL_NOMRATE1,TRL_NOMRATE_LO,"TRL_NOMRATE_LO",0,8,CHIP_UNSIGNED);

            /*  TRL_NOMRATE2    00000000 */
            /*ChipAddReg(hChip,R_TRL_NOMRATE2,"TRL_NOMRATE2",0x9c,0x61,STCHIP_ACCESS_WR);gbgb*/
            ChipAddReg(hChip,R_TRL_NOMRATE2,"TRL_NOMRATE2",0x9c,*DefVal++/*0x55*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TRL_NOMRATE2,TRL_NOMRATE_HI,"TRL_NOMRATE_HI",0,8,CHIP_UNSIGNED);

            /*  TRL_TIME1   00000000 */
            ChipAddReg(hChip,R_TRL_TIME1,"TRL_TIME1",0x9d,*DefVal++/*0xC1*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TRL_TIME1,TRL_TOFFSET_LO,"TRL_TOFFSET_LO",0,8,CHIP_UNSIGNED);

            /*  TRL_TIME2   00000000 */
            ChipAddReg(hChip,R_TRL_TIME2,"TRL_TIME2",0x9e,*DefVal++/*0xF8*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TRL_TIME2,TRL_TOFFSET_HI,"TRL_TOFFSET_HI",0,8,CHIP_UNSIGNED);

            /*  CRL_CTL 0R000111 01000111 */
            ChipAddReg(hChip,R_CRL_CTL,"CRL_CTL",0x9f,*DefVal++/*0x4F*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CRL_CTL,CRL_DIS,"CRL_DIS",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CRL_CTL,CRL_TR_GAIN,"CRL_TR_GAIN",3,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CRL_CTL,CRL_LOOPGAIN,"CRL_LOOPGAIN",0,3,CHIP_UNSIGNED);

            /*  CRL_FREQ1  00000000 */
            ChipAddReg(hChip,R_CRL_FREQ1,"CRL_FREQ1",0xa0,*DefVal++/*0xDC*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CRL_FREQ1,CRL_FOFFSET_LO,"CRL_FOFFSET_LO",0,8,CHIP_UNSIGNED);

            /*  CRL_FREQ2   00000000*/
            ChipAddReg(hChip,R_CRL_FREQ2,"CRL_FREQ2",0xa1,*DefVal++/*0xF1*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CRL_FREQ2,CRL_FOFFSET_HI,"CRL_FOFFSET_HI",0,8,CHIP_UNSIGNED);

            /*  CRL_FREQ3   01111111 */
            ChipAddReg(hChip,R_CRL_FREQ3,"CRL_FREQ3",0xa2,*DefVal++/*0xFF*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CRL_FREQ3,SEXT,"SEXT",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CRL_FREQ3,CRL_FOFFSET_VHI,"CRL_FOFFSET_VHI",0,7,CHIP_UNSIGNED);

            /*  CHC_CTL1    00010101 */
            ChipAddReg(hChip,R_CHC_CTL1,"CHC_CTL1",0xa3,*DefVal++/*0x01*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CHC_CTL1,MEAN_PILOT_GAIN,"MEAN_PILOT_GAIN",5,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CHC_CTL1,MMEANP,"MMEANP",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CHC_CTL1,DBADP,"DBADP",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CHC_CTL1,DNOISEN,"DNOISEN",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CHC_CTL1,DCHCPRED,"DCHCPRED",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CHC_CTL1,CHC_INT,"CHC_INT",0,1,CHIP_UNSIGNED);

            /*  CHC_SNR 00000000 */
            ChipAddReg(hChip,R_CHC_SNR,"CHC_SNR",0xa4,*DefVal++/*0xE8*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CHC_SNR,CHC_SNR,"CHC_SNR",0,8,CHIP_UNSIGNED);

            /*  BDI_CTL RRRRRR10 00000010*/
            ChipAddReg(hChip,R_BDI_CTL,"BDI_CTL",0xa5,*DefVal++/*0x60*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_BDI_CTL,BDI_LPSEL,"BDI_LPSEL",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_BDI_CTL,BDI_SERIAL,"BDI_SERIAL",0,1,CHIP_UNSIGNED);

            /*  DMP_CTL RRRRRRR1 00000001 */
            ChipAddReg(hChip,R_DMP_CTL,"DMP_CTL",0xa6,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_DMP_CTL,DMP_SDDIS,"DMP_SDDIS",0,1,CHIP_UNSIGNED);

            /*  TPS_RCVD1   R101RR11 01010011*/
            ChipAddReg(hChip,R_TPS_RCVD1,"TPS_RCVD1",0xa7,*DefVal++/*0x32*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TPS_RCVD1,TPS_CHANGE,"TPS_CHANGE",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TPS_RCVD1,BCH_OK,"BCH_OK",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TPS_RCVD1,TPS_SYNC,"TPS_SYNC",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TPS_RCVD1,TPS_FRAME,"TPS_FRAME",0,2,CHIP_UNSIGNED);

            /*  TPS_RCVD2   R111RR11 01110011 */
            ChipAddReg(hChip,R_TPS_RCVD2,"TPS_RCVD2",0xa8,*DefVal++/*0x2*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TPS_RCVD2,TPS_HIERMODE,"TPS_HIERMODE",4,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TPS_RCVD2,TPS_CONST,"TPS_CONST",0,2,CHIP_UNSIGNED);

            /*  TPS_RCVD3   R111R111 01110111 */
            ChipAddReg(hChip,R_TPS_RCVD3,"TPS_RCVD3",0xa9,*DefVal++/*0x01*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TPS_RCVD3,TPS_LPCODE,"TPS_LPCODE",4,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TPS_RCVD3,TPS_HPCODE,"TPS_HPCODE",0,3,CHIP_UNSIGNED);

            /*  TPS_RCVD4   RR11RR11 00110011*/
            ChipAddReg(hChip,R_TPS_RCVD4,"TPS_RCVD4",0xaa,*DefVal++/*0x30*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TPS_RCVD4,TPS_GUARD,"TPS_GUARD",4,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TPS_RCVD4,TPS_MODE,"TPS_MODE",0,2,CHIP_UNSIGNED);

            /*  TPS_CELLID   00000000 */
            ChipAddReg(hChip,R_TPS_CELLID,"TPS_CELLID",0xab,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TPS_CELLID,TPS_CELLID,"TPS_CELLID",0,8,CHIP_UNSIGNED);
            /*  TPS_FREE2   0x00000000 */
            ChipAddReg(hChip,R_TPS_FREE2,"TPS_FREE2",0xac,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);

            /*  TPS_SET1    RRRRRR11 00000011 */
            ChipAddReg(hChip,R_TPS_SET1,"TPS_SET1",0xad,*DefVal++/*0x01*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TPS_SET1,TPS_SETFRAME,"TPS_SETFRAME",0,2,CHIP_UNSIGNED);

            /*  TPS_SET2    R111RR11 01110011 */
            ChipAddReg(hChip,R_TPS_SET2,"TPS_SET2",0xae,*DefVal++/*0x02*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TPS_SET2,TPS_SETHIERMODE,"TPS_SETHIERMODE",4,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TPS_SET2,TPS_SETCONST,"TPS_SETCONST",0,2,CHIP_UNSIGNED);

            /*  TPS_SET3    R111R111 01110111 */
            ChipAddReg(hChip,R_TPS_SET3,"TPS_SET3",0xaf,*DefVal++/*0x01*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TPS_SET3,TPS_SETLPCODE,"TPS_SETLPCODE",4,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TPS_SET3,TPS_SETHPCODE,"TPS_SETHPCODE",0,3,CHIP_UNSIGNED);

            /*  TPS_CTL RRRRR101 00000101 */
            ChipAddReg(hChip,R_TPS_CTL,"TPS_CTL",0xb0,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TPS_CTL,TPS_IMM,"TPS_IMM",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TPS_CTL,TPS_BCHDIS,"TPS_BCHDIS",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TPS_CTL,TPS_UPDDIS,"TPS_UPDDIS",0,1,CHIP_UNSIGNED);

            /*  CTL_FFTOSNUM   00000000 */
            ChipAddReg(hChip,R_CTL_FFTOSNUM,"CTL_FFTOSNUM",0xb1,*DefVal++/*0x27*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CTL_FFTOSNUM,SYMBOL_NUMBER,"SYMBOL_NUMBER",0,8,CHIP_UNSIGNED);

            /*  CAR_DISP_SEL  RRRR1111 00001111 */
            ChipAddReg(hChip,R_CAR_DISP_SEL,"CAR_DISP_SEL",0xb2,*DefVal++/*0x0c*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CAR_DISP_SEL,CAR_DISP_SEL,"CAR_DISP_SEL",0,4,CHIP_UNSIGNED);


            /*  MSC_REV 00000000 */
            ChipAddReg(hChip,R_MSC_REV,"MSC_REV",0xb3,*DefVal++/*0x0A*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_MSC_REV,REV_NUMBER,"REV_NUMBER",0,8,CHIP_UNSIGNED);

            /*  PIR_CTL RRRRRRR1 00000001*/
            ChipAddReg(hChip,R_PIR_CTL,"PIR_CTL",0xb4,*DefVal++/*0x0*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_PIR_CTL,FREEZE,"FREEZE",0,1,CHIP_UNSIGNED);

            /*  SNR_CARRIER1    00000000*/
            ChipAddReg(hChip,R_SNR_CARRIER1,"SNR_CARRIER1",0xb5,*DefVal++/*0xA8*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SNR_CARRIER1,SNR_CARRIER_LO,"SNR_CARRIER_LO",0,8,CHIP_UNSIGNED);

            /*  SNR_CARRIER2    0RR00000 01100000 */
            ChipAddReg(hChip,R_SNR_CARRIER2,"SNR_CARRIER2",0xb6,*DefVal++/*0x86*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SNR_CARRIER2,MEAN,"MEAN",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SNR_CARRIER2,SNR_CARRIER_HI,"SNR_CARRIER_HI",0,5,CHIP_UNSIGNED);

            /* PPM_CPAMP    00000000    */
            ChipAddReg(hChip,R_PPM_CPAMP,"PPM_CPAMP",0xb7,*DefVal++/*0x2C*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_PPM_CPAMP ,PPM_CPC,"PPM_CPC",0,8,CHIP_UNSIGNED);

            /*  TSM_AP0  00000000 */
            ChipAddReg(hChip,R_TSM_AP0,"TSM_AP0",0xb8,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSM_AP0,ADDRESS_BYTE_0,"ADDRESS_BYTE_0",0,8,CHIP_UNSIGNED);

            /*  TSM_AP1 00000000 */
            ChipAddReg(hChip,R_TSM_AP1,"TSM_AP1",0xb9,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSM_AP1,ADDRESS_BYTE_1,"ADDRESS_BYTE_1",0,8,CHIP_UNSIGNED);

            /*  TSM_AP2 00000000 */
            ChipAddReg(hChip,R_TSM_AP2,"TSM_AP2",0xba,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSM_AP2,DATA_BYTE_0,"DATA_BYTE_0",0,8,CHIP_UNSIGNED);

            /*  TSM_AP3 00000000 */
            ChipAddReg(hChip,R_TSM_AP3,"TSM_AP3",0xbb,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSM_AP3,DATA_BYTE_1,"DATA_BYTE_1",0,8,CHIP_UNSIGNED);

            /*  TSM_AP4 00000000 */
            ChipAddReg(hChip,R_TSM_AP4,"TSM_AP4",0xbc,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSM_AP4,DATA_BYTE_2,"DATA_BYTE_2",0,8,CHIP_UNSIGNED);

            /*  TSM_AP5 00000000 */
            ChipAddReg(hChip,R_TSM_AP5,"TSM_AP5",0xbd,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSM_AP5,DATA_BYTE_3,"DATA_BYTE_3",0,8,CHIP_UNSIGNED);

            /*  TSM_AP6 RRRRRRRR 00000000 */
            ChipAddReg(hChip,R_TSM_AP6,"TSM_AP6",0xbe,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);

            /*  TSM_AP7 00000000 */
            ChipAddReg(hChip,R_TSM_AP7,"TSM_AP7",0xbf,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSM_AP7,MEM_SELECT_BYTE,"MEM_SELECT_BYTE",0,8,CHIP_UNSIGNED);



            /*  CONSTMODE   RRR11011 00011011 */
            ChipAddReg(hChip,R_CONSTMODE,"CONSTMODE",0xcb,*DefVal++/*0x02*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CONSTMODE,CAR_TYPE,"CAR_TYPE",3,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CONSTMODE,IQ_RANGE,"IQ_RANGE",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_CONSTMODE,CONST_MODE,"CONST_MODE",0,2,CHIP_UNSIGNED);

            /*  CONSTCARR1  00000000 */
            ChipAddReg(hChip,R_CONSTCARR1,"CONSTCARR1",0xcc,*DefVal++/*0xD2*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CONSTCARR1,CONST_CARR_LO,"CONST_CARR_LO",0,8,CHIP_UNSIGNED);

            /*  CONSTCARR2  RRR11111 00011111 */
            ChipAddReg(hChip,R_CONSTCARR2,"CONSTCARR2",0xcd,*DefVal++/*0x04*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_CONSTCARR2,CONST_CARR_HI,"CONST_CARR_HI",0,5,CHIP_UNSIGNED);

            /*  ICONSTEL  00000000  */
            ChipAddReg(hChip,R_ICONSTEL,"ICONSTEL",0xce,*DefVal++/*0xDC*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ICONSTEL,ICONSTEL,"ICONSTEL",0,8,CHIP_SIGNED);

            /*  QCONSTEL    00000000 */
            ChipAddReg(hChip,R_QCONSTEL,"QCONSTEL",0xcf,*DefVal++/*0xDB*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_QCONSTEL,QCONSTEL,"QCONSTEL",0,8,CHIP_SIGNED);


            /* AGC1RF 00000000 */
            ChipAddReg(hChip,R_AGC1RF,"AGC1RF",0xD4,*DefVal++/*0xAB*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_AGC1RF,RF_AGC1_LEVEL,"RF_AGC1_LEVEL",0,8,CHIP_UNSIGNED);

            /* EN_RF_AGC1 0RRRRRRR 01111111 */
            ChipAddReg(hChip,R_EN_RF_AGC1,"EN_RF_AGC1",0xD5,*DefVal++/*0x03*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_EN_RF_AGC1,START_ADC,"START_ADC",7,1,CHIP_UNSIGNED);


            /*  FECM    0000R010 00001010 */
            ChipAddReg(hChip,R_FECM,"FECM",0x40,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_FECM,FEC_MODE,"FEC_MODE",4,4,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FECM,VIT_DIFF,"VIT_DIFF",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FECM,SYNC,"SYNC",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FECM,SYM,"SYM",0,1,CHIP_UNSIGNED);

            /*  VTH0   R1111111 01111111 */
            ChipAddReg(hChip,R_VTH0,"VTH0",0x41,*DefVal++/*0x1E*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_VTH0,VTH0,"VTH0",0,7,CHIP_UNSIGNED);

            /*  VTH1   R1111111 01111111 */
            ChipAddReg(hChip,R_VTH1,"VTH1",0x42,*DefVal++/*0x14*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_VTH1,VTH1,"VTH1",0,7,CHIP_UNSIGNED);

            /*  VTH2   R1111111 01111111 */
            ChipAddReg(hChip,R_VTH2,"VTH2",0x43,*DefVal++/*0x0F*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_VTH2,VTH2,"VTH2",0,7,CHIP_UNSIGNED);

            /*  VTH3    R1111111 01111111*/
            ChipAddReg(hChip,R_VTH3,"VTH3",0x44,*DefVal++/*0x09*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_VTH3,VTH3,"VTH3",0,7,CHIP_UNSIGNED);

            /*  VTH4    R1111111 01111111 */
            ChipAddReg(hChip,R_VTH4,"VTH4",0x45,*DefVal++/*0x0*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_VTH4,VTH4,"VTH4",0,7,CHIP_UNSIGNED);

            /*  VTH5    R1111111 01111111 */
            ChipAddReg(hChip,R_VTH5,"VTH5",0x46,*DefVal++/*0x05*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_VTH5,VTH5,"VTH5",0,7,CHIP_UNSIGNED);

            /*  FREEVIT  RRRRRRRR 00000000*/
            ChipAddReg(hChip,R_FREEVIT,"FREEVIT",0x47,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);

            /*  VITPROG 00RRRR00 00111100 */
            ChipAddReg(hChip,R_VITPROG,"VITPROG",0x49,*DefVal++/*0x92*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_VITPROG,FORCE_ROTA,"FORCE_ROTA",6,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_VITPROG,MDIVIDER,"MDIVIDER",0,2,CHIP_UNSIGNED);

            /*  PR  0R010101 01010101 */
            ChipAddReg(hChip,R_PR,"PR",0x4a,*DefVal++/*0xAF*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_PR,FRAPTCR,"FRAPTCR",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_PR,E7_8,"E7_8",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_PR,E6_7,"E6_7",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_PR,E5_6,"E5_6",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_PR,E3_4,"E3_4",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_PR,E2_3,"E2_3",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_PR,E1_2,"E1_2",0,1,CHIP_UNSIGNED);

            /*  VSEARCH  01001100 */
            ChipAddReg(hChip,R_VSEARCH,"VSEARCH",0x4b,*DefVal++/*0x30*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_VSEARCH,PR_AUTO,"PR_AUTO",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_VSEARCH,PR_FREEZE,"PR_FREEZE",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_VSEARCH,SAMPNUM,"SAMPNUM",4,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_VSEARCH,TIMEOUT,"TIMEOUT",2,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_VSEARCH,HYSTER,"HYSTER",0,2,CHIP_UNSIGNED);

            /*  RS  01010101*/
            ChipAddReg(hChip,R_RS,"RS",0x4c,*DefVal++/*0xFE*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_RS,DEINT_ENA,"DEINT_ENA",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RS,OUTRS_SP,"OUTRS_SP",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RS,RS_ENA,"RS_ENA",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RS,DESCR_ENA,"DESCR_ENA",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RS,ERRBIT_ENA,"ERRBIT_ENA",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RS,FORCE47,"FORCE47",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RS,CLK_POL,"CLK_POL",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RS,CLK_CFG,"CLK_CFG",0,1,CHIP_UNSIGNED);

            /*  RSOUT   RRR10000 00010000 */
            ChipAddReg(hChip,R_RSOUT,"RSOUT",0x4d,*DefVal++/*0x15*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_RSOUT,ENA_STBACKEND,"ENA_STBACKEND",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RSOUT,ENA8_LEVEL,"ENA8_LEVEL",0,4,CHIP_UNSIGNED);

            /*  ERRCTRL1 0100R011 01001011   */
            ChipAddReg(hChip,R_ERRCTRL1,"ERRCTRL1",0x4e,*DefVal++/*0x12*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ERRCTRL1,ERRMODE1,"ERRMODE1",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL1,TESTERS1,"TESTERS1",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL1,ERR_SOURCE1,"ERR_SOURCE1",4,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL1,RESET_CNTR1,"RESET_CNTR1",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL1,NUM_EVENT1,"NUM_EVENT1",0,2,CHIP_UNSIGNED);

            /*  ERRCNTM1  00000000  */
            ChipAddReg(hChip,R_ERRCNTM1,"ERRCNTM1",0x4f,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ERRCNTM1,ERROR_COUNT1_HI,"ERROR_COUNT1_HI",0,8,CHIP_UNSIGNED);

            /*  ERRCNTL1  00000000  */
            ChipAddReg(hChip,R_ERRCNTL1,"ERRCNTL1",0x50,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ERRCNTL1,ERROR_COUNT1_LO,"ERROR_COUNT1_LO",0,8,CHIP_UNSIGNED);

            /*  ERRCTRL2  0100R011 01001011  */
            ChipAddReg(hChip,R_ERRCTRL2,"ERRCTRL2",0x51,*DefVal++/*0x12*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ERRCTRL2,ERRMODE2,"ERRMODE2",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL2,TESTERS2,"TESTERS2",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL2,ERR_SOURCE2,"ERR_SOURCE2",4,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL2,RESET_CNTR2,"RESET_CNTR2",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL2,NUM_EVENT2,"NUM_EVENT2",0,2,CHIP_UNSIGNED);

            /*  ERRCNTM2    00000000*/
            ChipAddReg(hChip,R_ERRCNTM2,"ERRCNTM2",0x52,*DefVal++/*0x0*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ERRCNTM2,ERROR_COUNT2_HI,"ERROR_COUNT2_HI",0,8,CHIP_UNSIGNED);

            /*  ERRCNTL2    00000000 */
            ChipAddReg(hChip,R_ERRCNTL2,"ERRCNTL2",0x53,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ERRCNTL2,ERROR_COUNT2_LO,"ERROR_COUNT2_LO",0,8,CHIP_UNSIGNED);



            /*  ERRCTRL3    0100R011 01001011 */
            ChipAddReg(hChip,R_ERRCTRL3,"ERRCTRL3",0x56,*DefVal++/*0x12*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ERRCTRL3,ERRMODE3,"ERRMODE3",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL3,TESTERS3,"TESTERS3",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL3,ERR_SOURCE3,"ERR_SOURCE3",4,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL3,RESET_CNTR3,"RESET_CNTR3",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ERRCTRL3,NUM_EVENT3,"NUM_EVENT3",0,2,CHIP_UNSIGNED);


            /*   ERRCNTM3   00000000 */
            ChipAddReg(hChip,R_ERRCNTM3,"ERRCNTM3",0x57,*DefVal++/*0x0*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ERRCNTM3,ERROR_COUNT3_HI,"ERROR_COUNT3_HI",0,8,CHIP_UNSIGNED);

            /*  ERRCNTL3   00000000 */
            ChipAddReg(hChip,R_ERRCNTL3,"ERRCNTL3",0x58,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ERRCNTL3,ERROR_COUNT3_LO,"ERROR_COUNT3_LO",0,8,CHIP_UNSIGNED);

            /* DILSTK1  00000000 */
            ChipAddReg(hChip,R_DILSTK1,"DILSTK1",0x59,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_DILSTK1,DILSTK_HI,"DILSTK_HI",0,8,CHIP_UNSIGNED);

            /* DILSTK0 00000000 */
            ChipAddReg(hChip,R_DILSTK0,"DILSTK0",0x5A,*DefVal++/*0x0C*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_DILSTK0,DILSTK_LO,"DILSTK_LO",0,8,CHIP_UNSIGNED);

            /* DILBWSTK1 00000000 */
            ChipAddReg(hChip,R_DILBWSTK1,"DILBWSTK1",0x5B,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);

            /* DILBWSTK0 00000000 */
            ChipAddReg(hChip,R_DILBWST0,"DILBWST0",0x5C,*DefVal++/*0x03*/,STCHIP_ACCESS_WR);


            /*  LNBRX   01RRRRRR 01000000 */
            ChipAddReg(hChip,R_LNBRX,"LNBRX",0x5D,*DefVal++/*0x80*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_LNBRX,LINE_OK,"LINE_OK",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_LNBRX,OCCURRED_ERR,"OCCURRED_ERR",6,1,CHIP_UNSIGNED);

            /*  RSTC    0101R101 01010101 */
            ChipAddReg(hChip,R_RSTC,"RSTC",0x5E,*DefVal++/*0xB0*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_RSTC,DEINNTE,"DEINNTE",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RSTC,DIL64_ON,"DIL64_ON",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RSTC,RSTC,"RSTC",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RSTC,DESCRAMTC,"DESCRAMTC",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RSTC,MODSYNCBYTE,"MODSYNCBYTE",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RSTC,LOWP_DIS,"LOWP_DIS",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_RSTC,HI,"HI",0,1,CHIP_UNSIGNED);

            /*  VIT_BIST    R1000111 01000111 */
            ChipAddReg(hChip,R_VIT_BIST,"VIT_BIST",0x5F,*DefVal++/*0x07*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_VIT_BIST,RAND_RAMP,"RAND_RAMP",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_VIT_BIST,NOISE_LEVEL,"NOISE_LEVEL",3,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_VIT_BIST,PR_VIT_BIST,"PR_VIT_BIST",0,3,CHIP_UNSIGNED);


            /*  FREEDRS 00000000 */
            ChipAddReg(hChip,R_FREEDRS,"FREEDRS",0x54,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);

            /*  VERROR  00000000 */
            ChipAddReg(hChip,R_VERROR,"VERROR",0x55,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_VERROR,ERROR_VALUE,"ERROR_VALUE",0,8,CHIP_UNSIGNED);


            /*  TSTRES  01010101 */
            ChipAddReg(hChip,R_TSTRES,"TSTRES",0xc0,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTRES,FRESI2C,"FRESI2C",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRES,FRESRS,"FRESRS",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRES,FRESACS,"FRESACS",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRES,FRES_PRIF,"FRES_PRIF",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRES,FRESFEC1_2,"FRESFEC1_2",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRES,FRESFEC,"FRESFEC",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRES,FRESCORE1_2,"FRESCORE1_2",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRES,FRESCORE,"FRESCORE",0,1,CHIP_UNSIGNED);

            /*  ANACTRL  01010101 */
            ChipAddReg(hChip,R_ANACTRL,"ANACTRL",0xc1,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_ANACTRL,STDBY_PLL,"STDBY_PLL",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ANACTRL,BYPASS_XTAL,"BYPASS_XTAL",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ANACTRL,STDBY_PGA,"STDBY_PGA",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ANACTRL,TEST_PGA,"TEST_PGA",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ANACTRL,STDBY_ADC,"STDBY_ADC",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ANACTRL,BYPASS_ADC,"BYPASS_ADC",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ANACTRL,SGN_ADC,"SGN_ADC",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_ANACTRL,TEST_ADC,"TEST_ADC",0,1,CHIP_UNSIGNED);

            /*  TSTBUS  01000111 */
            ChipAddReg(hChip,R_TSTBUS,"TSTBUS",0xc2,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTBUS,EXT_TESTIN,"EXT_TESTIN",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBUS,EXT_ADC,"EXT_ADC",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBUS,TEST_IN,"TEST_IN",3,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBUS,TS,"TS",0,3,CHIP_UNSIGNED);

            /*  TSTCK  0RRR0101 01110101 */
            ChipAddReg(hChip,R_TSTCK,"TSTCK",0xc3,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTCK,CKFECEXT,"CKFECEXT",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTCK,FORCERATE1,"FORCERATE1",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTCK,TSTCKRS,"TSTCKRS",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTCK,TSTCKDIL,"TSTCKDIL",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTCK,DIRCKINT,"DIRCKINT",0,1,CHIP_UNSIGNED);

            /*  TSTI2C  01101000 */
            ChipAddReg(hChip,R_TSTI2C,"TSTI2C",0xc4,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTI2C,EN_VI2C,"EN_VI2C",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTI2C,TI2C,"TI2C",5,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTI2C,BFAIL_BAD,"BFAIL_BAD",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTI2C,RBACT,"RBACT",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTI2C,TST_PRIF,"TST_PRIF",0,3,CHIP_UNSIGNED);

            /*  TSTRAM1 01011100*/
            ChipAddReg(hChip,R_TSTRAM1,"TSTRAM1",0xc5,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTRAM1,SELADR1,"SELADR1",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRAM1,FSELRAM1,"FSELRAM1",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRAM1,FSELDEC,"FSELDEC",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRAM1,FOEB,"FOEB",2,3,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRAM1,FADR,"FADR",0,2,CHIP_UNSIGNED);

            /*  TSTRATE 0RR01000 01101000 */
            ChipAddReg(hChip,R_TSTRATE,"TSTRATE",0xc6,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTRATE,FORCEPHA,"FORCEPHA",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRATE,FNEWPHA,"FNEWPHA",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRATE,FROT90,"FROT90",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRATE,FR,"FR",0,3,CHIP_UNSIGNED);

            /*  SELOUT  01010101 */
            ChipAddReg(hChip,R_SELOUT,"SELOUT",0xc7,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_SELOUT,EN_VLOG,"EN_VLOG",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SELOUT,SELVIT60,"SELVIT60",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SELOUT,SELSYN3,"SELSYN3",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SELOUT,SELSYN2,"SELSYN2",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SELOUT,SELSYN1,"SELSYN1",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SELOUT,SELLIFO,"SELLIFO",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SELOUT,SELFIFO,"SELFIFO",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_SELOUT,TSTFIFO_SELOUT,"TSTFIFO_SELOUT",0,1,CHIP_UNSIGNED);

            /*  FORCEIN 01010101 */
            ChipAddReg(hChip,R_FORCEIN,"FORCEIN",0xc8,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_FORCEIN,SEL_VITDATAIN,"SEL_VITDATAIN",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FORCEIN,FORCE_ACS,"FORCE_ACS",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FORCEIN,TSTSYN,"TSTSYN",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FORCEIN,TSTRAM64,"TSTRAM64",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FORCEIN,TSTRAM,"TSTRAM",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FORCEIN,TSTERR2,"TSTERR2",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FORCEIN,TSTERR,"TSTERR",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_FORCEIN,TSTACS,"TSTACS",0,1,CHIP_UNSIGNED);

            /*  TSTFIFO RRRRR101 00000101 */
            ChipAddReg(hChip,R_TSTFIFO,"TSTFIFO",0xc9,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTFIFO,FORMSB,"FORMSB",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTFIFO,FORLSB,"FORLSB",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTFIFO,TSTFIFO_TSTFIFO,"TSTFIFO_TSTFIFO",0,1,CHIP_UNSIGNED);

            /*  TSTRS  01001101 */
            ChipAddReg(hChip,R_TSTRS,"TSTRS",0xca,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTRS,TST_SCRA,"TST_SCRA",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRS,OLDRS6,"OLDRS6",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRS,ADCT,"ADCT",4,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRS,DILT,"DILT",2,2,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRS,SCARBIT,"SCARBIT",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTRS,TSTRS_EN,"TSTRS_EN",0,1,CHIP_UNSIGNED);

            /*  TSTBISTRES0  01010101 */
            ChipAddReg(hChip,R_TSTBISTRES0,"TSTBISTRES0",0xd0,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTBISTRES0,BEND_CHC2,"BEND_CHC2",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES0,BBAD_CHC2,"BBAD_CHC2",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES0,BEND_PPM,"BEND_PPM",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES0,BBAD_PPM,"BBAD_PPM",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES0,BEND_FFTW,"BEND_FFTW",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES0,BBAD_FFTW,"BBAD_FFTW",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES0,BEND_FFTI,"BEND_FFTI",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES0,BBAD_FFTI,"BBAD_FFTI",0,1,CHIP_UNSIGNED);

            /*  TSTBISTRES1 01010101*/
            ChipAddReg(hChip,R_TSTBISTRES1,"TSTBISTRES1",0xd1,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTBISTRES1,BEND_CHC1,"BEND_CHC1",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES1,BBAD_CHC1,"BBAD_CHC1",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES1,BEND_SYR,"BEND_SYR",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES1,BBAD_SYR,"BBAD_SYR",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES1,BEND_SDI,"BEND_SDI",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES1,BBAD_SDI,"BBAD_SDI",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES1,BEND_BDI,"BEND_BDI",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES1,BBAD_BDI,"BBAD_BDI",0,1,CHIP_UNSIGNED);

            /*  TSTBISTRES2 01010101*/
            ChipAddReg(hChip,R_TSTBISTRES2,"TSTBISTRES2",0xd2,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTBISTRES2,BEND_VIT2,"BEND_VIT2",7,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES2,BBAD_VIT2,"BBAD_VIT2",6,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES2,BEND_VIT1,"BEND_VIT1",5,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES2,BBAD_VIT1,"BBAD_VIT1",4,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES2,BEND_DIL,"BEND_DIL",3,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES2,BBAD_DIL,"BBAD_DIL",2,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES2,BEND_RS,"BEND_RS",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES2,BBAD_RS,"BBAD_RS",0,1,CHIP_UNSIGNED);

            /*  TSTBISTRES3 RRRRRR10 00000010 */
            ChipAddReg(hChip,R_TSTBISTRES3,"TSTBISTRES3",0xd3,*DefVal++/*0x00*/,STCHIP_ACCESS_WR);
            ChipAddField(hChip,R_TSTBISTRES3,BEND_FIFO,"BEND_FIFO",1,1,CHIP_UNSIGNED);
            ChipAddField(hChip,R_TSTBISTRES3,BBAD_FIFO,"BBAD_FIFO",0,1,CHIP_UNSIGNED);


		ChipApplyDefaultValues(hChip);
		
		
	}
	#endif /* of the beginning #ifndef STTUNER_REG_INIT_OLD_METHOD */
	
	return hChip;
}




