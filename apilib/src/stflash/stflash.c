/****************************************************************************

File Name   : stflash.c

Description : Flash Memory API Routines to support Direct Access to
              one or more banks of flash memory.

Supported parallel FLASH devices:
- M28F411
- M29W800T
- M29W160BT
- M29DW640D
- M29W320DT
- AM29LV160D
- AT49BV162AT
- E28F640
- M28W320CB
- M28W320FS
- M28W640FS
- M28W320FSU
- M28W640FSU
- M58LW032
- M58LW064D
- M58LT128GS:
- M58LT256GS: 

Supported serial FLASH devices:
- M25P10
- M25P80
- M25P16

History  :

14/01/05 Added STFLASH_BlockLock()and STFLASH_BlockUnlock() APIs

03/02/96 Added STFLASH_SetSPIClkDiv() and STFLASH_SetSPIConfig() APIs
         Added serial flash support
         
24/04/07 Added STFLASH_SetSPIModeSelect() and STFLASH_SetSPIDynamicMode() APIs for
		 SPIBOOT block
		 Added STFLASH_GetCFI API
Copyright (C) 2005, ST Microelectronics

References  :

$ClearCase (VOB: stflash)

API.PDF "Flash Memory API" Reference DVD-API-004 Revision 2.1

 ****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <string.h>
#ifndef ST_OS21
#include <ostime.h>
#endif
#include "stlite.h"
#include "stddefs.h"
#include "stos.h"

#include "stflash.h"
#include "stflashd.h"
#include "stcommon.h"
#include "stflashdat.h"
#include "hal.h"
#include "stpio.h"
#if defined(STFLASH_SPI_SUPPORT)
#include "stspi.h"
#endif
#include <stdio.h>

/* Private Constants ------------------------------------------------------ */
#define MAGIC_NUMBER    		0x31415927                  /* easy as pi(e) */
#define MASK_BANK_NUMBER 		0x000000FF
#define BANK_NUMBER_IDENITFIER	0xFFFFFF00
/* Private Variables ------------------------------------------------------ */

static semaphore_t      *Atomic_p = NULL;
static stflash_Inst_t   *stflash_Sentinel = NULL; /* ptr. to first node */

/* Driver revision number */

/* Private Macros --------------------------------------------------------- */
/* these are static because we DON'T want them used elsewhere! */
static ST_ErrorCode_t GetDeviceInfo( U32 BaseAddress,
                                     STFLASH_DeviceType_t DeviceType,
                                     U32 AccessWidth,
                                     U32 *ManufactCode,
                                     U32 *DeviceCode
                                      );

#if defined(STFLASH_MULTIBANK_SUPPORT)
static ST_ErrorCode_t GetBankNoFromOffset( stflash_Inst_t *ThisElem_p,
                                           U32  Offset,
                                           U32 *BankNumber);

#endif

ST_ErrorCode_t SetFunctions( stflash_Inst_t *ThisElem_p);

#if defined(STFLASH_SPI_SUPPORT)
static S32 GetPioPin( U8 Value );
#endif

BOOL flash_get_cfi(STFLASH_CFI_Query_t *CFI, U32* FlashBaseAddress );

/* Functions -------------------------------------------------------------- */

/****************************************************************************
Name         : STFLASH_GetRevision()

Description  : Returns a pointer to the Driver Revision String.
               May be called at any time.

Parameters   : None

Return Value : ST_Revision_t

See Also     : ST_Revision_t
 ****************************************************************************/

ST_Revision_t STFLASH_GetRevision( void )
{
  	return((ST_Revision_t)STFLASH_REVISION);
}/* STFLASH_GetRevision */


/****************************************************************************
Name         : STFLASH_Init()

Description  : Generates a linked list element for each instance called,
               into which the caller's InitParams are copied.  Also
               attempts to Read the Electronic Signature to identify
               the device, which is returned in STFLASH_Params_t when
               GetParams() is called.

Parameters   : ST_DeviceName_t Name of the Flash bank, and a pointer to
               STFLASH_InitParams_t InitParams data structure.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors occurred
               ST_ERROR_NO_MEMORY              Insufficient memory free
               ST_ERROR_ALREADY_INITIALISED    Bank already initialised
               ST_ERROR_BAD_PARAMETER          Error in parameters passed

See Also     : STFLASH_InitParams_t
               STFLASH_GetParams()
               STFLASH_Term()
 ****************************************************************************/

ST_ErrorCode_t STFLASH_Init( const ST_DeviceName_t      Name,
                             const STFLASH_InitParams_t *InitParams )
{
    stflash_Inst_t  *NextElem, *PrevElem = NULL;          /* search list pointers */
    stflash_Inst_t  *ThisElem;                     /* element for this Init */

    U32             LocalAccessMask = 0;
    U32             DeviceCode = 0;
    U32             ManufactCode = 0;
    U32             StructSize = 0;
    U32             i = 0;

#if defined(STFLASH_MULTIBANK_SUPPORT)
    U32 BankStartOffset =0, BankEndOffset =0, BankNextStartOffset =0;
    U32 count = 0;
    U32 j = 0;
#endif

#if defined(STFLASH_SPI_SUPPORT)
    STPIO_OpenParams_t  PIOOpenParams;
    STPIO_Handle_t      PIOHandle;
    STSPI_OpenParams_t  OpenParams;
    STSPI_Handle_t      Handle;
#endif
    ST_ErrorCode_t  RetVal = ST_NO_ERROR;

    /* Perform necessary validity checks */
    if ((Name == NULL)                       ||    /* NULL address for Name? */
        (Name[0] == '\0')                    ||    /* NUL Name string? */
        (strlen( Name ) >=
                    ST_MAX_DEVICE_NAME )     ||    /* Name string too long? */
        (InitParams == NULL )                ||    /* NULL address for InitParams? */
        (InitParams->DriverPartition == NULL)||    /* NULL address for partition */
        (InitParams->MinAccessWidth >
            InitParams->MaxAccessWidth))           /* Minimum > Maximum? */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
#if defined(STFLASH_SPI_SUPPORT)
    if (InitParams->IsSerialFlash == FALSE)
#endif
    {
        LocalAccessMask =
                InitParams->MaxAccessWidth - 1;       /* legality checked later */

#if !defined(ST_8010) && !defined(ST_5525)
        if ((InitParams->BaseAddress == NULL)   ||    /* BaseAddress omitted? */
           (((U32)InitParams->BaseAddress & LocalAccessMask ) != 0)) /* BaseAddress misalign? */
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
#endif

        if((((U32)InitParams->VppAddress &
              LocalAccessMask ) != 0)           ||    /* VppAddress misalign? */
           (InitParams->NumberOfBlocks == 0)    ||    /* no block definition? */
           (InitParams->Blocks == NULL))              /* Blocks data ptr omitted? */
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
    }
    /* Check DeviceType is supported */
    switch (InitParams->DeviceType)
    {
        case STFLASH_M58LW032:
        case STFLASH_M58LT128GS:
        case STFLASH_M58LT256GS: 
            switch (InitParams->MinAccessWidth)
            {
                case STFLASH_ACCESS_08_BITS:        /* no break */
                case STFLASH_ACCESS_16_BITS:        /* no break */
                    break;

                default:                            /* unsupported max width */
                    return(ST_ERROR_BAD_PARAMETER);
            }
            break;

        case STFLASH_M58LW064D:
            switch (InitParams->MinAccessWidth)
            {
                case STFLASH_ACCESS_08_BITS:        /* no break */
                case STFLASH_ACCESS_16_BITS:        /* no break */
                case STFLASH_ACCESS_32_BITS:
                break;

                default:                            /* unsupported max width */
                    return(ST_ERROR_BAD_PARAMETER);
            }
            break;

        case STFLASH_M28W320CB:
        case STFLASH_M28W320FS:  /* KRYPTO secured FLASH */
        case STFLASH_M28W640FS:  /* KRYPTO secured FLASH */
        case STFLASH_M28W320FSU: /* KRYPTO secured FLASH of uniform block size */
        case STFLASH_M28W640FSU: /* KRYPTO secured FLASH of uniform block size */
        case STFLASH_E28F640:
        case STFLASH_M28F411:
            /* Check min access width is supported for this device */
            switch (InitParams->MinAccessWidth)
            {
                case STFLASH_ACCESS_08_BITS:        /* no break */
                case STFLASH_ACCESS_16_BITS:        /* no break */
                case STFLASH_ACCESS_32_BITS:
                    break;

                default:                            /* unsupported max width */
                    return(ST_ERROR_BAD_PARAMETER);
            }
            break;

        case STFLASH_AM29LV160D:
        case STFLASH_M29W800T:
        case STFLASH_M29W160BT:
        case STFLASH_M29W640D:
        case STFLASH_AT49BV162AT:
        case STFLASH_M29W320DT:
            /* Check min access width is supported for this device */
            switch (InitParams->MinAccessWidth)
            {
                case STFLASH_ACCESS_16_BITS:
                case STFLASH_ACCESS_32_BITS:
                    break;

                default:                            /* unsupported max width */
                  return(ST_ERROR_BAD_PARAMETER);
            }
            break;

        case STFLASH_M25P10:
        case STFLASH_M25P80:
        case STFLASH_M25P16:
            /* no min Access width */
            break;

        default:                                    /* unsupported device */
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#if defined(STFLASH_SPI_SUPPORT)
    if (InitParams->IsSerialFlash == FALSE)
#endif
    {

        /* commence Read Electronic Signature Command Sequence, storing
           the Device and Manufacturer Codes appropriate for MaxAccessWidth */
        RetVal = GetDeviceInfo( (U32)InitParams->BaseAddress,
                            InitParams->DeviceType,
                            InitParams->MaxAccessWidth,
                            &ManufactCode,
                            &DeviceCode);

        if (RetVal != ST_NO_ERROR)
        {
            return RetVal;
        }
        /* No flash device connected */
        if((ManufactCode == 0x00) && (DeviceCode == 0x00))
        {
        	return STFLASH_ERROR_DEVICE_NOT_PRESENT;
        }
    }

     StructSize = sizeof( stflash_Inst_t ) +    /* determine structure size */
                        ( ( InitParams->NumberOfBlocks - 1 ) *
                          sizeof( STFLASH_Block_t ) );


    /* The following section is protected from re-entrancy whilst critical
       elements within the stflash_Sentinel linked list are both read and
       written.  It is not likely that Init will be called from a multi-
       tasking environment, but one never knows what a user might try.... */

    if (stflash_Sentinel == NULL)             /* first Init instance? */
    {
       /* initialize semaphore */
        Atomic_p = STOS_SemaphoreCreateFifo(NULL,1);
        STOS_SemaphoreWait( Atomic_p );             /* atomic sequence start */

        stflash_Sentinel = ThisElem =          /* keep a note of result */
            ( stflash_Inst_t* )STOS_MemoryAllocate(
              InitParams->DriverPartition, StructSize );
        memset (ThisElem, 0x00 , StructSize );              

#if defined(STFLASH_MULTIBANK_SUPPORT)
     ThisElem->OpenInfo = (STFLASH_OpenInfo_t *)STOS_MemoryAllocate(
                                InitParams->DriverPartition,
                                sizeof(STFLASH_OpenInfo_t) * (InitParams->NumberOfBanks - 1));   
     ThisElem->BankInfo = (STFLASH_BankParams_t *)STOS_MemoryAllocate(
                                InitParams->DriverPartition,
                                sizeof(STFLASH_BankParams_t) * (InitParams->NumberOfBanks - 1));                                             			    
                    
#endif              


    }
    else                                       /* already Init'ed */
    {
        STOS_SemaphoreWait( Atomic_p );             /* atomic sequence start */

        for (NextElem = stflash_Sentinel;      /* start at Sentinel node */
             NextElem != NULL;                 /* while not at list end */
             NextElem = NextElem->Next)       /* advance to next element */
        {
            if (strcmp( Name,
                NextElem->BankName) == 0)    /* existing DeviceName? */
            {
                STOS_SemaphoreSignal( Atomic_p );   /* unlock before exit */

                return ST_ERROR_ALREADY_INITIALIZED;
            }

            PrevElem = NextElem;               /* keep trailing pointer */
        }

        PrevElem->Next = ThisElem =            /* keep note of result */
            ( stflash_Inst_t* )STOS_MemoryAllocate(
                InitParams->DriverPartition, StructSize );
#if defined(STFLASH_MULTIBANK_SUPPORT)
     ThisElem->OpenInfo = (STFLASH_OpenInfo_t *)STOS_MemoryAllocate(
                                InitParams->DriverPartition,
                                sizeof(STFLASH_OpenInfo_t) * (InitParams->NumberOfBanks - 1));   
     ThisElem->BankInfo = (STFLASH_BankParams_t *)STOS_MemoryAllocate(
                                InitParams->DriverPartition,
                                sizeof(STFLASH_BankParams_t) * (InitParams->NumberOfBanks - 1));                                             			    
                    
#endif                   
    }

    if (ThisElem == NULL)                     /* STOS_MemoryAllocate failure? */
    {
        STOS_SemaphoreSignal( Atomic_p );           /* unlock before exit */

        return(ST_ERROR_NO_MEMORY);
    }


    ThisElem->Next = NULL;                     /* mark as end of list */

    strcpy( ThisElem->BankName, Name );        /* retain Name for Opens */

    STOS_SemaphoreSignal( Atomic_p );               /* end of atomic region */

#if defined(STFLASH_SPI_SUPPORT)

    if ( InitParams->IsSerialFlash == TRUE )
    {
	/* CS open */
        PIOOpenParams.IntHandler            =  NULL;
        PIOOpenParams.BitConfigure[GetPioPin(InitParams->PIOforCS.BitMask)] = STPIO_BIT_OUTPUT_HIGH;
        PIOOpenParams.ReservedBits          =  InitParams->PIOforCS.BitMask;
        RetVal =  STPIO_Open ( InitParams->PIOforCS.PortName,&PIOOpenParams, &PIOHandle );
        if (RetVal != ST_NO_ERROR)
        {
            return RetVal;
        }

    	strcpy(OpenParams.PIOforCS.PortName, InitParams->PIOforCS.PortName);
        OpenParams.PIOforCS.BitMask = InitParams->PIOforCS.BitMask;

        RetVal = STSPI_Open(InitParams->SPIDeviceName, &OpenParams, &Handle );
        if (RetVal != ST_NO_ERROR)
        {
            return RetVal;
        }
        /* copy initparams into control blk */
        ThisElem->IsSerialFlash   = TRUE;
    	ThisElem->DeviceCode      = 0;  /* stored for GetParams() */
        ThisElem->ManufactCode    = 0;  /* stored for GetParams() */
        ThisElem->VppAddress      = NULL;
        ThisElem->MinAccessWidth  = 0;
        ThisElem->MaxAccessWidth  = 0;
        ThisElem->MinAccessMask   = 0;
        ThisElem->MaxAccessMask   = 0;
        ThisElem->SPIHandle       = Handle;
        ThisElem->SPICSBit        = InitParams->PIOforCS.BitMask;
        ThisElem->CSHandle        = PIOHandle;
        ThisElem->IsFastRead      = FALSE;
    }
    else
#endif
    {
    	/* copy initparams into control blk */
#if defined(STFLASH_SPI_SUPPORT)
    	ThisElem->IsSerialFlash   = FALSE;
#endif
    	ThisElem->DeviceCode      = DeviceCode;    /* stored for GetParams() */
        ThisElem->ManufactCode    = ManufactCode;  /* stored for GetParams() */
        if ((InitParams->DeviceType == STFLASH_E28F640)    ||
           (InitParams->DeviceType  == STFLASH_M28W320CB)   ||
           (InitParams->DeviceType  == STFLASH_M28W320CB))
        {
            ThisElem->VppAddress  = NULL;
        }
        else
        {
            ThisElem->VppAddress  = InitParams->VppAddress;
        }

        ThisElem->MinAccessWidth  = InitParams->MinAccessWidth;
        ThisElem->MaxAccessWidth  = InitParams->MaxAccessWidth;
        ThisElem->MinAccessMask   = InitParams->MinAccessWidth - 1;
        ThisElem->MaxAccessMask   = LocalAccessMask;
#if defined(STFLASH_SPI_SUPPORT)
        ThisElem->SPIHandle       = 0;
        ThisElem->SPICSBit        = 0;
    	ThisElem->CSHandle        = 0;
#endif
    }
    /* common params for both serial/parallel flash */
    ThisElem->MagicNumber     = MAGIC_NUMBER;  /* flag instance as valid */
    ThisElem->BankOpen        = FALSE;         /* not opened yet */
    ThisElem->LastOffsP1      = 0;             /* calculated below */
    ThisElem->DeviceType      = InitParams->DeviceType;
    ThisElem->BaseAddress     = InitParams->BaseAddress;
    ThisElem->NumberOfBlocks  = InitParams->NumberOfBlocks;
    ThisElem->DriverPartition = InitParams->DriverPartition;


    /* InitParams->Blocks points to the caller's structure array,
    STFLASH_Block_t[].  Whilst its contents are copied into
    the current instance, the last address in the bank is
    calculated.  This simplifies the testing of subsequent
    Read/Write calls for legality of Address (offset)
    and NumberToRead/Write parameters.*/

    for (i = 0; i < ThisElem->NumberOfBlocks; i++)
    {
        ThisElem->Blocks[i]   = InitParams->Blocks[i];
        ThisElem->LastOffsP1 += InitParams->Blocks[i].Length;
    }
#if defined(STFLASH_MULTIBANK_SUPPORT)
    ThisElem->NumberOfBanks   = InitParams->NumberOfBanks;

    for(i = 0 ; i < ThisElem->NumberOfBanks; i++)
    {
       ThisElem->BankInfo[i].NoOfBlocksInBank = InitParams->BankInfo[i].NoOfBlocksInBank;

       for(j = 0; j < ThisElem->BankInfo[i].NoOfBlocksInBank; j++ )
       {
           BankEndOffset += ThisElem->Blocks[count].Length;
           count++;
       }
       BankNextStartOffset = BankEndOffset;
       ThisElem->OpenInfo[i].BankEndOffset   = BankEndOffset - 1;
       ThisElem->OpenInfo[i].BankStartOffset = BankStartOffset;
       ThisElem->OpenInfo[i].FlashBankOpen   = FALSE;  /* not opened yet */
       						BankStartOffset  = BankNextStartOffset;
    }

#endif/*STFLASH_MULTIBANK_SUPPORT*/

#if defined(STFLASH_SPI_SUPPORT)
    if ( InitParams->IsSerialFlash == FALSE )
#endif
    {
        RetVal = SetFunctions( ThisElem );
    }


    return(RetVal);

}/* STFLASH_Init */

/****************************************************************************
Name         : STFLASH_Open()

Description  : Searches the linked list of Init instances for a string
               match with Device/Bank name.   The address of the matching
               entry is returned as the Handle, simplifying subsequent
               accesses.

Parameters   : ST_DeviceName_t Name is the Init supplied Flash Bank name,
               STFLASH_OpenParams_t *OpenParams is a void *, and
               STFLASH_Handle_t *Handle is written through to the caller.

Return Value : ST_ErrorCode_t specified as
               ST_ERROR_UNKNOWN_DEVICE     Bank name not found
               ST_ERROR_BAD_PARAMETER       Bad parameter detected
               ST_ERROR_NO_FREE_HANDLES    Already open
               ST_NO_ERROR                 No errors occurred

See Also     : STFLASH_Handle_t
               STFLASH_Close()
 ****************************************************************************/

ST_ErrorCode_t STFLASH_Open( const ST_DeviceName_t       Name,
                             const STFLASH_OpenParams_t  *OpenParams,
                             STFLASH_Handle_t            *Handle )
{
    stflash_Inst_t  *ThisElem;
    ST_ErrorCode_t RetValue = ST_NO_ERROR;

#if defined(STFLASH_MULTIBANK_SUPPORT)
    U32 BankNo = 0;
    BankNo = OpenParams->BankNumber;
    U32 Address = 0;
#else
    UNUSED_PARAMETER(OpenParams);
#endif
    /* Perform parameter validity checks */

    if ((Name   == NULL)   ||               /* Name   ptr uninitialised? */
        (Handle == NULL))                  /* Handle ptr uninitialised? */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* One previous call to Init MUST have had the same Device Name */

    for (ThisElem = stflash_Sentinel;           /* start at Sentinel node */
         ThisElem != NULL;                      /* while not at list end */
         ThisElem = ThisElem->Next)            /* advance to next element */
    {
        if (strcmp( Name,
            ThisElem->BankName) == 0)     /* existing DeviceName? */
        {
            break;                              /* match, terminate search*/
        }
    }
    if (ThisElem == NULL)                      /* no Name match found? */
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

#if defined(STFLASH_MULTIBANK_SUPPORT)
    if(ThisElem ->OpenInfo[BankNo].FlashBankOpen == TRUE)
    {
        return(ST_ERROR_NO_FREE_HANDLES);
    }
    else
    {
    	 Address = ( STFLASH_Handle_t )ThisElem;
    	 
    	/*Handles assigned to different banks differ in lower nibble which is the bank number here*/

    	 *Handle  = (Address & BANK_NUMBER_IDENITFIER) | BankNo ;
    	 ThisElem ->OpenInfo[BankNo].Handle = *Handle;
         ThisElem ->OpenInfo[BankNo].FlashBankOpen = TRUE;
    }

#else/*!STFLASH_MULTIBANK_SUPPORT*/

    if (ThisElem->BankOpen)                    /* already open? */
    {
        return(ST_ERROR_NO_FREE_HANDLES);
    }

    ThisElem->BankOpen = TRUE;                  /* flag as open */
    *Handle = ( STFLASH_Handle_t )ThisElem;     /* write back Handle */

#endif
    return(RetValue);

}/* STFLASH_Open */

/****************************************************************************
Name         : STFLASH_BlockLock()

Description  : This function locks all the blocks.

Parameters   : STFLASH_Handle_t Handle identifies the bank,
               U32 Offset (from device BaseAddress).

Return Value : ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Rogue Flash bank indicator
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_TIMEOUT            Timeout error during write
               STFLASH_ERROR_WRITE         Error during write
               STFLASH_ERROR_VPP_LOW       Low Vpp detected during write

****************************************************************************/

ST_ErrorCode_t STFLASH_BlockLock( STFLASH_Handle_t Handle,
                                  U32              Offset)
{

    stflash_Inst_t  *ThisElem;
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;
    BOOL ParamsValid = FALSE;       /* initial assumption */
    U32  CurrOffset = 0;            /* start at BaseAddress */
    HAL_LockUnlockParams_t LockParams;
    U32 i = 0;

#if defined(STFLASH_MULTIBANK_SUPPORT)
 
    U32 BankNo = 0;
    U32 BankNumberFromHandle = 0;
    
    if(!HandleIsValid(Handle,&ThisElem))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    
    BankNumberFromHandle = Handle & MASK_BANK_NUMBER;
    
    RetValue = GetBankNoFromOffset(ThisElem,Offset,&BankNo);
    if (RetValue != ST_NO_ERROR)
    {
        return RetValue;
    }
    if(BankNo != BankNumberFromHandle)
    {
    	return (ST_ERROR_BAD_PARAMETER);
    }
#else
    ThisElem = (stflash_Inst_t *)Handle;
    if (!HandleIsValid(Handle))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
#endif
#if defined(STFLASH_SPI_SUPPORT)
    if (ThisElem->IsSerialFlash == TRUE)
    {

    	 RetValue = HAL_SPI_Lock( Handle, Offset );
    }
    else
#endif
    {
        /* walk through STFLASH_Block_t array checking
        that both Offset and NumberToErase are legal */
        for (i = 0; i < ThisElem->NumberOfBlocks; i++)
        {
            if (CurrOffset == Offset)       /* length match? */
            {
                ParamsValid = TRUE;
                break;                                  /* stop searching */
            }

            CurrOffset += ThisElem->Blocks[i].Length;   /* next block */
        }

        if (!ParamsValid)                   /* start/length mismatch block? */
        {
            return(ST_ERROR_BAD_PARAMETER);
        }

        LockParams.BaseAddress    = ThisElem->BaseAddress;
        LockParams.MaxAccessWidth = ThisElem->MaxAccessWidth;
        LockParams.VppAddress     = ThisElem->VppAddress;

        RetValue = HAL_Lock( ThisElem, LockParams, Offset );
    }

    return(RetValue);

} /* STFLASH_BlockLock */

/****************************************************************************
Name         : STFLASH_BlockUnlock()

Description  : This function unlocks all the blocks if they are locked.

Description  : This function locks all the blocks.

Parameters   : STFLASH_Handle_t Handle identifies the bank,
               U32 Offset (from device BaseAddress).

Return Value : ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Rogue Flash bank indicator
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_TIMEOUT            Timeout error during write
               STFLASH_ERROR_WRITE         Error during write
               STFLASH_ERROR_VPP_LOW       Low Vpp detected during write

*****************************************************************************/

ST_ErrorCode_t STFLASH_BlockUnlock( STFLASH_Handle_t Handle,
                                    U32              Offset)
{

    stflash_Inst_t  *ThisElem;
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;
    BOOL ParamsValid = FALSE;       /* initial assumption */
    U32  CurrOffset = 0;            /* start at BaseAddress */
    HAL_LockUnlockParams_t UnlockParams;
    U32 i = 0;
    /* Perform necessary validity checks */
#if defined(STFLASH_MULTIBANK_SUPPORT)
 
    U32 BankNo = 0;
    U32 BankNumberFromHandle = 0;
    if(!HandleIsValid(Handle,&ThisElem))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    
    BankNumberFromHandle = Handle & MASK_BANK_NUMBER;
    
    RetValue = GetBankNoFromOffset(ThisElem,Offset,&BankNo);
    if (RetValue != ST_NO_ERROR)
    {
        return RetValue;
    }
    if(BankNo != BankNumberFromHandle)
    {
    	return (ST_ERROR_BAD_PARAMETER);
    }
#else
    ThisElem = (stflash_Inst_t *)Handle;
    if (!HandleIsValid(Handle))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
#endif

#if defined(STFLASH_SPI_SUPPORT)
    if (ThisElem->IsSerialFlash == TRUE )
    {
    	RetValue = HAL_SPI_Unlock( Handle, Offset );
    }
    else
#endif
    {
        /* walk through STFLASH_Block_t array checking
           that both Offset and NumberToErase are legal */
       for (i = 0; i < ThisElem->NumberOfBlocks; i++)
        {
            if (CurrOffset == Offset)       /* length match? */
            {
                ParamsValid = TRUE;
                break;                                  /* stop searching */
            }

            CurrOffset += ThisElem->Blocks[i].Length;   /* next block */
        }

        if (!ParamsValid)                   /* start/length mismatch block? */
        {
            return(ST_ERROR_BAD_PARAMETER);
        }

        UnlockParams.BaseAddress    = ThisElem->BaseAddress;
        UnlockParams.MaxAccessWidth = ThisElem->MaxAccessWidth;
        UnlockParams.VppAddress     = ThisElem->VppAddress;

        RetValue = HAL_Unlock( ThisElem, UnlockParams, Offset );
    }
    return(RetValue);

}/* STFLASH_BlockUnlock */

/****************************************************************************
Name         : STFLASH_GetParams()

Description  : Returns information on the specified Flash bank, perloined
               from the Init call.

Parameters   : STFLASH_Handle_t Handle points to the Init instance, plus
               STFLASH_GetParams_t *GetParams structure pointer to which
               information is written through.

Return Value : ST_ErrorCode_t specified as
               ST_ERROR_INVALID_HANDLE
               ST_ERROR_BAD_PARAMETER      Bad parameter detected
               ST_NO_ERROR                 No errors occurred

See Also     : STFLASH_Params_t
               STFLASH_Init()
 ****************************************************************************/

ST_ErrorCode_t STFLASH_GetParams( STFLASH_Handle_t Handle,
                                  STFLASH_Params_t *GetParams )
{
    U32             i;
    stflash_Inst_t  *ThisElem;
    STFLASH_Block_t *Callers_Block_s;
    
#if defined(STFLASH_MULTIBANK_SUPPORT)
    if( !HandleIsValid(Handle,&ThisElem))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
   
#else
    ThisElem = (stflash_Inst_t *)Handle;
    if (!HandleIsValid(Handle))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
#endif
    if (GetParams == NULL)                   /* has caller forgotten ptr? */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    GetParams->InitParams.DeviceType      = ThisElem->DeviceType;
    GetParams->InitParams.BaseAddress     = ThisElem->BaseAddress;
    GetParams->InitParams.VppAddress      = ThisElem->VppAddress;
    GetParams->InitParams.MinAccessWidth  = ThisElem->MinAccessWidth;
    GetParams->InitParams.MaxAccessWidth  = ThisElem->MaxAccessWidth;
    GetParams->InitParams.NumberOfBlocks  = ThisElem->NumberOfBlocks;
    GetParams->InitParams.DriverPartition = ThisElem->DriverPartition;
    GetParams->DeviceCode                 = ThisElem->DeviceCode;
    GetParams->ManufactCode               = ThisElem->ManufactCode;
    /* the GetParams caller may not be the same as the Init caller, and
       thus may not know the NumberOfBlocks. This potentially makes it
       difficult for the caller to dimension the STFLASH_Block_t[]
       structure, pointed to by GetParams->InitParams.Blocks element.
       A NULL pointer is initially supplied if this is the case, to
       indicate that we must not yet copy stflash_Block_s data.        */
    if (GetParams->InitParams.Blocks != NULL)
    {
        Callers_Block_s = GetParams->InitParams.Blocks;  /* abbreviate ptr */

        for (i = 0; i < ThisElem->NumberOfBlocks; i++)
        {
            Callers_Block_s[i] = ThisElem->Blocks[i];
        }
    }

    return(ST_NO_ERROR);

}/* STFLASH_GetParams */


/****************************************************************************
Name         : STFLASH_Read()

Description  : Performs a direct read of the number of bytes
               requested from the specified Flash bank.  The
               maximum data width is used for as much of the
               operation as possible, provided that BOTH
               pointers are aligned on a boundary of the
               maximum access width specified during Init.

Parameters   : STFLASH_Handle_t Handle identifies the bank,
               U32 Offset (from device BaseAddress),
               U8 *Buffer into which data is to be written,
               U32 NumberToRead (bytes) from Offset, and
               U32 *NumberActuallyRead (written through
               by the routine).

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Rogue Handle value
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)

See Also     : STFLASH_Init()
               STFLASH_Write()
 ****************************************************************************/
ST_ErrorCode_t STFLASH_Read( STFLASH_Handle_t Handle,
                             U32              Offset,
                             U8               *Buffer,
                             U32              NumberToRead,
                             U32              *NumberActuallyRead )
{
    stflash_Inst_t  *ThisElem;
    U32             LastReadP1;     /* last address to be read plus one */
    U32             LeftToRead;     /* number of bytes left to read */
    U32             OperaWidth;     /* Operation Width (in bytes) */

    U8              *TmpAddress;    /* for recasting */
    U8              *AccessAddr;    /* BaseAddress + Offset */
    U8              *DestBuff08;    /* Buffer */
    U16             *DestBuff16;    /* (U16*)Buffer */
    U32             *DestBuff32;    /* (U32*)Buffer */
    DU8             *ReadAddr08;    /*  8 bit read access address */
    DU16            *ReadAddr16;    /* 16 bit read access address */
    DU32            *ReadAddr32;    /* 32 bit read access address */
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;  /* Default return code */

    /* Perform necessary validity checks */

#if defined(STFLASH_MULTIBANK_SUPPORT)
    U32 BankNo = 0;
    U32 BankNumberFromHandle = 0;
    if( !HandleIsValid(Handle,&ThisElem))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    BankNumberFromHandle = Handle & MASK_BANK_NUMBER;
    RetValue = GetBankNoFromOffset(ThisElem,Offset,&BankNo);
    if (RetValue != ST_NO_ERROR)
    {
        return RetValue;
    }
    if(BankNo != BankNumberFromHandle)
    {
    	return (ST_ERROR_BAD_PARAMETER);
    }
#else
    ThisElem = (stflash_Inst_t *)Handle;
    if (!HandleIsValid(Handle))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
#endif

    if (NumberToRead == 0)                         /* no bytes to read? */
    {
        *NumberActuallyRead = 0;
        return(ST_NO_ERROR);                      /* nothing to do, OK */
    }

#if defined(STFLASH_SPI_SUPPORT)
    if ( ThisElem->IsSerialFlash == TRUE )
    {

        RetValue = HAL_SPI_Read(Handle, Offset, Buffer, NumberToRead, NumberActuallyRead );

    }
    else
#endif
    {
         LastReadP1 = Offset + NumberToRead;

#if defined(STFLASH_MULTIBANK_SUPPORT)
         /* last read addr+1 */
        if ((LastReadP1 > ThisElem->OpenInfo[BankNo].BankEndOffset) ||\
           (Buffer == NULL))   /* no buffer address? */
       /* Past end of device? */

#else
             /* last read addr+1 */
        if ((LastReadP1 > ThisElem->LastOffsP1) ||   /* Past end of device? */
            (Buffer == NULL))                        /* no buffer address? */
#endif
        {
            *NumberActuallyRead = 0;
            return(ST_ERROR_BAD_PARAMETER);
        }

        TmpAddress = (U8*)ThisElem->BaseAddress;        /* cast for addition */
        AccessAddr = TmpAddress + Offset;               /* first read addr */

        /* Check addresses and length for MIN alignment.
           Takes no action for byte minimum,
           but will kick-in if minimum access width is bigger than a byte. */
        if ((((U32)AccessAddr &
            ThisElem->MinAccessMask) != 0) ||       /* source off MIN align? */
            (((U32)Buffer &
            ThisElem->MinAccessMask) != 0) ||       /* dest. off MIN align? */
             ((NumberToRead &
            ThisElem->MinAccessMask) != 0))        /* length off MIN align? */
        {
            *NumberActuallyRead = 0;
            return(ST_ERROR_BAD_PARAMETER);         /* sorry, can't do */
        }

        TmpAddress = (U8*)ThisElem->BaseAddress;      /* cast for computation */
        AccessAddr = TmpAddress + Offset;             /* form first read addr */

        /* determine the (initial) access width to be used */
        if ((((U32)AccessAddr &
               ThisElem->MaxAccessMask) == 0) &&  /* source on MAX boundary? */
            (((U32)Buffer &
               ThisElem->MaxAccessMask) == 0) &&  /* dest. on MAX boundary? */
            (NumberToRead >=
              ThisElem->MaxAccessWidth))           /* enough bytes for MAX? */
        {
            OperaWidth  = ThisElem->MaxAccessWidth;  /* high speed operation */
        }
        else
        {
            OperaWidth  = ThisElem->MinAccessWidth;  /* lower speed operation */
        }

        /* set up destination buffer and read pointers for all widths */
        DestBuff08  = Buffer;
        DestBuff16  = (U16*)Buffer;
        DestBuff32  = (U32*)Buffer;
        ReadAddr08  = AccessAddr;
        ReadAddr16  = (DU16*)AccessAddr;
        ReadAddr32  = (DU32*)AccessAddr;

        /* Clear Status Register (precautionary) */
        /* and commence Read Memory Array Command Sequence */
        /* Check DeviceType is supported */
        if (ThisElem->DeviceType != STFLASH_AT49BV162AT)
        {
            switch (ThisElem->MaxAccessWidth)          /* use maximum access width */
            {
                case STFLASH_ACCESS_08_BITS:
                *ReadAddr08 = CLEAR_SR08;
                break;

                case STFLASH_ACCESS_16_BITS:
                *ReadAddr16 = CLEAR_SR16;
                break;

                case STFLASH_ACCESS_32_BITS:
                *ReadAddr32 = CLEAR_SR32;
                break;

                default:
                break;
            }
        }

        if (ThisElem->DeviceType != STFLASH_AT49BV162AT)
        {
      	
            HAL_SetReadMemoryArray( ThisElem, ThisElem->MaxAccessWidth, ReadAddr08, ReadAddr16, ReadAddr32 );
           
        }

        LeftToRead = NumberToRead;                 /* initialize counter */
        while (LeftToRead > 0)                    /* whilst more bytes to read */
        {
            switch (OperaWidth)                   /* choose appropriate size */
            {
                case STFLASH_ACCESS_08_BITS:
                *DestBuff08++ = *ReadAddr08++;      /* read from Flash */
                LeftToRead   -= OperaWidth;
                break;

                case STFLASH_ACCESS_16_BITS:
                while (LeftToRead >= OperaWidth)
                {
                    *DestBuff16++ = *ReadAddr16++;  /* read from Flash */
                    LeftToRead   -= OperaWidth;
                }

                if (LeftToRead > 0)                /* any left to read? */
                {
                    OperaWidth =                    /* drop down to MIN */
                        ThisElem-> MinAccessWidth;

                    /* transfer ptrs */
                    ReadAddr08 = (DU8*)ReadAddr16;
                    DestBuff08 = (U8*)DestBuff16;
                }
                break;

                case STFLASH_ACCESS_32_BITS:
                while (LeftToRead >= OperaWidth)
                {
                    *DestBuff32++ = *ReadAddr32++;  /* read from Flash */
                    LeftToRead   -= OperaWidth;
                }
                if (LeftToRead > 0)                /* any left to read? */
                {
                    OperaWidth =                    /* drop down to MIN */
                        ThisElem-> MinAccessWidth;

                    /* transfer ptrs */
                    ReadAddr08 = (DU8*)ReadAddr32;
                    ReadAddr16 = (DU16*)ReadAddr32;
                    DestBuff08 = (U8*)DestBuff32;
                    DestBuff16 = (U16*)DestBuff32;
                }
                break;

                default:

                *NumberActuallyRead =           /* this SHOULDN'T happen */
                    NumberToRead - LeftToRead;
                return(ST_ERROR_BAD_PARAMETER);
            }
        }

        /* write back to caller number of bytes read */
        *NumberActuallyRead = NumberToRead - LeftToRead;

    }

    return(RetValue);

} /* STFLASH_Read */


/****************************************************************************
Name         : STFLASH_Erase()

Description  : Performs a direct erase of a whole block.  For the ST
               device supported, the Offset specified MUST fall on one
               of the declared block boundaries, and the NumberToErase
               MUST also match the Length declared for that block.
               The call will FAIL if these pre-conditions are not met.

Parameters   : STFLASH_Handle_t Handle identifies the Flash bank,
               U32 Offset (from device BaseAddress), and
               U32 NumberToErase (bytes to erase).

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Rogue Handle value
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_TIMEOUT            Erase Timeout limit reached
               STFLASH_ERROR_ERASE         Error during Erase sequence
               STFLASH_ERROR_VPP_LOW       Vpp below minimum

See Also     : STFLASH_Init()
               STFLASH_Write()
 ****************************************************************************/

ST_ErrorCode_t STFLASH_Erase( STFLASH_Handle_t Handle,
                              U32              Offset,
                              U32              NumberToErase )
{
    U32 i = 0;
    stflash_Inst_t  *ThisElem;
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;
    BOOL ParamsValid = FALSE;       /* initial assumption */
    U32  CurrOffset = 0;            /* start at BaseAddress */
    HAL_EraseParams_t EraseParams;

#if defined(STFLASH_MULTIBANK_SUPPORT)
    U32 BankNo = 0;
    U32 BankNumberFromHandle = 0;
   
    if( !HandleIsValid(Handle,&ThisElem))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    BankNumberFromHandle = Handle & MASK_BANK_NUMBER;
   
    RetValue = GetBankNoFromOffset(ThisElem,Offset,&BankNo);
    if (RetValue != ST_NO_ERROR)
    {
        return RetValue;
    }
    if(BankNo != BankNumberFromHandle)
    {
    	return (ST_ERROR_BAD_PARAMETER);
    }
    /* Make sure that checks are atomic */
    STOS_SemaphoreWait( Atomic_p );

#else
    ThisElem = (stflash_Inst_t *)Handle;
    if (!HandleIsValid(Handle))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
#endif

#if defined(STFLASH_SPI_SUPPORT)
    if ( ThisElem->IsSerialFlash == TRUE )
    {
        RetValue = HAL_SPI_Erase( Handle, Offset, NumberToErase );

    }
    else
#endif
    {
        /* walk through STFLASH_Block_t array checking
           that both Offset and NumberToErase are legal */
        for (i = 0; i < ThisElem->NumberOfBlocks; i++)
        {
            if ((CurrOffset == Offset) &&             /* start match? */
               (ThisElem->Blocks[i].Length == NumberToErase))           /* length match? */
            {
                ParamsValid = TRUE;
                break;                                  /* stop searching */
            }

            CurrOffset += ThisElem->Blocks[i].Length;   /* next block */
        }

        if (!ParamsValid)                   /* start/length mismatch block? */
        {
#if defined(STFLASH_MULTIBANK_SUPPORT)
            STOS_SemaphoreSignal(Atomic_p);
#endif
            return(ST_ERROR_BAD_PARAMETER);
        }

        EraseParams.BaseAddress    = ThisElem->BaseAddress;
        EraseParams.MaxAccessWidth = ThisElem->MaxAccessWidth;
        EraseParams.VppAddress     = ThisElem->VppAddress;

        RetValue = HAL_Erase( ThisElem , EraseParams, Offset, NumberToErase );
    }
#if defined(STFLASH_MULTIBANK_SUPPORT)
    STOS_SemaphoreSignal( Atomic_p );
#endif
    return(RetValue);

}/* STFLASH_Erase */


/****************************************************************************
Name         : STFLASH_Write()

Description  : Performs a direct write of the number of bytes requested
               to the specified Flash bank.  The maximum access width is
               used for as much of the operation as possible, provided
               that BOTH pointers are aligned onto boundaries of it.
               The Vpp programming voltage is enabled throughout to
               minimise the settling time overhead.  The Vpp Low error is
               not considered fatal; if raised it will be returned at the
               end when all locations have been Programmed, provided no
               Write Sequence failures are earlier determined.

Parameters   : STFLASH_Handle_t Handle identifies the Flash bank,
               U32 Offset (from device BaseAddress) to write to,
               U8 Buffer pointer from which data will be read,
               U32 Number of bytes to be written, followed by
               U32* Number of bytes actually written (return).

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Rogue Flash bank indicator
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_TIMEOUT            Timeout error during write
               STFLASH_ERROR_WRITE         Error during write
               STFLASH_ERROR_VPP_LOW       Low Vpp detected during write

See Also     : STFLASH_Init()
               STFLASH_Erase()
               STFLASH_Write()
 ****************************************************************************/

ST_ErrorCode_t STFLASH_Write( STFLASH_Handle_t Handle,
                              U32              Offset,
                              U8               *Buffer,
                              U32              NumberToWrite,
                              U32              *NumberActuallyWritten )
{
    stflash_Inst_t  *ThisElem;
    U32             LastRiteP1;         /* last address to be written plus one */
    ST_ErrorCode_t  RetValue;
    HAL_WriteParams_t WriteParams;

#if defined(STFLASH_MULTIBANK_SUPPORT)
    U32 BankNo = 0;
    U32 BankNumberFromHandle = 0;
    if( !HandleIsValid(Handle,&ThisElem))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    BankNumberFromHandle = Handle & MASK_BANK_NUMBER;
    RetValue = GetBankNoFromOffset(ThisElem,Offset,&BankNo);
    if (RetValue != ST_NO_ERROR)
    {
        return RetValue;
    }
    if(BankNo != BankNumberFromHandle)
    {
    	return (ST_ERROR_BAD_PARAMETER);
    }
    /* Make sure that checks are atomic */
    STOS_SemaphoreWait( Atomic_p );
#else
    ThisElem = (stflash_Inst_t *)Handle;
    if (!HandleIsValid(Handle))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
#endif

    if (NumberToWrite == 0)                    /* no bytes to write? */
    {
        *NumberActuallyWritten = 0;
        
#if defined(STFLASH_MULTIBANK_SUPPORT)
        STOS_SemaphoreSignal( Atomic_p );
#endif
        return (ST_NO_ERROR);                     /* nothing to do, OK */
    }

#if defined(STFLASH_SPI_SUPPORT)
    if ( ThisElem->IsSerialFlash == TRUE )
    {
     	    RetValue = HAL_SPI_Write(   Handle,
                                        Offset,
                                        Buffer,
                                        NumberToWrite,
                                        NumberActuallyWritten
                                    );
    }
    else
#endif
    {
        LastRiteP1 = Offset + NumberToWrite;        /* compute last address + 1 */
#if defined(STFLASH_MULTIBANK_SUPPORT)
        if ((Buffer == NULL) ||                   /* no buffer address? */
            (LastRiteP1 > ThisElem->OpenInfo[BankNo].BankEndOffset))  /* fallen off end of bank? */
#else
        if ((Buffer == NULL) ||                   /* no buffer address? */
           (LastRiteP1 > ThisElem->LastOffsP1)) /* fallen off end of bank? */
#endif
        {
            *NumberActuallyWritten = 0;

#if defined(STFLASH_MULTIBANK_SUPPORT)
            STOS_SemaphoreSignal( Atomic_p );
#endif
            return(ST_ERROR_BAD_PARAMETER);
        }

        WriteParams.BaseAddress    = ThisElem->BaseAddress;
        WriteParams.MinAccessWidth = ThisElem->MinAccessWidth;
        WriteParams.MaxAccessWidth = ThisElem->MaxAccessWidth;
        WriteParams.MinAccessMask  = ThisElem->MinAccessMask;
        WriteParams.MaxAccessMask  = ThisElem->MaxAccessMask;
        WriteParams.VppAddress     = ThisElem->VppAddress;
 
        RetValue = HAL_Write(ThisElem,
                            WriteParams,
                            Offset,
                            Buffer,
                            NumberToWrite,
                            NumberActuallyWritten
                        );
   }
#if defined(STFLASH_MULTIBANK_SUPPORT)
    STOS_SemaphoreSignal(Atomic_p );
#endif
    return(RetValue);

}/* STFLASH_Write */

/****************************************************************************
Name         : STFLASH_Close()

Description  : Flags the specified Flash Bank as closed

Parameters   : STFLASH_Handle_t Handle points to a Flash Bank Init

Return Value : ST_ErrorCode_t specified as
               ST_ERROR_INVALID_HANDLE     Rogue Handle value
               ST_NO_ERROR                 No errors occurred

See Also     : STFLASH_Handle_t
               STFLASH_Open()
 ****************************************************************************/
ST_ErrorCode_t STFLASH_Close( STFLASH_Handle_t Handle )
{
    stflash_Inst_t  *ThisElem;

#if defined(STFLASH_MULTIBANK_SUPPORT)
    U32 BankNo = 0;
    if( !HandleIsValid(Handle,&ThisElem))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    BankNo = Handle & MASK_BANK_NUMBER;
   
    ThisElem->OpenInfo[BankNo].FlashBankOpen = FALSE;   /* flag as closed */
    ThisElem->OpenInfo[BankNo].Handle = 0;
    
#else

    /* Perform parameter validity checks */

    ThisElem = (stflash_Inst_t *)Handle;

    if ((!HandleIsValid(Handle)) || /* rogue Handle value? */
       (!ThisElem->BankOpen ))      /* Bank not Open? */
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    ThisElem->BankOpen = FALSE;   /* flag as closed */
#endif
    return(ST_NO_ERROR);

}/* STFLASH_Close */


/****************************************************************************
Name         : STFLASH_GetCFI()

Description  : Get All the CFI information as described in JEDEC Standard

Parameters   : STFLASH_Handle_t Handle points to a Flash Bank Init

Return Value : ST_ErrorCode_t specified as
               ST_ERROR_INVALID_HANDLE     Rogue Handle value
               ST_NO_ERROR                 No errors occurred

See Also     : STFLASH_Handle_t
               STFLASH_Open()
 ****************************************************************************/

ST_ErrorCode_t STFLASH_GetCFI( STFLASH_Handle_t Handle, STFLASH_CFI_Query_t *CFI )
{
    stflash_Inst_t  *ThisElem;
    int ret = 0;
#if defined(STFLASH_MULTIBANK_SUPPORT)

    if( !HandleIsValid(Handle,&ThisElem))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
   
#else
    ThisElem = (stflash_Inst_t *)Handle;
    if (!HandleIsValid(Handle))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
#endif

    if (CFI == NULL)                   /* has caller forgotten ptr? */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ret = flash_get_cfi(CFI, ThisElem->BaseAddress );

    if ( ret == 0)
    {
        return(ST_NO_ERROR);
    }
    else
    {
    	return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

}/* STFLASH_GetCFI */

/****************************************************************************
Name         : STFLASH_Term()

Description  : Terminates the Flash Memory Interface, effectively
               supporting redefinition to another Flash device.

Parameters   : ST_DeviceName Name specifies name of Flash bank, and
               STFLASH_TermParams_t *TermParams holds ForceTerminate flag.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_UNKNOWN_DEVICE     Init not called

See Also     : STFLASH_Init()
 ****************************************************************************/

ST_ErrorCode_t STFLASH_Term( const ST_DeviceName_t Name,
                             const STFLASH_TermParams_t *TermParams )
{
    stflash_Inst_t  *ThisElem, *PrevElem = NULL;
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;
    /* Perform parameter validity checks */
    if ((Name       == NULL)   ||               /* NULL Name ptr? */
       (TermParams == NULL))                    /* NULL structure ptr? */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (stflash_Sentinel == NULL)              /* no Inits outstanding? */
    {
        return(ST_ERROR_UNKNOWN_DEVICE);       /* Semaphore not created */
    }

    STOS_SemaphoreWait( Atomic_p );                  /* start of atomic region */

    for (ThisElem = stflash_Sentinel;           /* start at Sentinel node */
         ThisElem != NULL;                      /* while not at list end */
         ThisElem = ThisElem->Next)             /* advance to next element */
    {
        if (strcmp(Name,
                   ThisElem->BankName) == 0)    /* existing BankName? */
        {
            break;
        }

        PrevElem = ThisElem;                    /* keep back marker */
    }

    if (ThisElem == NULL)                       /* no Name match found? */
    {
        STOS_SemaphoreSignal( Atomic_p );            /* end atomic before return */
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
#if defined(STFLASH_MULTIBANK_SUPPORT)
	{
		U32 i = 0;
        for( i = 0; i < ThisElem->NumberOfBanks; i++)
        {
            if (ThisElem->OpenInfo[i].FlashBankOpen)
            {
                 if (!TermParams->ForceTerminate)       /* unforced Closure? */
                 {
                    STOS_SemaphoreSignal( Atomic_p );        /* end atomic before return */
                    return(ST_ERROR_OPEN_HANDLE);     /* user needs to call Close */
                 }
            }
        }
    }
#else
    if (ThisElem->BankOpen)                    /* Flash bank open? */
    {
        if (!TermParams->ForceTerminate)       /* unforced Closure? */
        {
            STOS_SemaphoreSignal( Atomic_p );        /* end atomic before return */
            return(ST_ERROR_OPEN_HANDLE);     /* user needs to call Close */
        }
    }
#endif
    if (ThisElem == stflash_Sentinel)
    {
        stflash_Sentinel = ThisElem->Next;         /* remove first item */
    }
    else
    {
        PrevElem->Next = ThisElem->Next;           /* remove mid-list item */
    }

#if defined(STFLASH_SPI_SUPPORT)
    if ( ThisElem->IsSerialFlash == TRUE )
    {
	/* Close handle for CS */
        RetValue = STPIO_Close ( ThisElem->CSHandle );
        if (RetValue != ST_NO_ERROR)
        {
            return RetValue;
        }

        /* Close handle for SPI */
        RetValue = STSPI_Close ( ThisElem->SPIHandle );
        if (RetValue != ST_NO_ERROR)
        {
            return RetValue;
        }

    }
#endif
    STOS_SemaphoreSignal( Atomic_p );                   /* end of atomic region */

#if defined(STFLASH_MULTIBANK_SUPPORT)    
    STOS_MemoryDeallocate( ThisElem->DriverPartition, ThisElem->OpenInfo);
    STOS_MemoryDeallocate( ThisElem->DriverPartition, ThisElem->BankInfo);        
#endif    

    STOS_MemoryDeallocate( ThisElem->DriverPartition, ThisElem );

    if (stflash_Sentinel == NULL)                 /* no Inits outstanding? */
    {
        STOS_SemaphoreDelete(NULL,Atomic_p );             /* semaphore NLR */
    }

    return(RetValue);

}/* STFLASH_Term */


#if defined(STFLASH_SPI_SUPPORT)
/****************************************************************************
Name         : STFLASH_SetSPIClkDiv()

Description  : This function locks all the blocks.

Parameters   : STFLASH_Handle_t Handle identifies the bank,
               U8 Clock Divisor in even number

Return Value : ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Rogue Flash bank indicator
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_TIMEOUT            Timeout error during write

****************************************************************************/

ST_ErrorCode_t STFLASH_SetSPIClkDiv( STFLASH_Handle_t Handle,
                                     U8              Divisor)
{

    stflash_Inst_t  *ThisElem;
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;

    /* Perform necessary validity checks */

    ThisElem = (stflash_Inst_t *)Handle;
    if (!HandleIsValid(Handle))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    if (ThisElem->IsSerialFlash == TRUE )
    {
    	 HAL_SPI_ClkDiv( Divisor );
    }
    else
    {

         RetValue = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return(RetValue);
} /* STFLASH_SetSPIClkDiv */

/****************************************************************************
Name         : STFLASH_SetSPIConfig()

Description  : This function locks all the blocks.

Parameters   : STFLASH_Handle_t Handle identifies the bank,
               STFLASH_SPIConfig_t SPIConfig

Return Value : ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Rogue Flash bank indicator
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_FEATURE_NOT_SUPPORTED in case of parallel flash
               ST_ERROR_TIMEOUT            Timeout error during write

****************************************************************************/

ST_ErrorCode_t STFLASH_SetSPIConfig( STFLASH_Handle_t Handle,
                                     STFLASH_SPIConfig_t *SPIConfig)
{

    stflash_Inst_t  *ThisElem;
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;

    /* Perform necessary validity checks */
    ThisElem = (stflash_Inst_t *)Handle;
    if (!HandleIsValid(Handle))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    if (ThisElem->IsSerialFlash == TRUE )
    {

    	HAL_SPI_Config( SPIConfig->PartNum,
    	                SPIConfig->CSHighTime,
    	                SPIConfig->CSHoldTime,
    	                SPIConfig->DataHoldTime );
    }
    else
    {

        RetValue = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    return( RetValue );

} /* STFLASH_SetSPIConfig */

/****************************************************************************
Name         : STFLASH_SetSPIModeSelect()

Description  : This function is used to set the Config Mode and Fast Read Mode

Parameters   : STFLASH_Handle_t Handle identifies the bank,
               STFLASH_SPIModeSelect_t SPIMode

Return Value : ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Rogue Flash bank indicator
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_FEATURE_NOT_SUPPORTED in case of parallel flash
               ST_ERROR_TIMEOUT            Timeout error during write

****************************************************************************/

ST_ErrorCode_t STFLASH_SetSPIModeSelect( STFLASH_Handle_t Handle,
                                         STFLASH_SPIModeSelect_t  *SPIMode )
{

    stflash_Inst_t  *ThisElem;
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;

    /* Perform necessary validity checks */
    ThisElem = (stflash_Inst_t *)Handle;
    ThisElem->IsFastRead = SPIMode->IsFastRead;
    if (!HandleIsValid(Handle))
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    if (ThisElem->IsSerialFlash == TRUE )
    {
        HAL_SPI_ModeSelect(SPIMode->IsContigMode,
    	                       SPIMode->IsFastRead);

    }
    else
    {

        RetValue = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    return( RetValue );

} /* STFLASH_SetSPIModeSelect */

#endif
/****************************************************************************
Name         : GetDeviceInfo()

Description  : Retrieves Manufacturer ID and Device ID from flash device
               NOTE: This is an internal function and only performs
               limited error checking of parameters.

Parameters   : U32 BaseAddress                  Base address of device
               STFLASH_DeviceType_t DeviceType  Device to investigate
               U32 AccessWidth                  Access width to use
               U32 *ManufactCode                Returned Manufacturer ID
               U32 *DeviceCode                  Returned Device ID

Return Value : ST_ErrorCode_t
               ST_NO_ERROR              No errors
               ST_ERROR_BAD_PARAMETER   1 more parameter was invalid

See Also     : STFLASH_GetParams()
 ****************************************************************************/

static ST_ErrorCode_t GetDeviceInfo( U32 BaseAddress,
                                     STFLASH_DeviceType_t DeviceType,
                                     U32 AccessWidth,
                                     U32 *ManufactCode,
                                     U32 *DeviceCode
                                     )
{
    DU8     *InitAddr08;            /*  8 bit init access address */
    DU16    *InitAddr16;            /* 16 bit init access address */
    DU32    *InitAddr32;            /* 32 bit init access address */

    U8      *DeviceAddr;
    U8      *ManufactAddr;
    U8      *LockStatus;
    U32     LockStat;

    /* convert pointer to integer to apply A0 high - actually A2
       in Software because lines A1-0 are decoded to chip select  */

    switch (DeviceType)
    {
        case STFLASH_M58LW064D:
        case STFLASH_M58LT128GS:
        case STFLASH_M58LT256GS: 
        case STFLASH_M58LW032:
        case STFLASH_M28W320CB:
        case STFLASH_E28F640:
        case STFLASH_M28F411:
            if (DeviceType == STFLASH_M58LW032 || DeviceType == STFLASH_M58LW064D ||\
                DeviceType == STFLASH_M58LT128GS  || DeviceType ==  STFLASH_M58LT256GS)
            {
#if defined(ST_5528)
                DeviceAddr    = (U8*)( M28F411_DEV_ID_ACCESS   | BaseAddress );
#else
                DeviceAddr    = (U8*)( M58LW032_DEV_ID_ACCESS   | BaseAddress );
#endif
            }
            else
            {
                DeviceAddr    = (U8*)( M28F411_DEV_ID_ACCESS   | BaseAddress );
            }

            ManufactAddr  = (U8*)( M28F411_MANUF_ID_ACCESS | BaseAddress );
            LockStatus = (U8*)( 0x08 | BaseAddress );

            switch (AccessWidth)
            {
                case STFLASH_ACCESS_08_BITS:
                    InitAddr08   = DeviceAddr;
                    *InitAddr08  = READ_ES08;
                    *DeviceCode  = (U32)*InitAddr08;
                    InitAddr08   = ManufactAddr;
                    *ManufactCode = (U32)*InitAddr08;

                    /* restore device to read mode */
                    *InitAddr08 = M28_READ_MA08;
                    break;

                case STFLASH_ACCESS_16_BITS:
                    InitAddr16    = (U16*)DeviceAddr;
                    *InitAddr16   = READ_ES16;
                    *DeviceCode   = *InitAddr16;
                    InitAddr16    = (U16*)ManufactAddr;
                    *ManufactCode = *InitAddr16;

                    /* restore device to read mode */
                    *InitAddr16 = M28_READ_MA16;
                    break;

                case STFLASH_ACCESS_32_BITS:
                    InitAddr32    = (U32*)DeviceAddr;

                    *InitAddr32   = READ_ES32;
                    *DeviceCode   = *InitAddr32;

                    InitAddr32    = (U32*)ManufactAddr;
                    *ManufactCode = (U32)*InitAddr32;

                    InitAddr32    = (U32*)LockStatus;
                    LockStat      = (U32)*InitAddr32;

                    /* restore device to read mode */
                    *InitAddr32 = M28_READ_MA32;
                   break;

                default:
                    return (ST_ERROR_BAD_PARAMETER);
            }
            break;


        case STFLASH_M28W320FS:  /* KRYPTO secured FLASH */
        case STFLASH_M28W640FS:  /* KRYPTO secured FLASH */
        case STFLASH_M28W320FSU: /* KRYPTO secured FLASH of uniform blocks */
        case STFLASH_M28W640FSU: /* KRYPTO secured FLASH of uniform blocks */

            DeviceAddr    = (U8*)( M58LW032_DEV_ID_ACCESS  | BaseAddress );
            ManufactAddr  = (U8*)( M28F411_MANUF_ID_ACCESS | BaseAddress );
            LockStatus = (U8*)( 0x08 | BaseAddress );

            switch (AccessWidth)
            {
                case STFLASH_ACCESS_08_BITS:
                    InitAddr08   = DeviceAddr;
                    *InitAddr08  = READ_ES08;
                    *DeviceCode  = (U32)*InitAddr08;
                    InitAddr08   = ManufactAddr;
                    *ManufactCode = (U32)*InitAddr08;

                    /* restore device to read mode */
                    *InitAddr08 = M28_READ_MA08;
                    break;

                case STFLASH_ACCESS_16_BITS:
                    InitAddr16    = (U16*)DeviceAddr;
                    *InitAddr16   = READ_ES16;
                    *DeviceCode   = *InitAddr16;
                    InitAddr16    = (U16*)ManufactAddr;
                    *ManufactCode = *InitAddr16;

                    /* restore device to read mode */
                    *InitAddr16 = M28_READ_MA16;
                    break;

                case STFLASH_ACCESS_32_BITS:
                    InitAddr32    = (U32*)DeviceAddr;

                    *InitAddr32   = READ_ES32;
                    *DeviceCode   = *InitAddr32;

                    InitAddr32    = (U32*)ManufactAddr;
                    *ManufactCode = (U32)*InitAddr32;

                    InitAddr32    = (U32*)LockStatus;
                    LockStat      = (U32)*InitAddr32;

                    /* restore device to read mode */
                    *InitAddr32 = M28_READ_MA32;
                   break;

                default:
                    return (ST_ERROR_BAD_PARAMETER);
            }
            break;

        case STFLASH_AM29LV160D:
        case STFLASH_M29W800T:
        case STFLASH_M29W160BT:
        case STFLASH_M29W640D:
        /*case STFLASH_AT49BV162AT:*/
        case STFLASH_M29W320DT:
           switch (AccessWidth)
            {
                case STFLASH_ACCESS_32_BITS:
                    DeviceAddr   = (U8*)((U32*)BaseAddress +
                                         M29W800T_DEV_ID_ACCESS);
                    ManufactAddr = (U8*)((U32*)BaseAddress +
                                         M29W800T_MANUF_ID_ACCESS);
                   /* Cycle 1 - Coded cycle 1 */
                    InitAddr32    = (U32*)BaseAddress + M29W800T_CODED_ADDR1;
                    *InitAddr32   = M29W800T_CODED_DATA1;

                    /* Cycle 2 - Coded cycle 2 */
                    InitAddr32    = (U32*)BaseAddress + M29W800T_CODED_ADDR2;
                    *InitAddr32   = M29W800T_CODED_DATA2;

                    /* Cycle 3 - Auto Select Command */
                    InitAddr32    = (U32*)BaseAddress + M29W800T_CODED_ADDR1;
                    *InitAddr32   = M29W800T_AUTO_SELECT;

                    /* Cycle 4 - Read Device ID */
                    InitAddr32    = (U32*)DeviceAddr;
                    *DeviceCode   = *InitAddr32;

                    /* Cycle 5 - Read Manufact ID */
                    InitAddr32    = (U32*)ManufactAddr;
                    *ManufactCode = *InitAddr32;

                    /* Cycle 6 - Reset to RMA mode */
                    *InitAddr32   = M29_READ_MA32;
                    break;

                case STFLASH_ACCESS_16_BITS:
                    /* Cycle 1 - Coded cycle 1 */
                    InitAddr16    = (U16*)BaseAddress + M29W800T_CODED_ADDR1;
                    *InitAddr16   = M29W800T_CODED_DATA1 & 0xffff;

                    /* Cycle 2 - Coded cycle 2 */
                    InitAddr16    = (U16*)BaseAddress + M29W800T_CODED_ADDR2;
                    *InitAddr16   = M29W800T_CODED_DATA2 & 0xffff;

                    /* Cycle 3 - Auto Select Command */
                    InitAddr16    = (U16*)BaseAddress + M29W800T_CODED_ADDR1;
                    *InitAddr16   = M29W800T_AUTO_SELECT & 0xffff;

                    /* Cycle 4 - Read Device ID */
                    InitAddr16    = (U16*)BaseAddress + M29W800T_DEV_ID_ACCESS;
                    *DeviceCode   = *InitAddr16;

                    /* Cycle 5 - Read Manufact ID */
                    InitAddr16    = (U16*)BaseAddress + M29W800T_MANUF_ID_ACCESS;
                    *ManufactCode = *InitAddr16;

                    /* Cycle 6 - Reset to RMA mode */
                    *InitAddr16   = M29_READ_MA16;
                    break;

                default:
                    return (ST_ERROR_BAD_PARAMETER);
            }
            break;
            
        case STFLASH_AT49BV162AT:
        case STFLASH_M25P10:
        case STFLASH_M25P80:
        case STFLASH_M25P16:
            break;   
  		
  		default:
            break;                     
    }
    return (ST_NO_ERROR);
}
#if defined(STFLASH_MULTIBANK_SUPPORT)
BOOL HandleIsValid( STFLASH_Handle_t Handle,stflash_Inst_t **ThisElem_p)
#else
BOOL HandleIsValid( STFLASH_Handle_t Handle )
#endif
{
    stflash_Inst_t* InitInstance;

    InitInstance = stflash_Sentinel; /* set to first node */

    while (InitInstance!=NULL)
    {
#if defined(STFLASH_MULTIBANK_SUPPORT)
		{
			U32 i;
	        for(i = 0 ; i < InitInstance->NumberOfBanks; i++)
	        {
	            if((InitInstance->OpenInfo[i].Handle == Handle) && \
	               (InitInstance->OpenInfo[i].FlashBankOpen == TRUE))
	            {
	             	/* Handle found */
	             	 *ThisElem_p = InitInstance;
	            	 return (TRUE);
	            }
	        }
	        if(i == InitInstance->NumberOfBanks)
	        {
	        	  InitInstance = InitInstance->Next;
	        }
		}
#else
        if (InitInstance == (stflash_Inst_t *)Handle)
        {
            /* Handle found */
            return (TRUE);
        }
        else
        {
            InitInstance = InitInstance->Next;
        }
#endif        
     
    }

    return (FALSE);

}

#if defined(STFLASH_SPI_SUPPORT)
/******************************************************************************
Name        : GetPioPin
Description : Takes a PIO bitmask for one PIO pin and resolves it to a pin
              number between zero and seven. Returns 0xF on error.
Parameters  : A bitmask for a PIO pin
******************************************************************************/

static S32 GetPioPin( U8 Value )
{
    S32 i = 0;

    while ( (Value != 1) && (i < 8) )
    {
        i++;
        Value >>= 1;
    }
    if ( i > 7 )
    {
        i = 0xFF;
    }
    return( i );
}
#endif
/*****************************************************************************
GetBankNoFromOffset()
Description:

Parameters:
    Handle, an open handle.
Return Value:
    NULL, end of queue encountered - invalid handle.
    Otherwise, returns a pointer to the control block for the device.
See Also:

*****************************************************************************/
#if defined(STFLASH_MULTIBANK_SUPPORT)
static ST_ErrorCode_t GetBankNoFromOffset( stflash_Inst_t *ThisElem_p,
                                           U32  Offset,
                                           U32 *BankNumber)
{

      U32 i;
      if (Offset > ThisElem_p->LastOffsP1)
      {
         return(ST_ERROR_BAD_PARAMETER);
      }
      for( i = 0; i < ThisElem_p->NumberOfBanks; i++)
      {
         if ((Offset >= ThisElem_p->OpenInfo[i].BankStartOffset) &&
             (Offset <= ThisElem_p->OpenInfo[i].BankEndOffset))
         {
            if (ThisElem_p->OpenInfo[i].FlashBankOpen == TRUE)
            {
                    *BankNumber = i;
                   /* Bank found */
                     break;
            }
         }
     }/*for*/
     return(ST_NO_ERROR);

} /* GetBankNoFromOffset() */
#endif


/* This should be in the HAL */
ST_ErrorCode_t SetFunctions( stflash_Inst_t *ThisElem)
{
    switch (ThisElem->DeviceType)
    {
        case STFLASH_M28W320CB:
        case STFLASH_E28F640:
        case STFLASH_M28F411:
        case STFLASH_M28W320FS:  /* KRYPTO secured FLASH */
        case STFLASH_M28W640FS:  /* KRYPTO secured FLASH */
        case STFLASH_M28W320FSU: /* KRYPTO secured FLASH of uniform block size */
        case STFLASH_M28W640FSU: /* KRYPTO secured FLASH of uniform block size */

            ThisElem->WriteFunction = HAL_M28_Write;
            ThisElem->EraseFunction = HAL_M28_Erase;
            ThisElem->RMAFunction   = HAL_M28_SetReadMemoryArray;
            ThisElem->LockFunction  = HAL_M28_Lock;
            ThisElem->UnlockFunction= HAL_M28_Unlock;
            break;

        case STFLASH_M58LW064D:
        case STFLASH_M58LT128GS:
		case STFLASH_M58LT256GS:        

            ThisElem->WriteFunction = HAL_M28_WriteToBuffer;
            ThisElem->EraseFunction = HAL_M28_Erase;
            ThisElem->RMAFunction   = HAL_M28_SetReadMemoryArray;
            ThisElem->LockFunction  = HAL_M28_Lock;
            ThisElem->UnlockFunction= HAL_M28_Unlock;
            break;

        case STFLASH_M58LW032:

            ThisElem->WriteFunction = HAL_M28_Write;
            ThisElem->EraseFunction = HAL_M28_Erase;
            ThisElem->RMAFunction   = HAL_M28_SetReadMemoryArray;
            ThisElem->LockFunction  = HAL_M28_Lock;
            ThisElem->UnlockFunction= HAL_M28_Unlock;
            break;

        case STFLASH_AM29LV160D:
        case STFLASH_M29W800T:
        case STFLASH_M29W160BT:
        case STFLASH_M29W640D:
        case STFLASH_M29W320DT:
        case STFLASH_AT49BV162AT:

            ThisElem->WriteFunction = HAL_M29_Write;
            ThisElem->EraseFunction = HAL_M29_Erase;
            ThisElem->RMAFunction   = HAL_M29_SetReadMemoryArray;
            break;

        default:
            return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    return(ST_NO_ERROR);
}

BOOL flash_get_cfi( STFLASH_CFI_Query_t *Query, U32 *FlashBaseAddress )
{
    U16 *fwp; 			/* flash window */
    U32 temp=0, i=0;
    U32 offset=0;

    /* FLASH query cmd */
    fwp = (U16*)FlashBaseAddress;
    *fwp = CFI_QUERY_COMMAND; 			/* CFI Query Command */

    /* Initial house-cleaning */
    for(i=0; i < 8; i++)
    {
        Query->Erase_Block[i].Sector_Size = 0;
        Query->Erase_Block[i].Num_Sectors = 0;
    }

    /* CFI - Query Address and Data Output */
    
    Query->Query_String[0] = fwp[0x10];
    Query->Query_String[1] = fwp[0x11];
    Query->Query_String[2] = fwp[0x12];
    Query->Query_String[3] = '\0';

    /* If not 'QRY', then we dont have a CFI enabled device in the socket */
    if( Query->Query_String[0] != 'Q' && Query->Query_String[1] != 'R'  \
       && Query->Query_String[2] != 'Y')
    {
        return(1);
    }

    /* Manufacturer Code and Device Code */
    Query->ManufacturerCode = fwp[0x00];
    Query->DeviceCode       = fwp[0x01];

    Query->Primary_Vendor_Cmd_Set = fwp[0x13];
    Query->Primary_Table_Address  = fwp[0x15];      /* Important one */
    
    Query->Alt_Vendor_Cmd_Set     = fwp[0x17];
    Query->Alt_Table_Address      = fwp[0x19];

    /* CFI - Device Voltage and Timing Specification */
    Query->Vdd_Min.Volts      = ((fwp[0x1B] & 0xF0) >> 4);
    Query->Vdd_Min.MilliVolts = ((fwp[0x1B] & 0xF0) >> 4);
    
    Query->Vdd_Max.Volts  	  = ((fwp[0x1C] & 0xF0) >> 4);
    Query->Vdd_Max.MilliVolts = (fwp[0x1C] & 0x0F);
    
    Query->Vpp_Min.Volts  	  = ((fwp[0x1D] & 0xF0) >> 4);
    Query->Vpp_Min.MilliVolts = (fwp[0x1D] & 0x0F);
    
    Query->Vpp_Max.Volts      = ((fwp[0x1E] & 0xF0) >> 4);
    Query->Vpp_Max.MilliVolts = (fwp[0x1E] & 0x0F);

    /* Use a bit shift to calculate power */
    temp = fwp[0x1F];
    Query->Timeout_Single_Write = (1 << temp);

    temp = fwp[0x20];
    if (temp != 0x00)
    {
        Query->Timeout_Buffer_Write = (1 << temp);
    }
    else
    {
        Query->Timeout_Buffer_Write = 0x00;
    }

    temp = 0;
    temp = fwp[0x21];
    Query->Timeout_Block_Erase = (1 << temp);

    temp = fwp[0x22];
    if (temp != 0x00)
    {
        Query->Timeout_Chip_Erase = (1 << temp);
    }
    else
    {
        Query->Timeout_Chip_Erase = 0x00;
    }

    temp = fwp[0x23];
    Query->Max_Timeout_Single_Write = (1 << temp) * Query->Max_Timeout_Single_Write;

    temp = fwp[0x24];
    if (temp != 0x00)
    {
        Query->Max_Timeout_Buffer_Write = (1 << temp) * Query->Max_Timeout_Buffer_Write;
    }
    else
    {
        Query->Max_Timeout_Buffer_Write = 0x00;
    }

    temp = fwp[0x25];
    Query->Max_Timeout_Block_Erase = (1 << temp) * Query->Timeout_Block_Erase;

    temp = fwp[0x26];
    if (temp != 0x00)
    {
        Query->Max_Timeout_Chip_Erase = (1 << temp) * Query->Max_Timeout_Chip_Erase;
    }
    else
    {
        Query->Max_Timeout_Chip_Erase = 0x00;
    }

    /* Device Geometry */
    temp = fwp[0x27];
    Query->Device_Size = (long) (((long)1) << temp);

    Query->Interface_Description = fwp[0x28];

    temp = fwp[0x2A];

    if (temp != 0x00)
    {
        Query->Max_Multi_Byte_Write = (1 << temp);
    }
    else
    {
        Query->Max_Multi_Byte_Write = 0;
    }

    Query->Num_Erase_Blocks = fwp[0x2C];

    for (i=0; i < Query->Num_Erase_Blocks; i++)
    {
        Query->Erase_Block[i].Num_Sectors = fwp[(0x2D+(4*i))];
        Query->Erase_Block[i].Num_Sectors++;
        Query->Erase_Block[i].Sector_Size = ((long)256 *( ((long)256 * fwp[(0x30+(4*i))]) + fwp[(0x2F+(4*i))]));													 
    }

    /* EXTENDED Query Information */

    /* Store primary table offset in variable for clarity */
    offset =  fwp[0x16] << 8  | Query->Primary_Table_Address;

    Query->Primary_Extended_Query[0] = fwp[(offset)];
    Query->Primary_Extended_Query[1] = fwp[(offset + 1)];
    Query->Primary_Extended_Query[2] = fwp[(offset + 2)];
    Query->Primary_Extended_Query[3] = '\0';

    if( Query->Primary_Extended_Query[0] != 'P' &&
        Query->Primary_Extended_Query[1] != 'R' &&
        Query->Primary_Extended_Query[2] != 'I')
    {
        return(1);
    }

    Query->Major_Version = fwp[(offset + 3)];
    Query->Minor_Version = fwp[(offset + 4)];

    Query->Optional_Feature      = (U8) (fwp[(offset+5)]);

    /* Decipher Optional Feature Bits */
    Query->Is_Chip_Erase         = (Query->Optional_Feature & (1<< 0))? 1:0;
    Query->Is_Erase_Suspend      = (Query->Optional_Feature & (1<< 1))? 1:0;
    Query->Is_Program_Suspend    = (Query->Optional_Feature & (1<< 2))? 1:0;
    Query->Is_Locking            = (Query->Optional_Feature & (1<< 3))? 1:0;
    Query->Is_Queue_Erase        = (Query->Optional_Feature & (1<< 4))? 1:0;
    Query->Is_Instant_Block_Lock = (Query->Optional_Feature & (1<< 5))? 1:0;
    Query->Is_Protection_Bits    = (Query->Optional_Feature & (1<< 6))? 1:0;
    Query->Is_Page_Read          = (Query->Optional_Feature & (1<< 7))? 1:0;

    /* Continue with further offsets */
    Query->Burst_Read            = (U8) (fwp[(offset+6)] & 0x0F);
    Query->Erase_Suspend         = (U8) (fwp[(offset+9)] & 0x0F);
    Query->Block_Protect         = (U8)  (fwp[(offset+10)] & 0x0F);
    Query->Vdd_Optimum           = (U8) (fwp[(offset+12)]);
    Query->Vpp_Optimum           = (U8) (fwp[(offset+13)]);
    Query->OTP_Protection        = (U8) (fwp[(offset+14)] & 0x0F);
    Query->Protection_Reg_LSB    = (U8) (fwp[(offset+15)] & 0x0F);
    Query->Protection_Reg_MSB    = (U8) (fwp[(offset+16)] & 0x0F);
    Query->Factory_Prg_Bytes     = (U8) (fwp[(offset+17)] & 0x0F);
    Query->User_Prg_Bytes        = (U8) (fwp[(offset+18)] & 0x0F);
    Query->Page_Read             = (U8) (fwp[(offset+19)] & 0x0F);
    Query->Synchronous_Config    = (U8) (fwp[(offset+20)] & 0x0F);

    return(0);

}

/* End of stflash.c */
