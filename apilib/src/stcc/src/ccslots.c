/*******************************************************************************

File name   : ccslots.c

Description : STCC slots manager.
    -stcc_SlotsRecover
    -stcc_GetSlotIndex
    -stcc_FreeSlot
    -stcc_InsertSlot
    -stcc_ReleaseSlot

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
25 July 2001       Created                                     Michel Bruant
21 Jan  2002       Add traces                                        HSdLM
15 May  200        80 cols                                     Michel Bruant
*******************************************************************************/
/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stccinit.h"
#include "ccslots.h"

/* Constants ---------------------------------------------------------------- */

#define INSERT_TRY_NEXT         0
#define INSERT_AFTER_CURRENT    1
#define INSERT_BEFORE_CURRENT   2
#define HUGE_DIFF               (2 * 90000) /* 2s in pts unit */

/* Macros ------------------------------------------------------------------- */

#define IsThereHugeDiff(PTS1, PTS2)   \
    (                                                       \
    (((PTS1).PTS33 == (PTS2).PTS33) &&                          \
    ((((PTS1).PTS > (PTS2).PTS) && (((PTS1).PTS - (PTS2).PTS) > HUGE_DIFF)) \
     || \
     (((PTS2).PTS > (PTS1).PTS) && (((PTS2).PTS - (PTS1).PTS) > HUGE_DIFF))))\
    ||                                                      \
    (((PTS1).PTS33 == 1) && ((PTS2).PTS33 == 0)\
     && (!(          \
    (((PTS1).PTS < HUGE_DIFF) \
     && ((PTS2).PTS > (0xFFFFFFFF - (HUGE_DIFF - (PTS1).PTS)))) || \
    (((PTS1).PTS > (0xFFFFFFFF - HUGE_DIFF))\
     && ((PTS2).PTS < (HUGE_DIFF - (0xFFFFFFFF - (PTS1).PTS)))))))  \
    ||                                                      \
    (((PTS2).PTS33 == 1) && ((PTS1).PTS33 == 0) && (!(          \
    (((PTS2).PTS < HUGE_DIFF)\
     && ((PTS1).PTS > (0xFFFFFFFF - (HUGE_DIFF - (PTS2).PTS)))) || \
    (((PTS2).PTS > (0xFFFFFFFF - HUGE_DIFF)) \
     && ((PTS1).PTS < (HUGE_DIFF - (0xFFFFFFFF - (PTS2).PTS)))))))  \
    )

#define IsPTSLessOrEqual(PTS1, PTS2)   \
    ((((PTS1).PTS33 != (PTS2).PTS33) && ((PTS1).PTS > (PTS2).PTS))\
   || (((PTS1).PTS33 == (PTS2).PTS33) && ((PTS1).PTS <= (PTS2).PTS)))

/*------------------------------------------------------------------------------
Name:  stcc_SlotsRecover
Purpose:
Notes:
------------------------------------------------------------------------------
*/
void stcc_SlotsRecover(stcc_Device_t * const Device_p)
{
    U32 i;
    for (i=0; i<NB_SLOT; i++)
    {
        Device_p->DeviceData_p->CaptionSlot[i].SlotAvailable    = 1;
        Device_p->DeviceData_p->CaptionSlot[i].PTS.PTS          = 0xfffff;
        Device_p->DeviceData_p->CaptionSlot[i].Next             = BAD_SLOT;
        Device_p->DeviceData_p->CaptionSlot[i].Previous         = BAD_SLOT;
    }
    Device_p->DeviceData_p->FirstSlot                           = BAD_SLOT;
    Device_p->DeviceData_p->NbUsedSlot                          = 0;
}

/*------------------------------------------------------------------------------
Name:  stcc_GetSlotIndex
Purpose:
Notes:
------------------------------------------------------------------------------
*/
U32 stcc_GetSlotIndex(stcc_Device_t * const Device_p)
{
    U32 SlotIndex;
    for(SlotIndex =0; SlotIndex<NB_SLOT; SlotIndex++)
    {
        if(Device_p->DeviceData_p->CaptionSlot[SlotIndex].SlotAvailable == 1)
        {
            break;
        }
    }
    if(SlotIndex == NB_SLOT)/* I did not find an empty slot */
    {
        /* something is wrong .... */
        stcc_SlotsRecover(Device_p);
        SlotIndex = 0;
    }
    Device_p->DeviceData_p->CaptionSlot[SlotIndex].SlotAvailable = 0;
    return(SlotIndex);
}
/*------------------------------------------------------------------------------
Name:     stcc_ReleaseSlot
Purpose:
Notes:
------------------------------------------------------------------------------
*/
void stcc_ReleaseSlot(stcc_Device_t * const Device_p, U32 SlotIndex)
{
    stcc_DeviceData_t *     DeviceData_p;

    DeviceData_p = Device_p->DeviceData_p;

    DeviceData_p->CaptionSlot[SlotIndex].Next           = BAD_SLOT;
    DeviceData_p->CaptionSlot[SlotIndex].Previous       = BAD_SLOT;
    DeviceData_p->CaptionSlot[SlotIndex].SlotAvailable  = 1;
}

/*------------------------------------------------------------------------------
Name:     stcc_FreeSlot
Purpose:
Notes:
------------------------------------------------------------------------------
*/
void stcc_FreeSlot(stcc_Device_t * const Device_p)
{
    stcc_DeviceData_t *     DeviceData_p;
    U32                     FirstSlot;

    DeviceData_p = Device_p->DeviceData_p;
    FirstSlot = DeviceData_p->FirstSlot;

    if(FirstSlot == BAD_SLOT)
    {
        return; /* security ... */
    }
    /* unlink the first slot */
    DeviceData_p->FirstSlot = DeviceData_p->CaptionSlot[FirstSlot].Next;
    if(DeviceData_p->FirstSlot != BAD_SLOT)
    {
        DeviceData_p->CaptionSlot[DeviceData_p->FirstSlot].Previous = BAD_SLOT;
    }
    /* free the delinked slot */
    DeviceData_p->CaptionSlot[FirstSlot].Next           = BAD_SLOT;
    DeviceData_p->CaptionSlot[FirstSlot].Previous       = BAD_SLOT;
    DeviceData_p->CaptionSlot[FirstSlot].SlotAvailable  = 1;
    Device_p->DeviceData_p->NbUsedSlot --;
}

/*------------------------------------------------------------------------------
Name:  stcc_InsertSlot
Purpose:
Notes:
------------------------------------------------------------------------------
*/
void stcc_InsertSlot(stcc_Device_t * const Device_p, U32 NewSlot)
{
    stcc_DeviceData_t *     DeviceData_p;
    U32                     FirstSlot,CurrentSlot;
    U32                     Insertion;

    STCC_DebugStats.NBCaptionDataFound ++;

    DeviceData_p = Device_p->DeviceData_p;
    FirstSlot = DeviceData_p->FirstSlot;

    /* case : empty list */
    if(FirstSlot == BAD_SLOT)
    {
        DeviceData_p->FirstSlot                     = NewSlot;
        DeviceData_p->CaptionSlot[NewSlot].Next     = BAD_SLOT;
        DeviceData_p->CaptionSlot[NewSlot].Previous = BAD_SLOT;
        Device_p->DeviceData_p->NbUsedSlot =1;
        return;
    }
    else /* scan the list */
    {
        Device_p->DeviceData_p->NbUsedSlot ++;
        CurrentSlot = FirstSlot;
        Insertion   = INSERT_TRY_NEXT;
        do
        {
            if(IsThereHugeDiff(DeviceData_p->CaptionSlot[CurrentSlot].PTS,
                               DeviceData_p->CaptionSlot[NewSlot].PTS))
            {
                stcc_SlotsRecover(Device_p);
                return;
            }
            if(IsPTSLessOrEqual(DeviceData_p->CaptionSlot[CurrentSlot].PTS,
                                DeviceData_p->CaptionSlot[NewSlot].PTS))
            {
                /* Try the next slot */
                if (DeviceData_p->CaptionSlot[CurrentSlot].Next != BAD_SLOT)
                {
                    CurrentSlot = DeviceData_p->CaptionSlot[CurrentSlot].Next;
                }
                else /* reach end of the list */
                {
                    Insertion = INSERT_AFTER_CURRENT;
                }
            }
            else
            {
                Insertion = INSERT_BEFORE_CURRENT;
            }
        } while (Insertion == INSERT_TRY_NEXT);
        /* position found. Inser now */

        if(Insertion == INSERT_AFTER_CURRENT)
        {
            DeviceData_p->CaptionSlot[NewSlot].Next     = BAD_SLOT;
            DeviceData_p->CaptionSlot[NewSlot].Previous = CurrentSlot;
            DeviceData_p->CaptionSlot[CurrentSlot].Next = NewSlot;
        }
        else /* Insertion == INSERT_BEFORE_CURRENT */
        {
            DeviceData_p->CaptionSlot[NewSlot].Next = CurrentSlot;
            DeviceData_p->CaptionSlot[NewSlot].Previous
                            = DeviceData_p->CaptionSlot[CurrentSlot].Previous;
            if(DeviceData_p->CaptionSlot[NewSlot].Previous == BAD_SLOT)
            {
                DeviceData_p->FirstSlot = NewSlot;
            }
            else
            {
                DeviceData_p->CaptionSlot
                   [DeviceData_p->CaptionSlot[NewSlot].Previous].Next = NewSlot;
            }
            DeviceData_p->CaptionSlot[CurrentSlot].Previous = NewSlot;
        }
    }
}
/*----------------------------------------------------------------------------*/
