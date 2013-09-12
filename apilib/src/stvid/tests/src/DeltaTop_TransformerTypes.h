/*
**  @file     : DeltaTop_TransformerTypes.h
**  @brief    : Delta Top LX Application specific types
**  @par OWNER: Pascal Chelius
**  @author   : Pascal Chelius
**  @par SCOPE:
**  @date     : 2007-06-08
**  &copy; 2007 ST Microelectronics. All Rights Reserved.
*/

#ifndef __DELTATOP_TRANSFORMERTYPES_H
#define __DELTATOP_TRANSFORMERTYPES_H

/*
**        Identifies the name of the transformer
*/
#define DELTATOP_MME_TRANSFORMER_NAME   "DELTATOP_TRANSFORMER"
#define PING_MME_TRANSFORMER_NAME       "PING_CPU"

/*
**      Identifies the version of the MME API for both preprocessor and firmware
*/
#if (DELTATOP_MME_VERSION==10)
    #define DELTATOP_MME_API_VERSION "1.0"
#else
    #define DELTATOP_MME_API_VERSION "Unknown"
#endif

/*
** DeltaTop_HeapAllocationStatus_t:
*     Information got from libc mallinfo() function
*/
typedef struct {
    U32 arena;    /* total space allocated from system */
    U32 ordblks;  /* number of non-inuse chunks */
    U32 hblks;    /* number of mmapped regions */
    U32 hblkhd;   /* total space in mmapped regions */
    U32 uordblks; /* total allocated space */
    U32 fordblks; /* total non-inuse space */
    U32 keepcost; /* top-most, releasable (via malloc_trim) space */
} DeltaTop_HeapAllocationStatus_t;

/*
** DeltaTop_TransformerCapability_t:
*/
typedef struct {
    DeltaTop_HeapAllocationStatus_t DeltaTop_HeapAllocationStatus;
} DeltaTop_TransformerCapability_t;

#endif /* #ifndef __DELTATOP_TRANSFORMERTYPES_H */
