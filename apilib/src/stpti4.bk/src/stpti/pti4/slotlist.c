/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: slotlist.c
 Description: A very simple allocator for partitioning the TC pid lookup table
              between sessions.

Initial version  (DTSTTCPW)         GJP     23 Sept 2003


1. With this code once a table of a a certain size has been allocated it can be
  shrunk, remain the same but not be grown (for each STPTI_Init/STPTI_Term cycle)

2. Once all sessions have been allocated any remaining free slots will 'disappear' 
  (remain unallocated) as there is not free session left for them to be used with. 

4. ALL sessions must be terminated before the whole lookup table
  can be reallocated for a larger lookup table.

  It is possible with this allocator to build a function to compact the tables to
 free up larger contiguous areas but this has not been done (yet) because it
 must be noted (by customers and internal users) that compacting the tables
 would glitch any active sessions as the PTI must be paused for a table update,
 so you might as well kill any sessions and start again.

Loader params
-------------

1. Searching memory range is defined as: base -> base + size - 1.
2. base is the initial memory location: 0x0000 -> 0x1FFE. ( Bit#0 is not considered )
3. size is the number of memory locations that will be searched up: 0x0000 -> 0x00FF.
4. The locations correspond to 16 bit words.



******************************************************************************/


/* Includes ------------------------------------------------------------ */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

#include "stddefs.h"
#include "stdevice.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #if !defined(ST_OSLINUX) */
#include "stpti.h"
#include "pti_loc.h"
#include "memget.h"
#include "pti_hal.h"
#include "pti4.h"


/* ------------------------------------------------------------------------- */


#define TC_DSRAM_BASE 0x8000    /* break OAOO rule, move later */


/* ------------------------------------------------------------------------- */


typedef enum SlotListState_e 
{
    SL_EMPTY,   /* the area is *not valid* (never been used) */
    SL_IN_USE,  /* the area is in active use by a session */
    SL_FREE     /* the area is valid but not in use */
} SlotListState_t;


typedef struct SlotList_s   /* we have as may of theses as there are sessions */
{
    U32  Start;
    U32  End;
    SlotListState_t State;
} SlotList_t;


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiHelper_SlotList_Init( FullHandle_t DeviceHandle )
{
    Device_t             *Device        = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t      *PrivateData_p = &Device->TCPrivateData;
    ST_Partition_t       *Partition_p   =  NULL;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    int a, NumberOfBytes;

#if !defined(ST_OSLINUX)
    Partition_p   =  Device->DriverPartition_p;
#endif /* #if !defined(ST_OSLINUX) */

    STTBX_Print(("\nstptiHelper_SlotList_Init()\n"));

    /* Prevent compiler warning with memory_allocate() for platforms which ignore the first parameter */
    Partition_p = Partition_p;

    PrivateData_p->SlotList = memory_allocate( Partition_p, sizeof( SlotList_t ) * TC_Params_p->TC_NumberOfSessions );

    if ( PrivateData_p->SlotList == NULL )
        return( ST_ERROR_NO_MEMORY );


    /* reset the data area to 'never been used' */
    for( a = 0; a < TC_Params_p->TC_NumberOfSessions; a++ )
    {
        PrivateData_p->SlotList[a].Start = 0;   /* an assert is done using this (elsewhere in the code) so keep it clean */
        PrivateData_p->SlotList[a].End   = 0;
        PrivateData_p->SlotList[a].State = SL_EMPTY;
    }
    
    NumberOfBytes = TC_Params_p->TC_NumberSlots * 2;    /* we are dealing with U16 (bit#0 never set) */

    /* alocate the whole of the lookup table to the very first area and mark it as free */

    PrivateData_p->SlotList[0].Start = (U32)TC_Params_p->TC_LookupTableStart;
    PrivateData_p->SlotList[0].End   = (U32)TC_Params_p->TC_LookupTableStart + NumberOfBytes - 1;
    PrivateData_p->SlotList[0].State = SL_FREE;

    STTBX_Print((" slotlist.c: Start: 0x%08x\n", PrivateData_p->SlotList[0].Start ));

    STTBX_Print(("               End: 0x%08x (%d)\n", PrivateData_p->SlotList[0].End,  
            PrivateData_p->SlotList[0].End - PrivateData_p->SlotList[0].Start + 1));

    STTBX_Print(("             Slots: %d\n", TC_Params_p->TC_NumberSlots ));
    STTBX_Print(("          Sessions: %d\n", TC_Params_p->TC_NumberOfSessions ));

    return( ST_NO_ERROR );
}


/* ------------------------------------------------------------------------- */


#if !defined ( STPTI_BSL_SUPPORT )
ST_ErrorCode_t stptiHelper_SlotList_Term( FullHandle_t DeviceHandle )
{
    Device_t             *Device        = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t      *PrivateData_p = &Device->TCPrivateData;
    ST_Partition_t       *Partition_p   =  NULL;


#if !defined(ST_OSLINUX)
    Partition_p   =  Device->DriverPartition_p;
#endif

    /* trust that this is the last session. */
    STTBX_Print(("\nstptiHelper_SlotList_Term()\n"));

    /* Prevent compiler warning with memory_allocate() for platforms which ignore the first parameter */
    Partition_p = Partition_p;

    if ( PrivateData_p->SlotList != NULL )
        memory_deallocate( Partition_p, PrivateData_p->SlotList );

    PrivateData_p->SlotList = NULL;

    return( ST_NO_ERROR );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/* ------------------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
static void DumpSlotList( TCPrivateData_t *PrivateData_p, STPTI_TCParameters_t *TC_Params_p )
{
    int a;

    for( a = 0; a < TC_Params_p->TC_NumberOfSessions; a++ )
    {
        STTBX_Print(("index %d\n", a));

        STTBX_Print(("  Start: 0x%08x\n", PrivateData_p->SlotList[a].Start ));
        STTBX_Print(("    End: 0x%08x (%d)\n", PrivateData_p->SlotList[a].End,  
            (PrivateData_p->SlotList[a].End - PrivateData_p->SlotList[a].Start + 1)/2 ));
        STTBX_Print(("  State: " ));

        switch( PrivateData_p->SlotList[a].State )
        {
            case SL_EMPTY:
                STTBX_Print(("EMPTY\n"));
                break;

            case SL_IN_USE:
                STTBX_Print(("IN_USE\n"));
                break;

            case SL_FREE:
                STTBX_Print(("FREE\n"));
                break;

            default:
                STTBX_Print(("???\n"));
                break;
                    
        }
    }
}
#endif


/* ------------------------------------------------------------------------- */

/* should move & export this in the TC HAL sometime */

static U32 St20ToTcRamAddress( U32 SomeSt20Address, STPTI_TCParameters_t *TC_Params_p )
{
    return ( (U32) ( (U8 *)SomeSt20Address - (U8 *)TC_Params_p->TC_DataStart + (U8 *)TC_DSRAM_BASE) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiHelper_SlotList_Alloc( FullHandle_t DeviceHandle )
{
    Device_t             *Device        = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t      *PrivateData_p = &Device->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;

    U16 SlotsToAllocate = Device->NumberOfSlots;
    U16 BytesToAllocate = SlotsToAllocate * 2;
    
    SlotList_t *SlotList;
    int a, b, remainder;

    STTBX_Print(("\nstptiHelper_SlotList_Alloc() session %d", Device->Session ));

    /* loop around the slot lookup table lists in search of a free area */

    for( a = 0; a < TC_Params_p->TC_NumberOfSessions; a++ )
    {
        SlotList = &PrivateData_p->SlotList[a];

        STTBX_Print(("\nlooking at index %d...", a ));

        if ( SlotList->State == SL_FREE )    /* found a free area! */
        {
            STTBX_Print(("its Free.\n"));

            STTBX_Print(("Free space test 0x%08x > 0x%08x ?\n", (SlotList->Start + BytesToAllocate - 1), SlotList->End ));

            if ( (SlotList->Start + BytesToAllocate - 1) > SlotList->End )
                continue;   /* Ahhh! its too small, go round again to see if another area is big enough */

            SlotList->State = SL_IN_USE;   /* mark it, as its ours now! */

            /* tell the TC which area it can program the SE for to scan for pids */
            TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device->Session ];
            STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionPIDFilterStartAddr,St20ToTcRamAddress( SlotList->Start, TC_Params_p ));
            STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionPIDFilterLength,SlotsToAllocate);

            STTBX_Print(("TC SessionPIDFilterStartAddr: 0x%04x\n", TCSessionInfo_p->SessionPIDFilterStartAddr ));
            STTBX_Print(("TC    SessionPIDFilterLength: 0x%04x\n", TCSessionInfo_p->SessionPIDFilterLength ));

            remainder = (SlotList->End - (SlotList->Start + BytesToAllocate-1)) / 2;

            STTBX_Print((" remainder: %d\n", remainder ));

            STTBX_Print((" SlotList->End   0x%08x\n", SlotList->End ));
            STTBX_Print((" SlotList->Start 0x%08x\n", SlotList->Start ));
            STTBX_Print((" BytesToAllocate %d\n", BytesToAllocate ));

            /* if we have not used up all the free slots in this area then look for an empty area (SL_EMPTY) 
              to put information about what is left free. */
            if ( remainder > 0 )
            {
                for( b = 0; b < TC_Params_p->TC_NumberOfSessions; b++ ) /* scan all areas from the beginning */
                {
                   if ( PrivateData_p->SlotList[b].State == SL_EMPTY )  /* found an empty area */
                   {
                       /* calc & program in what is left free */
                        PrivateData_p->SlotList[b].Start = SlotList->Start + BytesToAllocate;
                        PrivateData_p->SlotList[b].End   = SlotList->End;
                        PrivateData_p->SlotList[b].State = SL_FREE; /* mark as free for use */

                        break;
                   }
                }

                /* If we could not find a SL_EMPTY (therefore all sessions must have been 
                  allocated - even if free now) just forget the remainder as there is no
                  spare session for it. */
            }

            /* update our area once any new free area might have been setup */
            SlotList->End = SlotList->Start + BytesToAllocate - 1;

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
            DumpSlotList( PrivateData_p, TC_Params_p );
#endif

            return( ST_NO_ERROR );
        }
    }


    STTBX_Print(("cannot allocate!\n"));

    return( ST_ERROR_NO_MEMORY );
}


/* ------------------------------------------------------------------------- */

#if !defined ( STPTI_BSL_SUPPORT )
/* ST20 holds a unique session number which is an index into the TC data structure from
  which we can get the lookup table start and see if it matches one of our data
  areas. If there is a match then free the area for use and wipe the TC table
  data. */

ST_ErrorCode_t stptiHelper_SlotList_Free( FullHandle_t DeviceHandle )
{
    Device_t             *Device        = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t      *PrivateData_p = &Device->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;

    SlotList_t *SlotList;
    int a;

    if( TC_Params_p != NULL )
    {
        TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device->Session ];

        STTBX_Print(("\nstptiHelper_SlotList_Free() session %d\n", Device->Session ));

        for( a = 0; a < TC_Params_p->TC_NumberOfSessions; a++ )
        {
            SlotList = &PrivateData_p->SlotList[a]; /* cycle through the allocation list */

            if ( STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionPIDFilterStartAddr) == St20ToTcRamAddress( SlotList->Start, TC_Params_p ))
            {
                STTBX_Print(("Found match at index %d.\n", a ));
                STSYS_WriteRegDev16LE((void*)&TCSessionInfo_p->SessionPIDFilterStartAddr, 0);
                STSYS_WriteRegDev16LE((void*)&TCSessionInfo_p->SessionPIDFilterLength, 0);
                SlotList->State = SL_FREE;  /* for reuse by another session */
                break;
            }
        }

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        DumpSlotList( PrivateData_p, TC_Params_p );
#endif
    }    
    
    return( ST_NO_ERROR );
}

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/* EOF --------------------------------------------------------------------- */
