/*
  (c) Copyright changhong
  Name:CH_FlashMid.h
  Description:header file for flash initialize section for changhong STI7100 platform

  Authors:Yixiaoli
  Date          Remark
  2007-03-25    Created


*/

typedef U32 CHFLASH_Handle_t;

typedef enum CHFLASH_DeviceType_e
{
    FLASH_SPANSION_GL064AS2,
    FLASH_ST_M29,
	FLASH_ST_M28,
	FLASH_UNKNOWN
}CHFLASH_DeviceType_t;


typedef struct CHFLASH_InitParams_s
{
    U32                     *BaseAddress;           /* lowest address for bank */
    ST_Partition_t          *DriverPartition;
} CHFLASH_InitParams_t;

typedef struct CHFLASH_Block_s
{
    U32                     Length;                 /* in bytes */
} CHFLASH_Block_t;


extern BOOL CHFLASH_Init(const CHFLASH_InitParams_t *InitParams,
				  CHFLASH_Handle_t           *Handle);


extern BOOL CHFLASH_Erase(CHFLASH_Handle_t Handle,                  /* Flash bank identifier */
                   U32              Address,                 /* offset from BaseAddress */
                   U32              NumberToErase );  
extern BOOL CHFLASH_Read(CHFLASH_Handle_t Handle,                   /* Flash bank identifier */
                  U32              Address,                  /* offset from BaseAddress */
                  U8               *Buffer,                  /* base of receipt buffer */
                  U32              NumberToRead,             /* in bytes */
                  U32              *NumberActuallyRead );


extern BOOL CHFLASH_Write(CHFLASH_Handle_t Handle,                  /* Flash bank identifier */
                   U32              Address,                 /* offset from BaseAddress */
                   U8               *Buffer,                 /* base of source buffer */
                   U32              NumberToWrite,           /* in bytes */
                   U32              *NumberActuallyWritten );

/*end of file*/

