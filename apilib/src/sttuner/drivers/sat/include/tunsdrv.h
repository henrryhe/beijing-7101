/* ----------------------------------------------------------------------------
File Name: tunsdrv.h

Description: 

    EXTERNAL (ZIF) tuner drivers:

    VG1011
    S68G21
    TUA6100
    EVALMAX

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 3-July-2001
version: 3.1.0
 author: GJP from work by LW
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_TUNSDRV_H
#define __STTUNER_SAT_TUNSDRV_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */
#include "ioreg.h"


/* constants --------------------------------------------------------------- */

/* In Hz -- Usually 125KHz (Ver1 code in KHz) */
	#define TUNSDRV_TUNER_STEP  125000

	#define TUNSDRV_IOBUFFER_MAX  5
	#define TUNSDRV_BANDWIDTH_MAX 10


/* Tuner registers definition -----------------------------*/
/* sharp VG0011 tuner definition */
	
	#define VG0011_NBREGS	5
	#define VG0011_NBFIELDS	13
	
	/* sharp HZ1184_ tuner definition */
	
	/*	DIVM	*/
	#define RHZ1184_DIVM  		0x01
	#define FHZ1184_N_MSB		0x1007F
	
	/*	DIVL	*/
	#define RHZ1184_DIVL		0x02
	#define FHZ1184_N_LSB		0x200E0
	#define FHZ1184_A		0x2001F
	
	/*	CTRL1	*/
	#define RHZ1184_CTRL1  		0x03
	#define FHZ1184_N_HSB		0x30060
	#define FHZ1184_PD5		0x30010
	#define FHZ1184_PD4		0x30008
	#define FHZ1184_R		0x30007
	
	/*	CTRL2	*/
	#define RHZ1184_CTRL2  		0x04
	#define FHZ1184_CP		0x400C0
	#define FHZ1184_RE		0x40020
	#define FHZ1184_RTS		0x40010
	#define FHZ1184_PD3		0x40008
	#define FHZ1184_PD2		0x40004
	#define FHZ1184_PD1		0x40002
	#define FHZ1184_PD0		0x40001
	
	/*	STATUS	*/
	#define RHZ1184_STATUS  	0x05
	#define FHZ1184_POR		0x50080
	#define FHZ1184_LOCK		0x50040
	#define FHZ1184_MA		0x50006
	
	#define HZ1184_NBREGS	5
	#define HZ1184_NBFIELDS	17
	
	/* Maxim 2116 tuner definition */      
	#define RMAX2116_SUBADR  0x00
    	#define FMAX2116_SUBADR 0x000FF
    
    
    #define RMAX2116_NHIGH  		0x01
    #define FMAX2116_DIV2		0x10080
    #define FMAX2116_N_MSB		0x1007F
    
   
    #define RMAX2116_NLOW  		0x02
    #define FMAX2116_N_LSB		0x200FF
    
    
    #define RMAX2116_R_CP_VCO 		0x03
    #define FMAX2116_R			0x300E0
    #define FMAX2116_CP			0x30018
    #define FMAX2116_VCO		0x30007
    
   
    #define RMAX2116_FILT_OUT 		0x04
    #define FMAX2116_F_OUT		0x4007F
    
    #define RMAX2116_LPF_DAC		0x05
    #define FMAX2116_ADL		0x50080
    #define FMAX2116_ADE		0x50040
    #define FMAX2116_DL			0x50020
    #define FMAX2116_M			0x5001F
    
    #define RMAX2116_GC2_DIAG		0x06
    #define FMAX2116_D			0x600E0
    #define FMAX2116_G			0x6001F
    
   
    #define RMAX2116_STATUS		0x07
    #define FMAX2116_PWR		0x070040
    #define FMAX2116_ADC		0x07001C
    
    
    #define RMAX2116_FILT_IN		0x08
    #define FMAX2116_F_IN		0x800FF
    



	#define MAX2116_NBREGS	9
	#define MAX2116_NBFIELDS	17
	
	
	/* RFMagic STB6000 tuner definition */  
	/*	SUBADR	*/
	/*	SUBADR	*/
	#define RSTB6000_SUBADR		0x0000
	#define FSTB6000_SUBADR		0x000000ff

	/*	ODIV	*/
	#define RSTB6000_ODIV		0x0001
	#define FSTB6000_OSCH		0x00010080
	#define FSTB6000_OCK		0x00010060
	#define FSTB6000_ODIV		0x00010010
	#define FSTB6000_OSM		0x0001000f

	/*	N	*/
	#define RSTB6000_N		0x0002
	#define FSTB6000_N_MSB		0x000200ff

	/*	CP_A	*/
	#define RSTB6000_CP_A		0x0003
	#define FSTB6000_N_LSB		0x00030080
	#define FSTB6000_A		0x0003001f

	/*	K_R	*/
	#define RSTB6000_K_R		0x0004
	#define FSTB6000_K		0x000400c0
	#define FSTB6000_R		0x0004003f

	/*	G	*/
	#define RSTB6000_G		0x0005
	#define FSTB6000_G		0x0005000f

	/*	F	*/
	#define RSTB6000_F		0x0006
	#define FSTB6000_F		                0x0006001f

	/*	FCL	*/
	#define RSTB6000_FCL		0x0007
	#define FSTB6000_DLB		0x00070038
	#define FSTB6000_FCL		0x00070007

	/*	TEST1	*/
	#define RSTB6000_TEST1		0x0008
	#define FSTB6000_TEST1		0x000800ff

	/*	TEST2	*/
	#define RSTB6000_TEST2		0x0009
	#define FSTB6000_TEST2		0x000900ff

	/*	LPEN	*/
	#define RSTB6000_LPEN		0x000a
	#define FSTB6000_LPEN		0x000a0010

	/*	XOG	*/
	#define RSTB6000_XOG		0x000b
	#define FSTB6000_XOG		0x000b0080
	#define FSTB6000_XOGV		0x000b0040

	/*	LD	*/
	#define RSTB6000_LD		0x000c
	#define FSTB6000_LD		0x000c0001
	
	#define STB6000_NBREGS	13
	#define STB6000_NBFIELDS	20
	
	/**********************************************/
	/* STB6110 tuner definition */ 
		/*CTRL1*/
	#define RSTB6110_CTRL1  0x0000
	#define FSTB6110_K  0x000000f8
	#define FSTB6110_LPT  0x00000004
	#define FSTB6110_RX  0x00000002
	#define FSTB6110_SYN  0x00000001
	
	 	/*CTRL2*/
	#define RSTB6110_CTRL2  0x0001
	#define FSTB6110_CO_DIV  0x000100c0
	#define FSTB6110_REFOUTSEL  0x00010010
	#define FSTB6110_BBGAIN  0x0001000f
	
		/*TUNING0*/
	#define RSTB6110_TUNING0  0x0002
	#define FSTB6110_NDIV_LSB  0x000200ff
	
		/*TUNING1*/
	#define RSTB6110_TUNING1  0x0003
	#define FSTB6110_RDIV  0x000300c0
	#define FSTB6110_PRESC32ON  0x00030020
	#define FSTB6110_DIV4SEL  0x00030010
	#define FSTB6110_NDIV_MSB  0x0003000f
	
		/*CTRL3*/
	#define RSTB6110_CTRL3  0x0004
	#define FSTB6110_DCLOOP_OFF  0x00040080
	#define FSTB6110_ICP  0x00040020
	#define FSTB6110_CF  0x0004001f
	
		/*STAT1*/
	#define RSTB6110_STAT1  0x0005
	#define FSTB6110_TEST1  0x000500f8
	#define FSTB6110_CALVCOSTRT  0x00050004
	#define FSTB6110_CALRCSTRT  0x00050002
	#define FSTB6110_LOCKPLL  0x00050001
	
		/*STAT2*/
	#define RSTB6110_STAT2  0x0006
	#define FSTB6110_TEST2  0x000600ff
	
		/*STAT3*/
	#define RSTB6110_STAT3  0x0007
	#define FSTB6110_TEST3  0x000700ff
	
		#define	STB6110_NBREGS 8
		#define STB6110_NBFIELDS 21
	
	/***********************************************/
		/* RFMagic STB6100 tuner definition */ 	
	/*	SUBADR	*/
	/*	SUBADR	*/
	/*	SUBADR	*/

		/*	LD	*/
	#define RSTB6100_LD		0x0000
	#define FSTB6100_LD		0x00000001

		/*	VCO	*/
	#define RSTB6100_VCO		0x0001
	#define FSTB6100_OSCH		0x00010080
	#define FSTB6100_OCK		0x00010060
	#define FSTB6100_ODIV		0x00010010
	#define FSTB6100_OSM		0x0001000f

	/*	NI	*/
	#define RSTB6100_NI		0x0002
	#define FSTB6100_NI		0x000200ff

	
		/*	NF_LSB	*/
	#define RSTB6100_NF_LSB		0x0003
	#define FSTB6100_NF_LSB		0x000300ff


		/*	K	*/
	#define RSTB6100_K		0x0004
	#define FSTB6100_K		0x000400c0
	#define FSTB6100_PSD2		0x00040004
	#define FSTB6100_NF_MSB		0x00040003


		/*	G	*/
	#define RSTB6100_G		0x0005
	#define FSTB6100_GCT		0x000500e0
	#define FSTB6100_G		0x0005000f


	/*	F	*/
	#define RSTB6100_F		0x0006
	#define FSTB6100_F		0x0006001f


	/*	DLB	*/
	#define RSTB6100_DLB		0x0007
	#define FSTB6100_DLB		0x00070038


		/*	TEST1	*/
	#define RSTB6100_TEST1		0x0008
	#define FSTB6100_TEST1		0x000800ff


		/*	TEST2	*/
	#define RSTB6100_TEST2		0x0009
	#define FSTB6100_FCCK		0x00090040


	/*	LPEN	*/
	#define RSTB6100_LPEN		0x000a
	#define FSTB6100_BEN		0x000a0080
	#define FSTB6100_OSCP		0x000a0040
	#define FSTB6100_SYNP		0x000a0020
	#define FSTB6100_LPEN		0x000a0010
	
	/*	TEST3	*/
	#define RSTB6100_TEST3		0x000b
	#define FSTB6100_TEST3		0x000b00ff


	#define STB6100_NBREGS	12
	#define STB6100_NBFIELDS	21	
	
								
	/* added in LLA 1.9.3*/	
	/* Tuner Sat definition */
	/*	TUNING_LSB	*/
	#define RTUNERSAT_TUNING_LSB  	0x0000
	#define FTUNERSAT_N_LSB		0x000FF

	/*	TUNING_MSB	*/
	#define RTUNERSAT_TUNING_MSB 	0x0001
	#define FTUNERSAT_RDIV		0x100C0
	#define FTUNERSAT_ODIV		0x10030
	#define FTUNERSAT_N_MSB		0x1000F

	/*	CONTROL1	*/
	#define RTUNERSAT_CONTROL1 	0x0002
	#define FTUNERSAT_CALVCOSTRT	0x20080
	#define FTUNERSAT_DIV4SEL	0x20040
	#define FTUNERSAT_PRESC32ON	0x20020
	#define FTUNERSAT_CF		0x2001F

	/*	CONTROL2	*/
	#define RTUNERSAT_CONTROL2 	0x0003
	#define FTUNERSAT_LCP		0x300C0
	#define FTUNERSAT_XTALOUT	0x30020
	#define FTUNERSAT_XTALON	0x30010
	#define FTUNERSAT_CALOFF	0x30008
	#define FTUNERSAT_LPT		0x30004
	#define FTUNERSAT_RX		0x30002
	#define FTUNERSAT_SYN		0x30001

	/*	CONTROL3	*/
	#define RTUNERSAT_CONTROL3  	0x0004
	#define FTUNERSAT_DIVTEST	0x400C0
	#define FTUNERSAT_CPTEST	0x40030
	#define FTUNERSAT_BBGAIN	0x4000F

	/*	STATUS1	*/
	#define RTUNERSAT_STATUS1  	0x0005
	#define FTUNERSAT_COMP		0x50080
	#define FTUNERSAT_SEL		0x5007E
	#define FTUNERSAT_LOCK		0x50001

	/*	STATUS2	*/
	#define RTUNERSAT_STATUS2  	0x0006 
	#define FTUNERSAT_RON  		0x60080
	#define FTUNERSAT_RCCA_LOFF 	0x60040
	#define FTUNERSAT_RC		0x6003F

	/*	STATUS3	*/
	#define RTUNERSAT_STATUS3  	0x0007
	#define FTUNERSAT_RDK		0x70004
	#define FTUNERSAT_CALTIME	0x70002
	#define FTUNERSAT_CALRC_STRT	0x70001
	
	#define TUNERSAT_NBREGS	8
	#define TUNERSAT_NBFIELDS	27	
	
	/* sharp IX2476 tuner definition */
	/*	REGISTER Definition IX2476	*/
	/*	NHIGH	*/
	#define RIX2476_NHIGH  		0x0001
	#define FIX2476_BG		0x10060
	#define FIX2476_N_MSB		0x1001F

	/*	NLOW_SDIV	*/
	#define RIX2476_NLOW_SDIV  	0x0002
	#define FIX2476_N_LSB		0x200E0
	#define FIX2476_A		0x2001F

	/*	CP_LPF_R	*/
	#define  RIX2476_CP_LPF_R	0x0003
	#define  FIX2476_CP		0x30060
	#define  FIX2476_PD5		0x30010
	#define  FIX2476_PD4		0x30008
	#define  FIX2476_TM		0x30004
	#define  FIX2476_RTS		0x30002
	#define  FIX2476_REF		0x30001

	/*	VCO_LPF	*/
	#define RIX2476_VCO_LPF  	0x0004
	#define FIX2476_VCO		0x400E0
	#define FIX2476_PSC		0x40010
	#define FIX2476_PD3		0x40008
	#define FIX2476_PD2		0x40004
	#define FIX2476_DIV		0x40002
	#define FIX2476_P0		0x40001

	/*	STATUS	*/
	#define RIX2476_STATUS  	0x0005
	#define FIX2476_PWR		0x50080
	#define FIX2476_FL		0x50040
		
	#define IX2476_NBREGS	5
	#define IX2476_NBFIELDS	18	

/* enumerated types -------------------------------------------------------- */

    /* PLL type */
    typedef enum
    {
        TUNER_PLL_5522,
        TUNER_PLL_5655,
        TUNER_PLL_5659,
        TUNER_PLL_TUA6100,
        TUNER_PLL_ABSENT
    } 
    TUNSDRV_PLLType_t;
    /*For the Crystal Used.*/


#ifndef STTUNER_MINIDRIVER
/* tuner instance data */
typedef struct
{
    ST_DeviceName_t     *DeviceName;     /* unique name for opening under */
    STTUNER_Handle_t    TopLevelHandle; /* access tuner, lnb etc. using this */
    IOARCH_Handle_t     IOHandle;       /* passed in Open() to an instance of this driver */
    STTUNER_TunerType_t TunerType;      /* tuner ID number */
    TUNSDRV_PLLType_t   PLLType;        /* PLL on this type of tuner */
    U32                 SymbolRate;
    U32                 FreqFactor;
    TUNER_Status_t      Status;
    U8                  IOBuffer[TUNSDRV_IOBUFFER_MAX];    /* buffer for ioarch I/O */
    U32                 BandWidth[TUNSDRV_BANDWIDTH_MAX];
    ST_Partition_t      *MemoryPartition;     /* which partition this data block belongs to */
    void                *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                *InstanceChainNext;   /* next data block in chain or NULL if last */
    STTUNER_IOREG_DeviceMap_t  DeviceMap;           /* Devicemap for tuner registers */
    U8  *TunerRegVal;
     U32                         ExternalClock;            /* External VCO */
} TUNSDRV_InstanceData_t;
#endif
#ifdef STTUNER_MINIDRIVER
typedef struct
{
    ST_DeviceName_t     *DeviceName;     /* unique name for opening under */
    STTUNER_Handle_t    TopLevelHandle; /* access tuner, lnb etc. using this */
    IOARCH_Handle_t     IOHandle;       /* passed in Open() to an instance of this driver */
    STTUNER_TunerType_t TunerType;      /* tuner ID number */
    TUNSDRV_PLLType_t      PLLType;        /* PLL on this type of tuner */

    U32             FreqFactor;
    TUNER_Status_t  Status;
    U8              IOBuffer[TUNSDRV_IOBUFFER_MAX];    /* buffer for ioarch I/O */
    U32             BandWidth[TUNSDRV_BANDWIDTH_MAX];

    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */
         
} TUNSDRV_InstanceData_t;
#endif

typedef enum TUNER_Utility_e{

VCO_SEARCH_OFF
}TUNER_Utility_t;

/* functions --------------------------------------------------------------- */


/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_TUNER_VG1011_Install (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_S68G21_Install (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TUA6100_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_EVALMAX_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_VG0011_Install (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_HZ1184_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MAX2116_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DSF8910_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6000_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6110_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6100_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_IX2476_Install(STTUNER_tuner_dbase_t *Tuner);

/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_TUNER_VG1011_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_S68G21_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TUA6100_UnInstall(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_EVALMAX_UnInstall(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_VG0011_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_HZ1184_UnInstall(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MAX2116_UnInstall(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DSF8910_UnInstall(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6000_UnInstall(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6110_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_STB6100_UnInstall(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_IX2476_UnInstall(STTUNER_tuner_dbase_t *Tuner);

#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t tuner_tunsdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams);
ST_ErrorCode_t tuner_tunsdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams);

ST_ErrorCode_t tuner_tunsdrv_Open(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t  *TUNER_Capability, TUNER_Handle_t *Handle);

ST_ErrorCode_t tuner_tunsdrv_Close(TUNER_Handle_t Handle, TUNER_CloseParams_t *CloseParams);

#endif
ST_ErrorCode_t tuner_tunsdrv_Utilfunction(TUNER_Handle_t Handle, TUNER_Utility_t Utility);
#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_TUNSDRV_H */


/* End of tunsdrv.h */
