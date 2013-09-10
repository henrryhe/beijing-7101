/**********************************************************************

File Name   : stflash.h

Description : External function prototypes for STFLASH API
              Declares the function calls and their formal
              parameters.  Requires prior inclusion of
              stddefs.h file to define parameter sizes.

Copyright (C) 2000, ST Microelectronics

References  :

$ClearCase (VOB: stflash)

API.PDF "Flash Memory API" Reference DVD-API-004 Revision 1.3

***********************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STFLASH_H
#define __STFLASH_H

/* Includes --------------------------------------------------------- */
#include "stddefs.h"
#include "stpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------- */

typedef U32 STFLASH_Handle_t;

/* STFLASH Release Version*/
/*should be 30 chars long.
This is precisely this long:".............................."*/
#define STFLASH_REVISION	"STFLASH-REL_3.1.0             "

/* Flash memory device types */
typedef enum STFLASH_DeviceType_e
{
    STFLASH_M28F411,
    STFLASH_M29W800T,
    STFLASH_M29W160BT,
    STFLASH_M29W640D,
    STFLASH_M29W320DT,
    STFLASH_AM29LV160D,   /* Works the same way as M29W800T */
    STFLASH_AT49BV162AT,  /* Works the same way as M29W800T */
    STFLASH_E28F640,      /* Works the same way as M28F411 */
    STFLASH_M28W320CB,
    STFLASH_M28W320FS,    /* KRYPTO Secured FLASH */
    STFLASH_M28W640FS,    /* KRYPTO Secured FLASH */
    STFLASH_M28W320FSU,   /* KRYPTO Secured FLASH of uniform block size */
    STFLASH_M28W640FSU,   /* KRYPTO Secured FLASH of uniform block size */
    STFLASH_M58LW032,
    STFLASH_M58LW064D,
    STFLASH_M58LT128GS,   /*Multibank architecture*/
    STFLASH_M58LT256GS,
    STFLASH_M25P10,       /* serial flash */
    STFLASH_M25P80,       /* serial flash */
    STFLASH_M25P16        /* serial flash */
} STFLASH_DeviceType_t;

/* "provisionally defined" Type of Block, as
    stored in STFLASH_Block_t.Type element    */
typedef enum STFLASH_BlockTypes_e
{
    STFLASH_EMPTY_BLOCK,
    STFLASH_BOOT_BLOCK,
    STFLASH_PARAMETER_BLOCK,
    STFLASH_MAIN_BLOCK
} STFLASH_BlockTypes_t;

/* define an array of the STFLASH_Block_t for the number of
   blocks that the device contains, i.e. for ST M28F410/1:
   128k Main * 3, 96k Main, 8k Parameter * 2, 16k Boot */
typedef struct STFLASH_Block_s
{
    U32                     Length;                 /* in bytes */
    STFLASH_BlockTypes_t    Type;                   /* Empty/Boot/Parameter/Main */
} STFLASH_Block_t;

typedef struct STFLASH_BankParams_s
{
    U32 BankNumber;
    U32 NoOfBlocksInBank;
} STFLASH_BankParams_t;

typedef struct STFLASH_InitParams_s
{
    STFLASH_DeviceType_t    DeviceType;
    U32                     *BaseAddress;           /* lowest address for bank */
    U32                     *VppAddress;            /* programming voltage address */
    U32                     MinAccessWidth;         /* usually 1 byte  */
    U32                     MaxAccessWidth;         /* usually 4 bytes */
    U32                     NumberOfBlocks;
    STFLASH_Block_t         *Blocks;
    ST_Partition_t          *DriverPartition;
#if defined(STFLASH_MULTIBANK_SUPPORT)
    U32                     NumberOfBanks;
    STFLASH_BankParams_t    *BankInfo;
#endif
#if defined(STFLASH_SPI_SUPPORT)
    BOOL                    IsSerialFlash;
    ST_DeviceName_t         SPIDeviceName;
    STPIO_PIOBit_t          PIOforCS;
#endif
} STFLASH_InitParams_t;

typedef struct STFLASH_Params_s
{
    STFLASH_InitParams_t InitParams;
    U32                  DeviceCode;
    U32                  ManufactCode;
} STFLASH_Params_t;

#if defined(STFLASH_MULTIBANK_SUPPORT)
typedef struct STFLASH_OpenParams_s
{
    U32   BankNumber;
}STFLASH_OpenParams_t;
#else
typedef void *STFLASH_OpenParams_t;
#endif

typedef struct STFLASH_TermParams_s
{
    BOOL ForceTerminate;
} STFLASH_TermParams_t;

#if defined(STFLASH_SPI_SUPPORT)
typedef struct STFLASH_SPIConfig_s
{
    U8    PartNum;      /* 3:0 */
    U16   CSHighTime;   /* 15:4 */
    U8    CSHoldTime;   /* 23:16  */
    U8    DataHoldTime; /* 31:24 */
} STFLASH_SPIConfig_t;
typedef struct STFLASH_SPIModeSelect_s
{
    BOOL  IsContigMode;   /*bit 0*/
    BOOL  IsFastRead;     /* bit 1*/
} STFLASH_SPIModeSelect_t;
#endif

/*
 * This structure holds all CFI Query information as defined
 * in the JEDEC standard.
 */
typedef struct STFLASH_Voltage_s
{
    	U16 Volts;
    	U16 MilliVolts;
}STFLASH_Voltage_t; 

typedef struct STFLASH_CFI_Query_s
{
    /* CFI Query Address and Data Output */
    char Query_String[4];	   	   /* Should be 'QRY' */
    U8 DeviceCode;
    U8 ManufacturerCode;
    U32 Primary_Vendor_Cmd_Set;    /* Command set */
    U32 Primary_Table_Address;     /* Address of extended table */
    U32 Alt_Vendor_Cmd_Set;        /* Alternate table */
    U32 Alt_Table_Address;         /* Alternate table address */

    /* Device Voltage and Timing Specification */
    STFLASH_Voltage_t Vdd_Min;     /* Vdd minimum */
    STFLASH_Voltage_t Vdd_Max;	   /* Vdd maximum */
    STFLASH_Voltage_t Vpp_Min;     /* Vpp minimum, if supported */
    STFLASH_Voltage_t Vpp_Max;     /* Vpp maximum, if supported */

    U32 Timeout_Single_Write;      /* Time of single write */
    U32 Timeout_Buffer_Write;	   /* Time of buffer write */
    U32 Timeout_Block_Erase;	   /* Time of sector erase */
    U32 Timeout_Chip_Erase;	   	   /* Time of chip erase */
    U32 Max_Timeout_Single_Write;  /* Max time of single write */
    U32 Max_Timeout_Buffer_Write;  /* Max time of buffer write */
    U32 Max_Timeout_Block_Erase;   /* Max time of sector erase */
    U32 Max_Timeout_Chip_Erase;    /* Max time of chip erase */

    /* Device Geometry Definition */
    long Device_Size;		   		/* Device size in bytes */
    U32 Interface_Description;	    /* Interface description */
    U32 Max_Multi_Byte_Write;	    /* Time of multi-byte write */
    U32 Num_Erase_Blocks;           /* Number of sector defs. */
    struct 
    {
		unsigned long Sector_Size;  /* Byte size of sector */
		U32 Num_Sectors;            /* Num sectors of this size */
    } Erase_Block[8];               /* Max of 256, but 8 is good */

    /* Extended Query Information */
    
    char  Primary_Extended_Query[4];	 /* Vendor specific info here */
    U8    Major_Version;         	     /* Major code version */
    U8    Minor_Version;           		 /* Minor code version */

    U8   Optional_Feature;
    BOOL Is_Chip_Erase;
    BOOL Is_Erase_Suspend;
    BOOL Is_Program_Suspend;
    BOOL Is_Locking;
    BOOL Is_Queue_Erase;
    BOOL Is_Instant_Block_Lock;
    BOOL Is_Protection_Bits;
    BOOL Is_Page_Read;


    U8    Burst_Read;               /* Is burst read supported */
    U8    Erase_Suspend;	    	/* Capable of erase suspend? */
    U8    Block_Protect;	    	/* Can Sector protect? */
    U8    Vdd_Optimum;              /* Can we temporarily unprotect? */
    U8    Vpp_Optimum;	            /* Scheme of unprotection */
    U8    OTP_Protection;	    	/* Scheme of unprotection */
    U8    Protection_Reg_LSB;	    /* Scheme of unprotection */
    U8    Protection_Reg_MSB;	    /* Scheme of unprotection */
    U8    Factory_Prg_Bytes;        /* Is a burst mode part? */
    U8    User_Prg_Bytes;           /* Is a page mode part? */
    U8    Page_Read;                /* Is a burst mode part? */
    U8    Synchronous_Config;       /* Is a page mode part? */

}STFLASH_CFI_Query_t;

/* Exported Constants ----------------------------------------------- */

#define STFLASH_DRIVER_ID          4   /* from DVD-API-004 */

#define STFLASH_DRIVER_BASE        		 ( STFLASH_DRIVER_ID << 16 )

#define STFLASH_ERROR_WRITE       		   STFLASH_DRIVER_BASE
#define STFLASH_ERROR_ERASE       		 ( STFLASH_DRIVER_BASE + 1 )
#define STFLASH_ERROR_VPP_LOW            ( STFLASH_DRIVER_BASE + 2 )
#define STFLASH_ERROR_DEVICE_NOT_PRESENT ( STFLASH_DRIVER_BASE + 3 )


#define STFLASH_ACCESS_08_BITS     1
#define STFLASH_ACCESS_16_BITS     2
#define STFLASH_ACCESS_32_BITS     4


/* Exported Variables ----------------------------------------------- */

/* Exported Macros -------------------------------------------------- */

/* Exported Functions ----------------------------------------------- */

ST_Revision_t  STFLASH_GetRevision( void );

ST_ErrorCode_t STFLASH_Init( const ST_DeviceName_t      Name,           /* Flash bank name/number */
                             const STFLASH_InitParams_t *InitParams );

ST_ErrorCode_t STFLASH_Open( const ST_DeviceName_t      Name,           /* Flash bank name/number */
                             const STFLASH_OpenParams_t *OpenParams,    /* currently empty */
                             STFLASH_Handle_t           *Handle );      /* returned Handle */

ST_ErrorCode_t STFLASH_GetCFI( STFLASH_Handle_t     Handle, 
							   STFLASH_CFI_Query_t  *CFI );

ST_ErrorCode_t STFLASH_GetParams( STFLASH_Handle_t Handle,              /* Flash bank identifier */
                                  STFLASH_Params_t *GetParams );        /* pointer to structure */


ST_ErrorCode_t STFLASH_Read( STFLASH_Handle_t Handle,                   /* Flash bank identifier */
                             U32              Address,                  /* offset from BaseAddress */
                             U8               *Buffer,                  /* base of receipt buffer */
                             U32              NumberToRead,             /* in bytes */
                             U32              *NumberActuallyRead );

ST_ErrorCode_t STFLASH_Write( STFLASH_Handle_t Handle,                  /* Flash bank identifier */
                              U32              Address,                 /* offset from BaseAddress */
                              U8               *Buffer,                 /* base of source buffer */
                              U32              NumberToWrite,           /* in bytes */
                              U32              *NumberActuallyWritten );

ST_ErrorCode_t STFLASH_Erase( STFLASH_Handle_t Handle,                  /* Flash bank identifier */
                              U32              Address,                 /* offset from BaseAddress */
                              U32              NumberToErase );

ST_ErrorCode_t STFLASH_BlockLock( STFLASH_Handle_t Handle,    /* Flash bank identifier */
                                  U32              Offset ); /* offset from BaseAddress */

ST_ErrorCode_t STFLASH_BlockUnlock( STFLASH_Handle_t Handle,     /* Flash bank identifier */
                                    U32              Offset );  /* offset from BaseAddress */

ST_ErrorCode_t STFLASH_Close( STFLASH_Handle_t Handle );                /* Flash bank identifier */

ST_ErrorCode_t STFLASH_Term( const ST_DeviceName_t Name,                /* Flash bank name/number */
                             const STFLASH_TermParams_t *TermParams );       /* ForceTerminate */

#if defined(STFLASH_SPI_SUPPORT)
ST_ErrorCode_t STFLASH_SetSPIConfig( STFLASH_Handle_t Handle,          /* Flash bank identifier */
                                     STFLASH_SPIConfig_t *SPIConfig );  /* SPI config data */

ST_ErrorCode_t STFLASH_SetSPIClkDiv( STFLASH_Handle_t Handle,
                                     U8               Divisor ) ; /* Clock Divisor */


ST_ErrorCode_t STFLASH_SetSPIModeSelect( STFLASH_Handle_t Handle,
                                         STFLASH_SPIModeSelect_t  *SPIMode ) ; /* Mode select */

#endif/*STFLASH_SPI_SUPPORT*/

#ifdef __cplusplus
}
#endif

#endif   /* #ifndef __STFLASH_H */

/* End of stflash.h */

