/****************************************************************************

File Name   : stflasht.c

Description : Flash Memory API Test Routines

Copyright (C) 2000, ST Microelectronics

History     :

    11/07/03  Restructured test harness for clarity,added Testtool support.
              Added 5528 support.

    27/01/04  Added 5100 support.

    19/07/04  Added 7710 support.
              Ported to OS21
              
    09/12/04  Added 5105 support.

References  :

$ClearCase (VOB: stflash)

****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include "stdevice.h"
#include "stack.h"
#include "stlite.h"
#ifndef ST_OS21
#include "ostime.h"
#endif
#include "stddefs.h"
#include "stflash.h"
#include "stboot.h"
#include "sttbx.h"
#include "stcommon.h"

U8 WriteBuf[128*1024] = {0};

/* Private defines ---------------------------------------------------------- */


/* Private Types ------------------------------------------------------ */

typedef struct {
    U8  Bank;           /* Bank number */
    U32 Address;        /* Block start address */
    U32 Length;         /* Length of block */
} BlockInfo_t;

typedef struct {
    U8  Block;          /* Block number */
    U32 Address;        /* flash address */
    U32 Offset;         /* offset into readbuffer */
    U32 Size;           /* number of bytes */
} SectionInfo_t;

/* Private Constants -------------------------------------------------- */

/* Definitions for Flash */

#define STFLASH_BANK_0_BASE     FLASH_BANK_0_BASE_ADDRESS
#define STFLASH_MAIN_0_SIZE     STFLASH_MAIN_SIZE
#define STFLASH_MAIN_1_SIZE     STFLASH_MAIN_SIZE
#if defined(mb376)
#define STFLASH_VPP_0_ENABLE    0x42300000
#define STFLASH_VPP_1_ENABLE    0x42300000
#elif defined(mb390)
#define STFLASH_VPP_0_ENABLE    0x41400000
#define STFLASH_VPP_1_ENABLE    0
#elif defined(mb400)
#define STFLASH_VPP_0_ENABLE    0x45200000
#endif

#define BANK0   0

#define DEVICE_TYPE             STFLASH_M58LW064D

#if defined(mb376)
#define MIN_ACCESS_WIDTH        STFLASH_ACCESS_32_BITS
#define MAX_ACCESS_WIDTH        STFLASH_ACCESS_32_BITS
#elif defined(mb390) || defined(mb400) || defined(mb391)
#define MIN_ACCESS_WIDTH        STFLASH_ACCESS_16_BITS
#define MAX_ACCESS_WIDTH        STFLASH_ACCESS_16_BITS
#endif

#define NUM_BLOCKS              64
#define STFLASH_MAIN_0_SIZE     STFLASH_MAIN_SIZE
#define STFLASH_MAIN_1_SIZE     STFLASH_MAIN_SIZE
#define NUM_BANKS               1
#define NUM_SECTIONS            40
#define TOTAL_BLOCKS            (NUM_BANKS * NUM_BLOCKS)

#define CR                      13
#define LF                      10

/* Private Variables -------------------------------------------------- */

STFLASH_Block_t      BlockData_s[NUM_BLOCKS] =
    { { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK },
      { STFLASH_MAIN_SIZE, STFLASH_MAIN_BLOCK }};

static U32              FlashSize = 0;          /* Non-zero = file converted */
static char             *FlashData_p = NULL;
static SectionInfo_t    SectionInfo[NUM_SECTIONS];
static BlockInfo_t      BlockInfo[TOTAL_BLOCKS + 1];
static U32              BaseAddress[NUM_BANKS];
static STFLASH_Block_t  GetBlkDat_s[NUM_BLOCKS];

/* Private Macros ----------------------------------------------------- */

/* Global Variables ------------------------------------------------------- */

STFLASH_Handle_t        FLASHHndl[2];
ST_DeviceName_t         FLASHDeviceName[2] = {"Bank0", "Bank1"};
static int BootFromFlash( void );
void ErrorReport( int *ErrorCount,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr );
                  
void ParamsReport( STFLASH_Params_t *RetParams );

/* Functions -------------------------------------------------------------- */
/* Declarations for memory partitions */
#ifndef ST_OS21

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            0x100000

/* Memory partitions */
#define TEST_PARTITION_1        &the_system_partition
#define TEST_PARTITION_2        &the_internal_partition

/* Declarations for memory partitions */
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( internal_block, "internal_section")
#endif

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( system_block,   "system_section")
#endif

/* This is to avoid a linker warning */
static unsigned char    internal_block_noinit[1];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( internal_block_noinit, "internal_section_noinit")
#endif

static unsigned char    system_block_noinit[1];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( system_block_noinit, "system_section_noinit")
#endif

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5528)
static unsigned char    data_section[1];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( data_section, "data_section")
#endif
#endif

#else /* ST_OS21 */

#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;

#endif /* ST_OS21 */


static ST_ErrorCode_t       retVal;
STFLASH_InitParams_t    InitParams_s;
STFLASH_OpenParams_t    OpenParams_s;
STFLASH_Params_t        GetParams_s;
STFLASH_TermParams_t    TermParams_s;

/****************************************************************************
Name         : main()

Description  : Calls the specific test functions

Parameters   : none

Return Value : int

See Also     : OS20_main
 ****************************************************************************/
int main(int argc, char *argv[])
{
    ST_ErrorCode_t Error  = ST_NO_ERROR;

    STTBX_InitParams_t      sttbx_InitPars;
    STBOOT_InitParams_t     stboot_InitPars;

#ifndef DISABLE_DCACHE

#if defined CACHEABLE_BASE_ADDRESS

    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS},
        { NULL, NULL }
    };

#elif defined(ST_5514) || defined(ST_5516)

    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40200000, (U32 *)0x407FFFFF }, /* ok */
        { NULL, NULL }
    };

#elif defined(ST_5528) || defined(ST_5100)

    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40000000, (U32 *)0x5FFFFFFF}, /* ok */
        { NULL, NULL }
    };

#else

    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40080000, (U32 *)0x7FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#endif

#endif /* end of !defined DISABLE_DCACHE */

#ifdef ARCHITECTURE_ST20
    /* to avoid linker warnings */
    internal_block_noinit[0] = 0;
    system_block_noinit[0] = 0;

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5528)
    data_section[0] = 0;
#endif
#endif

#ifdef ST_OS21

    /* Create memory partitions */
    system_partition = partition_create_heap((U8*)external_block, sizeof(external_block));

#else /* ifndef ST_OS21 */
    partition_init_heap (&the_internal_partition, internal_block,
                         sizeof(internal_block));
    partition_init_heap (&the_system_partition,   system_block,
                         sizeof(system_block));
#endif /* ST_OS21 */
    
    /* Initialize the toolbox */
#ifdef ST_OS21
    sttbx_InitPars.CPUPartition_p      = system_partition;
#else
    sttbx_InitPars.CPUPartition_p      = TEST_PARTITION_1;
#endif
    sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
    sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
    sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;
    
    /* Initialize stboot */
#ifdef DISABLE_ICACHE
    stboot_InitPars.ICacheEnabled             = FALSE;
#else
    stboot_InitPars.ICacheEnabled             = TRUE;
#endif

    stboot_InitPars.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    stboot_InitPars.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitPars.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitPars.CacheBaseAddress          = (U32*) CACHE_BASE_ADDRESS;

#ifdef DISABLE_DCACHE
    stboot_InitPars.DCacheMap                 = NULL;
#else
    stboot_InitPars.DCacheMap                 = DCacheMap;
#endif

    stboot_InitPars.MemorySize                = (STBOOT_DramMemorySize_t)SDRAM_SIZE;
    stboot_InitPars.SDRAMFrequency            = SDRAM_FREQUENCY;
   
    printf ("\nStarting BOOT ...\n");
    retVal = STBOOT_Init( "FLASHTest", &stboot_InitPars );
    if (retVal != ST_NO_ERROR)
    {
        printf( "ERROR: STBOOT_Init() returned %d\n", retVal );
    }
    else
    {         
    	printf ("\nStarting TBX ...\n");
        retVal = STTBX_Init( "tbx", &sttbx_InitPars );
        if (retVal != ST_NO_ERROR)
        {
            printf( "ERROR: STTBX_Init() returned %d\n", retVal );
        }
        else              
        {  
                	              
            STFLASH_Print(("============================================================\n"));
            STFLASH_Print(("            STFLASH Test Harness - BOOT FROM FLASH          \n"));
            STFLASH_Print(("            Driver Revision: %s\n", STFLASH_GetRevision()));
            STFLASH_Print(( "============================================================\n" ));

            Error = BootFromFlash();
            
            STFLASH_Print(( "Overall Test result is: %u",Error ));
            STFLASH_Print(( "\n============================================================\n" ));
        }
    }/* boot_init */
}

/*-------------------------------------------------------------------------
 * Function : GetBlockNumber
 *            Get block number from address
 * Input    :
 * Output   :
 * Return   : Block number of address
 * ----------------------------------------------------------------------*/
static U8 GetBlockNumber( U32 Address )
{
    U8  Block;

    /* find the block this address is in */
    for ( Block = 0; Block < TOTAL_BLOCKS; Block++ )
    {
        if (( Address >= BlockInfo[Block].Address ) &&
            ( Address < (BlockInfo[Block].Address+BlockInfo[Block].Length)))
        {
            break;
        }
    }
    return Block;

} /* end GetBlockNumber */
/*-------------------------------------------------------------------------
 * Function : ConvertMemory
 *            Convert Raw Hex file into memory buffer suitable to write to
 *            flash copying created data over data read from filem, the
 *            created data will be smaller as each byte takes three bytes
 *            in the Hex file.
 * Input    :
 * Output   :
 * Return   : size of data in buffer
 * ----------------------------------------------------------------------*/
static U32 ConvertMemory( long unsigned int MemSize )
{
    char                *OldPtr, *NewPtr;
    int                 Section;
    U32                 InOffset;
    U32                 OutOffset;
    U32                 BlockOffset;
    U32                 Value=0;
    U8                  Count=0 , Block=0;
    
    for ( Section = 0; Section < NUM_SECTIONS; Section ++ )
    {
        SectionInfo[Section].Block = 0xff;
        SectionInfo[Section].Address = 0;
        SectionInfo[Section].Offset = 0;
        SectionInfo[Section].Size = 0;
    }

    /* Convert File */
    Section = 0;
    OldPtr = FlashData_p;
    NewPtr = (char*) NULL;
    
    for ( InOffset = 0, OutOffset = 0; InOffset < MemSize; )
    {
        /* Line format =
        ** Address  Data  (both in hex)
        ** 12345678 11 22 33 44 55 etc. CR/LF
        */
        Value = (U32) strtol( OldPtr, &NewPtr, 16 );
        if ( NewPtr != OldPtr ) /* if conversion possible */
        {
            if ( Value > 255 )  /* which means it is an address */
            {
                if (( Block = GetBlockNumber( Value )) == TOTAL_BLOCKS )
                {
                    STFLASH_Print(("Invalid address 0x%x\n", Value ));
                    break;
                }
                else
                {
                    /*
                    ** check if the address is non-contiguous
                    ** or if it is a new block
                    */
                    if ((( SectionInfo[Section].Address + SectionInfo[Section].Size ) != Value ) ||
                        ( SectionInfo[Section].Block != Block ))
                    {
                        Section ++;
                    }

                    /* Start New Block */
                    if ( SectionInfo[Section].Block == 0xff )
                    {
                        SectionInfo[Section].Address = Value;
                        SectionInfo[Section].Block = Block;
                        SectionInfo[Section].Offset = OutOffset;
                        SectionInfo[Section].Size = 0;
                    }
                }
            }
            else
            {
                BlockOffset = OutOffset;
                for ( Count = 0; Count < Section; Count++ )
                    BlockOffset -= SectionInfo[Count].Size;

                if (( BlockOffset+1 ) > BlockInfo[SectionInfo[Section].Block].Length )
                {
                    Section++;
                }

                /* Start New Block */
                if ( SectionInfo[Section].Block == 0xff )
                {
                    SectionInfo[Section].Address = BlockInfo[Block].Address + BlockOffset;
                    SectionInfo[Section].Block = GetBlockNumber(SectionInfo[Section].Address);
                    SectionInfo[Section].Offset = OutOffset;
                    SectionInfo[Section].Size = 0;
                }

                SectionInfo[Section].Size ++;
                FlashData_p[OutOffset++] = (char) Value;
             }

            InOffset += (U32)( NewPtr - OldPtr );
            OldPtr = NewPtr;

            if ( Section > NUM_SECTIONS )
            {
                STFLASH_Print(("Program Limitation\n"));
                exit(0);
            }

       }
        else
            break;
    } /* end of for loop */

    STFLASH_Print(("\n"));

    /* display the first few bytes of each section */
    for ( Section = 0; Section < NUM_SECTIONS; Section ++ )
    {
        if ( SectionInfo[Section].Block != 0xFF )
        {
            STFLASH_Print(("Section %2d, Block %3d, Addr 0x%08x, Offset 0x%08x, Size %6d\n",
                         Section,
                         SectionInfo[Section].Block,
                         SectionInfo[Section].Address,
                         SectionInfo[Section].Offset,
                         SectionInfo[Section].Size ));

        }
    }
    return ( OutOffset );

} /* end ConvertMemory */

/*-------------------------------------------------------------------------
 * Function : EraseFlash
 *            Function to Read Hex file and erase areas required
 * Input    : EraseAll 
 * Output   :
 * Return   : EraseFailed = TRUE else FALSE
 * ----------------------------------------------------------------------*/
BOOL EraseFlash( BOOL EraseAll )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    U8   Section, Bank;
    U32  EraseOffset;
    BOOL EraseFailed = TRUE;

    /* check data size */
    if (( FlashSize != 0 ) || EraseAll )
    {
        EraseFailed = FALSE;
        STFLASH_Print(("\t\t+------------+------------+\n"));

        if ( EraseAll )
        {
            for ( Section = 0;( Section < TOTAL_BLOCKS ) && ( ErrCode == ST_NO_ERROR ); Section ++)
            {
                Bank = BlockInfo[Section].Bank;
                EraseOffset = BlockInfo[Section].Address - BaseAddress[Bank];
                STFLASH_Print(("%d block %2d\t| 0x%08x | 0x%08x | ",
                             Bank, Section,
                             BlockInfo[Section].Address,
                             BlockInfo[Section].Address + BlockInfo[Section].Length - 1 ));

                ErrCode = STFLASH_Erase( FLASHHndl[Bank],
                                         EraseOffset,
                                         BlockInfo[Section].Length );
                if ( ErrCode != ST_NO_ERROR )
                {
                    STFLASH_Print(("Erase FAILED\n"));
                    EraseFailed = TRUE;
                }
                else
                    STFLASH_Print(("ERASED\n"));
            }
        }
        else
        {
            /* Find out which blocks are to be Erased */

            for ( Section = 0; ( Section < NUM_SECTIONS ) && ( EraseFailed == FALSE ); Section ++ )
            {
                if ( SectionInfo[Section].Block != 0xFF )
                {
                    /*
                    ** Erase = erase whole block, valid parameters are block base address & block size
                    */
                    Bank = BlockInfo[ SectionInfo[Section].Block ].Bank;
                    EraseOffset = BlockInfo[ SectionInfo[Section].Block ].Address - BaseAddress[Bank];

                    STFLASH_Print(("block %2d\t| 0x%08x | 0x%08x | ",
                                 SectionInfo[Section].Block,
                                 BlockInfo[ SectionInfo[Section].Block ].Address,
                                 BlockInfo[ SectionInfo[Section].Block ].Address +
                                 BlockInfo[ SectionInfo[Section].Block ].Length - 1 ));
                                 
                    ErrCode = STFLASH_Erase( FLASHHndl[Bank],
                                             EraseOffset,
                                             BlockInfo[ SectionInfo[Section].Block ].Length );

                    if ( ErrCode != ST_NO_ERROR )
                    {
                        STFLASH_Print(("ErrCode = 0x%8x\n",ErrCode));
                        STFLASH_Print(("Erase FAILED\n"));
                        STFLASH_Print(("FLASHHndl[Bank] = 0x%8X\n",FLASHHndl[Bank]));
                        EraseFailed = TRUE;
                    }
                    else
                        STFLASH_Print(("ERASED\n"));
                }
            }
        }
        STFLASH_Print(("\t\t+------------+------------+\n"));
    }

    if ( EraseFailed == TRUE )
    {
        if ( FlashData_p != NULL )
        {
            memory_deallocate( system_partition, FlashData_p );
        }
        STFLASH_Print(("!!!! ERASE Failed !!!!\n"));
        return TRUE;
    }

    return EraseFailed;

} /* end EraseFlash */

/*-------------------------------------------------------------------------
 * Function : ProgramFlash
 *            Function to Read Hex file and program flash with contents
 * Input    :
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/

BOOL ProgramFlash( void )
{
    ST_ErrorCode_t      ErrCode;
    int                 Section, Bank;
    U32                 NumberOfBytes;
    BOOL                ProgramFailed;

    /* check data size */
    if ( FlashSize == 0 )
    {
        ProgramFailed = TRUE;
    }
    else
    {
        ProgramFailed = FALSE;
        STFLASH_Print(("\t\t+------------+------------+\n"));
    }

    for ( Section = 0; ( Section < NUM_SECTIONS ) && ( ProgramFailed == FALSE ); Section ++ )
    {
        if ( SectionInfo[Section].Block != 0xFF )
        {
            U32  WriteOffset;

            /*
            ** Write = write bytes from Offset
            */
            Bank = BlockInfo[ SectionInfo[Section].Block ].Bank;
            STFLASH_Print(("Section=%d\t", Section ));
            STFLASH_Print(("SectionInfo[Section]=%d\n", SectionInfo[Section] ));
            STFLASH_Print(("SectionInfo[Section].Block=%d\t", SectionInfo[Section].Block ));
            STFLASH_Print(("BlockInfo[ SectionInfo[Section].Block ].Bank=%d\n",BlockInfo[ SectionInfo[Section].Block ].Bank));
            WriteOffset = SectionInfo[Section].Address - BaseAddress[Bank];
            STFLASH_Print(("block %2d\t| 0x%08x | 0x%08x | ",
                         SectionInfo[Section].Block,
                         SectionInfo[Section].Address,
                         SectionInfo[Section].Address + SectionInfo[Section].Size - 1 ));

            ErrCode = STFLASH_Write( FLASHHndl[Bank],
                                     WriteOffset,
                                     (U8*)&FlashData_p[SectionInfo[Section].Offset],
                                     SectionInfo[Section].Size,
                                     &NumberOfBytes ); 
                                     

            if ( ErrCode != ST_NO_ERROR )
            {
                STFLASH_Print(("Program FAILED\n"));
                ProgramFailed = TRUE;
            }
            else
            {
                STFLASH_Print(("PROGRAMMED\n"));
            }
        }
    }
    
    if ( ProgramFailed == TRUE )
    {
        if ( FlashData_p != NULL )
        {
            memory_deallocate( system_partition, FlashData_p );
        }
        STFLASH_Print(("!!!! PROGRAM Failed !!!!\n"));
        return TRUE;
    }
    else
    {
        STFLASH_Print(("\t\t+------------+------------+\n"));
        return FALSE;
    }

} /* end ProgramFlash */


/*-------------------------------------------------------------------------
 * Function : VerifyFlash
 *            Function to Read Hex file and verify flash with contents
 * Input    :
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
BOOL VerifyFlash( void )
{
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    int                 Section, Bank ;
    U32                 Length;
    U32                 ReadOffset;
    U32                 NumberOfBytes;
    U8                  *Buffer_p;
    BOOL                VerifyFailed;
    int                 ErrorCount  = 0;
    
    /* check data size */
    if ( FlashSize == 0 )
    {
        VerifyFailed = TRUE;
    }
    else
    {
        VerifyFailed = FALSE;
        STFLASH_Print(("\t\t+------------+------------+\n"));
    }

    /* Find out which blocks are to be verified */
    /* Read the data and compare */

    for ( Section = 0; ( Section < NUM_SECTIONS ) && ( VerifyFailed == FALSE ); Section ++ )
    {
        if ( SectionInfo[Section].Block != 0xFF )
        {
            Bank = BlockInfo[ SectionInfo[Section].Block ].Bank;
            Length = BlockInfo[ SectionInfo[Section].Block ].Length;
            ReadOffset = SectionInfo[Section].Address - BaseAddress[Bank];

            /* allocate temp buffer to put flash data in */
            Buffer_p = memory_allocate( system_partition, Length );
            if ( Buffer_p != NULL )
            {
                STFLASH_Print(("block %2d\t| 0x%08x | 0x%08x | ",
                         SectionInfo[Section].Block,
                         SectionInfo[Section].Address,
                         SectionInfo[Section].Address + SectionInfo[Section].Size - 1 ));

                ErrCode = STFLASH_Read( FLASHHndl[Bank],
                                        ReadOffset,
                                        Buffer_p,
                                        SectionInfo[Section].Size,
                                        &NumberOfBytes );
                                        
                if (ErrCode != ST_NO_ERROR)
                {
                    ErrorReport(&ErrorCount, ErrCode, ST_NO_ERROR);
                }

               if ( memcmp( Buffer_p,(U8*)&FlashData_p[SectionInfo[Section].Offset],
                             SectionInfo[Section].Size ) != 0 )
                {
                    STFLASH_Print(("Verify FAILED\n"));
                    VerifyFailed = TRUE;
                }
                else
                {
                    STFLASH_Print(("VERIFIED\n"));
                }
               
                /* deallocate temp buffer */
                memory_deallocate( system_partition, Buffer_p );
            }
        }
    }

    if ( FlashData_p != NULL )
    {
        memory_deallocate( system_partition, FlashData_p );
    }
    
    if ( VerifyFailed == TRUE )
    {
        STFLASH_Print(("!!!! VERIFY Failed !!!!\n"));
        return TRUE;
    }
    else
    {
        STFLASH_Print(("\t\t+------------+------------+\n"));
        return FALSE;
    }

} /* end VerifyFlash */


/****************************************************************************
Name         : BootFromFlash()

Description  : 
               Note that no errors should be raised during this test.
               Else the first error encountered will be returned.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_ALREADY_INITIALIZED More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE      Open call w/o prior Init
               ST_ERROR_INVALID_HANDLE      Rogue Flash bank reference
               ST_ERROR_BAD_PARAMETER       One or more invalid parameters
               ST_ERROR_TIMEOUT             Timeout error during Erase/Write
               STFLASH_ERROR_WRITE          Error during programming sequence
               STFLASH_ERROR_ERASE          Error during erasure sequence
               STFLASH_ERROR_VPP_LOW        Vpp voltage low during either of
                                            Write or Erase sequences

See Also     : STFLASH_ErrorCode_t
 ****************************************************************************/

int BootFromFlash( void )
{
 
    long int    HexFile_p   = 0;            /* datafile descriptor          */
    long int    HexFileSize = 0;          /* size in bytes of the file    */
    int         ErrorCount  = 0;
    ST_ErrorCode_t ReturnError = ST_NO_ERROR;
    char Filename[]         = "flash.hex";
    U8                  Block;
    U32                 BlockStart;
    
    STFLASH_Print(( "\n" ));
    STFLASH_Print(( "============================================================\n" ));
    STFLASH_Print(( "Commencing Boot From Flash Test  ..\n" ));
    STFLASH_Print(( "============================================================\n" ));

   
    /* Init Bank 0, Vpp 0 */
    InitParams_s.DeviceType      = DEVICE_TYPE;
    InitParams_s.BaseAddress     = (U32*)STFLASH_BANK_0_BASE;
    InitParams_s.VppAddress      = (U32*)STFLASH_VPP_0_ENABLE;
    InitParams_s.MinAccessWidth  = MIN_ACCESS_WIDTH;
    InitParams_s.MaxAccessWidth  = MAX_ACCESS_WIDTH;
    InitParams_s.NumberOfBlocks  = NUM_BLOCKS;
    InitParams_s.Blocks          = BlockData_s;
#ifdef ST_OS21
    InitParams_s.DriverPartition = system_partition;
#else
    InitParams_s.DriverPartition = TEST_PARTITION_1;
#endif

    STFLASH_Print(( "FLASH_BANK_0_BASE = %x\n", STFLASH_BANK_0_BASE ));

    STFLASH_Print(( "Calling STFLASH_Init() Bank0 ..........\n" ));
    ReturnError = STFLASH_Init( "Bank0", &InitParams_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    
    /* Open Bank 0 */
    STFLASH_Print(( "Calling STFLASH_Open() Bank0 ..........\n" ));
    ReturnError = STFLASH_Open( "Bank0",
                                &OpenParams_s,
                                &FLASHHndl[0] );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    
    /* GetParams for Bank 0 */
    GetParams_s.InitParams.Blocks = GetBlkDat_s;
    STFLASH_Print(( "Calling STFLASH_GetParams() Bank 0 ....\n" ));
    ReturnError = STFLASH_GetParams( FLASHHndl[0], &GetParams_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    ParamsReport( &GetParams_s );
    
    BlockStart  = BaseAddress[BANK0] = (U32) InitParams_s.BaseAddress;
    for ( Block = 0; Block < InitParams_s.NumberOfBlocks; Block ++)
    {
        BlockInfo[Block].Bank    = 0;
        BlockInfo[Block].Address = BlockStart;
        BlockInfo[Block].Length  = InitParams_s.Blocks[Block].Length;
        BlockStart += InitParams_s.Blocks[Block].Length;
    }

    /* Open and Read file into memory */
    HexFile_p = debugopen(Filename, "rb");
    STFLASH_Print(("HexFile_p = 0x%8x\n",HexFile_p));
    
    if (HexFile_p < 0)
    {
        STFLASH_Print(("Error opening file \'%s\'\n", Filename ));
        return (0);
    }
    else
    {
        HexFileSize = debugfilesize(HexFile_p);
        STFLASH_Print(("HexFileSize = 0x%8x\n",HexFileSize));
        
        /* allocate File data buffer */
        FlashData_p = (char*) memory_allocate( system_partition, (U32) HexFileSize );
        if ( FlashData_p != NULL )
        {
            STFLASH_Print(("Loading \'%s\' into memory, wait .. ", Filename ));
            debugread(HexFile_p, FlashData_p, (size_t) HexFileSize);
            STFLASH_Print(("%d bytes\n", HexFileSize ));
        }
        else
        {
            STFLASH_Print(("Not enough memory for HEX file (%d bytes)\n", HexFileSize));
            HexFileSize = 0;
        }

        debugclose(HexFile_p);
    }

    if ( HexFileSize > 0 )
    {
        /* convert buffer to binary and resize memory */
        STFLASH_Print(("Converting file in memory, wait .. "));
        FlashSize = ConvertMemory( HexFileSize );
        if ( FlashSize > 0 )
        {
            STFLASH_Print(("%d bytes\n", FlashSize ));
            FlashData_p = (char*) memory_reallocate( system_partition, FlashData_p, FlashSize );
        }
        else
        {
            STFLASH_Print(("Invalid file\n"));
        }
    }
    
    if ( EraseFlash(FALSE) == FALSE )
    {
        if ( ProgramFlash() == FALSE )
        {
            VerifyFlash();
        }
    }
    
    /* Close Bank 0 */
    STFLASH_Print(( "Calling STFLASH_Close() Bank 0 ........\n" ));
    ReturnError = STFLASH_Close( FLASHHndl[0] );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );


    /* Term Bank 0 */
    TermParams_s.ForceTerminate = FALSE;
    STFLASH_Print(( "Calling STFLASH_Term() Bank 0 .........\n" ));
    ReturnError = STFLASH_Term( "Bank0", &TermParams_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    
    return( ErrorCount );
    
}

/****************************************************************************
Name         : ErrorReport()

Description  : Expands the returned error code to a message string,
               followed by the expected code/message if different.

Parameters   : ST_ErrorCode_t pointer to an error store, the latest
               ST_ErrorCode_t error code returned, followed by the
               ST_ErrorCode_t expected error code.

Return Value : none

See Also     : STFLASH_ErrorCode_t
 ****************************************************************************/

void ErrorReport( int *ErrorCount,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr )
{
    ST_ErrorCode_t  Temp = ErrorGiven;

    switch (Temp)
    {
        case ST_NO_ERROR:
            STFLASH_Print(( "ST_NO_ERROR - Successful Return\n" ));
            break;

        case ST_ERROR_ALREADY_INITIALIZED:
            STFLASH_Print(( "ST_ERROR_ALREADY_INITIALIZED - Prior Init call w/o Term\n" ));
            break;

        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            STFLASH_Print(( "ST_ERROR_FEATURE_NOT_SUPPORTED - Device mismatch\n" ));
            break;

        case ST_ERROR_UNKNOWN_DEVICE:
            STFLASH_Print(( "ST_ERROR_UNKNOWN_DEVICE - Init call must precede access\n" ));
            break;

        case ST_ERROR_INVALID_HANDLE:
            STFLASH_Print(( "ST_ERROR_INVALID_HANDLE - Rogue Handle value or Closed\n" ));
            break;

        case ST_ERROR_OPEN_HANDLE:
            STFLASH_Print(( "ST_ERROR_OPEN_HANDLE - Term called with Open Handle\n" ));
            break;

        case ST_ERROR_BAD_PARAMETER:
            STFLASH_Print(( "ST_ERROR_BAD_PARAMETER - Parameter(s) out of valid range\n" ));
            break;

        case ST_ERROR_NO_FREE_HANDLES:
            STFLASH_Print(( "ST_ERROR_NO_FREE_HANDLES - DeviceName already Open\n" ));
            break;

        case ST_ERROR_TIMEOUT:
            STFLASH_Print(( "ST_ERROR_TIMEOUT - Timeout limit reached\n" ));
            break;

        case STFLASH_ERROR_WRITE:
            STFLASH_Print(( "STFLASH_ERROR_WRITE - Error during Flash Write\n" ));
            break;

        case STFLASH_ERROR_ERASE:
            STFLASH_Print(( "STFLASH_ERROR_ERASE - Error during Flash Erase\n" ));
            break;

        case STFLASH_ERROR_VPP_LOW:
            STFLASH_Print(( "STFLASH_ERROR_VPP_LOW - Flash Vpp Voltage too low\n" ));
            break;

        default:
            STFLASH_Print(( "*** Unrecognised return code %x ***\n" ));
        }

        if (ErrorGiven != ExpectedErr)
        {
            (*ErrorCount)++;
        }

    STFLASH_Print(( "----------------------------------------------------------\n" ));
}

/****************************************************************************
Name         : ParamsReport()

Description  : Outputs the GetParams_s values as text

Parameters   : STFLASH_GetParams_t pointer to returned Init data

Return Value : none

See Also     : STFLASH_Params_t
 ****************************************************************************/

void ParamsReport( STFLASH_Params_t *RetParams )
{
    U32 i;
    STFLASH_Block_t   *RetBlock_p;

    switch (RetParams->InitParams.DeviceType)
    {
        case STFLASH_M28F411:
            STFLASH_Print(( "DeviceType reported as STFLASH_M28F411\n" ));
            break;
        case STFLASH_M29W800T:
            STFLASH_Print(( "DeviceType reported as STFLASH_M29W800T\n" ));
            break;
        case STFLASH_M29W160BT:
            STFLASH_Print(( "DeviceType reported as STFLASH_M29W160BT\n" ));
            break;
        case STFLASH_AM29LV160D:          /* Works the same way as M29W800T */
            STFLASH_Print(( "DeviceType reported as STFLASH_AM29LV160D\n" ));
            break;
        case STFLASH_E28F640:              /* Works the same way as M28F411 */
            STFLASH_Print(( "DeviceType reported as STFLASH_E28F640\n" ));
            break;
        case STFLASH_M28W320CB:
            STFLASH_Print(( "DeviceType reported as STFLASH_M28W320CB\n" ));
            break;
        case STFLASH_M58LW032:
            STFLASH_Print(( "DeviceType reported as STFLASH_M58LW032\n" ));
            break;
        case STFLASH_M58LW064D:
            STFLASH_Print(( "DeviceType reported as STFLASH_M58LW064D\n" ));
            break;
        default:
            STFLASH_Print(( "Unknown DeviceType\n" ));
            break;
    }

    switch ((U32)RetParams->InitParams.BaseAddress)
    {
        case STFLASH_BANK_0_BASE:
            STFLASH_Print(( "BaseAddress reported as STFLASH_BANK_0_BASE\n" ));
            break;

#ifdef STFLASH_BANK_1_BASE
        case STFLASH_BANK_1_BASE:
            STFLASH_Print(( "BaseAddress reported as STFLASH_BANK_1_BASE\n" ));
            break;
#endif
        default:
        STFLASH_Print(( "BaseAddress incorrectly reported as 0x%8X\n",
                       (U32)RetParams->InitParams.BaseAddress ));
    }

    if ((U32)RetParams->InitParams.VppAddress == STFLASH_VPP_0_ENABLE)
    {
        STFLASH_Print(( "VppAddress reported as STFLASH_VPP_0_ENABLE\n" ));
    }
    else if ((U32)RetParams->InitParams.VppAddress == STFLASH_VPP_1_ENABLE)
    {
        STFLASH_Print(( "VppAddress reported as STFLASH_VPP_1_ENABLE\n" ));
    }
    else
    {
        STFLASH_Print(( "VppAddress unrecognized, reported as 0x%8X\n",
                      (U32)RetParams->InitParams.VppAddress ));
    }

    STFLASH_Print(( "MinAccessWidth reported as %d\n",
                   RetParams->InitParams.MinAccessWidth ));

    STFLASH_Print(( "MaxAccessWidth reported as %d\n",
                   RetParams->InitParams.MaxAccessWidth ));

    STFLASH_Print(( "Number of Blocks reported as %d\n",
                   RetParams->InitParams.NumberOfBlocks ));

    RetBlock_p = RetParams->InitParams.Blocks;

    for (i = 0; i < RetParams->InitParams.NumberOfBlocks; i++)
    {
        switch (RetBlock_p[i].Type)
        {
            case STFLASH_EMPTY_BLOCK:
                STFLASH_Print(( "Empty Block \t\t" ));
                break;

            case STFLASH_BOOT_BLOCK:
                STFLASH_Print(( "Boot Block \t\t" ));
                break;

            case STFLASH_PARAMETER_BLOCK:
                STFLASH_Print(( "Parameter Block \t" ));
                break;

            case STFLASH_MAIN_BLOCK:
                STFLASH_Print(( "Main Block \t\t" ));
                break;

            default:
                STFLASH_Print(( "Unknown Block \t\t" ));
        }
        STFLASH_Print(( "0x%08X bytes long\n", RetBlock_p[i].Length ));
    }

    STFLASH_Print(( "Manufacturer Code returned as 0x%08X\n", RetParams->ManufactCode ));
    STFLASH_Print(( "Device Code returned as 0x%08X\n", RetParams->DeviceCode ));

    STFLASH_Print(( "--------------------------------------------------------------\n" ));
}
