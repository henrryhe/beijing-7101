/******************************************************************************

    File Name   : context.c

    Description : Context related API functions

******************************************************************************/

/* Includes ---------------------------------------------------------------- */
#if defined (ST_OSLINUX)
#include <linux/stddef.h>
#include <linux/interrupt.h>
#include <asm/semaphore.h>
#include <linux/errno.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#endif

#include "local.h"

/* Private Constants ------------------------------------------------------- */
#if defined (ST_5525)
#define MAX_CONTEXTS 4
#elif defined (ST_7200)
#define MAX_CONTEXTS 7
#else
#define MAX_CONTEXTS 3
#endif

#if defined (STFDMA_USE_VIRTUAL_CONTEXT)
#define MAX_VIRTUAL_CONTEXTS 16
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

#if defined (ST_7100)
#define CONTEXT_OFFSET 0X9580
#elif defined (ST_7109)
#define CONTEXT_OFFSET 0X9200
#elif defined (ST_7200)
#define CONTEXT_OFFSET 0x9200
#elif defined (ST_5188)
#define CONTEXT_OFFSET 0x84F0
#elif defined (ST_5162)
#define CONTEXT_OFFSET 0x9200
#elif defined (ST_5525)
#define CONTEXT_OFFSET 0X9400
#else
#define CONTEXT_OFFSET 0X44F0
#endif

#if defined (ST_7200)
#define STFDMA_PES STFDMA_2
#else
#define STFDMA_PES STFDMA_1
#endif

/* Private Types ----------------------------------------------------------- */

typedef struct ContextState_s
{
    U16 Allocated;
    U16 MagicNo;
}ContextState_t;

typedef volatile struct SCCtrl_s
{
    U32 Start           :  8;
    U32 End             :  8;
#if defined (ST_7100)
    U32 Pad             : 12;
    U32 SCDModeSelect   :  1;
#elif defined (ST_7109) || defined (ST_7200) 
    U32 Pad             : 11;
    U32 SCDModeSelect   :  2;
#else
    U32 Pad             : 13;
#endif
    U32 PTS_OneShot     :  1;
    U32 Enable          :  1;
    U32 InNotOut        :  1;
}SCCtrl_t;

typedef volatile struct PESCtrl_s
{
    U32 Start           :  8;
    U32 End             :  8;
    U32 Pad             : 11;
    U32 PPLDisable      :  1;
    U32 Pad2            :  1;
    U32 PTS_OneShot     :  1;
    U32 Enable          :  1;
    U32 InNotOut        :  1;
}PESCtrl_t;

typedef volatile struct ContextLayout_s
{
    U32 SCWrite;      /* Start code list write position      */
    U32 SCSize;       /* Remaining code list available space */

    U32 ESTop;        /* Top of the ES buffer */
    U32 ESRead;       /* Current read  position */
    U32 ESWrite;      /* Current write position */
    U32 ESBottom;     /* Bottom of the ES buffer */

    PESCtrl_t PES0;    /* PES start code configuration */
    SCCtrl_t ES0;     /*  ES start code configuration */
    SCCtrl_t ES1;     /*  ES start code configuration */

    U32 Pad[13-9];    /* Unused data */
    U32 InitialState; /* The PES/SCD initial state */
    U32 Reserved[2];  /* Reserved for internal configuration */
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) || defined (ST_5525) || defined (ST_5162)
    U32 Pad1[16];
#endif
}ContextLayout_t;

/* Private Variables ------------------------------------------------------- */

static ContextState_t ContextState[MAX_CONTEXTS];
#if defined (ST_OS20) || defined (ST_OSLINUX)
static semaphore_t    g_APILockSem[STFDMA_MAX];
#endif
static semaphore_t*   APILockSem[STFDMA_MAX];
static int            SemaSet[STFDMA_MAX] = {FALSE, FALSE};

static ContextLayout_t *ContextLayout[STFDMA_MAX] = {NULL, NULL};

#if defined (STFDMA_USE_VIRTUAL_CONTEXT)
static ContextState_t  VirtualContextState[MAX_VIRTUAL_CONTEXTS];
static ContextLayout_t VirtualContextLayout[MAX_VIRTUAL_CONTEXTS];
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

/* Private Macros ---------------------------------------------------------- */

#define CONTEXT_ALLOCATED(x)    (ContextState[x].Allocated)
#define CONTEXT_ALLOCATE(x)     (ContextState[x].Allocated = TRUE)
#define CONTEXT_DEALLOCATE(x)   (ContextState[x].Allocated = FALSE)
#define CONTEXT_MAGIC_NO(x)     (ContextState[x].MagicNo)
#define CONTEXT_CONTEXT_ID(x) (((++(ContextState[x].MagicNo))<<16) | (x))

#if defined (STFDMA_USE_VIRTUAL_CONTEXT)
#define VIRTUAL_CONTEXT_ALLOCATED(x)    (VirtualContextState[x].Allocated)
#define VIRTUAL_CONTEXT_ALLOCATE(x)     (VirtualContextState[x].Allocated = TRUE)
#define VIRTUAL_CONTEXT_DEALLOCATE(x)   (VirtualContextState[x].Allocated = FALSE)
#define VIRTUAL_CONTEXT_MAGIC_NO(x)     (VirtualContextState[x].MagicNo)
#define VIRTUAL_CONTEXT_CONTEXT_ID(x)   (((++(VirtualContextState[x].MagicNo))<<16) | (x))
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */

/****************************************************************************
Name         : STFDMA_AllocateContext
Description  : Reserve an unused Context
Parameters   :
Return Value : ST_ERROR_BAD_PARAMETER
               STFDMA_ERROR_NO_FREE_CONTEXTS
****************************************************************************/
ST_ErrorCode_t STFDMA_AllocateContext       (STFDMA_ContextId_t *ContextId)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

#if !defined(ST_5517)
    U16 Context = 0;
    int i;
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if (ContextId == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    /* Find an unused context */
#if defined (ST_7200)
    /* Context 3 is reserved for SPDIF */
    for (Context = 0; (Context < 3) && CONTEXT_ALLOCATED(Context); Context++);

    if (Context == 3)
        for (Context = 4; (Context < MAX_CONTEXTS) && CONTEXT_ALLOCATED(Context); Context++);
#else
    for (Context = 0; (Context < MAX_CONTEXTS) && CONTEXT_ALLOCATED(Context); Context++);
#endif

    if (Context < MAX_CONTEXTS)
    {
        /* Allocate this Context */
        CONTEXT_ALLOCATE(Context);
        *ContextId = CONTEXT_CONTEXT_ID(Context);

        /* Initialise the context */	
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) || defined (ST_5525) || defined (ST_5162)	
        for (i = 0; (i < sizeof(ContextLayout_t)/sizeof(U32) - 18); i++)
#else
        for (i = 0; (i < sizeof(ContextLayout_t)/sizeof(U32) - 2); i++)
#endif
        {
#if defined (ST_OSLINUX)
            iowrite32(0, (void*)((U8*)(ContextLayout[STFDMA_PES]+Context) + (i * sizeof(U32))));
#else
            ((U32*)(ContextLayout[STFDMA_PES]+Context))[i] = 0;
#endif
        }

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) || defined (ST_5525)  || defined (ST_5162)
        for (i = 16; (i < sizeof(ContextLayout_t)/sizeof(U32)); i++)
        {
#if defined (ST_OSLINUX)
            iowrite32(0, (void*)((U8*)(ContextLayout[STFDMA_PES]+Context) + (i * sizeof(U32))));
#else
            ((U32*)(ContextLayout[STFDMA_PES]+Context))[i] = 0;
#endif
        }
#endif
    }
    else
    {
        /* All the Contexts are in use */
        ErrorCode = STFDMA_ERROR_NO_FREE_CONTEXTS;
    }

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_DeallocateContext
Description  : Release a reserved Context
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_DeallocateContext     (STFDMA_ContextId_t  ContextId)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U16             Context = ContextId&0X0000FFFF;
    U16             MagicNo = ContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    CONTEXT_DEALLOCATE(Context);

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_ContextGetSCList
Description  : Return the current write position, remaining size and
               overflow status of the Start Code list in the Context.
Parameters   :
Return Value : STFDMA_ERROR_NOT_INITIALIZED
               STFDMA_ERROR_INVALID_CONTEXT_ID
               ST_ERROR_BAD_PARAMETER
****************************************************************************/
ST_ErrorCode_t STFDMA_ContextGetSCList  (STFDMA_ContextId_t   ContextId,
	                                     STFDMA_SCEntry_t   **SCList,
	                                     U32                 *Size,
	                                     BOOL                *Overflow)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U16             Context = ContextId&0X0000FFFF;
    U16             MagicNo = ContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if ((SCList   == NULL) ||
             (Size     == NULL) ||
             (Overflow == NULL))
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

#if defined (ST_OSLINUX)
    *SCList    = (STFDMA_SCEntry_t*)(ioread32((void*)&(ContextLayout[STFDMA_PES][Context].SCWrite)) & ~0X0F);
    *Size      = ioread32((void*)&(ContextLayout[STFDMA_PES][Context].SCSize));
    *Overflow  = ioread32((void*)&(ContextLayout[STFDMA_PES][Context].SCWrite)) & 0X01;
#else
    *SCList    = (STFDMA_SCEntry_t*)(ContextLayout[STFDMA_PES][Context].SCWrite & ~0X0F);
    *Size      = (ContextLayout[STFDMA_PES][Context].SCSize);
    *Overflow  = (ContextLayout[STFDMA_PES][Context].SCWrite & 0X01);
#endif

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_ContextSetSCList
Description  : Set the strat code lists current write position and remaining
               size.
Parameters   :
Return Value :
****************************************************************************/
ST_ErrorCode_t STFDMA_ContextSetSCList  (STFDMA_ContextId_t   ContextId,
	                                     STFDMA_SCEntry_t    *SCList,
	                                     U32                  Size)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U16             Context = ContextId&0X0000FFFF;
    U16             MagicNo = ContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (SCList == NULL)
    {
        STTBX_Print(("ERROR: SC List %08X\n", (U32)(SCList)));
        return ST_ERROR_BAD_PARAMETER;
    }
    else if (((U32)(SCList)) & 0X0F)
    {
        STTBX_Print(("ERROR: SC List %08X\n", (U32)(SCList)));
        return ST_ERROR_BAD_PARAMETER;
    }
    else if (Size & 0X0F)
    {
        STTBX_Print(("ERROR: SC List Size %08X\n", Size));
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

#if defined (ST_OSLINUX)
    iowrite32(((U32)(SCList)) & ~0X0F, (void*)&(ContextLayout[STFDMA_PES][Context].SCWrite));
    iowrite32(Size & ~0X0F, (void*)&(ContextLayout[STFDMA_PES][Context].SCSize));
#else
    ContextLayout[STFDMA_PES][Context].SCWrite = ((U32)(SCList)) & ~0X0F;
    ContextLayout[STFDMA_PES][Context].SCSize  = Size & ~0X0F;
#endif
    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_ContextGetSCState
Description  : Return the currect configuration of the start code range.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_ContextGetSCState (STFDMA_ContextId_t  ContextId,
	                                     STFDMA_SCState_t   *State,
	                                     STFDMA_SCRange_t    Range)
{
    ST_ErrorCode_t  ErrorCode   = ST_NO_ERROR;
    U16             Context     = ContextId&0X0000FFFF;
    U16             MagicNo     = ContextId>>16;
#if defined (ST_OSLINUX)
    U32             SCControl, PESControl;
#else
#if !defined(ST_5517)
    SCCtrl_t        SCControl   = {0};
    PESCtrl_t       PESControl  = {0};
#endif
#endif

#if !defined(ST_5517)
    int InRange = 0;
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (State == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    /* Under OS21 we must read and write using 32 bit variables, hence the use
       of SCControl, reading/writing to individual bit fields does not work */

    switch (Range)
    {
        case STFDMA_DEVICE_PES_RANGE_0 :
            State->OneShotEnabled = 0;
#if defined (ST_OSLINUX)
            PESControl = ioread32((void*)&(ContextLayout[STFDMA_PES][Context].PES0));
            State->RangeStart   = PESControl & 0x000000FF;
            State->RangeEnd     = PESControl & 0x0000FF00 >> 8;
            State->PTSEnabled   = PESControl & 0x20000000 >> 29;
            State->RangeEnabled = PESControl & 0x40000000 >> 30;
            InRange             = PESControl & 0x80000000 >> 31;
#else
            PESControl = ContextLayout[STFDMA_PES][Context].PES0;
            State->RangeStart     = PESControl.Start;
            State->RangeEnd       = PESControl.End;
            State->PTSEnabled     = PESControl.PTS_OneShot;
            State->RangeEnabled   = PESControl.Enable;
            InRange               = PESControl.InNotOut;
#endif
            break;

        case STFDMA_DEVICE_ES_RANGE_0 :
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
        case STFDMA_DEVICE_H264_RANGE_0:
#endif
            State->PTSEnabled     = 0;
#if defined (ST_OSLINUX)
            SCControl = ioread32((void*)&(ContextLayout[STFDMA_PES][Context].ES0));
            State->RangeStart     = SCControl & 0x000000FF;
            State->RangeEnd       = SCControl & 0x0000FF00 >> 8;
            State->OneShotEnabled = SCControl & 0x20000000 >> 29;
            State->RangeEnabled   = SCControl & 0x40000000 >> 30;
            InRange               = SCControl & 0x80000000 >> 31;
#else
            SCControl = ContextLayout[STFDMA_PES][Context].ES0;
            State->RangeStart     = SCControl.Start;
            State->RangeEnd       = SCControl.End;
            State->OneShotEnabled = SCControl.PTS_OneShot;
            State->RangeEnabled   = SCControl.Enable;
            InRange               = SCControl.InNotOut;
#endif
            break;

        case STFDMA_DEVICE_ES_RANGE_1 :
            State->PTSEnabled     = 0;
#if defined (ST_OSLINUX)
            SCControl = ioread32((void*)&(ContextLayout[STFDMA_PES][Context].ES1));
            State->RangeStart     = SCControl & 0x000000FF;
            State->RangeEnd       = SCControl & 0x0000FF00 >> 8;
            State->OneShotEnabled = SCControl & 0x20000000 >> 29;
            State->RangeEnabled   = SCControl & 0x40000000 >> 30;
            InRange               = SCControl & 0x80000000 >> 31;
#else
            SCControl = ContextLayout[STFDMA_PES][Context].ES1;
            State->RangeStart     = SCControl.Start;
            State->RangeEnd       = SCControl.End;
            State->OneShotEnabled = SCControl.PTS_OneShot;
            State->RangeEnabled   = SCControl.Enable;
            InRange               = SCControl.InNotOut;
#endif
            break;

        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);

    /* If checking for outsize the range then invert the range */
    if (!InRange)
    {
        U8 tmp   = State->RangeEnd;
        State->RangeEnd      = State->RangeStart+1;
        State->RangeStart    = tmp-1;
    }

#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_ContextSetSCState
Description  : Set the current configuration of the start code range.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_ContextSetSCState (STFDMA_ContextId_t  ContextId,
	                                     STFDMA_SCState_t   *State,
	                                     STFDMA_SCRange_t    Range)
{
    ST_ErrorCode_t ErrorCode    = ST_NO_ERROR;
    U16             Context     = ContextId&0X0000FFFF;
    U16             MagicNo     = ContextId>>16;
#if defined (ST_OSLINUX)
    U32             SCControl, PESControl;
#else
#if !defined (ST_5517)
    SCCtrl_t        SCControl   = {0};
    PESCtrl_t       PESControl  = {0};
#endif
#endif

#if !defined(ST_5517)
    U8  Start;
    U8  End;
    int InRange;
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (State == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else

    Start    = State->RangeStart;
    End      = State->RangeEnd;
    InRange  = 1;

    /* If the range wraps the check for an outside range */
    if (End < Start)
    {
        End      = State->RangeStart-1;
        Start    = State->RangeEnd+1;
        InRange  = 0;
    }

    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    /* Under OS21 we must read and write using 32 bit variables, hence the use
       of SCControl, reading/writing to individual bit fields does not work */

    switch (Range)
    {
        case STFDMA_DEVICE_PES_RANGE_0 :
#if defined (ST_OSLINUX)
            PESControl = Start;
            PESControl |= (End << 8);
#if defined (STFDMA_DISABLE_PES_PACKET_LENGTH_PARSING)
            PESControl |= 0x08000000;   /* PPLDisable */
#endif
            PESControl |= (State->PTSEnabled) ? (0x20000000):(0x00000000);
            PESControl |= (State->RangeEnabled)?(0x40000000):(0x00000000);
            PESControl |= (InRange)?(0x80000000):(0x00000000);
            iowrite32(PESControl, (void*)&(ContextLayout[STFDMA_PES][Context].PES0));
#else
#if defined (STFDMA_DISABLE_PES_PACKET_LENGTH_PARSING)
            PESControl.PPLDisable    = 1;
#endif
            PESControl.Start         = Start;
            PESControl.End           = End;
            PESControl.PTS_OneShot   = (State->PTSEnabled)?(1):(0);
            PESControl.Enable        = (State->RangeEnabled)?(1):(0);
            PESControl.InNotOut      = InRange;
            ContextLayout[STFDMA_PES][Context].PES0 = PESControl;
#endif
            break;

        case STFDMA_DEVICE_ES_RANGE_0 :
#if defined (ST_OSLINUX)
            SCControl = Start;
            SCControl |= (End << 8);
            SCControl |= (State->OneShotEnabled) ? (0x20000000):(0x00000000);
            SCControl |= (State->RangeEnabled)?(0x40000000):(0x00000000);
            SCControl |= (InRange)?(0x80000000):(0x00000000);
            iowrite32((U32)SCControl, (void*)&(ContextLayout[STFDMA_PES][Context].ES0));
#else
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
            SCControl.SCDModeSelect    = 0;
#endif
            SCControl.Start         = Start;
            SCControl.End           = End;
            SCControl.PTS_OneShot   = (State->OneShotEnabled)?(1):(0);
            SCControl.Enable        = (State->RangeEnabled)?(1):(0);
            SCControl.InNotOut      = InRange;
            ContextLayout[STFDMA_PES][Context].ES0 = SCControl;
#endif
            break;

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
        case STFDMA_DEVICE_H264_RANGE_0:
#if defined (ST_OSLINUX)
            SCControl = Start;
            SCControl |= (End << 8);
            SCControl |= (State->OneShotEnabled) ? (0x20000000):(0x00000000);
            SCControl |= (State->RangeEnabled)?(0x40000000):(0x00000000);
            SCControl |= (InRange)?(0x80000000):(0x00000000);
            SCControl |= 0x10000000;
            iowrite32((U32)SCControl, (void*)&(ContextLayout[STFDMA_PES][Context].ES0));
#else
            SCControl.Start         = Start;
            SCControl.End           = End;
            SCControl.PTS_OneShot   = (State->OneShotEnabled)?(1):(0);
            SCControl.Enable        = (State->RangeEnabled)?(1):(0);
            SCControl.InNotOut      = InRange;
/* Due to specific bit positions of the 'mode' field the following difference is not apparent for ST_OSLINUX */
#if defined (ST_7100)
            SCControl.SCDModeSelect = 1;
#elif defined (ST_7109) || defined (ST_7200)
            SCControl.SCDModeSelect = 2;
#endif
            ContextLayout[STFDMA_PES][Context].ES0 = SCControl;
#endif
            break;
#endif


#if defined (ST_7109) || defined (ST_7200)
        case STFDMA_DEVICE_VC1_RANGE_0:
#if defined (ST_OSLINUX)
            SCControl = Start;
            SCControl |= (End << 8);
            SCControl |= (State->OneShotEnabled) ? (0x20000000):(0x00000000);
            SCControl |= (State->RangeEnabled)?(0x40000000):(0x00000000);
            SCControl |= (InRange)?(0x80000000):(0x00000000);
            SCControl |= 0x08000000;
            iowrite32((U32)SCControl, (void*)&(ContextLayout[STFDMA_PES][Context].ES0));
#else
            SCControl.Start         = Start;
            SCControl.End           = End;
            SCControl.PTS_OneShot   = (State->OneShotEnabled)?(1):(0);
            SCControl.Enable        = (State->RangeEnabled)?(1):(0);
            SCControl.InNotOut      = InRange;
            SCControl.SCDModeSelect = 1;
            ContextLayout[STFDMA_PES][Context].ES0 = SCControl;
#endif
            break;
#endif

        case STFDMA_DEVICE_ES_RANGE_1 :
#if defined (ST_OSLINUX)
            SCControl = Start;
            SCControl |= (End << 8);
            SCControl |= (State->OneShotEnabled) ? (0x20000000):(0x00000000);
            SCControl |= (State->RangeEnabled)?(0x40000000):(0x00000000);
            SCControl |= (InRange)?(0x80000000):(0x00000000);
            iowrite32((U32)SCControl, (void*)&(ContextLayout[STFDMA_PES][Context].ES1));
#else
            SCControl.Start         = Start;
            SCControl.End           = End;
            SCControl.PTS_OneShot   = (State->OneShotEnabled)?(1):(0);
            SCControl.Enable        = (State->RangeEnabled)?(1):(0);
            SCControl.InNotOut      = InRange;
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200) 
            SCControl.SCDModeSelect = 0;
#endif
            ContextLayout[STFDMA_PES][Context].ES1 = SCControl;
#endif
            break;

        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_ContextSetESBuffer
Description  : Configure the output buffer in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_ContextSetESBuffer    (STFDMA_ContextId_t ContextId, void *Buffer, U32 Size)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U16             Context = ContextId&0X0000FFFF;
    U16             MagicNo = ContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (Buffer == NULL)
    {
        /* Bad buffer address */
        STTBX_Print(("ERROR: Buffer %08X\n", Buffer));
        return(ST_ERROR_BAD_PARAMETER);
    }
    else if ((U32)Buffer & 0X1F)
    {
        /* Bad alignment */
        STTBX_Print(("ERROR: Buffer %08X\n", Buffer));
        return(ST_ERROR_BAD_PARAMETER);
    }
    else if (Size & 0X1F)
    {
        /* The size is a bad multiple */
        STTBX_Print(("ERROR: Size %d\n", Size));
        return(ST_ERROR_BAD_PARAMETER);
    }
    else if (Size == 0)
    {
        /* The size is a bad multiple */
        STTBX_Print(("ERROR: Size %d\n", Size));
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

#if defined (ST_OSLINUX)
    iowrite32(((U32)Buffer + Size) & ~0X1F, (void*)&(ContextLayout[STFDMA_PES][Context].ESTop));
    iowrite32((U32)Buffer, (void*)&(ContextLayout[STFDMA_PES][Context].ESRead));
    iowrite32((U32)Buffer, (void*)&(ContextLayout[STFDMA_PES][Context].ESWrite));
    iowrite32(((U32)Buffer) & ~0X1F, (void*)&(ContextLayout[STFDMA_PES][Context].ESBottom));
#else
    ContextLayout[STFDMA_PES][Context].ESTop    = ((U32)Buffer +Size) & ~0X1F;
    ContextLayout[STFDMA_PES][Context].ESRead   =  (U32)Buffer;
    ContextLayout[STFDMA_PES][Context].ESWrite  =  (U32)Buffer;
    ContextLayout[STFDMA_PES][Context].ESBottom = ((U32)Buffer) & ~0X1F;
#endif

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_ContextSetESReadPtr
Description  : Set the current read pointer for the output buffer in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_ContextSetESReadPtr   (STFDMA_ContextId_t ContextId, void *Read)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U16             Context = ContextId&0X0000FFFF;
    U16             MagicNo = ContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

#if defined (ST_OSLINUX)
    iowrite32((U32)Read, (void*)&(ContextLayout[STFDMA_PES][Context].ESRead));
#else
    ContextLayout[STFDMA_PES][Context].ESRead = (U32)Read;
#endif

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_ContextSetESWritePtr
Description  : Set the current write pointer for the output buffer in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_ContextSetESWritePtr  (STFDMA_ContextId_t ContextId, void *Write)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U16             Context = ContextId&0X0000FFFF;
    U16             MagicNo = ContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if ((U32)Write & 0X03)
    {
        /* Bad alignment */
        STTBX_Print(("ERROR: Write ptr %08X\n", Write));
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

#if defined (ST_OSLINUX)
    iowrite32((U32)Write, (void*)&(ContextLayout[STFDMA_PES][Context].ESWrite));
#else
    ContextLayout[STFDMA_PES][Context].ESWrite = (U32)Write;
#endif

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_ContextSetESReadPtr
Description  : Get the current read pointer for the output buffer in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_ContextGetESReadPtr   (STFDMA_ContextId_t ContextId, void **Read)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U16             Context = ContextId&0X0000FFFF;
    U16             MagicNo = ContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (Read == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

#if defined (ST_OSLINUX)
    *Read = (void*)ioread32((void*)&(ContextLayout[STFDMA_PES][Context].ESRead));
#else
    *Read = (void*)ContextLayout[STFDMA_PES][Context].ESRead;
#endif

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_ContextGetESWritePtr
Description  : Get the current write pointer for the output buffer in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_ContextGetESWritePtr  (STFDMA_ContextId_t ContextId, void **Write, BOOL *Overflow)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U16             Context = ContextId&0X0000FFFF;
    U16             MagicNo = ContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
                !CONTEXT_ALLOCATED(Context) ||
                (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (Write == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

#if defined (ST_OSLINUX)
    *Write = (void*)(ioread32((void*)&(ContextLayout[STFDMA_PES][Context].ESWrite)) & ~0x03);
    if (NULL != Overflow) *Overflow = (ioread32((void*)&(ContextLayout[STFDMA_PES][Context].ESWrite)) & ~0x01);
#else
    *Write = (void*)(ContextLayout[STFDMA_PES][Context].ESWrite & ~0X3);
    if (NULL != Overflow) *Overflow = ContextLayout[STFDMA_PES][Context].ESWrite & 0X1;
#endif

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : stfdma_InitContexts
Description  : Initialise the internal structure for the Context management.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
void stfdma_InitContexts(U32 *BaseAddress_p, STFDMA_Block_t DeviceNo)
{
    int i;

    /* Initialse the control structure */
    for (i = 0; (i < MAX_CONTEXTS); i++)
    {
        ContextState[i].Allocated = FALSE;
        ContextState[i].MagicNo   = 0;
    }

#if defined(STFDMA_USE_VIRTUAL_CONTEXT)
    /* Initialse the virtual contexts stuff */
    for (i = 0; (i < MAX_VIRTUAL_CONTEXTS); i++)
    {
        VirtualContextState[i].Allocated = FALSE;
        VirtualContextState[i].MagicNo   = 0;
    }
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    /* Initialise the API semaphore if it has not already been done */
    if (!SemaSet[DeviceNo])
    {
#if defined (ST_OS20) || defined (ST_OSLINUX)
        semaphore_init_fifo(&g_APILockSem[DeviceNo], 1);
        APILockSem[DeviceNo] = &g_APILockSem[DeviceNo];
#elif defined (ST_OS21) || defined (ST_OSWINCE)
        APILockSem[DeviceNo] = semaphore_create_fifo(1);
#else
    #error APILockSem undefined
#endif
        SemaSet[DeviceNo] = TRUE;
    }

    /* Set the memory address for the Context structure in FDMA */
    ContextLayout[DeviceNo] = (ContextLayout_t*)((U32)BaseAddress_p + CONTEXT_OFFSET);
}

/****************************************************************************
Name         : stfdma_TermContexts
Description  : Clean up the internal structure for the Context management.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
int stfdma_TermContexts(STFDMA_Block_t DeviceNo)
{
    int i;

    STOS_SemaphoreWait(APILockSem[DeviceNo]);

    /* Reset the control structure */
    for (i = 0; (i < MAX_CONTEXTS); i++)
    {
        ContextState[i].Allocated = FALSE;
        ContextState[i].MagicNo   = 0;
    }
#if defined(STFDMA_USE_VIRTUAL_CONTEXT)
    /* Initialse the virtual contexts stuff */
    for (i = 0; (i < MAX_VIRTUAL_CONTEXTS); i++)
    {
        VirtualContextState[i].Allocated = FALSE;
        VirtualContextState[i].MagicNo   = 0;
    }
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */

    STOS_SemaphoreSignal(APILockSem[DeviceNo]);
    semaphore_delete(APILockSem[DeviceNo]);
    SemaSet[DeviceNo]	= FALSE;
#if defined (ST_OS20)
    APILockSem[DeviceNo] = NULL;
#endif

    return 0;
}

/****************************************************************************
Name         : STFDMA_SetAddDataRegionParameter
Description  : Set the value at a particular offset in one of the additional
               data regions.
Parameters   : DeviceNo - STFDMA_1/STDFMA_2
               RegionNo - Additional data region (0-3) to be written to
               Offset - Offset from the start of the additional data region
               Value - Value to be written
Return Value : ST_NO_ERROR
               STFDMA_ERROR_NOT_INTIALIZED
               STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER
               STFDMA_ERROR_UNKNOWN_REQUEST_SIGNAL
****************************************************************************/
ST_ErrorCode_t STFDMA_SetAddDataRegionParameter(STFDMA_Block_t DeviceNo, STFDMA_AdditionalDataRegion_t RegionNo, STFDMA_AdditionalDataRegionParameter_t ADRParameter, U32 Value)
{
    U32 Address;

#if defined (ST_5517)
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    if (DeviceNo >= STFDMA_MAX)
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;

    if (RegionNo >= ADDITIONAL_DATA_REGION_MAX)
        return STFDMA_ERROR_UNKNOWN_REGION_NUMBER;

    if (ADRParameter >= PES_LAST || ADRParameter >= SPDIF_LAST)
        return STFDMA_ERROR_UNKNOWN_ADR_PARAMETER;

    Address = ADD_DATA_REGION_0 + REGION_INTERVAL * (U32)RegionNo + (U32)ADRParameter * REGION_WORD_SIZE;

    STOS_SemaphoreWait(APILockSem[DeviceNo]);
    SET_REG(Address, Value, DeviceNo);
    STOS_SemaphoreSignal(APILockSem[DeviceNo]);

    return ST_NO_ERROR;
#endif
}


#if defined (STFDMA_USE_VIRTUAL_CONTEXT)
/****************************************************************************
   APIs function allowing to work with "virtual" context.
   A virtual FDMA context is a memory located structure mapped as an actual
   FDMA registers context. A virtual context can be written to an actual
   FDMA context. An actual FDMA context can be read (to be written to a
   virtual FDMA context.
   This feature aims at allowing the multiplexing of several FDMA through
   a single FDMA/PES parsing channel.
****************************************************************************/

/****************************************************************************
Name         : STFDMA_AllocateVirtualContext
Description  : Reserve an unused Context
Parameters   :
Return Value : ST_ERROR_BAD_PARAMETER
               STFDMA_ERROR_NO_FREE_CONTEXTS
****************************************************************************/
ST_ErrorCode_t STFDMA_AllocateVirtualContext       (STFDMA_VirtualContextId_t *VirtualContextId)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

#if !defined(ST_5517)
    U16 VirtualContext = 0;
    int i;
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if (VirtualContextId == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    /* Find an unused virtual context */
    for ( VirtualContext = 0;
         (VirtualContext < MAX_VIRTUAL_CONTEXTS) && VIRTUAL_CONTEXT_ALLOCATED(VirtualContext);
         VirtualContext++)
    {
    }

    if (VirtualContext < MAX_VIRTUAL_CONTEXTS)
    {
        /* Allocate this Context */
        VIRTUAL_CONTEXT_ALLOCATE(VirtualContext);
        *VirtualContextId = VIRTUAL_CONTEXT_CONTEXT_ID(VirtualContext);

        /* Initialise the context */
        for (i = 0; i < (sizeof(ContextLayout_t)/sizeof(U32)); i++)
        {
            ((U32*)&(VirtualContextLayout[VirtualContext]))[i] = 0;
        }
    }
    else
    {
        /* All the Contexts are in use */
        ErrorCode = STFDMA_ERROR_NO_FREE_CONTEXTS;
    }

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_DeallocateVirtualContext
Description  : Release a reserved Context
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_DeallocateVirtualContext     (STFDMA_VirtualContextId_t  VirtualContextId)
{
    ST_ErrorCode_t  ErrorCode      = ST_NO_ERROR;
    U16             VirtualContext = VirtualContextId&0X0000FFFF;
    U16             VirtualMagicNo = VirtualContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
            !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
            (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    VIRTUAL_CONTEXT_DEALLOCATE(VirtualContext);

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_VirtualContextRestore
Description  : Restore a virtual context to an actual context
Parameters   :
Return Value :
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextRestore (STFDMA_VirtualContextId_t  VirtualContextId,
                                             STFDMA_ContextId_t         ContextId)
{
    ST_ErrorCode_t    ErrorCode        = ST_NO_ERROR;
    U16               MagicNo          = ContextId>>16;
    U16               Context          = ContextId&0X0000FFFF;
    U16               VirtualMagicNo   = VirtualContextId>>16;
    U16               VirtualContext   = VirtualContextId&0X0000FFFF;
    U32              *VirtualContextLayout_p32;
    int               i;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
            !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
            (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
#endif /* STFDMA_NO_USAGE_CHECK */

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else /* ST_5517 */

    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    /* Point to source virtual context entry */
    VirtualContextLayout_p32 = (U32*)&VirtualContextLayout[VirtualContext];

    /* Copy context data from memory to actual FDMA context data region */
#if defined (ST_7100) || defined (ST_7109) || defined (ST_5525) || defined (ST_5162)
    for (i = 0; (i < sizeof(ContextLayout_t)/sizeof(U32) - 18); i++)
#else /* ST_7100 || ST_7109) || ST_5525 */
    for (i = 0; (i < sizeof(ContextLayout_t)/sizeof(U32) - 2); i++)
#endif /* ST_7100 || ST_7109) || ST_5525 */
    {
#if defined (ST_OSLINUX)
       iowrite32(VirtualContextLayout_p32[i], (void*)((U8*)(ContextLayout[STFDMA_PES]+Context) + (i * sizeof(U32))));
#else /* ST_OSLINUX */
       ((U32*)(ContextLayout[STFDMA_PES]+Context))[i] = VirtualContextLayout_p32[i];
#endif /* ST_OSLINUX */
    }

#if defined (ST_7100) || defined (ST_7109) || defined (ST_5525) || defined (ST_5162)
    for (i = 16; (i < sizeof(ContextLayout_t)/sizeof(U32)); i++)
    {
#if defined (ST_OSLINUX)
       iowrite32(VirtualContextLayout_p32[i], (void*)((U8*)(ContextLayout[STFDMA_PES]+Context) + (i * sizeof(U32))));
#else /* ST_OSLINUX */
       ((U32*)(ContextLayout[STFDMA_PES]+Context))[i] = VirtualContextLayout_p32[i];
#endif /* ST_OSLINUX */
    }
#endif /* ST_7100 || ST_7109) || ST_5525 */

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif  /* ST_5517 */

	return ErrorCode;
} /* STFDMA_VirtualContextRestore */

/****************************************************************************
Name         : STFDMA_VirtualContextSave
Description  : Save an actual context into a virtual context
Parameters   :
Return Value :
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextSave (STFDMA_ContextId_t         ContextId,
                                          STFDMA_VirtualContextId_t  VirtualContextId)
{
    ST_ErrorCode_t    ErrorCode        = ST_NO_ERROR;
    U16               MagicNo          = ContextId>>16;
    U16               Context          = ContextId&0X0000FFFF;
    U16               VirtualMagicNo   = VirtualContextId>>16;
    U16               VirtualContext   = VirtualContextId&0X0000FFFF;
    U32              *VirtualContextLayout_p32;
    int               i;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (!IS_INITIALISED(STFDMA_PES))
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
    else if ((MAX_CONTEXTS <= Context) ||
            !CONTEXT_ALLOCATED(Context) ||
            (CONTEXT_MAGIC_NO(Context) != MagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
            !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
            (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
#endif /* STFDMA_NO_USAGE_CHECK */

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else /* ST_5517 */

    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    /* Point to destination virtual context entry */
    VirtualContextLayout_p32 = (U32*)&VirtualContextLayout[VirtualContext];

    /* Copy context data from memory to actual FDMA context data region */
#if defined (ST_7100) || defined (ST_7109) || defined (ST_5525) || defined (ST_5162)
    for (i = 0; (i < sizeof(ContextLayout_t)/sizeof(U32) - 18); i++)
#else /* ST_7100 || ST_7109) || ST_5525 */
    for (i = 0; (i < sizeof(ContextLayout_t)/sizeof(U32) - 2); i++)
#endif /* ST_7100 || ST_7109) || ST_5525 */
    {
#if defined (ST_OSLINUX)
        VirtualContextLayout_p32[i] = (U32)(ioread32((void*)((U8*)(ContextLayout[STFDMA_PES]+Context) + (i * sizeof(U32))));
#else /* ST_OSLINUX */
        VirtualContextLayout_p32[i] = ((U32*)(ContextLayout[STFDMA_PES]+Context))[i];
#endif /* ST_OSLINUX */
    }

#if defined (ST_7100) || defined (ST_7109) || defined (ST_5525) || defined (ST_5162)
    for (i = 16; (i < sizeof(ContextLayout_t)/sizeof(U32)); i++)
    {
#if defined (ST_OSLINUX)
        VirtualContextLayout_p32[i] = (U32)(ioread32((void*)((U8*)(ContextLayout[STFDMA_PES]+Context) + (i * sizeof(U32))));
#else /* ST_OSLINUX */
        VirtualContextLayout_p32[i] = ((U32*)(ContextLayout[STFDMA_PES]+Context))[i];
#endif /* ST_OSLINUX */
    }
#endif /* ST_7100 || ST_7109) || ST_5525 */

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif  /* ST_5517 */

	return ErrorCode;
} /* STFDMA_VirtualContextSave */

/****************************************************************************
Name         : STFDMA_VirtualContextGetSCList
Description  : Return the current write position, remaining size and
               overflow status of the Start Code list in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
               ST_ERROR_BAD_PARAMETER
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextGetSCList (STFDMA_VirtualContextId_t   VirtualContextId,
	                                            STFDMA_SCEntry_t          **SCList,
	                                            U32                        *Size,
	                                            BOOL                       *Overflow)
{
    ST_ErrorCode_t    ErrorCode        = ST_NO_ERROR;
    U16               VirtualMagicNo   = VirtualContextId>>16;
    U16               VirtualContext   = VirtualContextId&0X0000FFFF;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
        !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
        (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if ((SCList   == NULL) ||
             (Size     == NULL) ||
             (Overflow == NULL))
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    *SCList    = (STFDMA_SCEntry_t*)(VirtualContextLayout[VirtualContext].SCWrite & ~0X0F);
    *Size      = (VirtualContextLayout[VirtualContext].SCSize);
    *Overflow  = (VirtualContextLayout[VirtualContext].SCWrite & 0X01);

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
} /* STFDMA_VirtualContextGetSCList */

/****************************************************************************
Name         : STFDMA_VirtualContextSetSCList
Description  : Set the strat code lists current write position and remaining
               size.
Parameters   :
Return Value :
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextSetSCList (STFDMA_VirtualContextId_t  VirtualContextId,
	                                            STFDMA_SCEntry_t          *SCList,
	                                            U32                        Size)
{
    ST_ErrorCode_t    ErrorCode        = ST_NO_ERROR;
    U16               VirtualMagicNo   = VirtualContextId>>16;
    U16               VirtualContext   = VirtualContextId&0X0000FFFF;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
        !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
        (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (SCList == NULL)
    {
        STTBX_Print(("ERROR: SC List %08X\n", (U32)(SCList)));
        return ST_ERROR_BAD_PARAMETER;
    }
    else if (((U32)(SCList)) & 0X0F)
    {
        STTBX_Print(("ERROR: SC List %08X\n", (U32)(SCList)));
        return ST_ERROR_BAD_PARAMETER;
    }
    else if (Size & 0X0F)
    {
        STTBX_Print(("ERROR: SC List Size %08X\n", Size));
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    VirtualContextLayout[VirtualContext].SCWrite = ((U32)(SCList)) & ~0X0F;
    VirtualContextLayout[VirtualContext].SCSize  = Size & ~0X0F;

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
} /* STFDMA_VirtualContextSetSCList */

/****************************************************************************
Name         : STFDMA_VirtualContextGetSCState
Description  : Return the currect configuration of the start code range.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextGetSCState (STFDMA_VirtualContextId_t  VirtualContextId,
	                                             STFDMA_SCState_t          *State,
	                                             STFDMA_SCRange_t           Range)
{
    ST_ErrorCode_t    ErrorCode        = ST_NO_ERROR;
    U16               VirtualMagicNo   = VirtualContextId>>16;
    U16               VirtualContext   = VirtualContextId&0X0000FFFF;
#if defined (ST_OSLINUX)
    U32             SCControl, PESControl;
#else
#if !defined(ST_5517)
    SCCtrl_t        SCControl   = {0};
    PESCtrl_t       PESControl  = {0};
#endif
#endif

#if !defined(ST_5517)
    int InRange = 0;
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
        !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
        (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (State == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    /* Under OS21 we must read and write using 32 bit variables, hence the use
       of SCControl, reading/writing to individual bit fields does not work */

    switch (Range)
    {
        case STFDMA_DEVICE_PES_RANGE_0 :
            State->OneShotEnabled = 0;
#if defined (ST_OSLINUX)
            PESControl = ioread32((void*)&(VirtualContextLayout[VirtualContext].PES0));
            State->RangeStart   = PESControl & 0x000000FF;
            State->RangeEnd     = PESControl & 0x0000FF00 >> 8;
            State->PTSEnabled   = PESControl & 0x20000000 >> 29;
            State->RangeEnabled = PESControl & 0x40000000 >> 30;
            InRange             = PESControl & 0x80000000 >> 31;
#else
            PESControl = VirtualContextLayout[VirtualContext].PES0;
            State->RangeStart     = PESControl.Start;
            State->RangeEnd       = PESControl.End;
            State->PTSEnabled     = PESControl.PTS_OneShot;
            State->RangeEnabled   = PESControl.Enable;
            InRange               = PESControl.InNotOut;
#endif
            break;

        case STFDMA_DEVICE_ES_RANGE_0 :
#if defined (ST_7100) || defined (ST_7109)
        case STFDMA_DEVICE_H264_RANGE_0:
#endif
            State->PTSEnabled     = 0;
#if defined (ST_OSLINUX)
            SCControl = VirtualContextLayout[VirtualContext].ES0;
            State->RangeStart     = SCControl & 0x000000FF;
            State->RangeEnd       = SCControl & 0x0000FF00 >> 8;
            State->OneShotEnabled = SCControl & 0x20000000 >> 29;
            State->RangeEnabled   = SCControl & 0x40000000 >> 30;
            InRange               = SCControl & 0x80000000 >> 31;
#else
            SCControl = VirtualContextLayout[VirtualContext].ES0;
            State->RangeStart     = SCControl.Start;
            State->RangeEnd       = SCControl.End;
            State->OneShotEnabled = SCControl.PTS_OneShot;
            State->RangeEnabled   = SCControl.Enable;
            InRange               = SCControl.InNotOut;
#endif
            break;

        case STFDMA_DEVICE_ES_RANGE_1 :
            State->PTSEnabled     = 0;
#if defined (ST_OSLINUX)
            SCControl = VirtualContextLayout[VirtualContext].ES1;
            State->RangeStart     = SCControl & 0x000000FF;
            State->RangeEnd       = SCControl & 0x0000FF00 >> 8;
            State->OneShotEnabled = SCControl & 0x20000000 >> 29;
            State->RangeEnabled   = SCControl & 0x40000000 >> 30;
            InRange               = SCControl & 0x80000000 >> 31;
#else
            SCControl = VirtualContextLayout[VirtualContext].ES1;
            State->RangeStart     = SCControl.Start;
            State->RangeEnd       = SCControl.End;
            State->OneShotEnabled = SCControl.PTS_OneShot;
            State->RangeEnabled   = SCControl.Enable;
            InRange               = SCControl.InNotOut;
#endif
            break;

        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);

    /* If checking for outsize the range then invert the range */
    if (!InRange)
    {
        U8 tmp   = State->RangeEnd;
        State->RangeEnd      = State->RangeStart+1;
        State->RangeStart    = tmp-1;
    }

#endif

    return ErrorCode;
} /* STFDMA_VirtualContextGetSCState */

/****************************************************************************
Name         : STFDMA_VirtualContextSetSCState
Description  : Set the current configuration of the start code range.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextSetSCState (STFDMA_VirtualContextId_t  VirtualContextId,
	                                             STFDMA_SCState_t          *State,
	                                             STFDMA_SCRange_t           Range)
{
    ST_ErrorCode_t    ErrorCode        = ST_NO_ERROR;
    U16               VirtualMagicNo   = VirtualContextId>>16;
    U16               VirtualContext   = VirtualContextId&0X0000FFFF;
#if defined (ST_OSLINUX)
    U32             SCControl, PESControl;
#else
#if !defined (ST_5517)
    SCCtrl_t        SCControl   = {0};
    PESCtrl_t       PESControl  = {0};
#endif
#endif

#if !defined(ST_5517)
    U8  Start;
    U8  End;
    int InRange;
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
        !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
        (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (State == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else

    Start    = State->RangeStart;
    End      = State->RangeEnd;
    InRange  = 1;

    /* If the range wraps the check for an outside range */
    if (End < Start)
    {
        End      = State->RangeStart-1;
        Start    = State->RangeEnd+1;
        InRange  = 0;
    }

    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    /* Under OS21 we must read and write using 32 bit variables, hence the use
       of SCControl, reading/writing to individual bit fields does not work */

    switch (Range)
    {
        case STFDMA_DEVICE_PES_RANGE_0 :
#if defined (ST_OSLINUX)
            PESControl = Start;
            PESControl |= (End << 8);
#if defined (STFDMA_DISABLE_PES_PACKET_LENGTH_PARSING)
            PESControl |= 0x08000000;   /* PPLDisable */
#endif
            PESControl |= (State->PTSEnabled) ? (0x20000000):(0x00000000);
            PESControl |= (State->RangeEnabled)?(0x40000000):(0x00000000);
            PESControl |= (InRange)?(0x80000000):(0x00000000);
            iowrite32(SCControl, (void*)&(VirtualContextLayout[VirtualContext].PES0));
#else
            PESControl.Start         = Start;
            PESControl.End           = End;
#if defined (STFDMA_DISABLE_PES_PACKET_LENGTH_PARSING)
            PESControl.PPLDisable    = 1;
#endif
            PESControl.PTS_OneShot   = (State->PTSEnabled)?(1):(0);
            PESControl.Enable        = (State->RangeEnabled)?(1):(0);
            PESControl.InNotOut      = InRange;
            VirtualContextLayout[VirtualContext].PES0 = PESControl;
#endif
            break;

        case STFDMA_DEVICE_ES_RANGE_0 :
#if defined (ST_OSLINUX)
            SCControl = Start;
            SCControl |= (End << 8);
            SCControl |= (State->OneShotEnabled) ? (0x20000000):(0x00000000);
            SCControl |= (State->RangeEnabled)?(0x40000000):(0x00000000);
            SCControl |= (InRange)?(0x80000000):(0x00000000);
            iowrite32(SCControl, (void*)&(VirtualContextLayout[VirtualContext].ES0));
#else
#if defined (ST_7100) || defined (ST_7109)
            SCControl.SCDModeSelect    = 0;
#endif
            SCControl.Start         = Start;
            SCControl.End           = End;
            SCControl.PTS_OneShot   = (State->OneShotEnabled)?(1):(0);
            SCControl.Enable        = (State->RangeEnabled)?(1):(0);
            SCControl.InNotOut      = InRange;
            VirtualContextLayout[VirtualContext].ES0 = SCControl;
#endif
            break;

#if defined (ST_7100) || defined (ST_7109)
        case STFDMA_DEVICE_H264_RANGE_0:
#if defined (ST_OSLINUX)
            SCControl = Start;
            SCControl |= (End << 8);
            SCControl |= (State->OneShotEnabled) ? (0x20000000):(0x00000000);
            SCControl |= (State->RangeEnabled)?(0x40000000):(0x00000000);
            SCControl |= (InRange)?(0x80000000):(0x00000000);
            SCControl |= 0x10000000;
            iowrite32(SCControl, (void*)&(VirtualContextLayout[VirtualContext].ES0));
#else
            SCControl.Start         = Start;
            SCControl.End           = End;
            SCControl.PTS_OneShot   = (State->OneShotEnabled)?(1):(0);
            SCControl.Enable        = (State->RangeEnabled)?(1):(0);
            SCControl.InNotOut      = InRange;
/* Due to specific bit positions of the 'mode' field the following difference is not apparent for ST_OSLINUX */
#if defined (ST_7100)
            SCControl.SCDModeSelect = 1;
#elif defined (ST_7109)
            SCControl.SCDModeSelect = 2;
#endif
            VirtualContextLayout[VirtualContext].ES0 = SCControl;
#endif
            break;
#endif

#if defined (ST_7109)
        case STFDMA_DEVICE_VC1_RANGE_0:
#if defined (ST_OSLINUX)
            SCControl = Start;
            SCControl |= (End << 8);
            SCControl |= (State->OneShotEnabled) ? (0x20000000):(0x00000000);
            SCControl |= (State->RangeEnabled)?(0x40000000):(0x00000000);
            SCControl |= (InRange)?(0x80000000):(0x00000000);
            SCControl |= 0x08000000;
            iowrite32(SCControl, (void*)&(VirtualContextLayout[VirtualContext].ES0));
#else
            SCControl.Start         = Start;
            SCControl.End           = End;
            SCControl.PTS_OneShot   = (State->OneShotEnabled)?(1):(0);
            SCControl.Enable        = (State->RangeEnabled)?(1):(0);
            SCControl.InNotOut      = InRange;
            SCControl.SCDModeSelect = 1;
            VirtualContextLayout[VirtualContext].ES0 = SCControl;
#endif
            break;
#endif

        case STFDMA_DEVICE_ES_RANGE_1 :
#if defined (ST_OSLINUX)
            SCControl = Start;
            SCControl |= (End << 8);
            SCControl |= (State->OneShotEnabled) ? (0x20000000):(0x00000000);
            SCControl |= (State->RangeEnabled)?(0x40000000):(0x00000000);
            SCControl |= (InRange)?(0x80000000):(0x00000000);
            iowrite32(SCControl, (void*)&(VirtualContextLayout[VirtualContext].ES1));
#else
            SCControl.Start         = Start;
            SCControl.End           = End;
            SCControl.PTS_OneShot   = (State->OneShotEnabled)?(1):(0);
            SCControl.Enable        = (State->RangeEnabled)?(1):(0);
            SCControl.InNotOut      = InRange;
#if defined (ST_7100) || defined (ST_7109)
            SCControl.SCDModeSelect = 0;
#endif
            VirtualContextLayout[VirtualContext].ES1 = SCControl;
#endif
            break;

        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
} /* STFDMA_VirtualContextSetSCState */

/****************************************************************************
Name         : STFDMA_VirtualContextSetESBuffer
Description  : Configure the output buffer in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextSetESBuffer (STFDMA_VirtualContextId_t  VirtualContextId,
                                                 void                      *Buffer,
                                                 U32                        Size)
{
    ST_ErrorCode_t    ErrorCode        = ST_NO_ERROR;
    U16               VirtualMagicNo   = VirtualContextId>>16;
    U16               VirtualContext   = VirtualContextId&0X0000FFFF;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
        !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
        (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (Buffer == NULL)
    {
        /* Bad buffer address */
        STTBX_Print(("ERROR: Buffer %08X\n", Buffer));
        return(ST_ERROR_BAD_PARAMETER);
    }
    else if ((U32)Buffer & 0X7F)
    {
        /* Bad alignment */
        STTBX_Print(("ERROR: Buffer %08X\n", Buffer));
        return(ST_ERROR_BAD_PARAMETER);
    }
    else if (Size & 0X7F)
    {
        /* The size is a bad multiple */
        STTBX_Print(("ERROR: Size %d\n", Size));
        return(ST_ERROR_BAD_PARAMETER);
    }
    else if (Size == 0)
    {
        /* The size is a bad multiple */
        STTBX_Print(("ERROR: Size %d\n", Size));
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    VirtualContextLayout[VirtualContext].ESTop    = ((U32)Buffer +Size) & ~0X7F;
    VirtualContextLayout[VirtualContext].ESRead   =  (U32)Buffer;
    VirtualContextLayout[VirtualContext].ESWrite  =  (U32)Buffer;
    VirtualContextLayout[VirtualContext].ESBottom = ((U32)Buffer) & ~0X7F;

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
} /* STFDMA_VirtualContextSetESBuffer */

/****************************************************************************
Name         : STFDMA_VirtualContextSetESReadPtr
Description  : Set the current read pointer for the output buffer in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextSetESReadPtr (STFDMA_VirtualContextId_t  VirtualContextId,
                                                  void                      *Read)
{
    ST_ErrorCode_t  ErrorCode      = ST_NO_ERROR;
    U16             VirtualContext = VirtualContextId&0X0000FFFF;
    U16             VirtualMagicNo = VirtualContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
        !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
        (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    VirtualContextLayout[VirtualContext].ESRead = (U32)Read;

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
} /* STFDMA_VirtualContextSetESReadPtr */

/****************************************************************************
Name         : STFDMA_VirtualContextSetESWritePtr
Description  : Set the current write pointer for the output buffer in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextSetESWritePtr (STFDMA_VirtualContextId_t  VirtualContextId,
                                                   void                      *Write)
{
    ST_ErrorCode_t  ErrorCode      = ST_NO_ERROR;
    U16             VirtualContext = VirtualContextId&0X0000FFFF;
    U16             VirtualMagicNo = VirtualContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
        !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
        (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if ((U32)Write & 0X03)
    {
        /* Bad alignment */
        STTBX_Print(("ERROR: Write ptr %08X\n", Write));
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    VirtualContextLayout[VirtualContext].ESWrite = (U32)Write;

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
} /* STFDMA_VirtualContextSetESWritePtr */

/****************************************************************************
Name         : STFDMA_VirtualContextSetESReadPtr
Description  : Get the current read pointer for the output buffer in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextGetESReadPtr (STFDMA_VirtualContextId_t   VirtualContextId,
                                                  void                      **Read)
{
    ST_ErrorCode_t  ErrorCode      = ST_NO_ERROR;
    U16             VirtualContext = VirtualContextId&0X0000FFFF;
    U16             VirtualMagicNo = VirtualContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
        !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
        (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (Read == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    *Read = (void*)VirtualContextLayout[VirtualContext].ESRead;

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
} /* STFDMA_VirtualContextSetESReadPtr */

/****************************************************************************
Name         : STFDMA_VirtualContextGetESWritePtr
Description  : Get the current write pointer for the output buffer in the Context.
Parameters   :
Return Value : STFDMA_ERROR_INVALID_CONTEXT_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_VirtualContextGetESWritePtr (STFDMA_VirtualContextId_t   VirtualContextId,
                                                   void                      **Write,
                                                   BOOL                       *Overflow)
{
    ST_ErrorCode_t  ErrorCode      = ST_NO_ERROR;
    U16             VirtualContext = VirtualContextId&0X0000FFFF;
    U16             VirtualMagicNo = VirtualContextId>>16;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if ((MAX_VIRTUAL_CONTEXTS <= VirtualContext) ||
        !VIRTUAL_CONTEXT_ALLOCATED(VirtualContext) ||
        (VIRTUAL_CONTEXT_MAGIC_NO(VirtualContext) != VirtualMagicNo))
    {
        return STFDMA_ERROR_INVALID_CONTEXT_ID;
    }
    else if (Write == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif

#if defined(ST_5517)
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    STOS_SemaphoreWait(APILockSem[STFDMA_PES]);

    *Write = (void*)(VirtualContextLayout[VirtualContext].ESWrite & ~0X03);
    if (NULL != Overflow) *Overflow = VirtualContextLayout[VirtualContext].ESWrite & 0X01;

    STOS_SemaphoreSignal(APILockSem[STFDMA_PES]);
#endif

    return ErrorCode;
} /* STFDMA_VirtualContextGetESWritePtr */
#endif /* STFDMA_USE_VIRTUAL_CONTEXT */
/*eof*/
