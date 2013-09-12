/*******************************************************************************

File name   : clboot.c

Description : STBOOT initialization source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
31 Jan 2002        Created                                           HS
14 May 2002        Add support for STi5516B/mb361                    HS
11 Oct 2002        Add support for mb382                             HS
14 Sept 2004       Add support for OS21                              MH
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>

#ifdef ST_OS20
    #include <debug.h>
#endif /* ST_OS20 */

#ifdef ST_OS21
    #include "os21debug.h"
#endif /* ST_OS21 */

#define USE_AS_FRONTEND

#include "genadd.h"
#include "stddefs.h"
#include "stdevice.h"

#include "stboot.h"
#include "clboot.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#define STBOOT_DEVICE_NAME "STBOOT"

#if defined CACHEABLE_BASE_ADDRESS
#define DCACHE_START  CACHEABLE_BASE_ADDRESS /* assumed ok */
#define DCACHE_END    CACHEABLE_STOP_ADDRESS /* assumed ok */
#elif defined(mb314) || defined(mb361) || defined(mb382)
#if defined (UNIFIED_MEMORY)
#define DCACHE_START  0xC0380000
#define DCACHE_END    0xC07FFFFF
#else /* UNIFIED_MEMORY */
#define DCACHE_START  0x40200000
#define DCACHE_END    0x4FFFFFFF  /* STBOOT tests: 0x407FFFFF */
#endif /* UNIFIED_MEMORY */
#elif defined(mb290) /* assumption: no Unified memory configuration requested on mb290 */
#define DCACHE_START  0x40080000
#define DCACHE_END    0x4FFFFFFF  /* STBOOT tests: 0x407FFFFF */
#define DCACHE_SECOND_BANK
#else
#define DCACHE_START  0x40080000
#define DCACHE_END    0x4FFFFFFF  /* STBOOT tests: 0x7FFFFFFF */
#endif

/*if defined DCACHE_SECOND_BANK*/
#define DCACHE_START2       0x50000000
#define DCACHE_END2         0x5FFFFFFF

#ifdef DCACHE_TESTS_SDRAM
#define DCACHE_START3       SDRAM_BASE_ADDRESS
#define DCACHE_END3         (SDRAM_BASE_ADDRESS + 0x00080000 - 1)
#endif /* #ifdef DCACHE_TESTS_SDRAM */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

#ifdef DCACHE_ENABLE
STBOOT_DCache_Area_t DCacheMap[]  =
{
    { (U32 *)DCACHE_START, (U32 *)DCACHE_END },
#ifdef DCACHE_SECOND_BANK
    { (U32 *)DCACHE_START2, (U32 *)DCACHE_END2 },
#endif
#ifdef DCACHE_TESTS_SDRAM
    { (U32 *)DCACHE_START3, (U32 *)DCACHE_END3 },
#endif
    { NULL, NULL }
};
STBOOT_DCache_Area_t *DCacheMap_p = &DCacheMap[0];
size_t DCacheMapSize              = sizeof(DCacheMap);
#else  /* #ifdef DCACHE_ENABLE */
STBOOT_DCache_Area_t *DCacheMap   = NULL;
STBOOT_DCache_Area_t *DCacheMap_p = NULL;
size_t DCacheMapSize              = sizeof(STBOOT_DCache_Area_t);
#endif /* #ifdef DCACHE_ENABLE */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : BOOT_Init
Description : STBOOT initialisation
Parameters  : none
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL BOOT_Init(void)
{
    STBOOT_InitParams_t InitStboot;
    BOOL RetOk = TRUE;

    InitStboot.CacheBaseAddress          = (U32 *) CACHE_BASE_ADDRESS;
#ifdef ICACHE_ENABLE
    InitStboot.ICacheEnabled             = TRUE;
#else
    InitStboot.ICacheEnabled             = FALSE;
#endif
    InitStboot.DCacheMap = DCacheMap;
    InitStboot.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    InitStboot.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    InitStboot.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

    InitStboot.SDRAMFrequency            = SDRAM_FREQUENCY;    /* in kHz */
    InitStboot.MemorySize                = SDRAM_SIZE;
    /* Use default int config whatever compilation flag used for STBOOT driver */
    InitStboot.IntTriggerMask            = 0;
#if !(defined ST_OS21) && !defined(ST_OSWINCE) && !(defined ST_OSLINUX)
    InitStboot.IntTriggerTable_p         = NULL;
#endif /* NOT ST_OS21 */

#if defined(ST_OS21) && !defined(ST_OSWINCE)
    InitStboot.TimeslicingEnabled		 = TRUE;
    InitStboot.DCacheEnabled             = TRUE;
#endif /* ST_OS21 */


    if (STBOOT_Init(STBOOT_DEVICE_NAME, &InitStboot) != ST_NO_ERROR)
    {
        printf("Error: STBOOT failed to initialize !\n");
        RetOk = FALSE;
    }
    else
    {
        printf("STBOOT initialized,\trevision=%s\n",STBOOT_GetRevision());
    }
    return(RetOk);
} /* End of BOOT_Init() function */

/*******************************************************************************
Name        : BOOT_Term
Description : Terminate the BOOT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void BOOT_Term(void)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STBOOT_TermParams_t TermParams;

    ErrorCode = STBOOT_Term( STBOOT_DEVICE_NAME, &TermParams);

    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STBOOT_Term(%s): ok\n", STBOOT_DEVICE_NAME);
    }
    else
    {
        printf("STBOOT_Term(%s): error 0x%0x\n", STBOOT_DEVICE_NAME, ErrorCode);
    }
} /* end BOOT_Term() */

/* End of clboot.c */
