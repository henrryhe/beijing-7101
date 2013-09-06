/****************************************************************************

File Name   : ste2p.c

Description : EEPROM API Routines to support Direct Access to
              one or more ST M24C32/64 I2C-mapped EEPROM devices.

Copyright (C) 2005, STMicroelectronics

Revision History:

References  :

$ClearCase (VOB: ste2p)

STE2P.PDF "EEPROM API" Reference DVD-API-024 Revision 1.3

 ****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <string.h>

#ifndef ST_OS21
#include <ostime.h>
#include "task.h"
#endif

#include "stlite.h"
#include "stddefs.h"

#include "stpio.h"
#include "sti2c.h"
#include "ste2p.h"
#include "stcommon.h"

/* Private Types ---------------------------------------------------------- */

/* Linked list construct to support any number of EEPROM devices.
   Note that up to 8 M24C32/64 devices may be fitted per I2C bus.
   For a single device, normally Chip Enable lines E2..0 would
   be tied to Vcc/Vss lines to suitably define its I2C address.
   With multiple devices, E2..E0 (b3..1) are addressed with the
   Device Code select to chose which device is to be accessed.
   Multiple Open instances are supported by an extendable array
   which stores the segment start and length.  Only one element
   is defined in the structure, but 'C' performs no range checks
   on array accesses so this is safe to extend when memory is
   allocated for each linked-list element.  Zero length is used
   to flag a free slot, i.e. not Open.  A notional limit of 256
   Open instances is imposed by the partition in the Handle,
   identifying both the device instance and Open index in a U32. */

typedef struct inst_s
{
    struct inst_s        *Next;                      /* ptr. to next (else NULL) */
    ST_DeviceName_t     DeviceName;                 /* Initialization device name */
    U32                 BaseHandle;                 /* shifted pointer to this structure */
    BOOL                DeviceLock;                 /* lock flag to prevent multi-access */
    U32                 NumberOpen;                 /* Number of currently Open Handles */
    STI2C_Handle_t      I2C_Handle;                 /* returned on STI2C_Open() call */
    STE2P_InitParams_t  DeviceData;                 /* Device details from Init caller */
    U8*                 PageBuffer_p;              /* pointer to internal buffer allocated at Init */
    STE2P_OpenParams_t  MemSegment[1];              /* Open caller details (extendable) */
} ste2p_Inst_t;




/* Private Constants ------------------------------------------------------ */

#define DELAY_10MS          (ST_GetClocksPerSecond() / 100)
#define READ_SETTLING_TIME  0
#define WRITE_SETTLING_TIME (DELAY_10MS)

#if defined(ST_5105) || defined(ST_8010)
#define DEVICE_CODE_MSK     0xFE                    /* top nybble holds Device Code */
#else
#define DEVICE_CODE_MSK     0xF0                    /* top nybble holds Device Code */
#endif

#define I2C_BIT_WIDTH       8                       /* width of I2C data transfers */
#define I2C_WIDTH_MSK       ((1 << I2C_BIT_WIDTH )- 1)
#define I2C_10_BIT_ADDRESS  FALSE                   /* not extended device addressing */
#define I2C_BUS_XS_TIMEOUT  5                       /* bus access timeout in ms units */
#define I2C_BUS_RD_TIMEOUT  100                     /* read access timeout in ms units */
#define I2C_BUS_WR_TIMEOUT  100                     /* write access timeout in ms units */

/* The following control STE2P_Handle_t partitioning
   of the Instance Tag and the Open index number.
   If the width requires change, the masks self-modify */
#define OPEN_HANDLE_WID 8                           /* Open bits width */
#define OPEN_HANDLE_MSK (( 1 << OPEN_HANDLE_WID) - 1)
#define BASE_HANDLE_MSK (~OPEN_HANDLE_MSK)

/* Driver revision number */
static const ST_Revision_t ste2p_DriverRev = "STE2P-REL_1.1.12";

/* Private Variables ------------------------------------------------------ */
#ifdef ST_OS21
static  semaphore_t         *Atomic_p;
#else
static  semaphore_t         Atomic;
static  semaphore_t         *Atomic_p = NULL;   /* ptr. to first node */
#endif

static ste2p_Inst_t     *ste2p_Sentinel = NULL;     /* ptr. to first node */
/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

static BOOL IsHandleInvalid (STE2P_Handle_t  Handle,
                             ste2p_Inst_t    **Construct,
                             U32             *OpenIndex);

static BOOL IsOutsideRange (U32 ValuePassed,
                            U32 LowerBound,
                            U32 UpperBound);


/* Functions -------------------------------------------------------------- */

/****************************************************************************
Name         : STE2P_GetRevision()

Description  : Returns a pointer to the Driver Revision String.
               May be called at any time.

Parameters   : None

Return Value : ST_Revision_t

See Also     : ST_Revision_t
 ****************************************************************************/

ST_Revision_t STE2P_GetRevision(void)
{
    return (ste2p_DriverRev);
}


/****************************************************************************
Name         : STE2P_Init()

Description  : Generates a linked list element for each instance called,
               into which the caller's InitParams are copied, plus a few
               other vital parameters.

Parameters   : ST_DeviceName_t Name of the EEPROM device, and
               STE2P_InitParams_t *InitParams data structure pointer.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors occurred
               ST_ERROR_NO_MEMORY              Insufficient memory free
               ST_ERROR_ALREADY_INITIALISED    Device already initialized
               ST_ERROR_BAD_PARAMETER          Error in parameters passed

See Also     : STE2P_InitParams_t
               STE2P_GetParams()
               STE2P_Term()
 ****************************************************************************/

ST_ErrorCode_t STE2P_Init (const ST_DeviceName_t      Name,
                           const STE2P_InitParams_t   *InitParams)
{
    ste2p_Inst_t    *NextElem, *PrevElem;           /* search list pointers */
    ste2p_Inst_t    *ThisElem;                      /* element for this Init */

    U32             StructSize;
    int             i = 0;

    /* Perform necessary validity checks */
    if ((Name == NULL) || (Name[0] == '\0') || (strlen( Name ) >=  ST_MAX_DEVICE_NAME) || (InitParams == NULL))
    /* NUL Name string? */ /* NULL address for Name? */ /* Name string too long? */ /* NULL address for InitParams? */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if ((InitParams->BaseAddress != NULL)    ||    /* memory-mapped device? */
       (InitParams->I2CDeviceName[0] == '\0')     ||    /* I2C device name omitted? */
       (InitParams->SizeInBytes == 0)       ||    /* zero size device? */
       (( InitParams->DeviceCode & DEVICE_CODE_MSK ) != STE2P_SUPPORTED_DEVICE)   ||
       /* not M24C32/64 type device? */
       IsOutsideRange(InitParams->MaxSimOpen,STE2P_MIN_OPEN_CHANS,STE2P_MAX_OPEN_CHANS)
       /* invalid MaxSimOpen? */
       || (InitParams->DriverPartition == NULL))    /* driver partition omitted? */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Determine size of element for linked list, dependent upon MaxSimOpen */
    StructSize = sizeof(ste2p_Inst_t)+((InitParams->MaxSimOpen - 1)*sizeof(STE2P_OpenParams_t));

    /* first Open element */              /*  included in ste2p_Inst_t */
    /* The following section is protected from re-entrancy whilst critical
       elements within the ste2p_Sentinel linked list are both read and
       written.  It is not likely that Init will be called from a multi-
       tasking environment, but one never knows what a user might try....   */
    if (ste2p_Sentinel == NULL)                    /* first Init instance? */
    {
        task_lock();                                /* prevent re-entrancy */
        /* initialize semaphore */
#ifdef ST_OS21
        Atomic_p = semaphore_create_fifo_timeout(1);
#else
        Atomic_p = &Atomic;
        semaphore_init_fifo(Atomic_p, 1);
#endif

        semaphore_wait(Atomic_p);                  /* atomic sequence start */
        task_unlock();                              /* re-enable schedules etc */
        ste2p_Sentinel = ThisElem = (ste2p_Inst_t*)memory_allocate(InitParams->DriverPartition, StructSize);
        /* keep a note of result */
    }
    else                                            /* already Init'ed */
    {
        semaphore_wait(Atomic_p);                  /* atomic sequence start */

        for (NextElem = ste2p_Sentinel;NextElem != NULL;  NextElem = NextElem->Next)
        /* start at Sentinel node */ /* while not at list end */    /* advance to next element */
        {
            if (strcmp(Name,NextElem->DeviceName) == 0)       /* existing DeviceName? */
            {
                semaphore_signal(Atomic_p);        /* unlock before exit */
                return (ST_ERROR_ALREADY_INITIALIZED);
            }
            PrevElem = NextElem;                    /* keep trailing pointer */
        }

        PrevElem->Next = ThisElem = (ste2p_Inst_t*)memory_allocate(InitParams->DriverPartition, StructSize);
        /* append new element to list */
     }

    if (ThisElem == NULL)                          /* memory_allocate failure? */
    {
        semaphore_signal(Atomic_p);                /* unlock before exit */
        return (ST_ERROR_NO_MEMORY);
    }
    ThisElem->Next = NULL;                          /* mark as end of list */

    /* allocate memory block to hold pages when writing. 2 bytes are needed to hold
        address prefix */
    ThisElem->PageBuffer_p = (U8*)memory_allocate(InitParams->DriverPartition,
                                                  InitParams->PagingBytes == 0 ? 3 : InitParams->PagingBytes+2);

    if(ThisElem->PageBuffer_p == NULL)                          /* memory_allocate failure? */
    {
        semaphore_signal(Atomic_p);                /* unlock before exit */
        return (ST_ERROR_NO_MEMORY);
    }
    strcpy(ThisElem->DeviceName, Name);           /* retain Name for Inits/Opens/Terms */

    ThisElem->BaseHandle = (STE2P_Handle_t)((U32)ThisElem << OPEN_HANDLE_WID);

    /* use shifted pointer */                    /*  as handle base "tag" */
    ThisElem->DeviceLock = FALSE;                   /* device not currently accessed */
    ThisElem->NumberOpen = 0;                       /* No Open Handles yet */
    ThisElem->DeviceData = *InitParams;             /* copy Device details from caller */
    for(i = 0; i < InitParams->MaxSimOpen; i++)     /* loop on maximum Opens required */
    {
        ThisElem->MemSegment[i].Length = 0;         /* flag as empty slot (invalid length) */
    }
    semaphore_signal(Atomic_p);                    /* end of atomic region */
    return (ST_NO_ERROR);
}


/****************************************************************************
Name         : STE2P_Open()

Description  : Searches the linked list of Init instances for a string
               match with the Device name.   The address of the matching
               entry forms part of the Handle, the remaining part is the
               Open index.  For the first Open segment only, the I2C
               device needs to be opened.

Parameters   : ST_DeviceName_t Name is the Init supplied EEPROM Device,
               STE2P_OpenParams_t *OpenParams specifies the segment, and
               STE2P_Handle_t *Handle is written through to the caller.

Return Value : ST_ErrorCode_t specified as
               ST_ERROR_UNKNOWN_DEVICE     Bank name not found
               ST_ERROR_BAD_PARAMETER      Bad parameter detected
               ST_ERROR_NO_FREE_HANDLES    MaxSimOpen exceeded
               ST_NO_ERROR                 No errors occurred
               STE2P_ERROR_I2C             Error occured in I2C Open

See Also     : STE2P_Handle_t
               STE2P_Close()
 ****************************************************************************/

ST_ErrorCode_t STE2P_Open(const ST_DeviceName_t    Name,
                          const STE2P_OpenParams_t *OpenParams,
                          STE2P_Handle_t           *Handle)
{
    ste2p_Inst_t        *ThisElem;
    ST_ErrorCode_t      ErrorRet;
    STI2C_OpenParams_t  sti2c_OpenPars;
    U32                 i;

    /* Perform parameter validity checks */
    if ((Name  ==  NULL)   ||   (OpenParams  ==  NULL)   || (Handle  ==  NULL))
    /* Name ptr uninitialized? */ /* OpenParams ptr uninitialized? */  /* Handle ptr uninitialized? */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* One previous call to Init MUST have had the same Device Name */
    for (ThisElem = ste2p_Sentinel;ThisElem != NULL;  ThisElem = ThisElem->Next )
    /* start at Sentinel node */ /* while not at list end */  /* advance to next element */
    {
        if (strcmp( Name,ThisElem->DeviceName) == 0)       /* existing DeviceName? */
        {
            break;                                  /* match, terminate search */
        }
    }

    if (ThisElem == NULL)                          /* no Name match found? */
    {
        return (ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Now that the Init instance is determined, complete the validity checks */

    if (( OpenParams->Length == 0) || (( OpenParams->Offset +OpenParams->Length ) >  ThisElem->DeviceData.SizeInBytes ))
    /* Length passed as zero? */                        /* end address off device end? */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    if (ThisElem->NumberOpen >=ThisElem->DeviceData.MaxSimOpen)  /* all Open slots in use? */
    {
        return (ST_ERROR_NO_FREE_HANDLES);
    }

    /* Open call looks OK.  If first one, the I2C device needs to be opened.  */

    if (ThisElem->NumberOpen == 0)                 /* first valid Open call? */
    {
        sti2c_OpenPars.I2cAddress =ThisElem-> DeviceData.DeviceCode;

        /* device ID & index , Note the shift right */
        sti2c_OpenPars.AddressType = STI2C_ADDRESS_7_BITS;
        sti2c_OpenPars.BusAccessTimeOut = I2C_BUS_XS_TIMEOUT;
        ErrorRet = STI2C_Open(ThisElem->DeviceData.I2CDeviceName,&sti2c_OpenPars,&ThisElem->I2C_Handle);
        if (ErrorRet != ST_NO_ERROR)               /* I2C reporting Open failure? */
        {
            return (STE2P_ERROR_I2C);
        }
    }

    /* Locate a free MemSegment slot, protecting
       the structure in case of re-entrancy.
    */
    semaphore_wait(Atomic_p);                      /* atomic sequence start */

    for (i = 0;i < ThisElem->DeviceData.MaxSimOpen; i++)
    /* loop for max simultaneous Opens */
    {
        if (ThisElem->MemSegment[i].Length == 0)   /* slot free? */
        {
            ThisElem->MemSegment[i] = *OpenParams;  /* structure copy Offset & Length */
            ThisElem->NumberOpen++;                 /* increment Handles Open */
            *Handle = ThisElem->BaseHandle + i;     /* write back Handle to caller */
            semaphore_signal(Atomic_p);            /* end of critical region */
            return (ST_NO_ERROR);
        }
    }

    semaphore_signal(Atomic_p);                    /* end of critical region */
    return (ST_ERROR_NO_FREE_HANDLES);             /* this SHOULDN'T be taken */
}


/****************************************************************************
Name         : STE2P_GetParams()

Description  : Returns information on the specified EEPROM device, perloined
               from the Init and Open calls.

Parameters   : STE2P_Handle_t Handle points to the Init/Open instance, plus
               STE2P_GetParams_t *GetParams pointer to structure through
               which information is written.

Return Value : ST_ErrorCode_t specified as
               ST_ERROR_INVALID_HANDLE
               ST_ERROR_BAD_PARAMETER      Bad parameter detected
               ST_NO_ERROR                 No errors occurred

See Also     : STE2P_Params_t
               STE2P_Init()
               STE2P_Open()
 ****************************************************************************/

ST_ErrorCode_t STE2P_GetParams(STE2P_Handle_t Handle,
                                STE2P_Params_t *GetParams)
{
    ste2p_Inst_t        *ThisElem;                  /* element for this Open */
    U32                 OpenIndex;                  /* Open Handle number */

    /* Perform parameter validity checks */
    if (IsHandleInvalid(Handle,&ThisElem, &OpenIndex))       /* Handle passed invalid? */
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    if(GetParams == NULL)                                     /* has caller forgotten ptr? */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    GetParams->InitParams = ThisElem->DeviceData;             /* structure copy InitParams */
    GetParams->OpenParams = ThisElem->MemSegment[OpenIndex];  /* structure copy OpenParams */
    return (ST_NO_ERROR);
}


/****************************************************************************
Name         : STE2P_Read()

Description  : Performs a read over the range of bytes specified,
               provided that the SegOffset and NumberToRead range
               is completely contained within the Memory Segment
               specified at the time this Handle was Opened.
               If any bytes are outside the Segment, no device
               access is made and an error is returned.
               Note that reads are also paged (not mentioned as
               a requirement in the ST M24C64 data sheet).
               Previous attempts to cross page boundaries worked,
               but left the I2C bus line state invalid for the
               following EEPROM access (nice!).

Parameters   : STE2P_Handle_t Handle identifies the device,
               U32 SegOffset (from Segment base),
               U8 *Buffer into which data is to be written,
               U32 NumberToRead (bytes) from SegOffset, and
               U32 *NumberReadOK pointer (in bytes).

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Rogue Handle value
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_DEVICE_BUSY        Device is currently busy
               STE2P_ERROR_READ            Failure reading device
               STE2P_ERROR_I2C_READ        Error occured in I2C during read
               STE2P_ERROR_I2C_WRITE       Error occured in I2C during write


See Also     : STE2P_Init()
               STE2P_Write()
 ****************************************************************************/

ST_ErrorCode_t STE2P_Read(STE2P_Handle_t Handle,
                          U32            SegOffset,
                          U8             *Buffer,
                          U32            NumberToRead,
                          U32            *NumberReadOK)
{
    ste2p_Inst_t        *ThisElem;
    ST_ErrorCode_t      RetValue;                   /* set by STI2C_Write/Read() */
    U8                  I2CData[2];                 /* read address to device */
    U32                 SegBaseAd;                  /* Open Base of Segment */
    U32                 SegAccess;                  /* SegBaseAd + SegOffset */
    U32                 SegExtent;                  /* SegBaseAd + Length */
    U32                 OpenIndex;                  /* Open Handle number */
    U32                 NumReadOK;                  /* accumulative (paging) */
    U32                 I2CRetLen;                  /* return length from I2C */
    U32                 PageBytes;                  /* bytes in current page */
    U32                 i;                          /* general index */
    clock_t tstart;                                 /* tracking timeout */

    *NumberReadOK = NumReadOK = 0;                  /* no bytes read so far */

    /* Perform parameter validity checks */
    if (IsHandleInvalid(Handle,&ThisElem, &OpenIndex))  /* Handle passed invalid? */
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    if (NumberToRead == 0)                         /* no bytes to read? */
    {
        return (ST_NO_ERROR);                      /* nothing to do, OK */
    }

    /* Ensure area to be accessed doesn't overflow the segment */
    SegBaseAd = ThisElem->MemSegment[OpenIndex].Offset;
                                                /* localize segment base address */
    SegAccess = SegBaseAd + SegOffset;              /* device access address */
    SegExtent = SegBaseAd + ThisElem->MemSegment[OpenIndex].Length;
                                                   /* localize segment extent */


    if ((SegAccess + NumberToRead) > SegExtent)     /* above segment extent? */

    {
        return (ST_ERROR_BAD_PARAMETER);           /* no reads if ANY overrun */
    }

    task_lock();                                    /* protect during ReadModifyWrite */
    semaphore_wait(Atomic_p);                      /* atomic sequence start */
    if (ThisElem->DeviceLock)                      /* Device in use by another caller? */
    {
        semaphore_signal(Atomic_p);                /* atomic sequence end */
        task_unlock();
        return ( ST_ERROR_DEVICE_BUSY );
    }
    ThisElem->DeviceLock = TRUE;                    /* Grab the device */
    semaphore_signal(Atomic_p);                    /* atomic sequence end */
    task_unlock();
    if (ThisElem->DeviceData.PagingBytes <= 1)                      /* paging mode not enabled? */
    {
        for (i = 0; i < NumberToRead; i++)                          /* loop on NumberToRead */
        {
            I2CData[0] = ( SegAccess + i )>> I2C_BIT_WIDTH;          /* HO address byte first */
            I2CData[1] = (SegAccess + i) & I2C_WIDTH_MSK;          /*  then LO address byte */

            /* Try to address the device first -- note that the device may
             * be performing an internal write cycle, so we poll it until
             * it responds or a timeout occurs.
             */
            tstart = time_now();
            for (;;)
            {
                RetValue = STI2C_WriteNoStop(ThisElem->I2C_Handle,
                                             I2CData,
                                             2,
                                             I2C_BUS_WR_TIMEOUT,
                                             &I2CRetLen );
                /* Check for acknowledge */
                if (RetValue == ST_NO_ERROR)
                {
                    break;
                }
                /* Check for a timeout */
                if (time_minus(time_now(), tstart) > DELAY_10MS)
                {
                    break;
                }
                /* Deschedule and then try again */
                task_reschedule();
            }

            if (RetValue != ST_NO_ERROR)
            {
                ST_ErrorCode_t LocalRetValue;
                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberReadOK = NumReadOK;          /* report number read to date */

                /* ensure I2C bus is left unlocked */
                LocalRetValue = STI2C_Write(ThisElem->I2C_Handle,
                                             I2CData,
                                             0,
                                             I2C_BUS_WR_TIMEOUT,
                                             &I2CRetLen );
                return (STE2P_ERROR_I2C_WRITE);

            }

            if (I2CRetLen != 2)                    /* logical error? */
            {
                ST_ErrorCode_t LocalRetValue;

                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberReadOK = NumReadOK;          /* report number read to date */

                /* ensure I2C bus is left unlocked */
                LocalRetValue = STI2C_Write(ThisElem->I2C_Handle,
                                             I2CData,
                                             0,
                                             I2C_BUS_WR_TIMEOUT,
                                             &I2CRetLen );
                return (STE2P_ERROR_READ);
            }


            /* read single byte into Buffer */

            RetValue = STI2C_Read(                  /* perform single read */
                ThisElem->I2C_Handle,
                    &Buffer[i], 1, I2C_BUS_RD_TIMEOUT, &I2CRetLen);

            if (RetValue != ST_NO_ERROR)
            {
                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberReadOK = NumReadOK;          /* report number read to date */
                return (STE2P_ERROR_I2C_READ);

            }

            if (I2CRetLen != 1)                    /* not read a single byte? */
            {
                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberReadOK = NumReadOK;          /* report number read to date */
                return (STE2P_ERROR_READ);         /* indicate E2P internal error */
            }
            NumReadOK++;
        }                                           /* for each byte of NumberToRead */
    }
    else
    {
        /* Read operations exploit the page mode operation on the EEPROM
           device to minimize I2C access.  The page boundaries must be
           respected otherwise the I2C bus state appears to corrupt for
           subsequent EEPROM access attempts, although the correct data
           appears to be returned.  This "feature" is not documented in
           the data sheet, which infers that any number of sequential
           reads can be made.                                             */

        do
        {
            PageBytes =                             /* bytes to end of current page */
                ThisElem->DeviceData.PagingBytes -
                ( ( SegAccess + NumReadOK ) %
                ThisElem->DeviceData.PagingBytes );

            if (PageBytes > (NumberToRead - NumReadOK))     /* last (partial) page? */
            {
                PageBytes = NumberToRead - NumReadOK;       /* reduce count to finish */
            }
            I2CData[0] = (SegAccess +NumReadOK) >> I2C_BIT_WIDTH;    /* HO address byte first, */
            I2CData[1] = (SegAccess + NumReadOK)  & I2C_WIDTH_MSK;   /*  LO address byte next, */

            /* Try to address the device first -- note that the device may
             * be performing an internal write cycle, so we poll it until
             * it responds or a timeout occurs.
             */
            tstart = time_now();
            for (;;)
            {

                RetValue = STI2C_WriteNoStop(ThisElem->I2C_Handle,
                                             I2CData,
                                             2,
                                             I2C_BUS_WR_TIMEOUT,
                                             &I2CRetLen );
                /* Check for acknowledge */
                if (RetValue == ST_NO_ERROR)
                {
                    break;
                }
                /* Check for a timeout */
                if (time_minus(time_now(), tstart) > DELAY_10MS)
                {
                    break;
                }
                /* Deschedule and then try again */
                task_reschedule();
            }

            if (RetValue != ST_NO_ERROR)
            {
                ST_ErrorCode_t LocalRetValue;

                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberReadOK = NumReadOK;          /* report number read to date */

                /* ensure I2C bus is left unlocked */
                LocalRetValue = STI2C_Write(ThisElem->I2C_Handle,
                                             I2CData,
                                             0,
                                             I2C_BUS_WR_TIMEOUT,
                                             &I2CRetLen);
                return (STE2P_ERROR_I2C_WRITE);

            }

            if (I2CRetLen != 2)                    /* logical error? */
            {
                ST_ErrorCode_t LocalRetValue;
                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberReadOK = NumReadOK;          /* report number read to date */
                LocalRetValue = STI2C_Write(ThisElem->I2C_Handle,
                                             I2CData,
                                             0,
                                             I2C_BUS_WR_TIMEOUT,
                                             &I2CRetLen);
                return (STE2P_ERROR_WRITE);
            }
            /* read up to next page boundary */

            RetValue = STI2C_Read(                  /* input from I2C */
                ThisElem->I2C_Handle,
                    &Buffer[NumReadOK],
                        PageBytes, I2C_BUS_RD_TIMEOUT, &I2CRetLen);

            if (RetValue != ST_NO_ERROR)             /* I2C error? */
            {
                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberReadOK = NumReadOK;          /* report number read to date */
                return (STE2P_ERROR_I2C_READ);
            }

            if (I2CRetLen != PageBytes)              /* logical error? */
            {
                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberReadOK = (NumReadOK + I2CRetLen);  /* advise of number believed OK */
                return (STE2P_ERROR_READ);          /* indicate E2P internal error */
            }

            NumReadOK += PageBytes;                 /* update for last page */

        } while (NumReadOK < NumberToRead);        /* more page(s) to read? */
    }

    ThisElem->DeviceLock = FALSE;                   /* device access completed */

    *NumberReadOK = NumReadOK;                      /* report total number read */

    return (ST_NO_ERROR);
}


/****************************************************************************
Name         : STE2P_Write()

Description  : Performs a write over the range of bytes specified,
               provided that the SegOffset and NumberToWrite range
               is completely contained within the Memory Segment
               specified at the time this Handle was Opened.
               If any bytes are outside the Segment, no device access
               is made and an error is returned.

Parameters   : STE2P_Handle_t Handle identifies the EEPROM device,
               U32 Offset (from device base) to write to,
               U8 Buffer pointer from which data will be read,
               U32 NumberToWrite in bytes, and
               U32 *NumberWrittenOK pointer (bytes).

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Rogue Flash bank indicator
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_DEVICE_BUSY        Device is currently busy
               STE2P_ERROR_WRITE           Error during write
               STE2P_ERROR_I2C_WRITE       Error occured in I2C during write

See Also     : STE2P_Init()
               STE2P_Write()
 ****************************************************************************/

ST_ErrorCode_t STE2P_Write( STE2P_Handle_t Handle,
                            U32            SegOffset,
                            U8             *Buffer,
                            U32            NumberToWrite,
                            U32            *NumberWrittenOK )
{
    ste2p_Inst_t    *ThisElem;
    ST_ErrorCode_t  RetValue;
    U8*             I2CData;
    U32             SegBaseAd;                      /* Open Base of Segment */
    U32             SegAccess;                      /* SegBaseAd + SegOffset */
    U32             SegExtent;                      /* SegBaseAd + Length */
    U32             OpenIndex;                      /* Open Handle number */
    U32             I2CRetLen;                      /* return length from I2C */
    U32             WrittenOK;                      /* local copy */
    U32             PageBytes;                      /* bytes in current page */
    U32             i;                              /* general index */
    clock_t         tstart;                         /* timeout tracking */

    *NumberWrittenOK = WrittenOK = 0;               /* no bytes written yet */

    /* Perform parameter validity checks */

    if (IsHandleInvalid(Handle,&ThisElem,&OpenIndex)) /* Handle passed invalid? */
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    if (NumberToWrite == 0)                        /* no bytes to write? */
    {
        return (ST_NO_ERROR);                      /* nothing to do, OK */
    }

    if (Buffer == NULL)                            /* source pointer omitted? */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Ensure area to be accessed doesn't overflow the segment */

    SegBaseAd = ThisElem->MemSegment[OpenIndex].Offset; /* localize segment base address */


    SegAccess = SegBaseAd + SegOffset;                  /* device access address */

    SegExtent = SegBaseAd + ThisElem->MemSegment[OpenIndex].Length; /* localize segment extent */

    if ((SegAccess+NumberToWrite) > SegExtent)                              /* above segment extent? */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    task_lock();                                    /* protect during ReadModifyWrite */
    semaphore_wait(Atomic_p);                        /* atomic sequence start */
    if (ThisElem->DeviceLock)                      /* Device in use by another caller? */
    {
        semaphore_signal(Atomic_p);                /* atomic sequence end */
        task_unlock();
        return (ST_ERROR_DEVICE_BUSY);
    }
    ThisElem->DeviceLock = TRUE;                    /* Grab the device */
    semaphore_signal(Atomic_p);                    /* atomic sequence end */
    task_unlock();

    I2CData = ThisElem->PageBuffer_p;               /* buffer allocated during Init */
    /* Guaranteed thread safe by DeviceLock; alternatively use WriteNoStop
      address and the Write for data */

    if (ThisElem->DeviceData.PagingBytes <= 1)     /* paging mode not enabled? */
    {
        for (i = 0; i < NumberToWrite; i++)        /* loop on NumberToWrite */
        {
            I2CData[0] = (SegAccess + i) >> I2C_BIT_WIDTH;          /* HO address byte first, */
            I2CData[1] = (SegAccess + i)& I2C_WIDTH_MSK;          /*  LO address byte second */
            I2CData[2] = Buffer[i];                              /*   then data byte */

            /* Note that the device may be performing an internal
             * write cycle, so we poll it until it responds or a timeout occurs.
             */
            tstart = time_now();
            for (;;)
            {
                RetValue = STI2C_Write(ThisElem->I2C_Handle,
                                       I2CData,
                                       3,
                                       I2C_BUS_WR_TIMEOUT,
                                       &I2CRetLen );

                /* Check for acknowledge */
                if (RetValue == ST_NO_ERROR)
                {
                    break;
                }
                /* Check for a timeout */
                if (time_minus(time_now(), tstart) > DELAY_10MS)
                {
                    break;
                }
                /* Deschedule and then try again */
                task_reschedule();
            }

            if (RetValue != ST_NO_ERROR)
            {
                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberWrittenOK = WrittenOK;
                return (STE2P_ERROR_I2C_WRITE);
            }

            if (I2CRetLen != 3)                    /* logical error? */
            {
                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberWrittenOK = WrittenOK;
                return (STE2P_ERROR_WRITE);
            }
            WrittenOK++;                            /* for each byte of NumberToWrite */
        }
    }
    else                                            /* page write mode enabled */
    {
        /* Write operations exploit the page mode operation on the EEPROM
           device to minimize I2C access.  The page boundaries MUST be
           respected otherwise a wrap-around overwrite will occur.  There
           is still the WRITE_SETTLING_TIME required between writes, but
           this is per page rather than per byte as in the above code.     */

        do
        {
            PageBytes =                             /* bytes to end of current page */
                ThisElem->DeviceData.PagingBytes - (( SegAccess + WrittenOK) %ThisElem->DeviceData.PagingBytes);

            if (PageBytes > (NumberToWrite - WrittenOK ))  /* last (partial) page? */
            {
                PageBytes = NumberToWrite - WrittenOK;      /* reduce count to finish */
            }
            I2CData[0] = (SegAccess +WrittenOK ) >> I2C_BIT_WIDTH;   /* HO address byte first, */
            I2CData[1] = (SegAccess + WrittenOK) & I2C_WIDTH_MSK;    /*  LO address byte next, */
            memcpy(&I2CData[2], &Buffer[WrittenOK], PageBytes) ;     /*   then PageBytes of data */

            /* Note that the device may be performing an internal
             * write cycle, so we poll it until it responds or a timeout occurs.
             */
            tstart = time_now();
            for (;;)
            {
                RetValue = STI2C_Write(ThisElem->I2C_Handle,
                                       I2CData, PageBytes + 2,
                                       I2C_BUS_WR_TIMEOUT,
                                       &I2CRetLen);

                /* Check for acknowledge */
                if (RetValue == ST_NO_ERROR)
                {
                    break;
                }
                /* Check for a timeout */
                if (time_minus(time_now(), tstart) > DELAY_10MS)
                {
                    break;
                }
                /* Deschedule and then try again */
                task_reschedule();
            }

            if (RetValue != ST_NO_ERROR)           /* I2C error? */
            {
                ThisElem->DeviceLock = FALSE;       /* Free the device */
                *NumberWrittenOK = WrittenOK;       /* advise of successful pages */
                return (STE2P_ERROR_I2C_WRITE);
            }

            if (I2CRetLen != (PageBytes + 2))    /* logical error? */
            {
                ThisElem->DeviceLock = FALSE;       /* Free the device */

                *NumberWrittenOK =                  /* advise of number believed OK */
                    (WrittenOK + I2CRetLen - 2);

                return (STE2P_ERROR_WRITE);        /* indicate E2P internal error */
            }

            WrittenOK += PageBytes;                 /* update for last page */

        } while (WrittenOK < NumberToWrite);       /* more page(s) to write? */
    }

    ThisElem->DeviceLock = FALSE;                   /* Free the device */
    *NumberWrittenOK = WrittenOK;                   /* return count to caller */

    return ( ST_NO_ERROR );
}


/****************************************************************************
Name         : STE2P_Close()
Description  : Flags the specified EEPROM device segment as closed.  For
               the last segment closed, the I2C Handle is also closed.

Parameters   : STE2P_Handle_t Handle identifies an EEPROM device & segment

Return Value : ST_ErrorCode_t specified as
               ST_ERROR_INVALID_HANDLE     Rogue Handle value
               ST_NO_ERROR                 No errors occurred
               STE2P_ERROR_I2C             Error occured in I2C Close

See Also     : STE2P_Handle_t
               STE2P_Open()
 ****************************************************************************/

ST_ErrorCode_t STE2P_Close( STE2P_Handle_t Handle )
{
    ste2p_Inst_t        *ThisElem;
    ST_ErrorCode_t      ErrorRet = ST_NO_ERROR;
    U32                 OpenIndex;                  /* Open Handle number */

    /* Perform parameter validity checks */
    if (IsHandleInvalid(Handle,&ThisElem, &OpenIndex)) /* Handle passed invalid? */
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    /* protected section */
    semaphore_wait(Atomic_p);                      /* atomic sequence start */
    ThisElem->MemSegment[OpenIndex].Length = 0;     /* free the Open slot */
    ThisElem->NumberOpen--;                         /* decrement Handles Open */
    semaphore_signal(Atomic_p);

    if (ThisElem->NumberOpen == 0)                 /* last segment closed? */
    {
        ErrorRet = STI2C_Close(ThisElem->I2C_Handle);/* close the I2C Handle */
        if (ErrorRet != ST_NO_ERROR)
        {
            ErrorRet = STE2P_ERROR_I2C;
        }
    }
    return (ErrorRet);
}


/****************************************************************************
Name         : STE2P_Term()

Description  : Terminates an instance of the EEPROM Interface, optionally
               closing any Open Handles if forced, including I2C Handle.

Parameters   : ST_DeviceName Name specifies name of EEPROM device, and
               STE2P_TermParams_t *TermParams holds the ForceTerminate flag.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_UNKNOWN_DEVICE     Init not called
               ST_ERROR_OPEN_HANDLE        Close not called
               STE2P_ERROR_I2C             Error occured in I2C Close

See Also     : STE2P_Init()
 ****************************************************************************/

ST_ErrorCode_t STE2P_Term(const ST_DeviceName_t Name,
                          const STE2P_TermParams_t *TermParams)
{
    ste2p_Inst_t        *ThisElem, *PrevElem;
    ST_ErrorCode_t      ErrorRet;

    /* Perform parameter validity checks */
    if ((Name  == NULL)  || (TermParams == NULL))  /* NULL Name ptr? */ /* NULL structure ptr? */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if (ste2p_Sentinel == NULL)                    /* no Inits outstanding? */
    {
        return(ST_ERROR_UNKNOWN_DEVICE);          /* Semaphore not created */
    }

    task_lock();                                    /* make sure following completes */
    semaphore_wait(Atomic_p);                      /* start of atomic region */
    task_unlock();

    for (ThisElem = ste2p_Sentinel;  ThisElem != NULL;   ThisElem = ThisElem->Next)
    /* start at Sentinel node */ /* while not at list end */  /* advance to next element */
    {
        if (strcmp(Name,ThisElem->DeviceName) == 0)   /* existing DeviceName? */
        {
            break;
        }

        PrevElem = ThisElem;                        /* keep back marker */
    }

    if (ThisElem == NULL)                          /* no Name match found? */
    {
        semaphore_signal(Atomic_p);                /* end atomic before return */
        return (ST_ERROR_UNKNOWN_DEVICE);
    }

    if (ThisElem->NumberOpen > 0)                  /* any Handles Open? */
    {
        if (!TermParams->ForceTerminate)           /* unforced Closure? */
        {
            semaphore_signal(Atomic_p);            /* end atomic before return */

            return (ST_ERROR_OPEN_HANDLE);         /* user needs to call Close(s) */
        }

        ErrorRet = STI2C_Close(ThisElem->I2C_Handle); /* close the I2C Handle */

        if (ErrorRet != ST_NO_ERROR)               /* hand-me-down error code */
        {
            semaphore_signal(Atomic_p);            /* end atomic before return */
            return (STE2P_ERROR_I2C);              /* memory not freed */
        }
    }

    if (ThisElem == ste2p_Sentinel)
    {
        ste2p_Sentinel = ThisElem->Next;            /* remove first item */
    }
    else
    {
        PrevElem->Next = ThisElem->Next;            /* remove mid-list item */
    }

    semaphore_signal(Atomic_p);                    /* end of atomic region */
    memory_deallocate(ThisElem->DeviceData.DriverPartition, ThisElem);
    memory_deallocate(ThisElem->DeviceData.DriverPartition, ThisElem->PageBuffer_p);

    if (ste2p_Sentinel == NULL)                    /* no Inits outstanding? */
    {
        semaphore_delete(Atomic_p);                /* semaphore NLR */
    }

    return (ST_NO_ERROR);
}


/****************************************************************************
Name         : IsHandleInvalid()

Description  : Multi-purpose procedure to check Handle validity, plus return
               the address of its data structure element and the Open Index.

Parameters   : STE2P_Handle_t Handle number,
               STE2P_Inst_t *Construct pointer to element within linked list,and
               U32* OpenIndex.

Return Value : TRUE  if Handle is invalid
               FALSE if Handle appears to be legal
 ****************************************************************************/

static BOOL IsHandleInvalid( STE2P_Handle_t  Handle,
                             ste2p_Inst_t    **Construct,
                             U32             *OpenIndex )
{
    STE2P_Handle_t      TestHandle;
    ste2p_Inst_t        *ThisElem;
    U32                 OpenOffset;

    TestHandle = Handle & BASE_HANDLE_MSK;          /* strip off Open count */

    for (ThisElem = ste2p_Sentinel; ThisElem != NULL; ThisElem = ThisElem->Next)
    /* start at Sentinel node */  /* while not at list end */  /* advance to next element */
    {
        if (ThisElem->BaseHandle == TestHandle)    /* located Construct? */
        {
            OpenOffset = Handle & OPEN_HANDLE_MSK;  /* reveal Open offset only */

            if (ThisElem->MemSegment[OpenOffset].Length == 0) /* closed? */
            {
                return TRUE;                        /* Invalid Handle */
            }

            *Construct = ThisElem;                  /* write-back location */
            *OpenIndex = OpenOffset;                /*  and Open Index */
            return FALSE;                           /* indicate valid Handle */
        }
    }

    return TRUE;                                    /* Invalid BaseHandle */
}

/****************************************************************************
Name         : IsOutsideRange()

Description  : Checks if U32 formal parameter 1 is outside the range of
               bounds (U32 formal parameters 2 and 3).

Parameters   : Value for checking, followed by the two bounds, preferably
               the lower bound followed by the upper bound.

Return Value : TRUE  if parameter is outside given range
               FALSE if parameter is within  given range
 ****************************************************************************/

static BOOL IsOutsideRange(U32 ValuePassed,
                           U32 LowerBound,
                           U32 UpperBound)
{
    U32 Temp;

    /* ensure bounds are the correct way around */
    if (UpperBound < LowerBound) /* bounds require swapping */
    {
        Temp       = LowerBound;
        LowerBound = UpperBound;
        UpperBound = Temp;
    }

    if ((ValuePassed < LowerBound) || (ValuePassed > UpperBound))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


/* End of ste2p.c module */

