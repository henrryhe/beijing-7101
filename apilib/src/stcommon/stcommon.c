/********************************************************************************
COPYRIGHT (C) STMicroelectronics 1999

Source file name : stcommon.c

********************************************************************************/

/* global include files */
#ifndef ST_OSLINUX
#include <stdio.h>
#endif

#include "stddefs.h"
#include "stcommon.h"

#ifdef ST_OSWINCE
#include "stdarg.h"
#endif

#ifdef ST_OSLINUX
#include "stos.h"
#else
#include "stsys.h"
#endif
#include "stdevice.h"

/* include file info - defined here temporarily */
typedef enum
{
    GET_DRIVER_PREFIX,
    GET_REL_STRING,
    GET_MAJOR_REVISION,
    GET_MINOR_REVISION,
    GET_PATCH_REVISION
} PARSE_STATE;


/* local variables */
void * ConfigBaseAddress_p = (void *)CFG_BASE_ADDRESS; /*for ST_GetCutRevision API*/


/* function prototypes */

static BOOL bIsNum( char cData, U32 *pu32Value );
static BOOL bStrncmp( char *pcData, char *pcString, int iNumberChars );

static BOOL ST_GetRevision( ST_Revision_t *pcRevisionInformation,
                            U32  *pu32MajorRevision,
                            U32  *pu32MinorRevision,
                            U32  *pu32PatchRevision );


/********************************************************************
 *                                                                  *
 * Function:    bIsNum                                              *
 *                                                                  *
 * Input:       cData - character to be verified                    *
 *                                                                  *
 * Output:      u32Value - value of converted numeric character     *
 *                                                                  *
 * Returns:     TRUE - success or FALSE - failure                   *
 *                                                                  *
 * Action:      Determines if the data passed is ASCII numeric      *
 *                                                                  *
 ********************************************************************/
static BOOL bIsNum( char cData, U32 *pu32Value )
{
    if (( cData < '0' ) || ( cData > '9' ))
    {
        /* ERROR - not a numeric digit */
        return( FALSE );
    }

    *pu32Value = (U32)( cData - '0' );
    return ( TRUE );
}   /* end of bIsNum() */



/********************************************************************
 *                                                                  *
 * Function:    ST_AreStringsEqual                                  *
 *                                                                  *
 * Input:       DeviceName1_p - 1st Device Name/string to be checked*
 *              DeviceName2_p - 2nd Device Name/string to be checked*
 *                                                                  *
 * Output:      None                                                *
 *                                                                  *
 * Returns:     TRUE - strings equal or FALSE - strings not equal   *
 *                                                                  *
 * Action:      Compare & report whether two STAPI device names     *
 *              or strings are equal or not                         *
 *                                                                  *
 ********************************************************************/
BOOL ST_AreStringsEqual(const char *DeviceName1_p, const char *DeviceName2_p)
{
    for (; (*DeviceName1_p == *DeviceName2_p); DeviceName1_p++,DeviceName2_p++)
    {
        if (*DeviceName1_p == '\0')
        {
            return(TRUE);
        }
    }
    return(FALSE);
} /* end of ST_AreStringsEqual() */



/********************************************************************
 *                                                                  *
 * Function:    bStrncmp                                            *
 *                                                                  *
 * Input:       pcData       - Data to check for occurance of       *
 *                             string in                            *
 *              pcString     - String to be checked for in Data     *
 *              iNumberChars - Number fo characters of String to    *
 *                             be checked for in Data               *
 *                                                                  *
 * Output:      None                                                *
 *                                                                  *
 * Returns:     TRUE - success or FALSE - failure                   *
 *                                                                  *
 * Action:      Compare iNumberChars characters in Data and String  *
 *                                                                  *
 ********************************************************************/
static BOOL bStrncmp( char *pcData, char *pcString, int iNumberChars )
{
    char    *pcDataPtr,
            *pcStringPtr;

    if (( pcData == (char *) NULL )     ||
        ( pcString == (char *) NULL )   ||
        ( iNumberChars == 0 ))
    {
        /* ERROR - NULL pointer or iNumberChars is 0 */
        return( FALSE );
    }

    pcDataPtr = pcData;
    pcStringPtr = pcString;

    do
    {
        if (( *pcDataPtr != *pcStringPtr ))
        {
            /* ERROR - data and string are not equal */
            return( FALSE );
        }

        /* move to the next character pair to be compared */
        pcDataPtr++;
        pcStringPtr++;
        iNumberChars--;
        
    } while ( iNumberChars > 0);

    return( TRUE );
    
}   /* end of bStrncmp() */


/********************************************************************
 *                                                                  *
 * Function:    ST_ConvRevToNum                                     *
 *                                                                  *
 * Input:       ptRevision - pointer to revision information        *
 *                                                                  *
 * Output:      None                                                *
 *                                                                  *
 * Returns:     pu32RevInfo - 32 bit representation of revision     *
 *                                information                       *
 *                                If conversion fails returns 0     *
 *                                                                  *
 *                                MMSB - 0                          *
 *                                MLSB - Major Revision             *
 *                                LMSB - Minor Revision             *
 *                                LLSB - Patch Revision             *
 *                                                                  *
 ********************************************************************/
U32 ST_ConvRevToNum( ST_Revision_t *ptRevision )
{
    U32    u32MajorRevision,
           u32MinorRevision,
           u32NumRevInfo,
           u32PatchRevision;

    if ( ST_GetRevision( ptRevision,
                         &u32MajorRevision,
                         &u32MinorRevision,
                         &u32PatchRevision ) != TRUE )
    {
        /* ERROR - failed to convert string */
        return( 0 );
    }

    u32NumRevInfo = u32MajorRevision << 16;
    u32NumRevInfo |= u32MinorRevision << 8;
    u32NumRevInfo |= u32PatchRevision;

    return( u32NumRevInfo );
}   /* end of ST_ConvRevToNum() */


/********************************************************************
 *                                                                  *
 * Function:    ST_GetMajorRevision                                 *
 *                                                                  *
 * Input:       ptRevision  - pointer to revision information       *
 *                                                                  *
 * Output:      MajorRevision - pointer to major revision number    *
 *                                                                  *
 * Returns:     Major Revision Number - success                     *
 *              -1                    - failure                     *
 * Action:      Fetches the Major Revision number of a driver.      *
 *                                                                  *
 ********************************************************************/
U32 ST_GetMajorRevision( ST_Revision_t *ptRevision )
{
    U32     u32Dummy1,
            u32Dummy2,
            u32MajorRevision;
    
    if ( ST_GetRevision( ptRevision,
                         &u32MajorRevision,
                         &u32Dummy1,
                         &u32Dummy2 ) != TRUE )
    {
        /* ERROR - could not get revision information */
        return((U32)-1 );
    }

    return( u32MajorRevision );
}   /* end of ST_GetMajorRevision() */


/********************************************************************
 *                                                                  *
 * Function:    ST_GetMinorRevision                                 *
 *                                                                  *
 * Input:       ptRevision  - pointer to revision information       *
 *                                                                  *
 * Output:      MinorRevision - pointer to minor revision number    *
 *                                                                  *
 * Returns:     Minor Revision Number - success                     *
 *              -1                    - failure                     *
 *                                                                  *
 * Action:      Fetches the Minor Revision number of a driver.      *
 *                                                                  *
 ********************************************************************/
U32 ST_GetMinorRevision( ST_Revision_t *ptRevision )
{
    U32     u32Dummy1,
            u32Dummy2,
            u32MinorRevision;
    
    if ( ST_GetRevision( ptRevision,
                         &u32Dummy1,
                         &u32MinorRevision,
                         &u32Dummy2 ) != TRUE )
    {
        /* ERROR - could not get revision information */
        return((U32)-1 );
    }

    return( u32MinorRevision );
}   /* end of ST_GetMinorRevision() */


/********************************************************************
 *                                                                  *
 * Function:    ST_GetPatchRevision                                 *
 *                                                                  *
 * Input:       ptRevision  - pointer to revision information       *
 *                                                                  *
 * Output:      PatchRevision - pointer to patch revision number    *
 *                                                                  *
 * Returns:     Patch Revision Number - success                     *
 *              -1                    - failure                     *
 *                                                                  *
 ********************************************************************/
U32 ST_GetPatchRevision( ST_Revision_t *ptRevision )
{
    U32     u32Dummy1,
            u32Dummy2,
            u32PatchRevision;
    
    if ( ST_GetRevision( ptRevision,
                         &u32Dummy1,
                         &u32Dummy2,
                         &u32PatchRevision ) != TRUE )
    {
        /* ERROR - could not get revision information */
        return((U32)-1 );
    }

    return( u32PatchRevision );
}   /* end of ST_GetPatchRevision() */



/********************************************************************
 *                                                                  *
 * Function:    ST_GetRevision                                      *
 *                                                                  *
 * Input:       ptRevision  - pointer to revision information       *
 *                                                                  *
 * Output:      pu32MajorRevision - pointer to major rev number     *
 *              pu32MinorRevision - pointer to minor rev number     *
 *              pu32PatchRevision - pointer to patch rev number     *
 *                                                                  *
 * Returns:     TRUE - success or FALSE - failure                   *
 *                                                                  *
 ********************************************************************/
static BOOL ST_GetRevision( ST_Revision_t *ptRevisionInformation,
                            U32  *pu32MajorRevision,
                            U32  *pu32MinorRevision,
                            U32  *pu32PatchRevision )
{
    BOOL        bComplete = FALSE;
    char        *pcData;
    PARSE_STATE enParseState = GET_DRIVER_PREFIX;
    U32         u32Value; 

    /* validate input parameters */
    if (( ptRevisionInformation == (ST_Revision_t *) NULL )  ||
        ( pu32MajorRevision == (U32 *) NULL )               ||
        ( pu32MinorRevision == (U32 *) NULL )               ||
        ( pu32PatchRevision == (U32 *) NULL ))
    {
        /* ERROR - NULL pointer passed to function */
        return( FALSE );
    }

    pcData = (char *)(*ptRevisionInformation);
    *pu32MajorRevision = 0;
    *pu32MinorRevision = 0;
    *pu32PatchRevision = 0;

    do
    {
        switch( enParseState )
        {
            case GET_DRIVER_PREFIX:
                if (*pcData == '-' )
                {
                    /* end of driver prefix - search for REL string */
                    enParseState = GET_REL_STRING;
                }
                break;
                
            case GET_REL_STRING:
                if ( bStrncmp( pcData, "REL_", 4 ) != TRUE )
                {
                    /* ERROR - illegal format string */
                    return( FALSE );
                }

                pcData += 3;  /* 4th character skipped at bottom of the main loop */
                enParseState = GET_MAJOR_REVISION;
                break;
                
            case GET_MAJOR_REVISION:
                if ( bIsNum( *pcData, &u32Value ) == TRUE )
                {
                    /* numeric value */
                    *pu32MajorRevision *= 10;
                    *pu32MajorRevision += u32Value;
                }
                else if ( *pcData == '.' )
                {
                    /* major revision terminator */
                    enParseState = GET_MINOR_REVISION;
                }
                else
                {
                    /* ERROR - illegal format string */
                    return( FALSE );
                }
                break;
                
            case GET_MINOR_REVISION:
                if ( bIsNum( *pcData, &u32Value ) == TRUE )
                {
                    /* numeric value */
                    *pu32MinorRevision *= 10;
                    *pu32MinorRevision += u32Value;
                }
                else if ( *pcData == '.' )
                {
                    /* minor revision terminator */
                    enParseState = GET_PATCH_REVISION;
                }
                else
                {
                    /* ERROR - illegal format string */
                    return( FALSE );
                }
                break;
                
            case GET_PATCH_REVISION:
                if ( bIsNum( *pcData, &u32Value) == TRUE )
                {
                    /* numeric value */
                    *pu32PatchRevision *= 10;
                    *pu32PatchRevision += u32Value;
                }
                else if (( *pcData == 0 ) || ( *pcData == 0x20 ))
                {
                    /* assume the end of the option string has now been reached */
                    bComplete = TRUE; 
                }
                else
                {
                    /* non numeric character - illegal format string */
                    return( FALSE );
                }
                break;

            default:
                /* ERROR - this case should never occur */
                return( FALSE );
        }

        /* move to next character in revision string */
        pcData++;
    } while( bComplete == FALSE );

    return( TRUE );
}   /* end of ST_GetRevision() */



/********************************************************************
 *                                                                  *
 * Function:    ST_GetCutRevision                                   *
 *                                                                  *
 * Input:       None                                                *
 *                                                                  *
 * Output:      None                                                *
 *                                                                  *
 * Returns:     Device Cut number (Currently implemented for 5100,  *
 *              5105,5107,5528,7100,7109,7200)                      *
 *                                                                  *
 ********************************************************************/
U32 ST_GetCutRevision(void)
{

 U32 ChipVersion=0;

 /* For 5100 */
 /* -------- */
#if defined(ST_5100)
 {
  ChipVersion = STSYS_ReadRegDev32LE(0x2068001C);
  ChipVersion = ((ChipVersion&0xFF)+0x90);
 }
#endif

 /* For 5105 */
 /* -------- */
#if defined(ST_5105)
 {
  device_id_t DeviceId;
  DeviceId    = device_id();
  ChipVersion = DeviceId.id;
  ChipVersion = ((((ChipVersion>>28)&0x0F)-1)*0x10)+0xA0;
 }
#endif

 /* For 5107 */
 /* -------- */
#if defined(ST_5107)
 {
  device_id_t DeviceId;
  DeviceId    = device_id();
  ChipVersion = DeviceId.id;
  ChipVersion = ((((ChipVersion>>28)&0x0F)-1)*0x10)+0xA0;
 }
#endif

 /* For 5528 */
 /* -------- */
#if defined(ST_5528)
#if defined(ST_OS20)
 {
  device_id_t DeviceId;
  DeviceId    = device_id();
  ChipVersion = DeviceId.id;
  ChipVersion = ((((ChipVersion>>28)&0x0F)-1)*0x10)+0xA0;
 }
#else
 ChipVersion = STSYS_ReadRegDev32LE(ConfigBaseAddress_p+0x000/*DEVICE_ID*/);
 ChipVersion = ((((ChipVersion>>28)&0x0F))*0x10)+0x90;
#endif
#endif

 /* For 7100 */
 /* -------- */
#if defined(ST_7100)
  #if defined(ST_OSLINUX) && !defined(MODULE)
    /* Not available in user mode */
	return ChipVersion;
  #else
  ChipVersion = STSYS_ReadRegDev32LE(ConfigBaseAddress_p+0x000/*DEVICE_ID*/);
  ChipVersion = ((((ChipVersion>>28)&0x0F))*0x10)+0xA0;
  if (ChipVersion==0xC0)
   {
    U32 Value,MetalLayer,LotId;
    Value      = STSYS_ReadRegDev32LE(ST7100_CFG_BASE_ADDRESS+0x014/*SYS_STA3*/);
    MetalLayer = (Value&0xC0000000)>>30;
    Value      = STSYS_ReadRegDev32LE(ST7100_CFG_BASE_ADDRESS+0x01C/*SYS_STA5*/);
    MetalLayer = MetalLayer|((Value&0x2)<<1);
    Value      = STSYS_ReadRegDev32LE(ST7100_CFG_BASE_ADDRESS+0x014/*SYS_STA3*/);
    LotId      = (Value&0x00007FFF)<<3;
    Value      = STSYS_ReadRegDev32LE(ST7100_CFG_BASE_ADDRESS+0x010/*SYS_STA2*/);
    LotId      = LotId|(Value>>29);
    if (MetalLayer==0x0)
     {
      if (LotId==0x73EF) MetalLayer=0x1;
      if (LotId==0xBCD7) MetalLayer=0x4;
     }
    ChipVersion|=MetalLayer;
   }
  #endif
#endif


 /* For 7109 */
 /* -------- */
#if defined(ST_7109)
  #if defined(ST_OSLINUX) && !defined(MODULE)
    /* Not available in user mode */
	return ChipVersion;
  #else
  {
  U32 MetalLayer=0;
  ChipVersion = STSYS_ReadRegDev32LE(ConfigBaseAddress_p+0x000/*DEVICE_ID*/);
  ChipVersion = ((((ChipVersion>>28)&0x0F))*0x10)+0xA0;
  MetalLayer  = STSYS_ReadRegDev32LE(ConfigBaseAddress_p+0x02C/*SYS_STA9*/);
  MetalLayer  = MetalLayer&0xF;
  ChipVersion = ChipVersion|MetalLayer;
  }
  #endif
#endif


 /* For 7200 */
 /* -------- */
#if defined(ST_7200)
  #if defined(ST_OSLINUX) && !defined(MODULE)
    /* Not available in user mode */
 return ChipVersion;
  #else
  {
  ChipVersion = STSYS_ReadRegDev32LE(ConfigBaseAddress_p+0x000/*DEVICE_ID*/);
  if ((ChipVersion & 0x0fffffff) == 0x0D437041)
  {
    ChipVersion = ((((ChipVersion>>28)&0x0F))*0x10)+0xA0;
  }
  else
  {
    U32 pvr, prr, composite;
    pvr = STSYS_ReadRegDev32LE(0xff000030);
    prr = STSYS_ReadRegDev32LE(0xff000044);
    composite = (pvr & 0xffff0000) | (prr & 0x0000ffff);
    if ((composite & 0xffffff00) == 0x04069500)
    {
      ChipVersion = 0xA0; /* device_id is not working on cut 1.0. */
    }
    if ((composite & 0xffffff00) == 0x04909500)
    {
      ChipVersion = 0xA1; /* we assume that the GUMP is cut A1 */
    }
  }
  }
  #endif
#endif


 return(ChipVersion);
}


#if defined(ST_OSLINUX) && defined(MODULE)
EXPORT_SYMBOL(ST_GetCutRevision);
#endif



/*
 Implementation of external API ST_partition_*

 We should use ST_partition_t instead of void, but it does not compile

 Following code specific to WINCE and not compile in OS21 - So flag it !! */
#ifdef ST_OSWINCE

ST_Partition_t* ST_partition_create(void* memory, size_t size)
{
    return partition_create_heap(memory, size);
}

void ST_partition_delete(ST_Partition_t* partition)
{
    partition_delete_heap(partition);
}

void * ST_partition_malloc(ST_Partition_t* partition,unsigned int size)
{
    return memory_allocate(partition,size);
}

void ST_partition_free(ST_Partition_t* partition,void *ptr)
{
    memory_deallocate(partition,ptr);
}


ST_ErrorCode_t ST_partition_status(ST_Partition_t* partition, ST_partition_status_t* status)
{
    partition_status_t actual_status;
    
    if (partition_status(partition, &actual_status, 0) != OS21_SUCCESS)
        return ST_ERROR_BAD_PARAMETER;
    status->partition_status_size = actual_status.partition_status_size;
    status->partition_status_free = actual_status.partition_status_free;
    status->partition_status_free_largest = actual_status.partition_status_free_largest;
    status->partition_status_used = actual_status.partition_status_used;
    return OS21_SUCCESS;
}



ST_ErrorCode_t ST_SetOption(int code,...)
{
	va_list args;
	va_start(args, code);

	switch(code)
	{


#ifdef ST_OSWINCE

	case ST_SET_WINCE_SHARABLEMEMORY:
		{
			void *pBase = va_arg(args,void *);
			unsigned long size  = va_arg(args,unsigned long);
			WCE_SetSharableMemoryPartition(pBase,size);
			_WCE_MemoryInitilize();

      break;
		}

	case ST_INIT_WINCE_SHARABLEMEMORY:
		{
			_WCE_MemoryInitilize();
      break;
		}

	case ST_ENUM_CE_TASKS_NAME:
		{
			int index = va_arg(args,int);
			 char **pName = va_arg(args,char **);
			 *pName = _WCE_EnumStapiTaskName(index);
			 if(pName == 0)  return OS21_FAILURE;
			 break;

		}

	case ST_SET_CE_TASKS_PRIORITY:
		{
			int ThreadIndex   = va_arg(args,int );
			int priority	 = va_arg(args,int);
			if(!_WCE_SetCeTaskPriority(ThreadIndex,priority)) return OS21_FAILURE;
			 break;

		}

	case ST_GET_CE_TASKS_PRIORITY:
		{
			int ThreadIndex   = va_arg(args,int );
			int *priority	  = va_arg(args,int*);
			HANDLE  **pHandle  = va_arg(args,HANDLE**); // can be null
			if(priority == 0) return OS21_FAILURE;
			*priority = _WCE_GetCeTaskPriority(ThreadIndex,pHandle);
			if(*priority == -1) return OS21_FAILURE;
			 break;

		}

	case ST_ENUM_CE_IST_NAME:
		{
			int index = va_arg(args,int);
			 char **pName = va_arg(args,char **);
			 *pName = _WCE_EnumStapiISTName(index);
			 if(pName == 0)  return OS21_FAILURE;
			 break;


		}


	case ST_SET_CE_IST_PRIORITY:
		{
			int index       = va_arg(args,int );
			int priority	 = va_arg(args,int);
			if(!_WCE_SetCeISTPriority(index,priority)) return OS21_FAILURE;
			 break;

		}


	case ST_GET_CE_IST_PRIORITY:
		{
			int index   = va_arg(args,int);
			int *priority	 = va_arg(args,int*);
			if(priority == 0) return OS21_FAILURE;

			*priority = _WCE_GetCeISTPriority(index);
			if(*priority == -1) return OS21_FAILURE;
			 break;

		}





#endif



	default:
		return OS21_FAILURE;
	}
	va_end(args);

    return OS21_SUCCESS;

}

#endif /*ST_OSWINCE*/


/* End of stcommon.c */
