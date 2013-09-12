/*****************************************************************************

File name   : initterm.c

Description : Initialisation and Termination routines

COPYRIGHT (C) STMicroelectronics 1999.

Date          Modification                                    Initials
----          ------------                                    --------
18/6//99       Initial                                         APK
14/7/99        Ported to the ststill driver test               FS
23/7/99        Added STBOOT Initialization                     FS
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "stlite.h"    /* os20 for standard definitions,          */
#include "stddefs.h"   /* standard definitions                    */
#include "stdevice.h"

#include "stevt.h"
#include "stboot.h"
#ifndef STEVT_NO_TBX
#include "sttbx.h"
#endif
#include "stcommon.h"



#define GLOBAL_DATA
#include "app_data.h"
#include "evt_test.h"


ST_ErrorCode_t TBXInit( void );
ST_ErrorCode_t BOOTInit( void );


#if defined(ST_5508)
#define DVD_BACKEND_NAME "STi5508"
#elif defined(ST_5510)
#define DVD_BACKEND_NAME "STi5510"
#elif defined(ST_5512)
#define DVD_BACKEND_NAME "STi5512"
#elif defined(ST_5514)
#define DVD_BACKEND_NAME "STi5514"
#elif defined(ST_5516)
#define DVD_BACKEND_NAME "STi5516"
#elif defined(ST_5517)
#define DVD_BACKEND_NAME "STi5517"
#elif defined(ST_5518)
#define DVD_BACKEND_NAME "STi5518"
#elif defined(ST_5528)
#define DVD_BACKEND_NAME "STi5528"
#elif defined(ST_5100)
#define DVD_BACKEND_NAME "STi5100"
#elif defined(ST_7710)
#define DVD_BACKEND_NAME "STi7710"
#elif defined(ST_5105)
#define DVD_BACKEND_NAME "STi5105"
#elif defined(ST_5107)
#define DVD_BACKEND_NAME "STi5107"
#elif defined(ST_7015)
#define DVD_BACKEND_NAME "STi7015"
#elif defined(ST_5700)
#define DVD_BACKEND_NAME "STm5700"
#elif defined(ST_7100)
#define DVD_BACKEND_NAME "STb7100"
#elif defined(ST_7109)
#define DVD_BACKEND_NAME "STb7109"
#elif defined(ST_7200)
#define DVD_BACKEND_NAME "STb7200"
#elif defined(ST_5301)
#define DVD_BACKEND_NAME "STi5301"
#elif defined(ST_8010)
#define DVD_BACKEND_NAME "STi8010"
#elif defined(ST_GX1)
#define DVD_BACKEND_NAME "ST40GX1"
#elif defined(ST_5525)
#define DVD_BACKEND_NAME "STi5525"
#elif defined(ST_5188)
#define DVD_BACKEND_NAME "STi5188"
#else
#define DVD_BACKEND_NAME "UNRECOGNISED"
#endif

#if defined(ST_5508)
#define DVD_FRONTEND_NAME "STi5508"
#elif defined(ST_5510)
#define DVD_FRONTEND_NAME "STi5510"
#elif defined(ST_5512)
#define DVD_FRONTEND_NAME "STi5512"
#elif defined(ST_5514)
#define DVD_FRONTEND_NAME "STi5514"
#elif defined(ST_5516)
#define DVD_FRONTEND_NAME "STi5516"
#elif defined(ST_5517)
#define DVD_FRONTEND_NAME "STi5517"
#elif defined(ST_5518)
#define DVD_FRONTEND_NAME "STi5518"
#elif defined(ST_5528)
#define DVD_FRONTEND_NAME "STi5528"
#elif defined(ST_5100)
#define DVD_FRONTEND_NAME "STi5100"
#elif defined(ST_7710)
#define DVD_FRONTEND_NAME "STi7710"
#elif defined(ST_5105)
#define DVD_FRONTEND_NAME "STi5105"
#elif defined(ST_5107)
#define DVD_FRONTEND_NAME "STi5107"
#elif defined(ST_TP3)
#define DVD_FRONTEND_NAME "ST20TP3"
#elif defined(ST_GX1)
#define DVD_FRONTEND_NAME "ST40GX1"
#elif defined(ST_5700)
#define DVD_FRONTEND_NAME "STm5700"
#elif defined(ST_7100)
#define DVD_FRONTEND_NAME "STb7100"
#elif defined(ST_7109)
#define DVD_FRONTEND_NAME "STb7109"
#elif defined(ST_7200)
#define DVD_FRONTEND_NAME "STb7200"
#elif defined(ST_5301)
#define DVD_FRONTEND_NAME "STi5301"
#elif defined(ST_8010)
#define DVD_FRONTEND_NAME "STi8010"
#elif defined(ST_5525)
#define DVD_FRONTEND_NAME "STi5525"
#elif defined(ST_5188)
#define DVD_FRONTEND_NAME "STi5188"
#else
#define DVD_FRONTEND_NAME "UNRECOGNISED"
#endif

/* Board name */
#if defined (mb231)
#define DVD_PLATFORM_NAME "mb231"
#elif defined (mb282b)
#define DVD_PLATFORM_NAME "mb282b"
#elif defined (mb275)
#define DVD_PLATFORM_NAME "mb275"
#elif defined (mb275_64)
#define DVD_PLATFORM_NAME "mb275_64"
#elif defined (mb295)
#define DVD_PLATFORM_NAME "mb295"
#elif defined (mb314)
#define DVD_PLATFORM_NAME "mb314"
#elif defined (mb361)
#define DVD_PLATFORM_NAME "mb361"
#elif defined (mb382)
#define DVD_PLATFORM_NAME "mb382"
#elif defined (mb317a)
#define DVD_PLATFORM_NAME "mb317a"
#elif defined (mb317b)
#define DVD_PLATFORM_NAME "mb317b"
#elif defined (mb376)
#define DVD_PLATFORM_NAME "mb376"
#elif defined (mb390)
#define DVD_PLATFORM_NAME "mb390"
#elif defined (mb391)
#define DVD_PLATFORM_NAME "mb391"
#elif defined (mb400)
#define DVD_PLATFORM_NAME "mb400"
#elif defined (mb385)
#define DVD_PLATFORM_NAME "mb385"
#elif defined (mb411)
#define DVD_PLATFORM_NAME "mb411"
#elif defined (mb421)
#define DVD_PLATFORM_NAME "mb421"
#elif defined (mb428)
#define DVD_PLATFORM_NAME "mb428"
#elif defined (mb457)
#define DVD_PLATFORM_NAME "mb457"
#elif defined (mb436)
#define DVD_PLATFORM_NAME "mb436"
#else
#define DVD_PLATFORM_NAME "UNRECOGNISED"
#endif

#if !defined (ST_OS21) && !defined(ST_OSWINCE)


/* Allow room for OS20 segments in internal memory */
static U8               InternalBlock [ST20_INTERNAL_MEMORY_SIZE-1200];
static ST_Partition_t   the_internal_partition;
ST_Partition_t         *InternalPartition = &the_internal_partition;
ST_Partition_t         *internal_partition = &the_internal_partition;
#ifdef ARCHITECTURE_ST20
#if !defined(ST_5518) || !defined(UNIFIED_MEMORY) /* to Remove warnings*/
#pragma ST_section      ( InternalBlock, "internal_section_noinit")
#endif

#define NCACHE_MEMORY_SIZE          0x80000
/* small enough for 5508/18/10, and UNIFIED_MEMORY on 14/16/17 */
static U8               NcacheMemory [NCACHE_MEMORY_SIZE];
static ST_Partition_t   the_ncache_partition;
ST_Partition_t         *NcachePartition = &the_ncache_partition;
#pragma ST_section      ( NcacheMemory , "ncache_section" )
#endif    /* ARCHITECTURE_ST20 */

#define                 SYSTEM_MEMORY_SIZE          400000
static U8               ExternalBlock[SYSTEM_MEMORY_SIZE];
static ST_Partition_t   the_system_partition;
ST_Partition_t         *SystemPartition = &the_system_partition;
ST_Partition_t         *system_partition = &the_system_partition;
ST_Partition_t         *DriverPartition = &the_system_partition;
#ifdef ARCHITECTURE_ST20
#if !defined(ST_5518) || !defined(UNIFIED_MEMORY) /* to Remove warnings*/
#pragma ST_section      ( ExternalBlock, "system_section_noinit")
#endif
#endif
/* This is to avoid a linker warning */
static unsigned char    internal_block_init[1];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      (internal_block_init, "internal_section")
#endif
static unsigned char    system_block_init[1];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      (system_block_init, "system_section")
#endif

#if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined(ST_5107) || defined(ST_5700) \
|| defined(ST_7100) || defined(ST_5301) || defined(ST_5525) || defined(ST_5188) || defined(ST_7109)|| defined(ST_7200)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif

#ifdef UNIFIED_MEMORY
static unsigned char    ncache2_block[1];
#ifdef ARCHITECTURE_ST20
#if !defined(ST_5514) & !defined(ST_5518)
#pragma ST_section      (ncache2_block, "ncache2_section")
#endif
#endif
#endif

#else /* ST_OS21 || ST_OSWINCE */

#define                 SYSTEM_MEMORY_SIZE         0x800000
static U8               ExternalBlock[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;

#endif /* ST_OS21 || ST_OSWINCE */

/*=========================================================================
Name :       GlobalInit
Description: Global Init function
Parameters:  None
=========================================================================== */
ST_ErrorCode_t  GlobalInit( void )
{
    ST_ErrorCode_t      error= ST_NO_ERROR;

    /* Init BOOT */
    error = BOOTInit();
    if (error != ST_NO_ERROR)
    {
        printf("ERROR: STBOOT_Init = 0x%02X\n", error);
        return ( error );
    }

#if !defined(STEVT_NO_TBX)
    /* Init TBX */
    error = TBXInit();
    if (error != ST_NO_ERROR)
    {
        printf("ERROR: STTBX_Init = 0x%02X\n", error);
        return ( error );
    }
#endif

    GlobalGetRevision();

    return ( error );
}

/*=========================================================================
Name :       GlobalTerm
Description: Global Term function
Parameters:  None
=========================================================================== */
ST_ErrorCode_t  GlobalTerm( void )
{
    ST_ErrorCode_t          error= ST_NO_ERROR;
    STBOOT_TermParams_t     BOOTTermParams = { true };

    /* Driver Termination */

    error = STBOOT_Term (BOOTDeviceName, &BOOTTermParams);
    if (error != ST_NO_ERROR )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "STBOOT_Term = 0x%02X\n", error));
        return (error);
    }

    return ( error );
}

/*=========================================================================
Name :       GlobalGetRevision()
Description: Collect the revision strings of used components that allow GetRevision()
             This function has to be called only after the STTBX initialization.

Parameters:  None
=========================================================================== */
void  GlobalGetRevision( void )
{
    STEVT_Print(( "\n----- Chip and board used --------------------\n\n"));
    STEVT_Print(( "Board         : %s\n",(char *)DVD_PLATFORM_NAME));
    STEVT_Print(( "DVD_BACKEND   : %s\n",(char *)DVD_BACKEND_NAME));
    STEVT_Print(( "DVD_FRONTEND  : %s\n",(char *)DVD_FRONTEND_NAME));
#ifdef UNIFIED_MEMORY
    STEVT_Print(( "UNIFIED_MEMORY: TRUE\n"));
#else
    STEVT_Print(( "UNIFIED_MEMORY: FALSE\n"));
#endif
    STEVT_Print(( "\n----- Components revision strings ------------\n\n"));
    STEVT_Print(( "STBOOT        : %s\n",(char *)STBOOT_GetRevision()));
#if !defined(STEVT_NO_TBX)
    STEVT_Print(( "STTBX         : %s\n",(char *)STTBX_GetRevision()));
#endif

    STEVT_Print(( "STEVT         : %s\n",(char *)STEVT_GetRevision()));
    STEVT_Print(( "\n----------------------------------------------\n\n"));
    return ;
}

#if !defined(STEVT_NO_TBX)
/*=========================================================================
Name :       TBXInit
Description: TBX Init function
Parameters:  None
=========================================================================== */
ST_ErrorCode_t TBXInit( void )
{
    STTBX_InitParams_t  TBXInitParams;
    ST_ErrorCode_t      error= ST_NO_ERROR;

#if defined(ST_OS21)|| defined(ST_OSWINCE)
    TBXInitParams.CPUPartition_p = system_partition;
#else
    TBXInitParams.CPUPartition_p = SystemPartition;
#endif
    TBXInitParams.SupportedDevices    = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;

    /* Init TBX */
    if ((error = STTBX_Init( TBXDeviceName, &TBXInitParams )) != ST_NO_ERROR )
    {
        printf("Error in the STTBX_Init: 0x%08X\n",error);
    }

    return error;
}

#endif

/*=========================================================================
Name :       BOOTInit
Description: BOOT Init function
Parameters:  None
=========================================================================== */
ST_ErrorCode_t BOOTInit( void )
{
    ST_ErrorCode_t       error = ST_NO_ERROR;
    STBOOT_InitParams_t  BOOTInitParams;
    STBOOT_DCache_Area_t DCacheMap[] =
    {
#if defined(DISABLE_DCACHE)
        /* nothing */
#elif defined(CACHEABLE_BASE_ADDRESS) /* mb314/361/382/... */
         { (U32 *) CACHEABLE_BASE_ADDRESS, (U32 *) CACHEABLE_STOP_ADDRESS },
#elif defined(mb376)
         {{(U32 *) 0x40000000, (U32 *) 0x4007FFFF},    /* Cacheable */
#elif defined(mb282b) || defined(mb231)
         { (U32 *) 0x40080000, (U32 *) 0x5FFFFFFF },
#elif defined (mb314)
         { (U32 *) 0x40080000, (U32 *) 0x7FFFFFFF },
#else
         { (U32 *) 0x40080000, (U32 *) 0x4FFFFFFF },
#endif
        { NULL, NULL }
    };

    /* Avoids compiler warnings about unused blocks. */
#if !defined(ST_OS21) && !defined(ST_OSWINCE)
    internal_block_init[0] = 0;
    system_block_init[0] = 0;

#if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined(ST_5107) || defined(ST_5700)\
|| defined(ST_7100) || defined(ST_5301) || defined(ST_5525) || defined(ST_5188) || defined(ST_7109)|| defined(ST_7200)
    data_section[0] = 0;
#endif

#endif /* ST_OS21 && ST_OSWINCE*/

#ifdef UNIFIED_MEMORY
    ncache2_block[0] = 0;
#endif


#if defined(ST_OS21) || defined(ST_OSWINCE)
    /* Create memory partitions */
    system_partition = partition_create_heap((U8*)ExternalBlock, sizeof(ExternalBlock));

#else /* not defined ST_OS21 && not defined ST_OSWINCE */

    partition_init_simple(&the_internal_partition, (U8*)InternalBlock, sizeof(InternalBlock));
    partition_init_heap  (&the_system_partition, (U8*)ExternalBlock, sizeof(ExternalBlock));
#ifdef ARCHITECTURE_ST20
    partition_init_heap  (&the_ncache_partition, NcacheMemory, sizeof(NcacheMemory));
#endif
#endif
    BOOTInitParams.CacheBaseAddress = (U32*)CACHE_BASE_ADDRESS;
    BOOTInitParams.SDRAMFrequency   = SDRAM_FREQUENCY; /* in MHz */
#ifdef DISABLE_ICACHE
    BOOTInitParams.ICacheEnabled = FALSE;
#else
    BOOTInitParams.ICacheEnabled = TRUE;
#endif
    BOOTInitParams.DCacheMap = DCacheMap;
    BOOTInitParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    BOOTInitParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BOOTInitParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

#if defined(mb282b) || defined(mb275_64)
    BOOTInitParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_64_MBIT;
#elif defined(mb231) || defined(mb275)
    BOOTInitParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_32_MBIT;
#elif defined(mb295)
    /* Temporary value, not tested ! */
    BOOTInitParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_32_MBIT;
#else
    BOOTInitParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_32_MBIT;
    /* not used on mb314/361/382; set up by config file instead */
#endif

    /* Init BOOT */
    if ((error = STBOOT_Init( BOOTDeviceName, &BOOTInitParams)) != ST_NO_ERROR)
    {
        printf("Error in STBOOT_Init=0x%08x\n", error );
        return error;
    }

    return error;
}

/* EOF */

