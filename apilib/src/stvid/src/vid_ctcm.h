/*******************************************************************************

File name   : vid_ctcm.h

Description : Common things commands header file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
21 Mar 2000        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __COMMON_THINGS_COMMANDS_H
#define __COMMON_THINGS_COMMANDS_H


/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <limits.h>
#endif

#include "stddefs.h"

#include "stvid.h"
#include "vid_com.h"

#include "stos.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define DEFAULT_FILL_VALUE_AFTER_MEMORY_ALLOCATION      0xa5
#define DEFAULT_FILL_VALUE_BEFORE_MEMORY_DEALLOCATION   0xc3

/* Exported Types ----------------------------------------------------------- */

typedef struct {
    U8 * Data_p;
    U32  TotalSize;
    U32  UsedSize;
    U32  MaxUsedSize;       /* To monitor max size used in commands buffer */
    U8 * BeginPointer_p;    /* This is a circular buffer, with BeginPointer_p and UsedSize we know what is in */
} CommandsBuffer_t;

typedef struct {
    U32 * Data_p;
    U32  TotalSize;
    U32  UsedSize;
    U32  MaxUsedSize;       /* To monitor max size used in commands buffer */
    U32 * BeginPointer_p;    /* This is a circular buffer, with BeginPointer_p and UsedSize we know what is in */
} CommandsBuffer32_t;

typedef struct
{
    STVID_DeviceType_t          DeviceType;
    STVID_BroadcastProfile_t    BrdCstProfile;
    U32                         VTGFrameRate;
} BitmapDefaultFillParams_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */
#ifdef DEBUG_MEMORY_ALLOCATION
    #include "string.h"
    #define SAFE_MEMORY_ALLOCATE( DestAdd_p, PointerType, Partition_p, MemorySize)    (DestAdd_p) =   \
            (PointerType) STOS_MemoryAllocate( (Partition_p), (MemorySize) ) ;                              \
            memset((DestAdd_p), DEFAULT_FILL_VALUE_AFTER_MEMORY_ALLOCATION, (MemorySize))
    #define SAFE_MEMORY_DEALLOCATE( DestAdd_p, Partition_p, MemorySize)                                          \
            {                                                                                                    \
                /* Need to save the partition pointer because most of time, the partition pointer is included    \
                 * in the structure and we will loose after filling the structure */                             \
                ST_Partition_t *SavedPartition_p = (Partition_p);                                                \
                memset( (DestAdd_p), DEFAULT_FILL_VALUE_BEFORE_MEMORY_DEALLOCATION, (MemorySize));               \
                STOS_MemoryDeallocate( SavedPartition_p, (void *) (DestAdd_p) );                                     \
            }                                                                                                    \
            (DestAdd_p) = (DestAdd_p) /* Dummy line, lets macro work only with ; at the end */
#else
    #define SAFE_MEMORY_ALLOCATE( DestAdd_p, PointerType, Partition_p, MemorySize)    (DestAdd_p) =   \
            (PointerType) STOS_MemoryAllocate( (Partition_p), (MemorySize) )
    #define SAFE_MEMORY_DEALLOCATE( DestAdd_p, Partition_p, MemorySize)                                          \
            STOS_MemoryDeallocate( (Partition_p), (void *) (DestAdd_p) )
#endif

/* Substract value U32 from PTS which is U33 */
#define vidcom_SubstractU32ToPTS(PTS1, Tmp32)   \
{                                               \
    if ((PTS1).PTS < (Tmp32))                   \
    {                                           \
        if (!((PTS1).PTS33))                    \
        {                                       \
            (PTS1).PTS33 = TRUE;                \
        }                                       \
        else                                    \
        {                                       \
            (PTS1).PTS33 = FALSE;               \
        }                                       \
    }                                           \
    (PTS1).PTS -= (Tmp32);                      \
}

/* Add value U32 to PTS which is U33 */
#define vidcom_AddU32ToPTS(PTS1, Tmp32)         \
{                                               \
    if ((PTS1).PTS > (UINT_MAX - (Tmp32)))      \
    {                                           \
        if (!((PTS1).PTS33))                    \
        {                                       \
            (PTS1).PTS33 = TRUE;                \
        }                                       \
        else                                    \
        {                                       \
            (PTS1).PTS33 = FALSE;               \
        }                                       \
    }                                           \
    (PTS1).PTS += (Tmp32);                      \
}

/* Check conditions greater and less with the granularity of half a U33, so a U32.
  Example: 0x1ffffffff is bigger than 0x110000000, but smaller than 0 !
  So corresponding equation is:
  - if same bit 33: just compare the U32's
  - if different bit 33: just invert the comparison on the U32's. This has the effect
  of making a comparison with half of the U33 range as granularity.
  (Seen on a circle in which U33 PTS's are angles, going around from 0x0 to 0x1ffffffff,
  these comparisons will consider > or < depending on in which half of the circle the other value is.) */
#define vidcom_IsPTSGreaterThanPTS(PTS1, PTS2) ((((PTS1).PTS33 != (PTS2).PTS33) && ((PTS1).PTS < (PTS2).PTS)) || \
                                                (((PTS1).PTS33 == (PTS2).PTS33) && ((PTS1).PTS > (PTS2).PTS)))
#define vidcom_IsPTSLessThanPTS(PTS1, PTS2)    ((((PTS1).PTS33 != (PTS2).PTS33) && ((PTS1).PTS > (PTS2).PTS)) || \
                                                (((PTS1).PTS33 == (PTS2).PTS33) && ((PTS1).PTS < (PTS2).PTS)))
#define vidcom_IsPTSGreaterThanOrEqualToPTS(PTS1, PTS2) ((((PTS1).PTS33 != (PTS2).PTS33) && ((PTS1).PTS < (PTS2).PTS)) || \
                                                        (((PTS1).PTS33 == (PTS2).PTS33) && ((PTS1).PTS >= (PTS2).PTS)))
#define vidcom_IsPTSLessThanOrEqualToPTS(PTS1, PTS2)    ((((PTS1).PTS33 != (PTS2).PTS33) && ((PTS1).PTS > (PTS2).PTS)) || \
                                                        (((PTS1).PTS33 == (PTS2).PTS33) && ((PTS1).PTS <= (PTS2).PTS)))

/* Checks time difference which is considered large when continuously synchronising,
 i.e. not seeking synchronisation at start (prevents from synchronising on incoherent
 clocks for small steams looping on packet injector) */

/* Huge synchronisation difference: 3 seconds.      */
#define HUGE_DIFFERENCE_IN_STC   (3 * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION)

#define IsThereHugeDifferenceBetweenPTSandSTC(PTS1, PTS2)   \
    (                                                       \
    (((PTS1).PTS33 == (PTS2).PTS33) &&                          \
    ((((PTS1).PTS > (PTS2).PTS) && (((PTS1).PTS - (PTS2).PTS) > HUGE_DIFFERENCE_IN_STC)) || \
     (((PTS2).PTS > (PTS1).PTS) && (((PTS2).PTS - (PTS1).PTS) > HUGE_DIFFERENCE_IN_STC))))  \
    ||                                                      \
    (((PTS1).PTS33 == 1) && ((PTS2).PTS33 == 0) && (!(          \
    (((PTS1).PTS < HUGE_DIFFERENCE_IN_STC) && ((PTS2).PTS > (0xFFFFFFFF - (HUGE_DIFFERENCE_IN_STC - (PTS1).PTS)))) || \
    (((PTS1).PTS > (0xFFFFFFFF - HUGE_DIFFERENCE_IN_STC)) && ((PTS2).PTS < (HUGE_DIFFERENCE_IN_STC - (0xFFFFFFFF - (PTS1).PTS)))))))  \
    ||                                                      \
    (((PTS2).PTS33 == 1) && ((PTS1).PTS33 == 0) && (!(          \
    (((PTS2).PTS < HUGE_DIFFERENCE_IN_STC) && ((PTS1).PTS > (0xFFFFFFFF - (HUGE_DIFFERENCE_IN_STC - (PTS2).PTS)))) || \
    (((PTS2).PTS > (0xFFFFFFFF - HUGE_DIFFERENCE_IN_STC)) && ((PTS1).PTS < (HUGE_DIFFERENCE_IN_STC - (0xFFFFFFFF - (PTS2).PTS)))))))  \
    )

#define DeviceToVirtual(DevAdd, Mapping) (void*)((U32)(DevAdd)\
        -(U32)((Mapping).PhysicalAddressSeenFromDevice_p)\
        +(U32)((Mapping).VirtualBaseAddress_p))

#define VirtualToDevice(Virtual, Mapping) (void*)((U32)(Virtual)\
        +(U32)((Mapping).PhysicalAddressSeenFromDevice_p)\
        -(U32)((Mapping).VirtualBaseAddress_p))

#define CAST_CONST_HANDLE_TO_DATA_TYPE( DataType, Handle )  \
            ((DataType)(*(DataType*)((void*)&(Handle))))

/* Exported Functions ------------------------------------------------------- */

void vidcom_IncrementTimeCode(STVID_TimeCode_t * const TimeCode_p, const U32 FrameRate, BOOL DropFrameFlag);
void vidcom_PictureInfosToPictureParams(const STVID_PictureInfos_t * const PictureInfos_p, STVID_PictureParams_t * const Params_p, const S32 TemporalReference);

ST_ErrorCode_t vidcom_PopCommand(CommandsBuffer_t * const Buffer_p, U8 * const Data_p);
void vidcom_PushCommand(CommandsBuffer_t * const Buffer_p, const U8 Data);
ST_ErrorCode_t vidcom_PopCommand32(CommandsBuffer32_t * const Buffer_p, U32 * const Data_p);
void vidcom_PushCommand32(CommandsBuffer32_t * const Buffer_p, const U32 Data);
/* vidcom_PushCommand32DoOrValueIfFull : same as vidcom_PushCommand32, but don't throw away value if full, but OR value with last value */
void vidcom_PushCommand32DoOrValueIfFull(CommandsBuffer32_t * const Buffer_p, const U32 Data);

S32 vidcom_ComparePictureID(VIDCOM_PictureID_t *PictureID1_p, VIDCOM_PictureID_t *PictureID2_p);
void vidcom_FillBitmapWithDefaults(const BitmapDefaultFillParams_t * const BitmapDefaultFillParams_p, STGXOBJ_Bitmap_t * const Bitmap_p);

/* Special function for USER_DATA_EVT event since different management between OS2x and Linux */
void vidcom_NotifyUserData(STEVT_Handle_t EventsHandle, STEVT_EventID_t EventId, STVID_UserData_t * UserData_p);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __COMMON_THINGS_COMMANDS_H */

/* End of vid_ctcm.h */
