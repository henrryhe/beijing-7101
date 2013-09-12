/*****************************************************************************
File Name   : cache.c

Description : CACHE setup

              This module contains functions for configuring the system
              on-board instruction and data caches.

Copyright (C) 1999-2005 STMicroelectronics

History     :

    15/08/99  First attempt.

    15/02/00  Modified to import cachereg.h for chip-specific cache
              configuration register map and region look-up table.

    16/02/01  Changed to 16-bit access; required for later cache
              subsystems that use > 8 bits per register.
              
    12/05/05  St200 specific dcache configuration.

Reference   :
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stdio.h>
#include "stlite.h"                     /* STLite includes */

#include "stsys.h"                      /* STAPI includes */

#include "stbootcache.h"                /* Cache control interface */
#include "stboot.h"

#ifdef ST_OS21
#ifdef ARCHITECTURE_ST200
#include <machine/sysconf.h>
#include <machine/rtrecord.h>
#include <machine/bsp.h>
#include <sys/bitfields.h>
#include <machine/ctrlreg.h>
#endif
#endif

#ifdef ST_OS20 /*not required for "non OS20" - ST40, ST200 devices*/
#include "cachereg.h"                   /* Regs for required chip */

/* Private types/constants ------------------------------------------------ */

/* Private cache control structure */
typedef struct
{
    U32         StartAddress;
    U32         EndAddress;
    U32         CacheEnableOffset;
    U16         CacheEnableBitMask;
} CacheControlTable_t;

/* Private variables ------------------------------------------------------ */

/* See STixxxx.h for the definition of the cache control LUT.
 */
static const CacheControlTable_t CacheControlLUT[] = DCACHE_LUT;

/* Private function prototypes -------------------------------------------- */

/* System cache initialization */

/* Function definitions --------------------------------------------------- */

/*****************************************************************************
Name: stboot_InitICache()
Description:
    Initializes the instruction cache.

Parameters:
    CacheDevice_p,  pointer to device base address.
    ICacheEnable,   boolean reflecting whether to enable disable icache.

Return Value:
    None.

See Also:
    stboot_InitDCache()
*****************************************************************************/

void stboot_InitICache(CacheReg_t *CacheDevice_p,
                       BOOL ICacheEnable)
{
    /* Initialize instruction cache, if enabled */
    if (ICacheEnable)
    {
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100) || defined(ST_5101) || defined(ST_7710) || defined(ST_5105) || defined(ST_5700) || defined(ST_5188) || defined(ST_5107)
        /* Enabling instruction caching on all memory (see DDTS 14020) */
        
        /* SMI (usually used when UNIFIED_MEMORY defined) */
        STSYS_WriteRegDev16LE(CacheDevice_p+ICAC_Region1BlockEnable, 0xFFFF);
        
        #ifdef STBOOT_LMI_TOP_REGION_ICACHE_ENABLE
        STSYS_WriteRegDev16LE(CacheDevice_p+ICAC_Region1TopEnable, 0x0001);
        #endif

        /* EMI (usually used when UNIFIED_MEMORY not defined) */
        STSYS_WriteRegDev16LE(CacheDevice_p+ICAC_Region3BlockEnable, 0xFFFF);
        STSYS_WriteRegDev16LE(CacheDevice_p+ICAC_Region3BankEnable,  0x000F);
#endif

        /* Invalidate the instruction cache */
        CacheDevice_p[INVALIDATE_ICACHE] = 1;
        
        /* Enable the instruction cache */
        CacheDevice_p[ENABLE_ICACHE] = 1;
    }
            
} /* stboot_InitICache() */

/*****************************************************************************
Name: stboot_InitDCache()
Description:
    Initializes the system data cache.  This is done by reading an array
    of defined cacheable memory areas, checking the cache control look-up
    table, and setting the appropriate cache control registers.

    Assuming the cache control look-up table has been correctly defined,
    this code should be easily portable to any backend decoder.

Parameters:
    CacheDevice_p,  pointer to device base address.
    DCacheMap,      array of cacheable area definitions.

Return Value:
    TRUE,           there was a problem with the dcache map.
    FALSE,          the dcache was setup without error.

See Also:
    stboot_InitDCache()
*****************************************************************************/

BOOL stboot_InitDCache(CacheReg_t *CacheDevice_p,
                       STBOOT_DCache_Area_t *DCacheMap)
{
    const CacheControlTable_t *CacheLUT_p;
    STBOOT_DCache_Area_t *CacheMap_p;

#ifdef ST_5100
    S8 Error = 0;
    U32 CachingAddressStart, CachingAddressEnd;
#endif

    if(DCacheMap == NULL)
    {
        /* No cache map, do nothing, no error */
        return FALSE;
    }
    
    if( (DCacheMap->StartAddress == NULL) &&
        (DCacheMap->EndAddress == NULL) )
    {
        /* Empty regions specified, do nothing, no error */
        return FALSE;
    }
    
    /* Data cache initialization is done in two stages:
     * 1. The dcache map is validated against the cache control
     *    lookup table.  If all map entries are valid then
     *    stage 2 can be executed, otherwise an error is
     *    returned.
     *
     * 2. The dcache map settings are applied to the device
     *    cache control registers using the cache control
     *    lookup table.  No error checking is performed at this
     *    stage.
     */

    /* 1. Validate the dcache map against the cache control LUT.
     */
    for ( CacheMap_p = DCacheMap;
          ( CacheMap_p->StartAddress != NULL &&
            CacheMap_p->EndAddress != NULL );
          CacheMap_p++ )
    {
        BOOL RegionOk, StartAddressOk;

        RegionOk = FALSE;
        StartAddressOk = FALSE;

        /* Scan through the cache control LUT to check that the caller
         * supplied region is on a valid start/end boundary, and does not
         * span a non-cacheable area.
         */
        for ( CacheLUT_p = CacheControlLUT;
              CacheLUT_p->CacheEnableBitMask != 0;
              CacheLUT_p++ )
        {
            /* Check for end of region marker */
            if ( CacheLUT_p->CacheEnableBitMask == ((U8)-1) )
            {
                StartAddressOk = FALSE;
                continue;               /* Next region */
            }

            /* Check for valid start address */
            if ( !StartAddressOk )
            {
                if ( (U32)CacheMap_p->StartAddress == CacheLUT_p->StartAddress )
                    StartAddressOk = TRUE;
            }

            /* If start address ok, check for valid end address */
            if ( StartAddressOk )
            {
                if ( (U32)CacheMap_p->EndAddress == CacheLUT_p->EndAddress )
                {
                    RegionOk = TRUE;
                    break;
                }
            }
        }

        if ( !RegionOk )
            return TRUE;                /* Invalid region specified */
    }

#if !defined(ST_5100)
    /* 2. Iterate over the dcache map to configure the cache control
     *    registers.
     */

    for ( CacheMap_p = DCacheMap;
          ( CacheMap_p->StartAddress != NULL &&
            CacheMap_p->EndAddress != NULL );
          CacheMap_p++ )
    {
        /* Scan through the cache control LUT to configure the cache
         * configuration registers, according to the current cache
         * map entry.
         */
        for ( CacheLUT_p = CacheControlLUT;
              CacheLUT_p->CacheEnableBitMask != 0;
              CacheLUT_p++ )
        {
            /* We calculate whether or not the user-specified area is
             * completely "disconnected" from the cacheable area.
             * If not then we must enable the area.
             */

			BOOL DisableRegion = ((U32)CacheMap_p->StartAddress < CacheLUT_p->StartAddress &&
                  		(U32)CacheMap_p->EndAddress   <= CacheLUT_p->StartAddress) ||
                  		((U32)CacheMap_p->StartAddress > CacheLUT_p->EndAddress &&
                  		(U32)CacheMap_p->EndAddress   >= CacheLUT_p->EndAddress);

            if (!DisableRegion)
            {
                U16 Val;

                /* Read current value and set the new bitmask */
                Val = STSYS_ReadRegDev16LE(
                          (CacheDevice_p+CacheLUT_p->CacheEnableOffset));
                Val |= CacheLUT_p->CacheEnableBitMask;
                STSYS_WriteRegDev16LE(
                    (CacheDevice_p+CacheLUT_p->CacheEnableOffset), Val);
            }
        }
    }
    
    
    CacheDevice_p[INVALIDATE_DCACHE] = 1; /* Invalidate DCACHE */
    CacheDevice_p[DCACHE_NOT_SRAM] = 1;   /* Enable DCACHE */
#endif

#if defined(ST_5100)
    CacheMap_p = DCacheMap;
    do
    {
        if ((CacheMap_p->StartAddress != NULL) && (CacheMap_p->EndAddress != NULL))
        {
           	CachingAddressStart = (U32) CacheMap_p->StartAddress;
           	CachingAddressEnd = (U32) CacheMap_p->EndAddress;
        }
        Error = cache_config_data((void*) CachingAddressStart, (void*)CachingAddressEnd, cache_config_enable);
        if ( Error != 0 )
        {
       	    return TRUE;
        }
        
        CacheMap_p++;
    }
    while((CacheMap_p->StartAddress != NULL) || (CacheMap_p->EndAddress != NULL));
    
    Error = cache_enable_data(); /*typically cache should be enabled AFTER an invalidate call. OS20 takes care of this when using this call*/
    if(Error != 0)
    {
    	return TRUE;
    }
    
    Error = cache_flush_data(NULL, NULL);   /* Flush DCACHE -->required for 5100 dcache bug*/
    if(Error != ST_NO_ERROR)
    {
    	return TRUE;
    }
#endif

    return FALSE;                       /* Success */
} /* InitDCache() */


/*****************************************************************************
Name: stboot_LockCaches()
Description:
    Locks the cache control registers to prevent further
    changes to the cache configuration.
    
Parameters:
    CacheDevice_p,  pointer to device base address.

Return Value:
    TRUE,           there was a problem with the dcache map.
    FALSE,          the dcache was setup without error.

See Also:
    stboot_InitDCache()
*****************************************************************************/
void  stboot_LockCaches(CacheReg_t *CacheDevice_p)
{
#if !defined(ST_5100) && !defined(ST_5101) && !defined(ST_7710) && !defined(ST_5105) && !defined(ST_5700) && !defined(ST_5188) && !defined(ST_5107)
    /* Lock access to CACHE control regs */
    
#if defined (ST_5514) || defined (ST_5516) || defined(ST_5517) || defined(ST_5528)
    /* Separate lock control for ICACHE and DCACHE */
    STSYS_WriteRegDev8(CacheDevice_p+DCACHE_CONTROL_LOCK, 1);
    STSYS_WriteRegDev8(CacheDevice_p+ICACHE_CONTROL_LOCK, 1);
#else
    /* single lock control */
    STSYS_WriteRegDev8(CacheDevice_p+CACHE_CONTROL_LOCK, 1);
#endif

#endif    
}

#endif /*#ifdef ST_OS20*/


#ifdef ST_OS21
/********************************************** OS21 **********************************************/
#ifdef ARCHITECTURE_ST200

#ifndef STBOOT_SPECIFY_HOST_MEMORY_MAP
/*****************************************************************************
Name: stboot_st200_InitDCache()
Description:
    For configuration of the system data cache.

Parameters:
    InitParams_p                    the initialization parameters.

Return Value:
    ST_NO_ERROR                     No Errors during operation.
    STBOOT_ERROR_INVALID_DCACHE     Invalid dcache map.
    STBOOT_ERROR_BSP_FAILURE        Error from toolset BSP.
*****************************************************************************/

ST_ErrorCode_t stboot_st200_InitDCache(STBOOT_InitParams_t *InitParams_p)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	U32 CachingAddressStart, CachingAddressEnd;
	STBOOT_DCache_Area_t *CacheMap_p;
	
	/*The following calls display the current memory map in a table format, can be enabled for debug*/
    #ifdef STBOOT_DISPLAY_BSP_MEMORY_MAP
    printf("\nDefault Caching status:\n");
    printf("-----------------------------------------------------------------------\n");
    bsp_mmu_dump_TLB_Settings();
    bsp_scu_dump_SCU_Settings();
    #endif
	
    if((InitParams_p->DCacheMap == NULL) || (InitParams_p->DCacheEnabled == FALSE))
    {    	
    	#if defined(ST_5301)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(0xC0000000), 0x02000000); /*unmaps for 32MB of lmi 0xC000 0000 to 0xC1FF FFFF*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
        
        ErrorCode = (int) bsp_mmu_memory_map((void *)(0xc0000000),
                           0x02000000,
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(0xc0000000));

        if ( ErrorCode != 0xc0000000 )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*5301*/


        #if defined(ST_8010)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(0x80000000), 0x02000000); /*unmaps for 32MB of lmi 0x8000 0000 to 0x81FF FFFF*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
          
        ErrorCode = (int) bsp_mmu_memory_map((void *)(0x80000000),
                           0x02000000,
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(0x80000000));

        if ( ErrorCode != 0x80000000 )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*8010*/
        
        #if defined(ST_5525)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(0x80000000), 0x02000000); /*unmaps for 32MB of lmi 0x8000 0000 to 0x81FF FFFF*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
          
        ErrorCode = (int) bsp_mmu_memory_map((void *)(0x80000000),
                           0x02000000,
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(0x80000000));

        if ( ErrorCode != 0x80000000 )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*5525*/
        
        /*The following calls display the current memory map in a table format, can be enabled for debug*/
        #ifdef STBOOT_DISPLAY_BSP_MEMORY_MAP
        printf("\n\nDisabled all data caching:\n");
        printf("-----------------------------------------------------------------------\n");
        bsp_mmu_dump_TLB_Settings();
        bsp_scu_dump_SCU_Settings();
        #endif
        
        return ST_NO_ERROR;
        
    } /*corresponds to-> if((InitParams_p->DCacheMap == NULL) || (InitParams_p->DCacheEnabled == FALSE))*/
    
    else /*if not disabling the dcache completely*/
    {
    	if((InitParams_p->DCacheMap->StartAddress == NULL) && (InitParams_p->DCacheMap->EndAddress == NULL))
    	{
    		return ST_NO_ERROR; /*do no cache config in this case, the default remains*/
    	}
    	
    	#ifdef ST_5301
    	if((((int) InitParams_p->DCacheMap->StartAddress) > 0xC1C00000) || (((int) InitParams_p->DCacheMap->StartAddress) < 0xC0000000) || (((int) InitParams_p->DCacheMap->EndAddress) > 0xC1FFFFFF) || (((int) InitParams_p->DCacheMap->EndAddress) < 0xC03FFFFF))
    	{
    		return STBOOT_ERROR_INVALID_DCACHE;
    	}
    	#endif
    	
    	#ifdef ST_8010
    	if((((int) InitParams_p->DCacheMap->StartAddress) > 0x81C00000) || (((int) InitParams_p->DCacheMap->StartAddress) < 0x80000000) || (((int) InitParams_p->DCacheMap->EndAddress) > 0x81FFFFFF) || (((int) InitParams_p->DCacheMap->EndAddress) < 0x803FFFFF))
    	{
    		return STBOOT_ERROR_INVALID_DCACHE;
    	}
    	#endif
    	
    	#ifdef ST_5525
    	if((((int) InitParams_p->DCacheMap->StartAddress) > 0x81C00000) || (((int) InitParams_p->DCacheMap->StartAddress) < 0x80000000) || (((int) InitParams_p->DCacheMap->EndAddress) > 0x81FFFFFF) || (((int) InitParams_p->DCacheMap->EndAddress) < 0x803FFFFF))
    	{
    		return STBOOT_ERROR_INVALID_DCACHE;
    	}
    	#endif

    	CachingAddressStart = (U32) InitParams_p->DCacheMap->StartAddress;
    	CachingAddressEnd = (U32) InitParams_p->DCacheMap->EndAddress;
    	if(((CachingAddressEnd - CachingAddressStart + 1) % 4194304) != 0) /*check that it is multiple of 4MB (4194304 bytes) page size or not*/
    	{
    		return STBOOT_ERROR_INVALID_DCACHE;
    	}

    	#if defined(ST_5301) /*disable whole dcache(chip specific), then enable required region*/
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(0xC0000000), 0x02000000); /*unmaps for 32MB of lmi 0xC000 0000 to 0xC1FF FFFF*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
        
        ErrorCode = (int) bsp_mmu_memory_map((void *)(0xc0000000),
                           0x02000000,
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(0xc0000000));

        if ( ErrorCode != 0xc0000000 )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*5301*/


        #if defined(ST_8010)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(0x80000000), 0x02000000); /*unmaps for 32MB of lmi 0x8000 0000 to 0x81FF FFFF*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
          
        ErrorCode = (int) bsp_mmu_memory_map((void *)(0x80000000),
                           0x02000000,
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(0x80000000));

        if ( ErrorCode != 0x80000000 )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*8010*/
        
        #if defined(ST_5525)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(0x80000000), 0x02000000); /*unmaps for 32MB of lmi 0x8000 0000 to 0x81FF FFFF*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
          
        ErrorCode = (int) bsp_mmu_memory_map((void *)(0x80000000),
                           0x02000000,
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(0x80000000));

        if ( ErrorCode != 0x80000000 )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*5525*/

        /*The following calls display the current memory map in a table format, can be enabled for debug*/
        #ifdef STBOOT_DISPLAY_BSP_MEMORY_MAP
        printf("\n\nDisabled all data caching:\n");
        printf("-----------------------------------------------------------------------\n");
        bsp_mmu_dump_TLB_Settings();
        bsp_scu_dump_SCU_Settings();
        #endif

    	CacheMap_p = InitParams_p->DCacheMap;
        
    	do
    	{
    		/*Enable dcache for specific region now*/
    	    ErrorCode = bsp_mmu_memory_unmap( (void *)(CachingAddressStart), CachingAddressEnd - CachingAddressStart + 1);
    	    if ( ErrorCode != BSP_OK )
            {
        	    return STBOOT_ERROR_BSP_FAILURE;
            }

            ErrorCode = (int) bsp_mmu_memory_map((void *)(CachingAddressStart),
                           (CachingAddressEnd - CachingAddressStart + 1),
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ|
                           PROT_CACHEABLE,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(CachingAddressStart));
            if ( ErrorCode != CachingAddressStart )
            {
        	    return STBOOT_ERROR_BSP_FAILURE;
            }
            
            CacheMap_p++;
            if ((CacheMap_p->StartAddress != NULL) && (CacheMap_p->EndAddress != NULL))
            {
            	CachingAddressStart = (U32) CacheMap_p->StartAddress;
            	CachingAddressEnd = (U32) CacheMap_p->EndAddress;
            }
    	}
    	while((CacheMap_p->StartAddress != NULL) || (CacheMap_p->EndAddress != NULL));
    	
        
        /*The following calls display the current memory map in a table format, can be enabled for debug*/
        #ifdef STBOOT_DISPLAY_BSP_MEMORY_MAP
        printf("\n\nUser requested dcache configuration done (pre OS21 initialise):\n");
        printf("-----------------------------------------------------------------------\n");
        bsp_mmu_dump_TLB_Settings();
        bsp_scu_dump_SCU_Settings();
        #endif
        
        return ST_NO_ERROR;
    }
}
#endif /*#ifndef STBOOT_SPECIFY_HOST_MEMORY_MAP*/


#ifdef STBOOT_SPECIFY_HOST_MEMORY_MAP
/*****************************************************************************
Name: stboot_st200_InitDCache_Specified_Host_Map()
Description:
    For configuration of the system data cache when the host side memory limits are specified.

Parameters:
    InitParams_p                    the initialization parameters.

Return Value:
    ST_NO_ERROR                     No Errors during operation.
    STBOOT_ERROR_INVALID_DCACHE     Invalid dcache map.
    STBOOT_ERROR_BSP_FAILURE        Error from toolset BSP.
*****************************************************************************/

ST_ErrorCode_t stboot_st200_InitDCache_Specified_Host_Map(STBOOT_InitParams_t *InitParams_p)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	U32 CachingAddressStart, CachingAddressEnd, HostMemoryAddressStart, HostMemoryAddressEnd;
	STBOOT_DCache_Area_t *CacheMap_p;
	STBOOT_DCache_Area_t *HostMemoryMap_p;
	
	/*The following calls display the current memory map in a table format, can be enabled for debug*/
    #ifdef STBOOT_DISPLAY_BSP_MEMORY_MAP
    printf("\nDefault Caching status:\n");
    printf("-----------------------------------------------------------------------\n");
    bsp_mmu_dump_TLB_Settings();
    bsp_scu_dump_SCU_Settings();
    #endif
	
	if((InitParams_p->HostMemoryMap == NULL) || (InitParams_p->HostMemoryMap->StartAddress == NULL) || (InitParams_p->HostMemoryMap->EndAddress == NULL))
	{
		return STBOOT_ERROR_INVALID_DCACHE;
	}
	
    HostMemoryAddressStart = (U32) InitParams_p->HostMemoryMap->StartAddress;
    HostMemoryAddressEnd = (U32) InitParams_p->HostMemoryMap->EndAddress;
    if((InitParams_p->DCacheMap == NULL) || (InitParams_p->DCacheEnabled == FALSE))
    {    	
    	#if defined(ST_5301)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(HostMemoryAddressStart), (HostMemoryAddressEnd-HostMemoryAddressStart+1)); /*unmaps for user specified host range of lmi*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
        
        ErrorCode = (int) bsp_mmu_memory_map((void *)(HostMemoryAddressStart),
                           (HostMemoryAddressEnd-HostMemoryAddressStart+1),
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(HostMemoryAddressStart));

        if ( ErrorCode != HostMemoryAddressStart )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*5301*/


        #if defined(ST_8010)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(HostMemoryAddressStart), (HostMemoryAddressEnd-HostMemoryAddressStart+1)); /*unmaps for user specified host range of lmi*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
          
        ErrorCode = (int) bsp_mmu_memory_map((void *)(HostMemoryAddressStart),
                           (HostMemoryAddressEnd-HostMemoryAddressStart+1),
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(HostMemoryAddressStart));

        if ( ErrorCode != HostMemoryAddressStart )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*8010*/
        
        #if defined(ST_5525)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(HostMemoryAddressStart), (HostMemoryAddressEnd-HostMemoryAddressStart+1)); /*unmaps for user specified host range of lmi*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
          
        ErrorCode = (int) bsp_mmu_memory_map((void *)(HostMemoryAddressStart),
                           (HostMemoryAddressEnd-HostMemoryAddressStart+1),
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(HostMemoryAddressStart));

        if ( ErrorCode != HostMemoryAddressStart )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*5525*/
        
        /*The following calls display the current memory map in a table format, can be enabled for debug*/
        #ifdef STBOOT_DISPLAY_BSP_MEMORY_MAP
        printf("\n\nDisabled all data caching:\n");
        printf("-----------------------------------------------------------------------\n");
        bsp_mmu_dump_TLB_Settings();
        bsp_scu_dump_SCU_Settings();
        #endif
        
        return ST_NO_ERROR;
        
    } /*corresponds to-> if((InitParams_p->DCacheMap == NULL) || (InitParams_p->DCacheEnabled == FALSE))*/
    
    else /*if not disabling the dcache completely*/
    {
    	if((InitParams_p->DCacheMap->StartAddress == NULL) && (InitParams_p->DCacheMap->EndAddress == NULL))
    	{
    		return ST_NO_ERROR; /*do no cache config in this case, the default remains*/
    	}
    	
    	#ifdef ST_5301
    	if((((int) InitParams_p->DCacheMap->StartAddress) > (HostMemoryAddressEnd - 4194303)) || (((int) InitParams_p->DCacheMap->StartAddress) < HostMemoryAddressStart) || (((int) InitParams_p->DCacheMap->EndAddress) > HostMemoryAddressEnd) || (((int) InitParams_p->DCacheMap->EndAddress) < (HostMemoryAddressStart + 4194303))) /*4194304 - 1*/
    	{
    		return STBOOT_ERROR_INVALID_DCACHE;
    	}
    	#endif
    	
    	#ifdef ST_8010
    	if((((int) InitParams_p->DCacheMap->StartAddress) > (HostMemoryAddressEnd - 4194303)) || (((int) InitParams_p->DCacheMap->StartAddress) < HostMemoryAddressStart) || (((int) InitParams_p->DCacheMap->EndAddress) > HostMemoryAddressEnd) || (((int) InitParams_p->DCacheMap->EndAddress) < (HostMemoryAddressStart + 4194303)))
    	{
    		return STBOOT_ERROR_INVALID_DCACHE;
    	}
    	#endif
    	
    	#ifdef ST_5525
    	if((((int) InitParams_p->DCacheMap->StartAddress) > (HostMemoryAddressEnd - 4194303)) || (((int) InitParams_p->DCacheMap->StartAddress) < HostMemoryAddressStart) || (((int) InitParams_p->DCacheMap->EndAddress) > HostMemoryAddressEnd) || (((int) InitParams_p->DCacheMap->EndAddress) < (HostMemoryAddressStart + 4194303)))
    	{
    		return STBOOT_ERROR_INVALID_DCACHE;
    	}
    	#endif

    	CachingAddressStart = (U32) InitParams_p->DCacheMap->StartAddress;
    	CachingAddressEnd = (U32) InitParams_p->DCacheMap->EndAddress;
    	if(((CachingAddressEnd - CachingAddressStart + 1) % 4194304) != 0) /*check that it is multiple of 4MB (4194304 bytes) page size or not*/
    	{
    		return STBOOT_ERROR_INVALID_DCACHE;
    	}

    	#if defined(ST_5301)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(HostMemoryAddressStart), (HostMemoryAddressEnd-HostMemoryAddressStart+1)); /*unmaps for user specified host range of lmi*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
        
        ErrorCode = (int) bsp_mmu_memory_map((void *)(HostMemoryAddressStart),
                           (HostMemoryAddressEnd-HostMemoryAddressStart+1),
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(HostMemoryAddressStart));

        if ( ErrorCode != HostMemoryAddressStart )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*5301*/


        #if defined(ST_8010)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(HostMemoryAddressStart), (HostMemoryAddressEnd-HostMemoryAddressStart+1)); /*unmaps for user specified host range of lmi*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
          
        ErrorCode = (int) bsp_mmu_memory_map((void *)(HostMemoryAddressStart),
                           (HostMemoryAddressEnd-HostMemoryAddressStart+1),
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(HostMemoryAddressStart));

        if ( ErrorCode != HostMemoryAddressStart )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*8010*/
        
        #if defined(ST_5525)
    	ErrorCode = bsp_mmu_memory_unmap( (void *)(HostMemoryAddressStart), (HostMemoryAddressEnd-HostMemoryAddressStart+1)); /*unmaps for user specified host range of lmi*/
    	if ( ErrorCode != BSP_OK )
        {
            return STBOOT_ERROR_BSP_FAILURE;
        }
          
        ErrorCode = (int) bsp_mmu_memory_map((void *)(HostMemoryAddressStart),
                           (HostMemoryAddressEnd-HostMemoryAddressStart+1),
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(HostMemoryAddressStart));

        if ( ErrorCode != HostMemoryAddressStart )
        {
        	return STBOOT_ERROR_BSP_FAILURE;
        }
        #endif /*5525*/

        /*The following calls display the current memory map in a table format, can be enabled for debug*/
        #ifdef STBOOT_DISPLAY_BSP_MEMORY_MAP
        printf("\n\nDisabled all data caching:\n");
        printf("-----------------------------------------------------------------------\n");
        bsp_mmu_dump_TLB_Settings();
        bsp_scu_dump_SCU_Settings();
        #endif

    	CacheMap_p = InitParams_p->DCacheMap;
        
    	do
    	{
    		/*Enable dcache for specific region now*/
    	    ErrorCode = bsp_mmu_memory_unmap( (void *)(CachingAddressStart), CachingAddressEnd - CachingAddressStart + 1);
    	    if ( ErrorCode != BSP_OK )
            {
        	    return STBOOT_ERROR_BSP_FAILURE;
            }

            ErrorCode = (int) bsp_mmu_memory_map((void *)(CachingAddressStart),
                           (CachingAddressEnd - CachingAddressStart + 1),
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ|
                           PROT_CACHEABLE,
                           MAP_SPARE_RESOURCES|
                           MAP_FIXED|
                           MAP_OVERRIDE,
                           (void *)(CachingAddressStart));
            if ( ErrorCode != CachingAddressStart )
            {
        	    return STBOOT_ERROR_BSP_FAILURE;
            }
            
            CacheMap_p++;
            if ((CacheMap_p->StartAddress != NULL) && (CacheMap_p->EndAddress != NULL))
            {
            	CachingAddressStart = (U32) CacheMap_p->StartAddress;
            	CachingAddressEnd = (U32) CacheMap_p->EndAddress;
            }
    	}
    	while((CacheMap_p->StartAddress != NULL) || (CacheMap_p->EndAddress != NULL));
    	
        
        /*The following calls display the current memory map in a table format, can be enabled for debug*/
        #ifdef STBOOT_DISPLAY_BSP_MEMORY_MAP
        printf("\n\nUser requested dcache configuration done (pre OS21 initialise):\n");
        printf("-----------------------------------------------------------------------\n");
        bsp_mmu_dump_TLB_Settings();
        bsp_scu_dump_SCU_Settings();
        #endif
        
        return ST_NO_ERROR;
    }
}
#endif /*#ifdef STBOOT_SPECIFY_HOST_MEMORY_MAP*/

#endif /*#ifdef ARCHITECTURE_ST200*/
#endif /*#ifdef ST_OS21*/

/* End of cache.c */
