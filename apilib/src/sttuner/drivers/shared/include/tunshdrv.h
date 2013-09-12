/* ----------------------------------------------------------------------------
File Name: tunshdrv.h

Description:

    EXTERNAL shared tuner drivers:

   TD1336

Copyright (C) 2005-2006 STMicroelectronics

History:

   date: 
version: 
 author: 
comment:

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SHARED_TUNSHDRV_H
#define __STTUNER_SHARED_TUNSHDRV_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

/* constants --------------------------------------------------------------- */



#define TUN_MAX_BW 10
#define MAX_ZONES 48
#define MT2060_CNT  (1)    
#define MT_TUNER_CNT               (1)  /*  total num of MicroTuner tuners  */

#ifdef STTUNER_DRV_SHARED_TUN_MT2131
#define VERSION 10200             /*  Version 01.20 */
#endif
#ifdef STTUNER_DRV_SHARED_TUN_MT2060
#define VERSION 10105             /*  Version 01.15 */
#endif
/* PLL type */
typedef enum
{
    TUNER_PLL_TD1336 =0,
    TUNER_PLL_FQD1236 ,
    TUNER_PLL_T2000,
    TUNER_PLL_DTT7600,
    TUNER_PLL_MT2060,
    TUNER_PLL_MT2131,
    TUNER_PLL_TDVEH052F,
    TUNER_PLL_DTT761X ,
    TUNER_PLL_DTT768XX
}
TUNSHDRV_PLLType_t;


	#define TD1336_NBREGS	5
	#define TD1336_NBFIELDS	14
	/* Tuner Philips 1336 */		
	/*	DIV1	*/
	#define RTD1336_DIV1		0x0
	#define FTD1336_ZERO		0x80
	#define FTD1336_N_MSB		0x7f

	/*	DIV2	*/
	#define RTD1336_DIV2		0x1
	#define FTD1336_N_LSB		0x100ff

	/*	CTRL	*/
	#define RTD1336_CTRL		0x2
	#define FTD1336_CONST		0x0200f8
	#define FTD1336_RS		0x020006
	#define FTD1336_Z		0x020001

	/*	BB	*/
	#define RTD1336_BB		0x3
	#define FTD1336_ZEROS		0x0300e0
	#define FTD1336_AGC		0x030018
	#define FTD1336_SP		0x030007

	/*	STATUS	*/
	#define RTD1336_STATUS		0x4
	#define FTD1336_POR		0x040080
	#define FTD1336_FL		0x040040
	#define FTD1336_ONES		0x040038
	#define FTD1336_CA		0x040006
	#define FTD1336_RW		0x040001
	
	
	
	
	/* Tuner Philips PLL FQD1236 */	
	/*	DIV1	*/
	
	
	
	#define FQD1236_NBREGS	5
	#define FQD1236_NBFIELDS	19
	
	

#define RFQD1236_DIV1 0x0
#define FFQD1236_ZERO 0x80
#define FFQD1236_N_MSB 0x7f

#define RFQD1236_DIV2 0x1
#define FFQD1236_N_LSB 0x100ff

#define RFQD1236_CTRL 0x2
#define FFQD1236_ONE 0x20080
#define FFQD1236_CP 0x20040
#define FFQD1236_T 0x20038
#define FFQD1236_RS 0x20006
#define FFQD1236_Z 0x20001

#define RFQD1236_BB_AUX 0x3
#define FFQD1236_ATC 0x30080
#define FFQD1236_AL2 0x30040
#define FFQD1236_AL1 0x30020
#define FFQD1236_AL0_P4 0x30010
#define FFQD1236_P3 0x30008
#define FFQD1236_P0_1_2 0x30007

#define RFQD1236_STATUS 0x4
#define FFQD1236_POR 0x40080
#define FFQD1236_FL 0x40040
#define FFQD1236_ONES 0x40030
#define FFQD1236_AGC 0x40008
#define FFQD1236_A 0x40007

/* Tuner Philips PLL T2000 */	
	/*	DIV1	*/

	#define T2000_NBREGS		5
	#define T2000_NBFIELDS	15	
	
#define RT2000_DIV1 0x0
#define FT2000_ZERO 0x80
#define FT2000_N_MSB 0x7f

#define RT2000_DIV2 0x1
#define FT2000_N_LSB 0x100ff

#define RT2000_CTRL1 0x2
#define FT2000_ONE 0x20080
#define FT2000_C 0x20060
#define FT2000_R 0x2001f

#define RT2000_CTRL2 0x3
#define FT2000_Z1 0x30080
#define FT2000_BS 0x30060
#define FT2000_Z2 0x30010
#define FT2000_P_3_2 0x3000c
#define FT2000_P1 0x30002
#define FT2000_P0 0x30001

#define RT2000_STATUS 0x4
#define FT2000_POR 0x40080
#define FT2000_FL 0x40040
#define FT2000_ZEROS 0x4003f
	


	#define DTT7600_NBREGS	5
	#define DTT7600_NBFIELDS 15

#define RDTT7600_P_DIV1 0x0
#define FDTT7600_FIX 0x80
#define FDTT7600_N_MSB 0x7f

#define RDTT7600_P_DIV2 0x1
#define FDTT7600_N_LSB 0x100ff

#define RDTT7600_CTRL 0x2
#define FDTT7600_ONE 0x20080
#define FDTT7600_CP 0x20040
#define FDTT7600_T 0x20038
#define FDTT7600_RS 0x20006
#define FDTT7600_OS 0x20001

#define RDTT7600_BW_AUX 0x3
#define FDTT7600_ATC 0x30080
#define FDTT7600_AL 0x30070
#define FDTT7600_P 0x3000f

#define RDTT7600_STATUS 0x4
#define FDTT7600_POR 0x40080
#define FDTT7600_FL 0x40040
#define FDTT7600_ONES 0x40030
#define FDTT7600_AGC 0x40008
#define FDTT7600_A 0x40007	
	
/* LG Tuner initialisation */
	/*	DIV1	*/
	#define RTDVEH052F_DIV1		0x0
	#define FTDVEH052F_ZERO		0x80
	#define FTDVEH052F_N_MSB		0x7f

	/*	DIV2	*/
	#define RTDVEH052F_DIV2		0x1
	#define FTDVEH052F_N_LSB		0x100ff

	/*	CTRL	*/
	#define RTDVEH052F_CTRL		0x2
	#define FTDVEH052F_ONE		0x20080
	#define FTDVEH052F_CP		0x20040
	#define FTDVEH052F_T		0x20038
	#define FTDVEH052F_R		0x20006
	#define FTDVEH052F_OS		0x20001

	/*	BB_AUX	*/
	#define RTDVEH052F_BB_AUX		0x3
	#define FTDVEH052F_ATC		0x30080
	#define FTDVEH052F_AL20		0x30070
	#define FTDVEH052F_P3		0x3000f

	/*	STATUS	*/
	#define RTDVEH052F_STATUS		0x4
	#define FTDVEH052F_POR		0x40080
	#define FTDVEH052F_FL		0x40040
	#define FTDVEH052F_ONES		0x40030
	#define FTDVEH052F_AGC		0x40008
	#define FTDVEH052F_A		0x40007

	#define TDVEH052F_NBREGS	5
	#define TDVEH052F_NBFIELDS	16	

	
	/**TMM DTT761X*/

	
/*P_DIV1*/
#define RDTT761X_P_DIV1         0x0000
#define FDTT761X_ZERO1          0x00000080
#define FDTT761X_N_MSB          0x0000007F



/*P_DIV2*/
#define RDTT761X_P_DIV2         0x0001
#define FDTT761X_N_LSB          0x000100FF

/*CTRL*/
#define RDTT761X_CTRL           0x0002
#define FDTT761X_ONE            0x00020080
#define FDTT761X_CP             0x00020040
#define FDTT761X_T21            0x00020030
#define FDTT761X_T0             0x00020008
#define FDTT761X_RS             0x00020006
#define FDTT761X_OS             0x00020001

/*BB*/
#define RDTT761X_BB             0x0003
#define FDTT761X_FIX            0x000300C0
#define FDTT761X_P5             0x00030020
#define FDTT761X_P4             0x00030010
#define FDTT761X_P3             0x00030008
#define FDTT761X_P2_0           0x00030007

/*P_DIV1_2*/
#define RDTT761X_P_DIV1_2               0x0000
#define FDTT761X_ZERO_2         0x00000080
#define FDTT761X_N_MSB_2                0x0000007F

/*P_DIV2_2*/
#define RDTT761X_P_DIV2_2               0x0001
#define FDTT761X_N_LSB_2                0x000100FF

/*CTRL_2*/
#define RDTT761X_CTRL_2         0x0002
#define FDTT761X_ONE_2          0x00020080
#define FDTT761X_CP_2           0x00020040
#define FDTT761X_T21_2          0x00020030
#define FDTT761X_T0_2           0x00020008
#define FDTT761X_RS_2           0x00020006
#define FDTT761X_OS_2           0x00020001

/*AUX*/
#define RDTT761X_AUX            0x0003
#define FDTT761X_ATC            0x00030080
#define FDTT761X_AL20           0x00030070
#define FDTT761X_ZERO3          0x0003000F

/*STATUS*/
#define RDTT761X_STATUS         0x0004
#define FDTT761X_AFC            0x00040080
#define FDTT761X_VIF            0x00040040
#define FDTT761X_FM             0x00040020
#define FDTT761X_AFC41          0x0004001E
#define FDTT761X_POR            0x00040001

#define DTT761X_NBREGS	    9
	#define DTT761X_NBFIELDS	31





/*P_DIV1*/
#define RDTT768xx_P_DIV1         0x0000
#define FDTT768xx_ZERO1          0x00000080
#define FDTT768xx_N_MSB          0x0000007F

/*P_DIV2*/
#define RDTT768xx_P_DIV2         0x0001
#define FDTT768xx_N_LSB          0x000100FF

/*CTRL4*/
#define RDTT768xx_CTRL4          0x0002
#define FDTT768xx_ONE            0x00020080
#define FDTT768xx_C10            0x00020060
#define FDTT768xx_R40            0x0002001F

/*CTRL5*/
#define RDTT768xx_CTRL5          0x0003
#define FDTT768xx_BS10           0x000300C0
#define FDTT768xx_SL10           0x00030030
#define FDTT768xx_P30            0x0003000F

/*CTRL6*/
#define RDTT768xx_CTRL6          0x0002
#define FDTT768xx_X1             0x00020080
#define FDTT768xx_ZERO2          0x00020040
#define FDTT768xx_ATC            0x00020020
#define FDTT768xx_IFE            0x00020010
#define FDTT768xx_X2             0x00020008
#define FDTT768xx_AT20           0x00020007

/*CTRL7*/
#define RDTT768xx_CTRL7          0x0003
#define FDTT768xx_SAS            0x00030080
#define FDTT768xx_X3             0x00030040
#define FDTT768xx_AGD            0x00030020
#define FDTT768xx_ADS            0x00030010
#define FDTT768xx_T30            0x0003000F

/*STATUS*/
#define RDTT768xx_STATUS         0x0004
#define FDTT768xx_POR            0x00040080
#define FDTT768xx_FL             0x00040040
#define FDTT768xx_ZERO3          0x00040030
#define FDTT768xx_AGF            0x00040008
#define FDTT768xx_V20            0x00040007

#define DTT768XX_NBREGS		7
	#define DTT768XX_NBFIELDS	25


	#ifdef STTUNER_DRV_SHARED_TUN_MT2060
	

#define    PART_REV  0x0000   /*  0x00: Part/Rev Code        */
#define    LO1C_1     0x0001    /*  0x01: LO1C Byte 1          */
#define    LO1C_2    0x0002     /*  0x02: LO1C Byte 2          */
#define    LO2C_1   0x0003      /*  0x03: LO2C Byte 1          */
#define    LO2C_2 0x0004      /*  0x04: LO2C Byte 2          */
#define    LO2C_3   0x0005     /*  0x05: LO2C Byte 3          */
#define    LO_STATUS  0x0006    /*  0x06: LO Status Byte       */
#define    FM_FREQ    0x0007    /*  0x07: FM FREQ Byte         */
#define    MISC_STATUS  0x0008  /*  0x08: Misc Status Byte     */
#define    MISC_CTRL_1  0x0009  /*  0x09: Misc Ctrl Byte 1     */
#define    MISC_CTRL_2  0x000a  /*  0x0A: Misc Ctrl Byte 2     */
#define    MISC_CTRL_3  0x000b  /*  0x0B: Misc Ctrl Byte 3     */

#define    RSVD_0C	0x000c
#define    RSVD_0D	0x000d
#define    RSVD_0E	0x000e
#define    RSVD_0F	0x000f
#define    RSVD_10	0x0010
#define    RSVD_11	0x0011

	#define MT2060_NBREGS	18
	#define MT2060_NBFIELDS	18	
	#endif
	
#ifdef STTUNER_DRV_SHARED_TUN_MT2131	

#define  MT2131_PART_REV         0x0000/*Part/Rev Code        */
#define  MT2131_LO1C_1           0x0001/*LO1C Byte 1          */
#define  MT2131_LO1C_2           0x0002/*LO1C Byte 2          */
#define  MT2131_LO1C_3           0x0003/*LO1C Byte 2          */
#define  MT2131_LO2C_1           0x0004/*LO2C Byte 1          */
#define  MT2131_LO2C_2           0x0005/*LO2C Byte 2          */
#define  MT2131_LO2C_3           0x0006/*LO2C Byte 3          */
#define  MT2131_PWR              0x0007/*PWR Byte             */
#define  MT2131_LO_STATUS        0x0008/*LO Status Byte       */
#define  MT2131_AFC              0x0009/*AFC Byte             */
#define  MT2131_TEMP_STATUS      0x000A/*Temperature Byte     */
#define  MT2131_UPC_1            0x000B/*RF Filter Byte       */
#define  MT2131_UPC_2            0x000C/*Reserved Byte        */
#define  MT2131_AGC_1            0x000D/*RF AGC Byte 1        */
#define  MT2131_AGC_2            0x000E/*RF AGC Byte 2        */
#define  MT2131_AGC_3            0x000F/*RF AGC Byte 3        */
#define  MT2131_AGC_RL           0x0010/*RL Atten Byte        */
#define  MT2131_AGC_DNC          0x0011/*DNC Atten Byte       */
#define  MT2131_PDET_1           0x0012/*RF PD Byte           */
#define  MT2131_PDET_2           0x0013/*IF PD Byte           */
#define  MT2131_MISC_1           0x0014/*Reserved Byte        */
#define  MT2131_MISC_2           0x0015/*Reserved Byte        */
#define  MT2131_RSVD_16          0x0016/*Reserved Byte        */
#define  MT2131_RSVD_17          0x0017/*Reserved Byte        */
#define  MT2131_RSVD_18          0x0018/*Reserved Byte        */
#define  MT2131_RSVD_19          0x0019/*Reserved Byte        */
#define  MT2131_RSVD_1A          0x001A/*Reserved Byte        */
#define  MT2131_RSVD_1B          0x001B/*Reserved Byte        */
#define  MT2131_RSVD_1C          0x001C/*Reserved Byte        */
#define  MT2131_RSVD_1D          0x001D/*Reserved Byte        */
#define  MT2131_RSVD_1E          0x001E/*Reserved Byte        */
#define  MT2131_CAPTRIM          0x001F/*CapTrim Byte         */

#define MT2131_NBREGS	32
#define MT2131_NBFIELDS	32	
		
#endif	
	
	typedef enum
	{
		TUNER_NO_ERR = 0,
		TUNER_TYPE_ERR,
		TUNER_ACK_ERR
	} SHTUNER_Error_t;


 

struct MT_ExclZone_t
{
    U32         min_;
    U32         max_;
    struct MT_ExclZone_t*  next_;
};
struct MT_FIFZone_t
{
    S32         min_;
    S32         max_;
};


typedef struct
{
    U32 nAS_Algorithm;
    U32 f_ref;
    U32 f_in;
    U32 f_LO1;
    U32 f_if1_Center;
    U32 f_if1_Request;
    U32 f_if1_bw;
    U32 f_LO2;
    U32 f_out;
    U32 f_out_bw;
    U32 f_LO1_Step;
    U32 f_LO2_Step;
    U32 f_LO1_FracN_Avoid;
    U32 f_LO2_FracN_Avoid;
    U32 f_zif_bw;
    U32 f_min_LO_Separation;
    U32 maxH1;
    U32 maxH2;
    U32 bSpurPresent;
    U32 bSpurAvoided;
    U32 nSpursFound;
    U32 nZones;
    struct MT_ExclZone_t* freeZones;
    struct MT_ExclZone_t* usedZones;
    struct MT_ExclZone_t MT_ExclZones[MAX_ZONES];
} MT_AvoidSpursData_t;


#ifdef STTUNER_DRV_SHARED_TUN_MT2060
typedef struct
{
    /*Handle_t    handle;
    Handle_t    hUserData;*/
    U32     address;
    U32     version;
    U32     tuner_id;
    MT_AvoidSpursData_t AS_Data;
    U32     f_IF1_actual;
    U32     num_regs;
   /* U8      reg[END_REGS];*/
    U32     RF_Bands[10];
}  MT2060_Info_t;

#endif


#ifdef STTUNER_DRV_SHARED_TUN_MT2131
/*
**  Parameter for function MT2131_Info_t
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
   /* U8      reg[MT2131_NBREGS];*/
   
    U32     f_IF1_actual;
      
    U32     RF_Bands[19];
    U32     rcvr_mode;
    U32     CTrim_sum;
    U32     VGA_gain_mode;
    U32     LNAgain_mode;
     MT_AvoidSpursData_t AS_Data;
}  MT2131_Info_t;


#endif



/* tuner instance data */
typedef struct
{
    ST_DeviceName_t        *DeviceName;     /* unique name for opening under */
    STTUNER_Handle_t        TopLevelHandle; /* access tuner etc. using this */
    IOARCH_Handle_t         IOHandle;       /* passed in Open() to an instance of this driver */
    STTUNER_TunerType_t     TunerType;      /* tuner ID number */
    TUNSHDRV_PLLType_t       PLLType;        /* PLL on this type of tuner */
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
    SHTUNER_Error_t             Error;                      /* Last Error  */
    TUNER_Status_t  Status;
    STTUNER_IOREG_DeviceMap_t  DeviceMap;           /* Devicemap for tuner registers */
    U8    			*TunerRegVal;
     #ifdef STTUNER_DRV_SHARED_TUN_MT2060
    MT2060_Info_t        MT2060_Info;
    U32      FirstIF;
    #endif
    #ifdef STTUNER_DRV_SHARED_TUN_MT2131
    MT2131_Info_t        MT2131_Info;
    
    #endif
} TUNSHDRV_InstanceData_t;

/* enumerated types -------------------------------------------------------- */


/* functions --------------------------------------------------------------- */



ST_ErrorCode_t STTUNER_DRV_TUNER_TD1336_Install (STTUNER_tuner_dbase_t *Tuner);

ST_ErrorCode_t STTUNER_DRV_TUNER_TD1336_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_FQD1236_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_FQD1236_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_T2000_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_T2000_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7600_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7600_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT761X_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT761X_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT768XX_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT768XX_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2060_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2060_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2131_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2131_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDVEH052F_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDVEH052F_UnInstall (STTUNER_tuner_dbase_t *Tuner);


ST_ErrorCode_t tuner_tunshdrv_SetFrequency  (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency);
ST_ErrorCode_t tuner_tunshdrv_GetFrequency  (TUNER_Handle_t Handle, U32 *Frequency);
void           tuner_tunshdrv_GetProperties (TUNER_Handle_t Handle, void *tnr);
void           tuner_tunshdrv_Select        (TUNER_Handle_t Handle, STTUNER_TunerType_t type, unsigned char address);

void TunerReadWrite (TUNER_Handle_t Handle, int mode);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SHARED_TUNSHDRV_H */


/* End of tunshdrv.h */
