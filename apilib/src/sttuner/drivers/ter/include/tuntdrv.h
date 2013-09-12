/* ----------------------------------------------------------------------------
File Name: tuntdrv.h

Description:

    EXTERNAL (ZIF) tuner drivers:

    DTT7572
    DTT7578
    DTT7592
    TDLB7
    TDQD3
    DTT7300X
    ENG47402G1
    TDM1316
    ED5058
    MIVAR
    TDED4
    DTT7102
    TECC2849PG
    TDCC2345

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 13-Sept-2001
version: 3.1.0
 author: GB from work by GJP
comment:

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_TER_TUNTDRV_H
#define __STTUNER_TER_TUNTDRV_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

/* constants --------------------------------------------------------------- */

/* In Hz -- Usually 125KHz (Ver1 code in KHz) */
#define TUNTDRV_TUNER_STEP  125000

#define TUNTDRV_IOBUFFER_MAX  5
#define TUNTDRV_BANDWIDTH_MAX 10

#define TUN_MAX_BW 10

/*******************/
#define MAX_ZONES 48

#ifdef STTUNER_DRV_TER_TUN_MT2266
	#define VERSION 10000  
#endif
	/* Sumsung Tuner initialisation */
	/*	DIV1	*/
	#define RDTVS223_DIV1		0x00
	#define FDTVS223_ZERO		0x000080
	#define FDTVS223_N_MSB		0x00007f

	/*	DIV2	*/
	#define RDTVS223_DIV2		0x01
	#define FDTVS223_N_LSB		0x0100ff

	/*	CTRL1	*/
	#define RDTVS223_CTRL1		0x02
	#define FDTVS223_ONES		0x0200c0
	#define FDTVS223_T		0x020038
	#define FDTVS223_R		0x020007

	/*	CTRL2	*/
	#define RDTVS223_CTRL2		0x03
	#define FDTVS223_CP		0x0300e0
	#define FDTVS223_BS5		0x030010
	#define FDTVS223_BS4		0x030008
	#define FDTVS223_BS3		0x030004
	#define FDTVS223_BS21		0x030003

	/*	CTRL1_2	*/
	#define RDTVS223_CTRL1_2		0x04
	#define FDTVS223_ONE		0x040080
	#define FDTVS223_ZEROS		0x040070
	#define FDTVS223_ATC		0x040008
	#define FDTVS223_AL		0x040007

	/*	STATUS	*/
	#define RDTVS223_STATUS		0x05
	#define FDTVS223_POR		0x050080
	#define FDTVS223_FL		0x050040
	#define FDTVS223_ALBC		0x050020
	#define FDTVS223_R_1		0x050010
	#define FDTVS223_AGC		0x050008
	#define FDTVS223_A		0x050007

	#define DTVS223_NBREGS	6
	#define DTVS223_NBFIELDS	21
	


	/* Tuner ALPS TDQD3-002A */ 
	/*	DIV1	*/
	#define RTDQD3_DIV1		0x0000
	#define FTDQD3_ZERO		0x00000080
	#define FTDQD3_N_MSB		0x0000007F

	/*	DIV2	*/
	#define RTDQD3_DIV2		0x0001
	#define FTDQD3_N_LSB		0x000100FF

	/*	CTRL1	*/
	#define RTDQD3_CTRL1		0x0002
	#define FTDQD3_F1		0x000200C0
	#define FTDQD3_AGST		0x00020038
	#define FTDQD3_R		0x00020007

	/*	CTRL2	*/
	#define RTDQD3_CTRL2		0x0003
	#define FTDQD3_CP		0x000300C0
	#define FTDQD3_TS		0x00030030
	#define FTDQD3_BS4		0x00030008
	#define FTDQD3_BS3		0x00030004
	#define FTDQD3_BS21		0x00030003

	/*	CTRL3	*/
	#define RTDQD3_CTRL3		0x0004
	#define FTDQD3_F2		0x000400C0
	#define FTDQD3_IFW		0x00040030
	#define FTDQD3_CP2		0x00040008
	#define FTDQD3_XLO		0x00040006
	#define FTDQD3_POW		0x00040001

	/*	STATUS	*/
	#define RTDQD3_STATUS		0x0005
	#define FTDQD3_POR		0x00050080
	#define FTDQD3_FL		0x00050040
	#define FTDQD3_ONES		0x00050038
	#define FTDQD3_A		0x00050007

	#define TDQD3_NBREGS	6
	#define TDQD3_NBFIELDS	 20
	/*Thomson DTT7300X tuner definition*/
	/*	P_DIV1	*/
	#define RDTT7300X_P_DIV1		0x0000
	#define FDTT7300X_FIX		0x00000080
	#define FDTT7300X_N_MSB		0x0000007f

	/*	P_DIV2	*/
	#define RDTT7300X_P_DIV2		0x0001
	#define FDTT7300X_N_LSB		0x000100ff

	/*	CTRL	*/
	#define RDTT7300X_CTRL		0x0002
	#define FDTT7300X_ONE		0x00020080
	#define FDTT7300X_CP			0x00020040
	#define FDTT7300X_T			0x00020038
	#define FDTT7300X_RS			0x00020006
	#define FDTT7300X_ZERO		0x00020001

	/*	BW_AUX	*/
	#define RDTT7300X_BW_AUX		0x0003
	#define FDTT7300X_ATC		0x00030080
	#define FDTT7300X_BW_AUX		0x0003003f

	/*	STATUS	*/
	#define RDTT7300X_STATUS		0x0004
	#define FDTT7300X_POR		0x00040080
	#define FDTT7300X_FL		 0x00040040
	#define FDTT7300X_ONES		0x00040030
	#define FDTT7300X_AGC		0x00040008
	#define FDTT7300X_A	    	0x00040007

	
	#define DTT7300X_NBREGS		5
	#define DTT7300X_NBFIELDS	15
	
	
/************* Tuner TDTGD108**********************/
	
	/*LG TDTGD108 tuner definition*/
	/*	P_DIV1	*/
	#define RTDTGD108_P_DIV1		0x0000
	#define FTDTGD108_FIX		0x00000080
	#define FTDTGD108_N_MSB		0x0000007F

	/*	P_DIV2	*/
	#define RTDTGD108_P_DIV2		0x0001
	#define FTDTGD108_N_LSB		0x000100FF

	/*	CTRL	*/
	#define RTDTGD108_CTRL		0x0002
	#define FTDTGD108_ONE		0x00020080
	#define FTDTGD108_CP		0x00020040
	#define FTDTGD108_T21		0x00020030
	#define FTDTGD108_T0		0x00020008 
	#define FTDTGD108_RS		0x00020006
	#define FTDTGD108_ZERO		0x00020001

	/*	BB	*/
	#define RTDTGD108_BB		0x0003
	#define FTDTGD108_ZEROS		0x000300E0
	#define FTDTGD108_P4		0x00030010
	#define FTDTGD108_P3_0		0x0003000F

	/*	P_DIV1_2	*/
	#define RTDTGD108_P_DIV1_2		0x0004
	#define FTDTGD108_FIX_2		0x00040080
	#define FTDTGD108_N_MSB_2		0x0004007f

	/*	P_DIV2_2	*/
	#define RTDTGD108_P_DIV2_2		0x0005
	#define FTDTGD108_N_LSB_2		0x000500FF

	/*	CTRL_2	*/
	#define RTDTGD108_CTRL_2		0x0006
	#define FTDTGD108_ONE_2		0x00060080
	#define FTDTGD108_CP_2		0x00060040
	#define FTDTGD108_T21_2		0x00060030
	#define FTDTGD108_T0_2		0x00060008
	#define FTDTGD108_RS_2		0x00060006
	#define FTDTGD108_ZERO_2		0x00060001

	/*	AUX	*/
	#define RTDTGD108_AUX		0x0007
	#define FTDTGD108_ATC		0x00070080
	#define FTDTGD108_AL		0x00070070
	#define FTDTGD108_ZEROS_2		0x0007000F

	/*	STATUS	*/
	#define RTDTGD108_STATUS		0x0008
	#define FTDTGD108_POR		0x00080080
	#define FTDTGD108_FL		0x00080040
	#define FTDTGD108_ONES		0x00080030
	#define FTDTGD108_AGC		0x00080008
	#define FTDTGD108_A		0x00080007


	
	#define TDTGD108_NBREGS		9
	#define TDTGD108_NBFIELDS	30

	
	
		
	/*PART_REV*/
#define RMT2266_PART_REV           0x0000
#define FMT2266_PART_REV           0x000000FF

/*LO_CTRL_1*/
#define RMT2266_LO_CTRL_1          0x0001
#define FMT2266_LO_CTRL_1          0x000100FF

/*LO_CTRL_2*/
#define RMT2266_LO_CTRL_2          0x0002
#define FMT2266_LO_CTRL_2          0x000200FF

/*LO_CTRL_3*/
#define RMT2266_LO_CTRL_3          0x0003
#define FMT2266_LO_CTRL_3          0x000300FF

/*SMART_ANT*/
#define RMT2266_SMART_ANT          0x0004
#define FMT2266_SMART_ANT          0x000400FF

/*BAND_CTRL*/
#define RMT2266_BAND_CTRL          0x0005
#define FMT2266_BAND_CTRL          0x000500FF

/*RFBANDSEL_2*/
#define RMT2266_RFBANDSEL_2                0x0006
#define FMT2266_RFBANDSEL_2                0x000600FF

/*RFGAINSET*/
#define RMT2266_RFGAINSET          0x0007
#define FMT2266_RFGAINSET          0x000700FF

/*BBFILL0*/
#define RMT2266_BBFILL0            0x0008
#define FMT2266_BBFILL0            0x000800FF

/*BBFILL1*/
#define RMT2266_BBFILL1            0x0009
#define FMT2266_BBFILL1            0x000900FF

/*BBFILL2*/
#define RMT2266_BBFILL2            0x000A
#define FMT2266_BBFILL2            0x000A00FF

/*BBFILL3*/
#define RMT2266_BBFILL3            0x000B
#define FMT2266_BBFILL3            0x000B00FF

/*BBFILQ0*/
#define RMT2266_BBFILQ0            0x000C
#define FMT2266_BBFILQ0            0x000C00FF

/*BBFILQ1*/
#define RMT2266_BBFILQ1            0x000D
#define FMT2266_BBFILQ1            0x000D00FF

/*BBFILQ2*/
#define RMT2266_BBFILQ2            0x000E
#define FMT2266_BBFILQ2            0x000E00FF

/*BBFILQ3*/
#define RMT2266_BBFILQ3            0x000F
#define FMT2266_BBFILQ3            0x000F00FF

/*BBRC_CAL_CTL*/
#define RMT2266_BBRC_CAL_CTL               0x0010
#define FMT2266_BBRC_CAL_CTL               0x001000FF

/*FUSECTL*/
#define RMT2266_FUSECTL            0x0011
#define FMT2266_FUSECTL            0x001100FF

/*STATUS_1*/
#define RMT2266_STATUS_1           0x0012
#define FMT2266_STATUS_1           0x001200FF

/*STATUS_2*/
#define RMT2266_STATUS_2           0x0013
#define FMT2266_STATUS_2           0x001300FF

/*STATUS_3*/
#define RMT2266_STATUS_3           0x0014
#define FMT2266_STATUS_3           0x001400FF

/*STATUS_4*/
#define RMT2266_STATUS_4           0x0015
#define FMT2266_STATUS_4           0x001500FF

/*RSVD_16*/
#define RMT2266_RSVD_16            0x0016
#define FMT2266_RSVD_16            0x001600FF

/*SROADC_CTL*/
#define RMT2266_SROADC_CTL         0x0017
#define FMT2266_SROADC_CTL         0x001700FF

/*RSVD_18*/
#define RMT2266_RSVD_18            0x0018
#define FMT2266_RSVD_18            0x001800FF

/*LO_CTL_2*/
#define RMT2266_LO_CTL_2           0x0019
#define FMT2266_LO_CTL_2           0x001900FF

/*LO_CTL_3*/
#define RMT2266_LO_CTL_3           0x001A
#define FMT2266_LO_CTL_3           0x001A00FF

/*RSVD_1B*/
#define RMT2266_RSVD_1B            0x001B
#define FMT2266_RSVD_1B            0x001B00FF

/*ENABLE*/
#define RMT2266_ENABLE             0x001C
#define FMT2266_ENABLE             0x001C00FF

/*RSVD_1D*/
#define RMT2266_RSVD_1D            0x001D
#define FMT2266_RSVD_1D            0x001D00FF

/*RSVD_1E*/
#define RMT2266_RSVD_1E            0x001E
#define FMT2266_RSVD_1E            0x001E00FF

/*LNA_CTL_3*/
#define RMT2266_LNA_CTL_3          0x001F
#define FMT2266_LNA_CTL_3          0x001F00FF

/*LNA_CTL_4*/
#define RMT2266_LNA_CTL_4          0x0020
#define FMT2266_LNA_CTL_4          0x002000FF

/*RSVD_21*/
#define RMT2266_RSVD_21            0x0021
#define FMT2266_RSVD_21            0x002100FF

/*RSVD_22*/
#define RMT2266_RSVD_22            0x0022
#define FMT2266_RSVD_22            0x002200FF

/*RSVD_23*/
#define RMT2266_RSVD_23            0x0023
#define FMT2266_RSVD_23            0x002300FF

/*RSVD_24*/
#define RMT2266_RSVD_24            0x0024
#define FMT2266_RSVD_24            0x002400FF

/*RSVD_25*/
#define RMT2266_RSVD_25            0x0025
#define FMT2266_RSVD_25            0x002500FF

/*RSVD_26*/
#define RMT2266_RSVD_26            0x0026
#define FMT2266_RSVD_26            0x002600FF

/*RSVD_27*/
#define RMT2266_RSVD_27            0x0027
#define FMT2266_RSVD_27            0x002700FF

/*RSVD_28*/
#define RMT2266_RSVD_28            0x0028
#define FMT2266_RSVD_28            0x002800FF

/*RSVD_29*/
#define RMT2266_RSVD_29            0x0029
#define FMT2266_RSVD_29            0x002900FF

/*RSVD_2A*/
#define RMT2266_RSVD_2A            0x002A
#define FMT2266_RSVD_2A            0x002A00FF

/*RSVD_2B*/
#define RMT2266_RSVD_2B            0x002B
#define FMT2266_RSVD_2B            0x002B00FF

/*BB_CTL_1*/
#define RMT2266_BB_CTL_1           0x002C
#define FMT2266_BB_CTL_1           0x002C00FF

/*BB_CTL_2*/
#define RMT2266_BB_CTL_2           0x002D
#define FMT2266_BB_CTL_2           0x002D00FF

/*BB_CTL_3*/
#define RMT2266_BB_CTL_3           0x002E
#define FMT2266_BB_CTL_3           0x002E00FF

/*BB_CTL_4*/
#define RMT2266_BB_CTL_4           0x002F
#define FMT2266_BB_CTL_4           0x002F00FF

/*BB_CTL_5*/
#define RMT2266_BB_CTL_5           0x0030
#define FMT2266_BB_CTL_5           0x003000FF

/*BB_CTL_6*/
#define RMT2266_BB_CTL_6           0x0031
#define FMT2266_BB_CTL_6           0x003100FF

/*RSVD_32*/
#define RMT2266_RSVD_32            0x0032
#define FMT2266_RSVD_32            0x003200FF

/*BB_CTL_8*/
#define RMT2266_BB_CTL_8           0x0033
#define FMT2266_BB_CTL_8           0x003300FF

/*RSVD_34*/
#define RMT2266_RSVD_34            0x0034
#define FMT2266_RSVD_34            0x003400FF

/*RSVD_35*/
#define RMT2266_RSVD_35            0x0035
#define FMT2266_RSVD_35            0x003500FF

/*RSVD_36*/
#define RMT2266_RSVD_36            0x0036
#define FMT2266_RSVD_36            0x003600FF

/*RSVD_37*/
#define RMT2266_RSVD_37            0x0037
#define FMT2266_RSVD_37            0x003700FF

/*RSVD_38*/
#define RMT2266_RSVD_38            0x0038
#define FMT2266_RSVD_38            0x003800FF

/*RSVD_39*/
#define RMT2266_RSVD_39            0x0039
#define FMT2266_RSVD_39            0x003900FF

/*RSVD_3A*/
#define RMT2266_RSVD_3A            0x003A
#define FMT2266_RSVD_3A            0x003A00FF

/*RSVD_3B*/
#define RMT2266_RSVD_3B            0x003B
#define FMT2266_RSVD_3B            0x003B00FF

/*RSVD_3C*/
#define RMT2266_RSVD_3C            0x003C
#define FMT2266_RSVD_3C            0x003C00FF

#define MT2266_NBREGS	61
#define MT2266_NBFIELDS	61

	


	
	
/* PLL type */
typedef enum
{
    TUNER_PLL_DTT7572,
    TUNER_PLL_DTT7578,
    TUNER_PLL_DTT7592,
    TUNER_PLL_TDLB7,
    TUNER_PLL_TDQD3,
    TUNER_PLL_DTT7300X,
    TUNER_PLL_ENG47402G1,
    TUNER_PLL_EAL2780,
    TUNER_PLL_TDA6650,
    TUNER_PLL_TDM1316,
    TUNER_PLL_TDEB2 ,
    TUNER_PLL_ED5058 ,
    TUNER_PLL_MIVAR ,
    TUNER_PLL_TDED4 ,
    TUNER_PLL_DTT7102 ,
    TUNER_PLL_TECC2849PG ,
    TUNER_PLL_TDCC2345,
    /*TUNER_PLL_DTT7600,*/
    TUNER_PLL_STB4000,
    TUNER_PLL_DTVS223,
    TUNER_PLL_RF4000,
    TUNER_PLL_TDTGD108,
    TUNER_PLL_MT2266
}
TUNTDRV_PLLType_t;

typedef enum
{
    TNR_NO_ERR = 0,
    TNR_TYPE_ERR,
    TNR_ACK_ERR,
    TNR_PLL_ERR,
    TNR_STEP_ERR
}
TUNER_ERROR;

 #ifdef STTUNER_DRV_TER_TUN_MT2266

/*
**  Parameter for function MT2266_Info_t
*/
typedef struct
{
 /*   Handle_t    handle;*/
    /*Handle_t    hUserData;*/
    U32     address;
    U32     version;
    U32     tuner_id;
    U32     f_Ref;
    U32     f_Step;
    U32     f_in;
    U32     f_LO;
    U32     f_bw;
    U32     band;
    U32     num_regs;
    U32 LNAConfig;
    U32 UHFSens;
    U8      RC2_Value;
    U8      RC2_Nominal;
    U8      reg[MT2266_NBREGS];
}  MT2266_Info_t;


#endif



/* tuner instance data */
typedef struct
{
    ST_DeviceName_t        *DeviceName;     /* unique name for opening under */
    STTUNER_Handle_t        TopLevelHandle; /* access tuner etc. using this */
    IOARCH_Handle_t         IOHandle;       /* passed in Open() to an instance of this driver */
    STTUNER_TunerType_t     TunerType;      /* tuner ID number */
    TUNTDRV_PLLType_t       PLLType;        /* PLL on this type of tuner */
    unsigned char           Address;        /* I2C address of the tuner    */
    unsigned char           SubAddress;     /* I2C subaddress of the tuner    */
    int                     I_Q,                        /* IQ inversion    */
                            FreqFactor,                 /* Frequency Factor    */
                            StepSize,                   /* PLL step size   */
                            IF,                         /* IF */
                            SelectBW,                   /* Selected bandwidth  */
                            ChannelBW,                  /* Channel bandwidth  */
                            Repeator;                   /* Using repeator  */

    int                     WrSize;                     /* Size of data to write   */
    unsigned char           WrBuffer[20];               /* Data to write   */
    int                     WrSubSize;                  /* Size of data to write   */
    unsigned char           WrSubBuffer[20];            /* Data to write   */
    int                     RdSize;                     /* Size of data to read    */
    unsigned char           RdBuffer[20];               /* Data read   */

    U32                     BandWidth[TUN_MAX_BW + 1];  /* Bandwidths  */

    ST_Partition_t         *MemoryPartition;     /* which partition this data block belongs to */
    void                   *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                   *InstanceChainNext;   /* next data block in chain or NULL if last */
    TUNER_ERROR             Error;                      /* Last Error  */
    TUNER_Status_t  Status;
    #ifndef STTUNER_MINIDRIVER
    STTUNER_IOREG_DeviceMap_t  DeviceMap;           /* Devicemap for tuner registers */
    #endif
	U8    			*TunerRegVal;
    #ifdef STTUNER_DRV_TER_TUN_MT2266
    MT2266_Info_t        MT2266_Info;
    #endif
    
  
} TUNTDRV_InstanceData_t;

/* enumerated types -------------------------------------------------------- */


/* functions --------------------------------------------------------------- */


/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7572_Install      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7578_Install      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7592_Install      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDLB7_Install        (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDQD3_Install        (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7300X_Install        (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_ENG47402G1_Install   (STTUNER_tuner_dbase_t *Tuner);

ST_ErrorCode_t STTUNER_DRV_TUNER_EAL2780_Install            (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDA6650_Install            (STTUNER_tuner_dbase_t *Tuner);
/*** New Philips tuner added***/
ST_ErrorCode_t STTUNER_DRV_TUNER_TDM1316_Install            (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDEB2_Install              (STTUNER_tuner_dbase_t *Tuner);
/*********New tuner added ED5058*********************/
ST_ErrorCode_t STTUNER_DRV_TUNER_ED5058_Install              (STTUNER_tuner_dbase_t *Tuner);
/********Tuner MIVAR********************/
ST_ErrorCode_t STTUNER_DRV_TUNER_MIVAR_Install              (STTUNER_tuner_dbase_t *Tuner);
/*******Tuner TDED4**********************/
ST_ErrorCode_t STTUNER_DRV_TUNER_TDED4_Install              (STTUNER_tuner_dbase_t *Tuner);
/*******Tuner DTT7102*******************/
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7102_Install              (STTUNER_tuner_dbase_t *Tuner);

/*******Tuner TECC2849PG*******************/
ST_ErrorCode_t STTUNER_DRV_TUNER_TECC2849PG_Install              (STTUNER_tuner_dbase_t *Tuner);

/*******Tuner TDCC2345*******************/
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCC2345_Install              (STTUNER_tuner_dbase_t *Tuner);

 
/*******Tuner DTT7600*******************/
/*ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7600_Install              (STTUNER_tuner_dbase_t *Tuner);*/

/*******Tuner STB4000*******************/
ST_ErrorCode_t STTUNER_DRV_TUNER_STB4000_Install              (STTUNER_tuner_dbase_t *Tuner);

/*******Tuner DTSV223*******************/
ST_ErrorCode_t  STTUNER_DRV_TUNER_DTVS223_Install              (STTUNER_tuner_dbase_t *Tuner);
/*******Tuner RF4000*******************/
ST_ErrorCode_t  STTUNER_DRV_TUNER_RF4000_Install              (STTUNER_tuner_dbase_t *Tuner);
/*******Tuner TDTGD108*******************/
 ST_ErrorCode_t  STTUNER_DRV_TUNER_TDTGD108_Install              (STTUNER_tuner_dbase_t *Tuner);
/*******Tuner MT2266*******************/
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2266_Install(STTUNER_tuner_dbase_t *Tuner);

/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7572_UnInstall    (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7578_UnInstall    (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7592_UnInstall    (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDLB7_UnInstall      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDQD3_UnInstall      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7300X_UnInstall      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_ENG47402G1_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_EAL2780_UnInstall          (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDA6650_UnInstall          (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDM1316_UnInstall          (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDEB2_UnInstall            (STTUNER_tuner_dbase_t *Tuner);
/****************Tuner ED5058************************/
ST_ErrorCode_t STTUNER_DRV_TUNER_ED5058_UnInstall            (STTUNER_tuner_dbase_t *Tuner);
/**************MIVAR************************/
ST_ErrorCode_t STTUNER_DRV_TUNER_MIVAR_UnInstall            (STTUNER_tuner_dbase_t *Tuner);
/**************Tuner TDED4***********************/
ST_ErrorCode_t STTUNER_DRV_TUNER_TDED4_UnInstall            (STTUNER_tuner_dbase_t *Tuner);
/************Tuner DTT7102************************/
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7102_UnInstall            (STTUNER_tuner_dbase_t *Tuner);

/************Tuner TECC2849PG************************/
ST_ErrorCode_t STTUNER_DRV_TUNER_TECC2849PG_UnInstall            (STTUNER_tuner_dbase_t *Tuner);

/************Tuner TDCC2345************************/
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCC2345_UnInstall            (STTUNER_tuner_dbase_t *Tuner);
/************* Tuner DTT7600**********************/
/*ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7600_UnInstall            (STTUNER_tuner_dbase_t *Tuner);*/

/************* Tuner STB4000**********************/
ST_ErrorCode_t STTUNER_DRV_TUNER_STB4000_UnInstall            (STTUNER_tuner_dbase_t *Tuner);
/************* Tuner RF4000**********************/
ST_ErrorCode_t STTUNER_DRV_TUNER_RF4000_UnInstall            (STTUNER_tuner_dbase_t *Tuner);

/************* Tuner DTSV223**********************/
ST_ErrorCode_t STTUNER_DRV_TUNER_DTVS223_UnInstall            (STTUNER_tuner_dbase_t *Tuner);
/************* Tuner TDTGD108**********************/
ST_ErrorCode_t STTUNER_DRV_TUNER_TDTGD108_UnInstall            (STTUNER_tuner_dbase_t *Tuner);
/*******Tuner MT2266*******************/
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2266_UnInstall (STTUNER_tuner_dbase_t *Tuner);
TUNTDRV_InstanceData_t *TUNTDRV_GetInstFromHandle(TUNER_Handle_t Handle);
#ifndef STTUNER_MINIDRIVER
ST_ErrorCode_t tuner_tuntdrv_SetFrequency  (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency);
ST_ErrorCode_t tuner_tuntdrv_GetFrequency  (TUNER_Handle_t Handle, U32 *Frequency);
void           tuner_tuntdrv_GetProperties (TUNER_Handle_t Handle, void *tnr);
void           tuner_tuntdrv_Select        (TUNER_Handle_t Handle, STTUNER_TunerType_t type, unsigned char address);
void TunerReadWrite (TUNER_Handle_t Handle, int mode);
#endif


#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t tuner_tuntdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams);
ST_ErrorCode_t tuner_tuntdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams);
ST_ErrorCode_t tuner_tuntdrv_Open(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams,TUNER_Capability_t  *TUNER_Capability, TUNER_Handle_t *Handle);
ST_ErrorCode_t tuner_tuntdrv_Close( TUNER_CloseParams_t *CloseParams);
void tuner_tuntdrv_Select(STTUNER_TunerType_t type, unsigned char address);
ST_ErrorCode_t tuner_tuntdrv_SetFrequency  (  U32 Frequency, U32 *NewFrequency);
void TunerReadWrite ( int mode);
#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_TER_TUNTDRV_H */


/* End of tuntdrv.h */
