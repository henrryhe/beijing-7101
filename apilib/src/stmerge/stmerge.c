/************************************************************************

Source file name : stmerge.c

Description      : API Interface  for the STMERGE driver.

History          :

    14/11/03     : STMERGE v1.0.0A1 first release.
    02/04/04     : Added SWTS Trigger support for 5100.
                   Added STMERGE_ERROR_INVALID_TRIGGER error.
                   Removed warnings.
    22/07/04     : Added STMERGE_PacketLength_t structure.
                   Ported to 7710.
    10/01/05     : Added new elements in STMERGE_ObjectConfig_t structure.
                   Ported to 7100 & 5301.
    15/07/05     : Ported to LINUX OS
    02/02/06     : Ported to 5525
    23/03/07	  : Added new APIs STMERGE_Disconnect() and
    		           STMERGE_DuplicateInput()
   
Copyright (C) 2006 STMicroelectronics

*************************************************************************************/

/* Includes ------------------------------------------------------------------------*/
#if defined(ST_OS20) || defined(ST_OS21)
#include <string.h>
#include "stlite.h"
#include "stddefs.h"
#include "stmerge.h"
#include "mergereg.h"
#include "stsys.h"
#include "stdevice.h"
#elif defined(ST_OSLINUX)
#include "stddefs.h"
#include "stmerge.h"
#include "mergereg.h"
#include "stdevice.h"
extern void *TSMergeBasePtr_p; /* Needed when initialised. */
extern void *TSMergeDataPtr_p;
#endif

/* local #define */
#if defined(ST_5528)

#define STMERGE_NUM_TSIN     5
#define STMERGE_NUM_SWTS     2
#define STMERGE_NUM_P1394    1
#define STMERGE_NUM_ALTOUT   1
#define STMERGE_NUM_PTI      2

#elif defined(ST_5100) || defined(ST_7710) || defined(ST_7100) || defined(ST_5301)

#define STMERGE_NUM_TSIN      3
#define STMERGE_NUM_SWTS      1
#define STMERGE_NUM_ALTOUT    1
#define STMERGE_NUM_P1394     1 /* 1394_out */
#define STMERGE_NUM_PTI       1

#elif defined(ST_7109)

#define STMERGE_NUM_TSIN      3
#define STMERGE_NUM_SWTS      3
#define STMERGE_NUM_ALTOUT    1
#define STMERGE_NUM_P1394     1
#define STMERGE_NUM_PTI       2

#elif defined(ST_5525) || defined(ST_5524)

#define STMERGE_NUM_TSIN      5
#define STMERGE_NUM_SWTS      4
#define STMERGE_NUM_ALTOUT    1
#define STMERGE_NUM_P1394     1
#define STMERGE_NUM_PTI       2

#endif

#ifdef STMERGE_NO_1394_IN
#define ALL_STREAMS (STMERGE_NUM_TSIN + STMERGE_NUM_SWTS + STMERGE_NUM_ALTOUT)
#else
#define ALL_STREAMS (STMERGE_NUM_TSIN + STMERGE_NUM_SWTS + \
                     STMERGE_NUM_P1394 + STMERGE_NUM_ALTOUT)
#endif /* STMERGE_NO_1394_IN */

/* Driver control block structure */
typedef struct
{
    ST_DeviceName_t        DeviceName;
    STMERGE_Mode_t         Mode;               /* Operation mode of block */
    ST_Partition_t         *DriverPartition_p; /* Mem partion for driver use */
    U32                    *BaseAddress_p;     /* Base address of TSMERGER block */
#if defined(STMERGE_INTERRUPT_SUPPORT)
#ifdef ST_OS21
    interrupt_name_t    InterruptNumber;
#else
    U32                 InterruptNumber;
#endif
    U32                 InterruptLevel;
#endif

#if defined(ST_OSLINUX)
    U32                    *SWTS_DataAddress_p;  /* Address of SWTS Data register */
#endif
    STMERGE_ObjectConfig_t TSIN[STMERGE_NUM_TSIN];
    STMERGE_ObjectConfig_t SWTS[STMERGE_NUM_SWTS];
    STMERGE_ObjectConfig_t P1394[STMERGE_NUM_P1394];
    STMERGE_ObjectConfig_t ALTOUT[STMERGE_NUM_ALTOUT];
}ControlBlock_t;

static ControlBlock_t *Merge_p = NULL;

/* Bit manipulation */
#define merger_IsBitSet(Reg, number)        ((Reg & (1<< number)) != 0)
#define merger_ResetBit(Reg, number)        (Reg &= ~(U32)(1<<number))
#define merger_SetBit(Reg, number)          (Reg |= (1<<number))

/* Parameter validation */
static BOOL IsValidId(U32 StreamId);
static BOOL IsValidMode(U32 Mode);
static BOOL IsValidAddr(U32 Address);
static BOOL merger_IsParamsSet(U32 Id);

/* This semaphore ensures that only one user is accesing the driver */
#ifndef ST_OS21
static semaphore_t InitSemaphore;
#endif

static semaphore_t *InitSemaphore_p = NULL;

#define merger_LockDriver()            (void)semaphore_wait(InitSemaphore_p)
#define merger_UnLockDriver()          (void)semaphore_signal(InitSemaphore_p)

/* local function prototypes */
#if defined(ST_OS20) || defined(ST_OS21)
__inline void merger_WriteReg(STSYS_DU32 *Base, U32 Reg, U32 Value);
__inline int merger_ReadReg(STSYS_DU32 *Base, U32 RegOffset);
#elif defined(ST_OSLINUX)
#define merger_WriteReg(b,r,v) STSYS_WriteRegDev32LE((void*)(b)+(r),(v))
#define merger_ReadReg(b,r) STSYS_ReadRegDev32LE((void*)(b)+(r))
#endif

ST_ErrorCode_t merger_ConfigureUserSRAM(U32 *BaseAddr,U32 *SRAMMap);
ST_ErrorCode_t merger_Connect(U32 *BaseAddress_p,U32 SourceId, U32 DestinationId );
void merger_ConfigureDefaultSRAM(U32 *BaseAddr);
int RegisterOffset(U32 StreamID,U32 RegOffset);
void merger_ClearAll(U32 *BaseAddress_p);
void merger_SoftReset(U32 *BaseAddress_p);
#if defined(ST_OSLINUX)
void merger_SWTSXInjectBuf(STMERGE_ObjectId_t SWTS_Port,U32 Data);
#endif


#if defined(STMERGE_INTERRUPT_SUPPORT)
#if defined(ST_OSLINUX)
irqreturn_t MERGER_InterruptHandler(int irq, void* dev_id, struct pt_regs* regs)
{
        U32 RegOff = 0;
        U32 StatusReg = 0, Cfg2Reg =0;

        while(RegOff< LAST_OFFSET)
        {
             /* Read STREAM_STATUS_x register */
             StatusReg = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_STA_BASE + RegOff);

            if ( merger_IsBitSet(StatusReg,RAM_OVERFLOW_BIT))
            {
                /* Clear the Status register */
                merger_ResetBit(StatusReg,RAM_OVERFLOW_BIT);
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_STA_BASE + RegOff ,StatusReg);
             }

             /* Read TSM_STREAM_CFG2_BASE register */
             Cfg2Reg  = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + RegOff);

            /* Resetting the register */
            merger_SetBit(Cfg2Reg,CHANNEL_RST_BIT);

           /* reset SRAM by writing into channel_rst bit */
           merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + RegOff,Cfg2Reg);

           Cfg2Reg  = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + RegOff);
           merger_ResetBit(Cfg2Reg,CHANNEL_RST_BIT);
           merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + RegOff ,Cfg2Reg);

        RegOff += STREAM_OFFSET;
     }

       return IRQ_HANDLED;
}
#else

#if defined(ST_OS21)
int MERGER_InterruptHandler(ControlBlock_t *Merge_p)
#else
void MERGER_InterruptHandler(ControlBlock_t *Merge_p)
#endif
{

    U32 RegOff = 0, Val = 0;
    U32 StatusReg = 0;

    while( RegOff < LAST_OFFSET )
    {

        /* Read TSM_STREAM_STA_n register */
        StatusReg = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_STA_BASE + RegOff);

        if ( merger_IsBitSet(StatusReg,RAM_OVERFLOW_BIT))
        {
            /* Clear the overflow bit since it does not clear on reading */
            merger_ResetBit(StatusReg,RAM_OVERFLOW_BIT);
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_STA_BASE + RegOff ,StatusReg);

#if defined(ST_7100) || defined(ST_7109)

            /* Read TSM_STREAM_CFG_BASE register */
            Val = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + RegOff);

            /* stop the stream on bit of this channel */
            merger_ResetBit(Val,STREAM_ON_BIT);

            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + RegOff ,Val);

          
            /* Read TSM_STREAM_CFG2_BASE register */
            Val = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + RegOff);
            merger_SetBit(Val,CHANNEL_RST_BIT);

            /* reset SRAM by writing into channel_rst bit */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + RegOff ,Val);

            /* Read TSM_STREAM_CFG2_BASE register */
            Val = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + RegOff);

            merger_ResetBit(Val,CHANNEL_RST_BIT);

            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + RegOff ,Val);

			 /* set the stream on bit for this channel */
            Val = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + RegOff);

            merger_SetBit(Val,STREAM_ON_BIT);

            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + RegOff ,Val);

		

#else /* Switch OFF & switch ON the TS */

            /* Read TSM_STREAM_CFG_BASE register */
            Val = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + RegOff);

            /* stop the stream on bit of this channel */
            merger_ResetBit(Val,STREAM_ON_BIT);

            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + RegOff ,Val);

            /* set the stream on bit for this channel */
            Val = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + RegOff);

            merger_SetBit(Val,STREAM_ON_BIT);

            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + RegOff ,Val);

#endif
            break;
        }

        RegOff += STREAM_OFFSET;

    }


#ifdef ST_OS21
    return(OS21_SUCCESS);
#else
    return;
#endif
}
#endif
#endif

/****************************************************************************
Name         : STMERGE_Connect

Description  : Connects a Source to destintation.
               Destination can be PTI, ALTOUT or no desintation.
               If no destination is specified, the routing of the Source is
               switched off.

Parameters   : SourceId: Source to connect
               DestinationId: Destination to connect to.

Return Value : ST_NO_ERROR
               STMERGE_ERROR_NOT_INITIALIZED
               STMERGE_ERROR_BYPASS_MODE
               STMERGE_ERROR_INVALID_SOURCE_ID
               STMERGE_ERROR_INVALID_DESTINATION_ID
               STMERGE_ERROR_ILLEGAL_CONNECTION: Routing requested is not possible
                                                 (eg 1394in to 1394out)
****************************************************************************/

ST_ErrorCode_t STMERGE_Connect(STMERGE_ObjectId_t SourceId,
                               STMERGE_ObjectId_t DestinationId)

{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (InitSemaphore_p == NULL)
    {
        return(STMERGE_ERROR_NOT_INITIALIZED);
    }
    else
    {
        merger_LockDriver();
    }

    /* In BYPASS mode no API calls other than Init & Term are permitted */
    if (!IsValidMode(Merge_p->Mode) )
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_BYPASS_MODE);

    }

    /* call low level merger_connect function */
    Error = merger_Connect(Merge_p->BaseAddress_p,SourceId,DestinationId);

    merger_UnLockDriver();

    return(Error);

} /* STMERGE_Connect */

/****************************************************************************
Name         : STMERGE_ConnectGeneric

Description  : Connects a Source to destintation.
               Destination can be PTI, ALTOUT or no desintation.
               If no destination is specified, the routing of the Source is
               switched off.

Parameters   : SourceId: Source to connect
               DestinationId: Destination to connect to.

Return Value : ST_NO_ERROR
               STMERGE_ERROR_NOT_INITIALIZED
               STMERGE_ERROR_BYPASS_MODE
               STMERGE_ERROR_INVALID_SOURCE_ID
               STMERGE_ERROR_INVALID_DESTINATION_ID
               STMERGE_ERROR_ILLEGAL_CONNECTION: Routing requested is not possible
                                                 (eg 1394in to 1394out)
****************************************************************************/

ST_ErrorCode_t STMERGE_ConnectGeneric(STMERGE_ObjectId_t SourceId,
                                      STMERGE_ObjectId_t DestinationId,
                                      STMERGE_Mode_t Mode)

{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 Val = 0, Reg = 0, Src;

    if (InitSemaphore_p == NULL)
    {
        return(STMERGE_ERROR_NOT_INITIALIZED);
    }
    else
    {
        merger_LockDriver();
    }

    if ( Mode == STMERGE_NORMAL_OPERATION )
    {
    	switch( DestinationId )
        {

    	    case STMERGE_PTI_0:
    	        Val = merger_ReadReg(Merge_p->BaseAddress_p,TSM_SYS_CFG);
    	        merger_ResetBit(Val, NORMAL_MODE_PTI0_BIT); /* Bit 2 should be 0 for normal mode */
    	        break;

 #if (STMERGE_NUM_PTI > 1)
    	    case STMERGE_PTI_1:
    	        Val = merger_ReadReg(Merge_p->BaseAddress_p,TSM_SYS_CFG);
    	        merger_ResetBit(Val, NORMAL_MODE_PTI1_BIT); /* Bit 5 should be 0 for normal mode */
    	        break;
#endif
        }

    	/* reset sys_cfg register */
        merger_WriteReg(Merge_p->BaseAddress_p,TSM_SYS_CFG,Val);

    	  /* call low level merger_connect function */
        Error = merger_Connect(Merge_p->BaseAddress_p,SourceId,DestinationId);

    }
    else if ( Mode == STMERGE_BYPASS_GENERIC )
    {

        /* write this mode in cfg register */
        switch( SourceId )
        {

    	    case STMERGE_TSIN_0:
    	        Src = BYPASS_MODE_TSIN0_BIT;
    	        /* TSIN config:tagging off, Synced, serial input,byte aligned,Max Priority */
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE,TSIN_BYPASS_CFG);
    	        break;

            case STMERGE_TSIN_1:
    	        Src = BYPASS_MODE_TSIN1_BIT;
                Reg = RegisterOffset(STMERGE_TSIN_1,STREAM_OFFSET);
                /* TSIN config:tagging off, Synced, serial input,byte aligned,Max Priority */
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE+Reg,TSIN_BYPASS_CFG);
                break;

           case STMERGE_SWTS_0:
    	        Src = BYPASS_MODE_SWTS0_BIT;
    	        Reg = RegisterOffset(STMERGE_SWTS_0,STREAM_OFFSET);
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE+Reg,SWTS_BYPASS_CFG);
                break;

           default:
               Error = STMERGE_ERROR_FEATURE_NOT_SUPPORTED;
               break;

        }

        if ( Error == ST_NO_ERROR)
        {

    	    Val = merger_ReadReg(Merge_p->BaseAddress_p,TSM_SYS_CFG);
            Val &= MASK_FOR_BYPASS; /* mask for bypass bits */
            switch( DestinationId )
            {

    	        case STMERGE_PTI_0:
    	            Val |= Src;
    	            break;

#if (STMERGE_NUM_PTI > 1)
    	        case STMERGE_PTI_1:
    	            Val |= Src<<3;
    	            break;
#endif
    	        default:
                    Error = STMERGE_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
            }

            /* Finally write into register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_SYS_CFG,Val);
        }

    }
    else if ( Mode == STMERGE_BYPASS_TSIN_TO_PTI_ONLY || Mode == STMERGE_BYPASS_SWTS_TO_PTI_ONLY )
    {
    	/* do nothing */
    }
    else
    {
    	Error = STMERGE_ERROR_INVALID_MODE;

    }

    merger_UnLockDriver();
    return(Error);

} /* STMERGE_ConnectGeneric */

/****************************************************************************
Name         : STMERGE_Disconnect

Description  : Disconnects a Source from destintation.
               
Parameters   : SourceId: Source to disconnect
			DestinationId: Destination to be disconnected

Return Value : ST_NO_ERROR
               STMERGE_ERROR_NOT_INITIALIZED
               STMERGE_ERROR_BYPASS_MODE
               STMERGE_ERROR_INVALID_SOURCE_ID
               STMERGE_ERROR_INVALID_DESTINATION_ID
               STMERGE_ERROR_ILLEGAL_CONNECTION: Routing requested is not possible
                                                 (eg 1394in to 1394out)
		 STMERGE_ERROR_PARAMETERS_NOT_SET
****************************************************************************/

ST_ErrorCode_t STMERGE_Disconnect(STMERGE_ObjectId_t SourceId,
                               STMERGE_ObjectId_t DestinationId)
{

    U32 Val = 0,Src = 0;

    if (InitSemaphore_p == NULL)
    {
        return(STMERGE_ERROR_NOT_INITIALIZED);
    }
    else
    {
        merger_LockDriver();
    }

    /* In BYPASS mode no API calls other than Init & Term are permitted */
    if (!IsValidMode(Merge_p->Mode) )
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_BYPASS_MODE);

    }

   /* Check for Valid Stream ID passed by caller */
    if (!IsValidId(SourceId))
    {
         merger_UnLockDriver();
         return(STMERGE_ERROR_INVALID_SOURCE_ID);
    }

    /* Check for Valid Destination ID */
     switch(DestinationId)
    {
         case STMERGE_PTI_0:
         case STMERGE_1394OUT_0:

#if (STMERGE_NUM_PTI > 1)
         case STMERGE_PTI_1:
#endif
            break;

            default:
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_DESTINATION_ID);
    }

        /* Check for legal connection */
        if (SourceId == STMERGE_1394IN_0 && DestinationId == STMERGE_1394OUT_0)
        {
            merger_UnLockDriver();
            return(STMERGE_ERROR_ILLEGAL_CONNECTION);
        }

        /* Disconnection can only be made if stream parameters are set */
        if (!merger_IsParamsSet(SourceId))
        {
            merger_UnLockDriver();
            return(STMERGE_ERROR_PARAMETERS_NOT_SET);
        }

    /* Check for Valid Src ID & reset src bit from all dest reg */
    switch(SourceId)
    {
        case STMERGE_ALTOUT_0:
            Val++;
            /* No break */
     case STMERGE_SWTS_3:
#if (STMERGE_NUM_SWTS > 3)
            Val++;
            /* No break */
#endif

        case STMERGE_SWTS_2:
#if (STMERGE_NUM_SWTS > 2)
            Val++;
            /* No break */
#endif
        case STMERGE_SWTS_1:
#if (STMERGE_NUM_SWTS > 1)
            Val++;
            /* No break */
#endif

        case STMERGE_SWTS_0:
            Val++;
            /* No break */

        case STMERGE_1394IN_0:
#ifndef STMERGE_NO_1394_IN
            Val++;
            /* No break */
#endif

        case STMERGE_TSIN_4:
#if (STMERGE_NUM_TSIN > 4)
            Val++;
            /* No break */
#endif

        case STMERGE_TSIN_3:
#if (STMERGE_NUM_TSIN > 3)
            Val++;
            /* No break */
#endif

        case STMERGE_TSIN_2:
            Val++;
            /* No break */

        case STMERGE_TSIN_1:
            Val++;
            /* No break */

        case STMERGE_TSIN_0:
            break;

        default:
            merger_UnLockDriver();
            return(STMERGE_ERROR_INVALID_SOURCE_ID);

       }

    /* Normal disconnect procedure */
     switch(DestinationId)
    {
         case STMERGE_PTI_0:

              /* Read PTI0 Dest register */
                     Src = merger_ReadReg(Merge_p->BaseAddress_p,TSM_PTI0_DEST);
                     /* Get the src information to set the src bit */
                     merger_ResetBit(Src,Val);
                     /* Disconnect Src from PTI0 Destination */
                     merger_WriteReg(Merge_p->BaseAddress_p,TSM_PTI0_DEST, Src);
                     break;

         case STMERGE_1394OUT_0:

                     /* Read P1394 Dest register */
                     Src = merger_ReadReg(Merge_p->BaseAddress_p,TSM_P1394_DEST);
                     /* Get the src information to set the src bit */
                     merger_ResetBit(Src,Val);
                     /* Disconnect Src from P1394 Destination */
                     merger_WriteReg(Merge_p->BaseAddress_p,TSM_P1394_DEST, Src);
        return ST_NO_ERROR;
                     break;

#if (STMERGE_NUM_PTI > 1)
         case STMERGE_PTI_1:

                     /* Read PTI1 Dest register */
                     Src = merger_ReadReg(Merge_p->BaseAddress_p,TSM_PTI1_DEST);
                     /* Get the src information to set the src bit */
                     merger_ResetBit(Src,Val);
                     /* Disconnect Src from PTI1 Destination */
                     merger_WriteReg(Merge_p->BaseAddress_p,TSM_PTI1_DEST, Src);
            break;
#endif

            default:
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_DESTINATION_ID);
        }

	 merger_UnLockDriver();
        
        return ST_NO_ERROR;
}

/****************************************************************************
Name         : STMERGE_DuplicateInput

Description  : Duplicate a Source prior to the tsmerger.
                          
Parameters   : SourceId: Source to duplicate

Return Value : ST_NO_ERROR
               STMERGE_ERROR_NOT_INITIALIZED
               STMERGE_ERROR_BYPASS_MODE
               STMERGE_ERROR_INVALID_SOURCE_ID
               ST_ERROR_FEATURE_NOT_SUPPORTED
****************************************************************************/

ST_ErrorCode_t STMERGE_DuplicateInput(STMERGE_ObjectId_t SourceId)
{
#if !defined(ST_7100)&&!defined(ST_7109)&&!defined(ST_5100)
    return (ST_ERROR_FEATURE_NOT_SUPPORTED);
#else
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32            regVal = 0;
  
    if (InitSemaphore_p == NULL)
    {
        return(STMERGE_ERROR_NOT_INITIALIZED);
    }
    else
    {
        merger_LockDriver();
    }

    /* In BYPASS mode no API calls other than Init & Term are permitted */
    if (Merge_p->Mode == STMERGE_BYPASS_TSIN_TO_PTI_ONLY ||
        Merge_p->Mode == STMERGE_BYPASS_SWTS_TO_PTI_ONLY)
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_BYPASS_MODE);

    }
	
    /* Check for Valid Src ID  */
    if ((SourceId != STMERGE_TSIN_0) && (SourceId != STMERGE_TSIN_1) && (SourceId != STMERGE_NO_SRC))
    {
      merger_UnLockDriver();
      return (STMERGE_ERROR_INVALID_SOURCE_ID);
    }
    
    if (SourceId == STMERGE_NO_SRC)
    {
      regVal = 0; 
    }
	
    if (SourceId == STMERGE_TSIN_0)
    {
#if defined(ST_5100)
      regVal = 0x800;
#endif
#if defined(ST_7100)||defined(ST_7109)
      regVal = 0x2; 
#endif
    }
	
    if (SourceId == STMERGE_TSIN_1)
    {
#if defined(ST_5100)
      regVal = 0x10800;
#endif
#if defined(ST_7100)||defined(ST_7109)
      regVal = 0x6; 
#endif
    }
    
#if defined(ST_5100)
    merger_WriteReg((void*)ST5100_PTI_BASE_ADDRESS,0x58, regVal);
#endif
#if defined(ST_7100)
    merger_WriteReg((void*)ST7100_CFG_BASE_ADDRESS,0x100, regVal);
#endif
#if defined(ST_7109)
    merger_WriteReg((void*)ST7109_CFG_BASE_ADDRESS,0x100, regVal);
#endif
    merger_UnLockDriver();
    return (Error);
	
#endif      
}

/****************************************************************************
Name         : STMERGE_GetDefaultParams

Description  : Loads the given structure with default values depending on
               the Source type given.
               Default values may need tweaking to suite applications
               requirements.

Parameters   : SourceId
               Source_p

Return Value : ST_NO_ERROR
               STMERGE_ERROR_NOT_INITIALIZED
               STMERGE_ERROR_BYPASS_MODE
               ST_ERROR_BAD_PARAMETR: Null pointer given
               STMERGE_ERROR_INVALID_ID: Source not recognised.
               STMEGE_ERROR_FEATURE_NOT_SUPPORTED

****************************************************************************/

ST_ErrorCode_t STMERGE_GetDefaultParams(STMERGE_ObjectId_t Id,
                                        STMERGE_ObjectConfig_t *Source_p)

{

#if defined(STMERGE_DEFAULT_PARAMETERS)

    STMERGE_ObjectConfig_t DefaultParamsTSIN;
    STMERGE_ObjectConfig_t DefaultParamsSWTS;
    STMERGE_ObjectConfig_t DefaultParamsALTOUT;
#ifndef STMERGE_NO_1394_IN
    STMERGE_ObjectConfig_t DefaultParamsP1394;
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#endif /* STMERGE_DEFAULT_PARAMETERS */

    if (InitSemaphore_p == NULL)
    {
        return(STMERGE_ERROR_NOT_INITIALIZED);
    }
    else
    {
        merger_LockDriver();

    }

    /* In BYPASS mode no API calls other than Init & Term are permitted */
    if (!IsValidMode(Merge_p->Mode) )
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_BYPASS_MODE);

    }

    if (Source_p == NULL)
    {
        merger_UnLockDriver();
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!IsValidId(Id))
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_INVALID_ID);
    }


#if defined(STMERGE_DEFAULT_PARAMETERS)


    switch(Id)
    {
        case STMERGE_TSIN_0:
        case STMERGE_TSIN_1:
        case STMERGE_TSIN_2:

#if (STMERGE_NUM_TSIN > 3)
        case STMERGE_TSIN_3:
#endif

#if (STMERGE_NUM_TSIN > 4)
        case STMERGE_TSIN_4:
#endif

            /* For TSIN inputs */
            DefaultParamsTSIN.Priority                  = STMERGE_PRIORITY_HIGH;
            DefaultParamsTSIN.SyncLock                  = 1;
            DefaultParamsTSIN.SyncDrop                  = 3;
            DefaultParamsTSIN.SOPSymbol                 = 0x47;
            DefaultParamsTSIN.u.TSIN.SerialNotParallel  = TRUE;
            DefaultParamsTSIN.u.TSIN.InvertByteClk      = FALSE;
            DefaultParamsTSIN.u.TSIN.ByteAlignSOPSymbol = TRUE;

#if defined(STMERGE_DTV_PACKET)
            DefaultParamsTSIN.PacketLength              = STMERGE_PACKET_LENGTH_DTV;
            DefaultParamsTSIN.u.TSIN.SyncNotAsync       = FALSE;
            DefaultParamsTSIN.u.TSIN.ReplaceSOPSymbol   = TRUE;
#else
            DefaultParamsTSIN.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
            DefaultParamsTSIN.u.TSIN.SyncNotAsync       = TRUE;
            DefaultParamsTSIN.u.TSIN.ReplaceSOPSymbol   = FALSE;
#endif

            *Source_p = DefaultParamsTSIN;
            break;

        case STMERGE_SWTS_0:

#if (STMERGE_NUM_SWTS > 1)
        case STMERGE_SWTS_1:
#endif

#if (STMERGE_NUM_SWTS > 2)
        case STMERGE_SWTS_2:
#endif

#if (STMERGE_NUM_SWTS > 3)
        case STMERGE_SWTS_3:
#endif

            /* For SWTS inputs */
            DefaultParamsSWTS.Priority             = STMERGE_PRIORITY_MID;
            DefaultParamsSWTS.SyncLock             = 2;
            DefaultParamsSWTS.SyncDrop             = 2;
            DefaultParamsSWTS.SOPSymbol            = 0x47;
            DefaultParamsSWTS.PacketLength         = STMERGE_PACKET_LENGTH_DVB;
            DefaultParamsSWTS.u.SWTS.Tagging       = STMERGE_TAG_NO_CHANGE;
            DefaultParamsSWTS.u.SWTS.Pace          = STMERGE_PACE_AUTO;
            DefaultParamsSWTS.u.SWTS.Counter.Value = STMERGE_COUNTER_AUTO_LOAD;
            DefaultParamsSWTS.u.SWTS.Counter.Rate  = STMERGE_COUNTER_NO_INC;

            *Source_p = DefaultParamsSWTS;
            break;


        case STMERGE_ALTOUT_0:

            /* For ALTOUT input */
            DefaultParamsALTOUT.Priority               = STMERGE_PRIORITY_LOWEST;
            DefaultParamsALTOUT.SyncLock               = 1;
            DefaultParamsALTOUT.SyncDrop               = 3;
            DefaultParamsALTOUT.SOPSymbol              = 0x47;
            DefaultParamsALTOUT.PacketLength           = STMERGE_PACKET_LENGTH_DVB;
            DefaultParamsALTOUT.u.ALTOUT.PTISource     = STMERGE_PTI_0;
            DefaultParamsALTOUT.u.ALTOUT.Pace          = STMERGE_PACE_AUTO;
            DefaultParamsALTOUT.u.ALTOUT.Counter.Value = STMERGE_COUNTER_AUTO_LOAD;
            DefaultParamsALTOUT.u.ALTOUT.Counter.Rate  = STMERGE_COUNTER_NO_INC;

            *Source_p = DefaultParamsALTOUT;
            break;

        case STMERGE_1394IN_0:
#ifndef STMERGE_NO_1394_IN
            /* For P1394 input */
            DefaultParamsP1394.Priority        = STMERGE_PRIORITY_LOWEST;
            DefaultParamsP1394.SyncLock        = 1;
            DefaultParamsP1394.SyncDrop        = 3;
            DefaultParamsP1394.SOPSymbol       = 0x47;
            DefaultParamsP1394.PacketLength    = STMERGE_PACKET_LENGTH_DVB;
            DefaultParamsP1394.u.P1394.Pace    = STMERGE_PACE_AUTO;
            DefaultParamsP1394.u.P1394.Tagging = STMERGE_TAG_REPLACE_ID;

            *Source_p = DefaultParamsP1394;
#endif
            break;

        case STMERGE_1394OUT_0:

            /* For P1394 input */
            DefaultParamsP1394.Priority        = STMERGE_PRIORITY_LOWEST;
            DefaultParamsP1394.SyncLock        = 1;
            DefaultParamsP1394.SyncDrop        = 3;
            DefaultParamsP1394.SOPSymbol       = 0x47;
            DefaultParamsP1394.PacketLength    = STMERGE_PACKET_LENGTH_DVB;
            DefaultParamsP1394.u.P1394.Pace    = STMERGE_PACE_AUTO;
            DefaultParamsP1394.u.P1394.Tagging = STMERGE_TAG_REPLACE_ID;

            *Source_p = DefaultParamsP1394;
            break;

        default:
            break;
    }

    merger_UnLockDriver();
    return(Error);

#else

    merger_UnLockDriver();
    return(STMERGE_ERROR_FEATURE_NOT_SUPPORTED);

#endif /* STMERGE_DEFAULT_PARAMETERS */

} /* STMERGE_GetDefaultParams */

/****************************************************************************
Name         : STMERGE_GetParams

Description  : Loads the given structure with given Sources current values.

Parameters   : SourceId
               Source_p

Return Value : ST_NO_ERROR
               STMERGE_ERROR_NOT_INITIALIZED
               ST_ERROR_BAD_PARAMETR: Null pointer given
               STMERGE_ERROR_INVALID_ID: Source not recognised.
****************************************************************************/

ST_ErrorCode_t STMERGE_GetParams(STMERGE_ObjectId_t Id,
                                 STMERGE_ObjectConfig_t *Config_p)

{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 Instance = 0;

    if (InitSemaphore_p == NULL)
    {
        return(STMERGE_ERROR_NOT_INITIALIZED);
    }
    else
    {
        merger_LockDriver();
    }

    /* In BYPASS mode no API calls other than Init & Term are permitted */
    if (!IsValidMode(Merge_p->Mode) )
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_BYPASS_MODE);

    }

    /* Check the pointer passed by the caller */
    if (Config_p == NULL)
    {
        merger_UnLockDriver();
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!merger_IsParamsSet(Id))
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_PARAMETERS_NOT_SET);
    }


    /* Stream specific configuration parameters */
    switch (Id)
    {
        case STMERGE_TSIN_0:
        case STMERGE_TSIN_1:
        case STMERGE_TSIN_2:

#if (STMERGE_NUM_TSIN > 3)
        case STMERGE_TSIN_3:
#endif

#if (STMERGE_NUM_TSIN > 4)
        case STMERGE_TSIN_4:
#endif

            /* Get stream instance */
            Instance = GetInstanceFromId(STMERGE_TSIN_0,Id);
            *Config_p = Merge_p->TSIN[Instance];
            break;


        case STMERGE_SWTS_0:

#if (STMERGE_NUM_SWTS > 1)
        case STMERGE_SWTS_1:
#endif

#if (STMERGE_NUM_SWTS > 2)
        case STMERGE_SWTS_2:
#endif

#if (STMERGE_NUM_SWTS > 3)
        case STMERGE_SWTS_3:
#endif
            /* Get stream instance */
            Instance = GetInstanceFromId(STMERGE_SWTS_0,Id);
            *Config_p = Merge_p->SWTS[Instance];
            break;

        case STMERGE_ALTOUT_0:
            /* Get stream instance */
            Instance  = GetInstanceFromId(STMERGE_ALTOUT_0,Id);
            *Config_p = Merge_p->ALTOUT[Instance];
            break;

        case STMERGE_1394IN_0:
 #ifndef STMERGE_NO_1394_IN
            /* Get stream instance */
            Instance  = GetInstanceFromId(STMERGE_1394IN_0,Id);
            *Config_p = Merge_p->P1394[Instance];
 #endif
            break;

        case STMERGE_1394OUT_0:
            /* Get stream instance */
            Instance  = GetInstanceFromId(STMERGE_1394OUT_0,Id);
            *Config_p = Merge_p->P1394[Instance];
            break;

        default:
            merger_UnLockDriver();
            return(STMERGE_ERROR_INVALID_ID);

    }

    /* Unlock while exiting */
    merger_UnLockDriver();
    return(Error);

} /* STMERGE_GetParams */

/****************************************************************************
Name         : STMERGE_GetRevision

Description  : Returns revision string of driver

Parameters   :

Return Value :
****************************************************************************/

ST_Revision_t STMERGE_GetRevision(void)
{
    return ((ST_Revision_t)STMERGE_REVISION);

} /* STMERGE_GetRevision */

/****************************************************************************
Name         : STMERGE_GetStatus

Description  : Returns the current status information of the given Id.

Parameters   : SourceId
               SourceStatus_p

Return Value : ST_NO_ERROR
               STMERGE_ERROR_NOT_INITIALIZED
               STMERGE_ERROR_BYPASS_MODE
               STMERGE_ERROR_INVALID_ID
               ST_ERROR_BAD_PARAMETER: Null pointer
****************************************************************************/

ST_ErrorCode_t STMERGE_GetStatus(STMERGE_ObjectId_t Id,
                                 STMERGE_Status_t *Status_p)

{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 StatusReg,DestVal;

#if defined(ST_7100) || defined(ST_7109)
    U32 RetVal;
#endif

    U32 RegOff,Src = 0;
    U32 count = 0;

    if(InitSemaphore_p == NULL)
    {
        return(STMERGE_ERROR_NOT_INITIALIZED);
    }
    else
    {
        merger_LockDriver();

    }

    /* In BYPASS mode no API calls other than Init & Term are permitted */
    if (!IsValidMode(Merge_p->Mode) )
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_BYPASS_MODE);

    }

    /* Check for Valid Stream ID passed by caller */
    if (!IsValidId(Id))
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_INVALID_ID);
    }

    /* Check whether the Status_p pointer passed by caller is valid */
    if (Status_p == NULL)
    {
        merger_UnLockDriver();
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Get the offset of register from Base */
    RegOff = RegisterOffset(Id,STREAM_OFFSET);


    /* Read TSM_STREAM_STA_n register */
    StatusReg = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_STA_BASE + RegOff);

    Status_p->Lock          = merger_IsBitSet(StatusReg,STREAM_LOCK_BIT);

    Status_p->InputOverflow = merger_IsBitSet(StatusReg,INPUT_OVERFLOW_BIT);

    Status_p->RAMOverflow   = merger_IsBitSet(StatusReg,RAM_OVERFLOW_BIT);

    Status_p->ErrorPackets  = (U32)(StatusReg & ERROR_PACKETS);

    /* Counter value is applicable in SWTSs & ALTOUT */
    switch(Id)
    {
       case STMERGE_ALTOUT_0:
       case STMERGE_SWTS_0:

#if (STMERGE_NUM_SWTS > 1)
       case STMERGE_SWTS_1:
#endif

#if (STMERGE_NUM_SWTS > 2)
       case STMERGE_SWTS_2:
#endif

#if (STMERGE_NUM_SWTS > 3)
       case STMERGE_SWTS_3:
#endif

           Status_p->Counter = (U32)(StatusReg & COUNTER_VALUE);
           break;

       default:
           Status_p->Counter = 0x00000000;
           break;
    }

    /* Check for Valid Stream ID passed by caller */
    switch(Id)
    {

        case STMERGE_ALTOUT_0:
            count++;
            /* No break */

        case STMERGE_SWTS_3:
#if (STMERGE_NUM_SWTS > 3)
            count++;
            /* No break */
#endif

        case STMERGE_SWTS_2:
#if (STMERGE_NUM_SWTS > 2)
            count++;
            /* No break */
#endif

        case STMERGE_SWTS_1:
#if (STMERGE_NUM_SWTS > 1)
            count++;
            /* No break */
#endif

        case STMERGE_SWTS_0:
            count++;
            /* No break */

        case STMERGE_1394IN_0:
#ifndef STMERGE_NO_1394_IN
            count++;
            /* No break */
#endif

        case STMERGE_TSIN_4:
#if (STMERGE_NUM_TSIN > 4)
            count++;
#endif
            /* No break */

        case STMERGE_TSIN_3:
#if (STMERGE_NUM_TSIN > 3)
            count++;
            /* No break */
#endif

        case STMERGE_TSIN_2:
            count++;
            /* No break */

        case STMERGE_TSIN_1:
            count++;
            /* No break */

        case STMERGE_TSIN_0:
            break;

        default:
            break;

    }

    /* Get the required src bit */
    merger_SetBit(Src,count);

    /* Read TSM_XXXX_DEST register of the stream to know the source */
    DestVal = merger_ReadReg(Merge_p->BaseAddress_p,TSM_PTI0_DEST);
    
    Status_p->DestinationId=STMERGE_NO_DEST;

    if ((DestVal & Src) == Src)
    {
        Status_p->DestinationId = STMERGE_PTI_0;

    }

#if (STMERGE_NUM_PTI > 1)

        /* Read TSM_PTI1_DEST register & find which src bit is set */
        DestVal = merger_ReadReg(Merge_p->BaseAddress_p,TSM_PTI1_DEST);

        if ((DestVal & Src) == Src)
        {
            Status_p->DestinationId |= STMERGE_PTI_1;
        }
#endif

            /* Read TSM_P1394_DEST register & find which src bit is set */
            DestVal = merger_ReadReg(Merge_p->BaseAddress_p,TSM_P1394_DEST);
            if ((DestVal & Src) == Src)
            {
                Status_p->DestinationId |= STMERGE_1394OUT_0;
            }

#if defined(ST_7100) || defined(ST_7109)
    /* get information about read & write ptr */
    RetVal  = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + RegOff);

    Status_p->Read_p  = (U16)( RetVal & READ_PTR_MASK);
    Status_p->Write_p = (U16)( RetVal & WRITE_PTR_MASK);

#endif

    merger_UnLockDriver();
    return(Error);

}/* STMERGE_GetStatus */

/***************************************************************************************
Name         : STMERGE_Init

Description  : Block initialisation.
               Single instance, only one call is permitted unless STMERGE_Term
               is called. A re-initialisation of the driver/block results
               in current tTSMERGer operation stopping.
               The SourceMemoryMap can be configured at block init time only.
               If a Source is not given enough memory and more needs be allocated
               a term and re-init is required.

Parameters   : DeviceName: Name to associate with initialised TSMERG
               InitParams_p: Block initialisation parameters.

Return Value : ST_NO_ERROR
               ST_ERROR_ALREADY_INITIALISED
               ST_ERROR_BAD_PARAMETER: DriverPartition,InitParams & DeviceName is NULL
               ST_ERROR_NO_MEMORY: Not enough memory for driver initialisation
               STMERGE_ERROR_MERGE_MEMORY: Source memory map exceeds memory size
               STMERGE_ERROR_DEVICE_NOT_SUPPORTED:If DeviceType is not STMERGE_DEVICE_1
               STMERGE_ERROR_INVALID_MODE:Other than BYPASS or NORMAL mode.
****************************************************************************************/

ST_ErrorCode_t STMERGE_Init(const ST_DeviceName_t DeviceName,
                            const STMERGE_InitParams_t *InitParams_p)
{


    ST_ErrorCode_t Error = ST_NO_ERROR;
    partition_t * DriverPartition_p;
    U32 Reg;

#if defined(ST_OSLINUX)
    ((STMERGE_InitParams_t*)InitParams_p)->DriverPartition_p = (ST_Partition_t*)1; /* Dummy value*/
    ((STMERGE_InitParams_t*)InitParams_p)->BaseAddress_p     = (U32*)TSMergeBasePtr_p;
#endif

#if defined(ST_OS20) || defined(ST_OS21)
    task_lock();
#elif defined(ST_OSLINUX)
    preempt_disable();
#endif
    if (InitSemaphore_p == NULL)
    {
#ifndef ST_OS21
        InitSemaphore_p = &InitSemaphore;
#endif

#if defined(ST_OS20) || defined(ST_OS21)
        InitSemaphore_p = semaphore_create_fifo(1);
#elif defined(ST_OSLINUX)
        semaphore_init_fifo( InitSemaphore_p, 1 );
#endif
    }
    else
    {
#if defined(ST_OS20) || defined(ST_OS21)
        task_unlock();
#elif defined(ST_OSLINUX)
        preempt_enable();
#endif
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

#if defined(ST_OS20) || defined(ST_OS21)
    task_unlock();
#elif defined(ST_OSLINUX)
    preempt_enable();
#endif

    /* Perform parameter validity checks */

    if ((DeviceName           != NULL)   &&           /* NULL Name ptr? */
        (InitParams_p         != NULL)   &&           /* NULL structure ptr? */
        (DeviceName[0]        != '\0')   &&           /* Device Name undefined? */
        (strlen(DeviceName)   < ST_MAX_DEVICE_NAME))  /* string too long? */
    {

        /* This chunk needs to be atomic */
        merger_LockDriver();

        /* Re-clean all the TSMERGE registers before start */
        merger_ClearAll(InitParams_p->BaseAddress_p);

        if (InitParams_p->DriverPartition_p != NULL)
        {
            if (InitParams_p->DeviceType == STMERGE_DEVICE_1)
            {
                switch(InitParams_p->Mode)
                {

                    case STMERGE_NORMAL_OPERATION:

                        /* reset sys_cfg register */
                        merger_WriteReg(InitParams_p->BaseAddress_p,TSM_SYS_CFG,0);

                        break;


                    case STMERGE_BYPASS_TSIN_TO_PTI_ONLY:

                        merger_WriteReg(InitParams_p->BaseAddress_p,TSM_SYS_CFG,TSIN_BYPASS);

                        /* TSIN0 config:tagging off, Synced, serial input,byte aligned,Max Priority */
                        merger_WriteReg(InitParams_p->BaseAddress_p,TSM_STREAM_CFG_BASE,TSIN_BYPASS_CFG);

                        break;


                    case STMERGE_BYPASS_SWTS_TO_PTI_ONLY:

                        /* Configure SWTS in bypass mode of operation */
                        merger_WriteReg(InitParams_p->BaseAddress_p,TSM_SYS_CFG,SWTS_BYPASS);

                        /* Get SWTS0 register offset */
                        Reg = RegisterOffset(STMERGE_SWTS_0,STREAM_OFFSET);

                        /* SWTS0 config:tagging off,Synced,serial input,byte aligned,Priority set */
                        merger_WriteReg(InitParams_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg,SWTS_BYPASS_CFG);

                        break;


                    case STMERGE_BYPASS_GENERIC:
                        /* do nothing */
                        break;

                    default:
                        Error = STMERGE_ERROR_INVALID_MODE;

                        /* Signal semaphore before deleting it */
                        merger_UnLockDriver();
                        semaphore_delete(InitSemaphore_p);
                        InitSemaphore_p = (semaphore_t *)NULL;

                        return (Error);

                }

                if ( InitParams_p->Mode != STMERGE_BYPASS_TSIN_TO_PTI_ONLY && \
                     InitParams_p->Mode != STMERGE_BYPASS_SWTS_TO_PTI_ONLY )
                {
                      if (InitParams_p->MergeMemoryMap_p == STMERGE_DEFAULT_MERGE_MEMORY_MAP)
                      {
                             /* Default setting equally distributes SRAM to all the streams */
                             merger_ConfigureDefaultSRAM(InitParams_p->BaseAddress_p);
                      }
                      else
                      {
                            /* User configurations are validated & merge map is set in SRAM */
                            Error = merger_ConfigureUserSRAM(InitParams_p->BaseAddress_p,
                                                     InitParams_p->MergeMemoryMap_p);
                      }
                }
                if (Error == ST_NO_ERROR)
                {
                    DriverPartition_p = (partition_t *)InitParams_p->DriverPartition_p;

                    /* Allocate memory for the Merger Control Block */
                    Merge_p = (ControlBlock_t *)memory_allocate_clear
                              (DriverPartition_p,1,
                               sizeof(ControlBlock_t));
                    if (Merge_p != NULL)
                    {
                        /* Fill control block with initial parameters */
                        strncpy(Merge_p->DeviceName,DeviceName,ST_MAX_DEVICE_NAME);

                        Merge_p->BaseAddress_p     = InitParams_p->BaseAddress_p;
                        Merge_p->DriverPartition_p = InitParams_p->DriverPartition_p;
#if defined(ST_OSLINUX)
                        Merge_p->SWTS_DataAddress_p= TSMergeDataPtr_p;
#endif

                        if (InitParams_p->Mode == STMERGE_BYPASS_GENERIC)
                        {
                            Merge_p->Mode              = 0xFF; /* Mode has no meaning in BYPASS mode */
                        }
                        else
                        {
                            Merge_p->Mode              = InitParams_p->Mode;
                        }

                    }
                    else
                    {
                        Error = ST_ERROR_NO_MEMORY;
                    }
                }

            }
            else
            {
                /* DeviceType is not STMERGE_DEVICE_1 */
                Error = STMERGE_ERROR_DEVICE_NOT_SUPPORTED;
            }
        }
        else
        {
            /* DriverPartition is found NULL */
            Error = ST_ERROR_BAD_PARAMETER;

        }
    }
    else
    {
        /* DeviceName & InitParams passed are invalid */
        Error = ST_ERROR_BAD_PARAMETER;
    }

    if (Error != ST_NO_ERROR)
    {

        /* Signal semaphore before deleting it */
        merger_UnLockDriver();
        semaphore_delete(InitSemaphore_p);
        InitSemaphore_p = (semaphore_t *)NULL;

    }
    else
    {
#if defined(STMERGE_INTERRUPT_SUPPORT)
#if defined(ST_OSLINUX)
        enable_irq(InitParams_p->InterruptNumber);
        if(request_irq((int)InitParams_p->InterruptNumber,
                           MERGER_InterruptHandler,
                           SA_INTERRUPT,
                           "STMERGE",
                           NULL) == 0) /* Success */
        {
            Merge_p ->InterruptNumber = InitParams_p->InterruptNumber;
            Merge_p ->InterruptLevel = InitParams_p->InterruptLevel;
        }
        else
        {
            /* Error installing interrupts */
            Error = ST_ERROR_INTERRUPT_INSTALL;
        }
#else

        if (interrupt_install(InitParams_p->InterruptNumber,
                              InitParams_p->InterruptLevel,
                              (void(*)(void*))MERGER_InterruptHandler,
                              Merge_p ) == 0) /* Success */
        {
            Merge_p ->InterruptNumber = InitParams_p->InterruptNumber;
            Merge_p ->InterruptLevel = InitParams_p->InterruptLevel;
#ifndef ST_OS21
#if defined(STAPI_INTERRUPT_BY_NUMBER)

            /* interrupt_enable call will be redundant after change will be done in STBOOT */
            if (interrupt_enable_number(InitParams_p->InterruptNumber) == 0)
#else
            /* Enable interrupts at the specified level */
            if (interrupt_enable(InitParams_p->InterruptLevel) == 0)
#endif
#else /* ST_OS21 */
            if (interrupt_enable_number(InitParams_p->InterruptNumber) == 0)
#endif
            {
                /* do nothing */
            }
            else
            {
                /* Error interrupt enable */
                Error = ST_ERROR_BAD_PARAMETER;
            }
        }
        else
        {
            /* Error installing interrupts */
            Error = ST_ERROR_INTERRUPT_INSTALL;
        }
#endif
#endif

        merger_UnLockDriver();
    }

    return (Error);

} /* STMERGE_Init */

/****************************************************************************
Name         : STMERGE_Reset

Description  : Resets TSMERGER hw to reset SRAM Read & Write pointers.

Parameters   : None

Return Value : ST_NO_ERROR

****************************************************************************/

ST_ErrorCode_t STMERGE_Reset(void)
{

   merger_SoftReset(Merge_p->BaseAddress_p);
   return(ST_NO_ERROR);
}

/****************************************************************************
Name         : STMERGE_SetParams

Description  : Loads the configuration parameters for the specified object

Parameters   : SourceId: Const Id of the Source to configure
               Source_p: Parameters to load to Source

Return Value : ST_NO_ERROR
               STMERGE_ERROR_NOT_INITIALIZED
               STMERGE_ERROR_BYPASS_MODE
               STMERGE_ERROR_INVALID_ID
               ST_ERROR_BAD_PARAMETER: Source_p is NULL.
               STMERGE_ERROR_INVALID_PRIORITY
               STMERGE_ERROR_INVALID_SYNCLOCK
               STMERGE_ERROR_INVALID_SYNCDROP
               STMERGE_ERROR_INVALID_BYTEALIGN
               STMERGE_ERROR_INVALID_LENGTH
               STMERGE_ERROR_INVALID_TAGGING: Attempt to remove tags from a
                                              non-ALTOUT Source
               STMERGE_ERROR_INVALID_COUNTER_RATE: Rate is too big.
               STMERGE_ERROR_INVALID_COUNTER_VALUE: Value is too big.
               STMERGE_ERROR_INVALID_PACE
               STMERGE_ERROR_INVALID_TRIGGER
               STMERGE_ERROR_INVALID_PARALLEL
               STMERGE_ERROR_FEATURE_NOT_SUPPORTED

****************************************************************************/

ST_ErrorCode_t STMERGE_SetParams(STMERGE_ObjectId_t Id,
                                 STMERGE_ObjectConfig_t *Config_p)

{
    U32 Inst = 0,Config = 0,Reg,PrevVal = 0;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (InitSemaphore_p == NULL)
    {
        return(STMERGE_ERROR_NOT_INITIALIZED);
    }
    else
    {
        merger_LockDriver();
    }

    /* In BYPASS mode no API calls other than Init & Term are permitted */
    if (!IsValidMode(Merge_p->Mode) )
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_BYPASS_MODE);

    }

    /* Check for Valid Stream ID passed by caller */
    if (!IsValidId(Id))
    {
         merger_UnLockDriver();
         return(STMERGE_ERROR_INVALID_ID);
    }

    if (Config_p == NULL)
    {
        merger_UnLockDriver();
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check for valid priority */
    if ((Config_p->Priority < STMERGE_PRIORITY_LOWEST) ||
        (Config_p->Priority > STMERGE_PRIORITY_HIGHEST))
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_INVALID_PRIORITY);
    }

    /* Check for valid SyncLock */
    if (Config_p->SyncLock > MAX_LOCK_DROP )
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_INVALID_SYNCLOCK);
    }

    /* Check for valid SyncDrop */
    if (Config_p->SyncDrop > MAX_LOCK_DROP )
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_INVALID_SYNCDROP);
    }

    /* Check for valid packet length from the pool of supported lengths */
    switch(Config_p->PacketLength)
    {
#if defined(STMERGE_DTV_PACKET)
        case STMERGE_PACKET_LENGTH_DTV:
        case STMERGE_PACKET_LENGTH_DTV_WITH_1B_STUFFING:
        case STMERGE_PACKET_LENGTH_DTV_WITH_2B_STUFFING:
#endif
        case STMERGE_PACKET_LENGTH_DVB:
          break;

        default:
            merger_UnLockDriver();
            return(STMERGE_ERROR_INVALID_LENGTH);
    }

    /* Check whether TSIN3 & TSIN4 can be used together with various
       combination */
    switch(Id)
    {
        case STMERGE_TSIN_3:

            Reg    = RegisterOffset(STMERGE_TSIN_4,STREAM_OFFSET);
            Config = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg);

            /* TSIN4 in serial & TSIN3 in parallel cannot be used */
            if ((Config & SERIAL_NOT_PARALLEL) && (Config_p->u.TSIN.SerialNotParallel == FALSE))
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_FEATURE_NOT_SUPPORTED);
            }

            /* TSIN3 cannot be used if TSIN4 is already used in parallel */
            if ((Config & ~SERIAL_NOT_PARALLEL) != Config)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_FEATURE_NOT_SUPPORTED);
            }

            break;

        case STMERGE_TSIN_4:

            Reg    = RegisterOffset(STMERGE_TSIN_3,STREAM_OFFSET);
            Config = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg);

            /* TSIN3 in serial & TSIN4 in parallel cannot be used */
            if ((Config & SERIAL_NOT_PARALLEL) && (Config_p->u.TSIN.SerialNotParallel == FALSE))
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_FEATURE_NOT_SUPPORTED);
            }

            /* TSIN4 cannot be used if TSIN3 is already used in parallel */
            if ((Config & ~SERIAL_NOT_PARALLEL) != Config)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_FEATURE_NOT_SUPPORTED);
            }

            break;

        default:
            break;

    }

    switch(Id)
    {
        case STMERGE_TSIN_0:
        case STMERGE_TSIN_1:
        case STMERGE_TSIN_2:

#if (STMERGE_NUM_TSIN > 3)
        case STMERGE_TSIN_3:
#endif

#if (STMERGE_NUM_TSIN > 4)
        case STMERGE_TSIN_4:
#endif
            if (Config_p->u.TSIN.SerialNotParallel == FALSE &&
                Config_p->u.TSIN.ByteAlignSOPSymbol == TRUE )
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_BYTEALIGN);

            }

            /* At the moment it is not very clear which of the TSINs are serial Only,
               this patch will be added later */
            /*
            if (Config_p->u.TSIN.SerialNotParallel == FALSE)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_PARALLEL);
            }
            break;
            */

            break;

        case STMERGE_SWTS_0:
#if (STMERGE_NUM_SWTS > 1)
        case STMERGE_SWTS_1:
#endif

#if (STMERGE_NUM_SWTS > 2)
        case STMERGE_SWTS_2:
#endif

#if (STMERGE_NUM_SWTS > 3)
        case STMERGE_SWTS_3:
#endif

            switch(Config_p->u.SWTS.Tagging)
            {
                case STMERGE_TAG_NO_CHANGE:
                case STMERGE_TAG_ADD:
                case STMERGE_TAG_REMOVE:
                case STMERGE_TAG_REPLACE_ID:
                    break;

                default :
                    merger_UnLockDriver();
                    return(STMERGE_ERROR_INVALID_TAGGING);

            }

            if (Config_p->u.SWTS.Counter.Rate >MAX_CNT_RATE)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_COUNTER_RATE);
            }

            if (Config_p->u.SWTS.Counter.Value > MAX_CNT_VALUE)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_COUNTER_VALUE);

            }

            if (Config_p->u.SWTS.Pace > MAX_PACE_VALUE)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_PACE);
            }

#if !defined(ST_5528)
            if (Config_p->u.SWTS.Trigger > MAX_TRIGGER_VALUE)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_TRIGGER);
            }
#endif
            break;

        case STMERGE_ALTOUT_0:

            if (Config_p->u.ALTOUT.Counter.Rate > MAX_CNT_RATE)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_COUNTER_RATE);
            }

            if (Config_p->u.ALTOUT.Counter.Value > MAX_CNT_VALUE)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_COUNTER_VALUE);
            }

            if (Config_p->u.ALTOUT.Pace > MAX_PACE_VALUE)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_PACE);
            }
            break;


        case STMERGE_1394IN_0:
#ifndef STMERGE_NO_1394_IN
            switch(Config_p->u.P1394.Tagging)
            {
                case STMERGE_TAG_NO_CHANGE:
                case STMERGE_TAG_ADD:
                case STMERGE_TAG_REMOVE:
                case STMERGE_TAG_REPLACE_ID:
                    break;

                default :
                    merger_UnLockDriver();
                    return(STMERGE_ERROR_INVALID_TAGGING);

            }

            if (Config_p->u.P1394.Pace  > MAX_PACE_VALUE)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_PACE);
            }
#endif /* STMERGE_NO_1394_IN */
            break;

        case STMERGE_1394OUT_0:
            switch(Config_p->u.P1394.Tagging)
            {
                case STMERGE_TAG_NO_CHANGE:
                case STMERGE_TAG_ADD:
                    break;

                default :
                    merger_UnLockDriver();
                    return(STMERGE_ERROR_INVALID_TAGGING);

            }

            if (Config_p->u.P1394.Pace  > MAX_PACE_VALUE)
            {
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_PACE);
            }

            break;

        default:
            break;

    }

    /* Set Config to 0 if used previously */
    Config = 0;

    switch(Id)
    {
        case STMERGE_TSIN_0:
        case STMERGE_TSIN_1:
        case STMERGE_TSIN_2:

#if (STMERGE_NUM_TSIN > 3)
        case STMERGE_TSIN_3:
#endif

#if (STMERGE_NUM_TSIN > 4)
        case STMERGE_TSIN_4:
#endif
            /* Get stream instance */
            Inst = GetInstanceFromId(STMERGE_TSIN_0,Id);

            Merge_p->TSIN[Inst] = *Config_p;


            /* Get the values of TSM_STREAM_CFG_n register */
            if (Config_p->u.TSIN.SerialNotParallel)
            {
                merger_SetBit(Config, SERIAL_NOT_PARALLEL_BIT);
            }

            if (Config_p->u.TSIN.SyncNotAsync)
            {
                merger_SetBit(Config, SYNC_NOT_ASYNC_BIT);
            }

            if (Config_p->u.TSIN.ByteAlignSOPSymbol)
            {
                merger_SetBit (Config, ALIGN_BYTE_SOP_BIT);
            }

            if (Config_p->u.TSIN.InvertByteClk)
            {
                merger_SetBit(Config, INVERT_BYTECLK_BIT);
            }

            if (Config_p->u.TSIN.ReplaceSOPSymbol)
            {
                merger_SetBit(Config, REPLACE_ID_TAG_BIT);
            }

            Config = Config |((U32)Config_p->Priority << PRIORITY_BIT);

#ifdef STMERGE_NO_TAG

            /* TAGGING OFF at TSMERGE so PTI StreamId should be initialised to
             * STPTI_STREAM_ID_NOTAGS.Care should be taken to run a simple Route
             * test when driver is built with this option.Merging should never be
             * tested with this as the PTI will not be able to distinguish the
             * streams' source without TAG information.
             */
            Config = Config & 0xFFFFFFDF;
#else

            /* TAGGING ON */
            Config = Config | (1 << ADD_TAG_BYTES_BIT);
#endif

#if defined(ST_5525) || defined(ST_5524)
            Config = Config | (Config_p->Counter_num << COUNTER_BIT_27MHZ);
#endif
            /* Write into register after validating user supplied data */
            Reg     = RegisterOffset(Id,STREAM_OFFSET);

            /* The register is read for getting the memory config set during Init */
            PrevVal = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg);

            Config |= (PrevVal & MEMORY_MASK_VALUE);

            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg,Config);

            /* Reset Config for updating other register */
            Config = 0;

            /* Get the values of TSM_STREAM_SYNC_n register */
            Config = Config |(Config_p->SyncLock << SYNC_BIT);

            Config = Config |(Config_p->SyncDrop << DROP_BIT);

            Config = Config |(Config_p->SOPSymbol << SOP_TOKEN_BIT);

            Config = Config |(Config_p->PacketLength << PACKET_LENGTH_BIT);

            /* Write into register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_SYNC_BASE + Reg,Config);

#if defined(ST_7100) || defined(ST_7109)

            /* Reset Config for updating other register */
            Config = 0;

            /* Get the values of TSM_STREAM_CFG2_n register */

            Config = Config |(Config_p->Channel_rst << CHANNEL_RST_BIT);

            Config = Config |(Config_p->Disable_mid_pkt_err << MID_PKT_ERR_BIT);

            Config = Config |(Config_p->Disable_start_pkt_err << START_PKT_ERR_BIT);

            Config = Config |(Config_p->Short_pkt_count << SHORT_PKT_COUNT_BIT);

            /* Write into register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + Reg,Config);
#endif

            break;


        case STMERGE_SWTS_0:

#if (STMERGE_NUM_SWTS > 1)
        case STMERGE_SWTS_1:
#endif

#if (STMERGE_NUM_SWTS > 2)
        case STMERGE_SWTS_2:
#endif

#if (STMERGE_NUM_SWTS > 3)
        case STMERGE_SWTS_3:
#endif
            /* Get stream instance */
            Inst = GetInstanceFromId(STMERGE_SWTS_0,Id);

            Merge_p->SWTS[Inst] = *Config_p;

            /* Get values of TSM_STREAM_CFG_n register */
            Config = Config |((U32)Config_p->Priority << PRIORITY_BIT);

            /* Checking tag value */
            switch(Config_p->u.SWTS.Tagging)
            {
                case STMERGE_TAG_ADD:
                    merger_SetBit(Config, ADD_TAG_BYTES_BIT);
                    break;

                case STMERGE_TAG_NO_CHANGE:
                    merger_ResetBit(Config, ADD_TAG_BYTES_BIT);
                    break;

                case STMERGE_TAG_REPLACE_ID:
                    merger_SetBit(Config, REPLACE_ID_TAG_BIT);
                    break;

                default:
                    break;
            }

            /* Write the value in TSM_STREAM_CFG_n register */
            Reg     = RegisterOffset(Id,STREAM_OFFSET);

            /* The register is read for getting memory config bits set during Init */
            PrevVal = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg);

            Config |= (PrevVal & MEMORY_MASK_VALUE);

            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg,Config);

            /* Reset Val updating other register */
            Config = 0;

            /* Get values of TSM_STREAM_SYNC_n register */
            Config = Config |(Config_p->SyncLock << SYNC_BIT);

            Config = Config |(Config_p->SyncDrop << DROP_BIT);

            Config = Config |(Config_p->SOPSymbol << SOP_TOKEN_BIT);

            Config = Config |(Config_p->PacketLength << PACKET_LENGTH_BIT);

            /* Write values into TSM_STREAM_SYNC_n register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_SYNC_BASE + Reg,Config);

#if defined(ST_7100) || defined(ST_7109)
            /* Reset Config for updating other register */
            Config = 0;

            /* Get the values of TSM_STREAM_CFG2_n register */

            Config = Config |(Config_p->Channel_rst << CHANNEL_RST_BIT);

            Config = Config |(Config_p->Disable_mid_pkt_err << MID_PKT_ERR_BIT);

            Config = Config |(Config_p->Disable_start_pkt_err << START_PKT_ERR_BIT);

            Config = Config |(Config_p->Short_pkt_count << SHORT_PKT_COUNT_BIT);

            /* Write into register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + Reg,Config);
#endif

            /* Reset Val for updating other register */
            Config = 0;

            /* There are 2 methods for pacing the SWTS:
             * 1)Using counters,this method is fully flow controlled(not supported at
             * the moment because PTI4 removes tagged bytes from each packet).
             * 2)Think of the stream as a "live" stream where the data rate is
             * controlled by the speed at which data is read from the SWTS FIFO. In
             * this method, auto_pace is turned off,and a pace value is inserted into
             * the swts_pace register.This will take one byte from the SWTS FIFO every
             * X clock cycles.This is how SWTS was done on chips like the 5514 using
             * TSSUB.
             */

            if (Config_p->u.SWTS.Pace == STMERGE_PACE_AUTO) /* Using Counters */
            {
                /* Get auto pace bit */
                merger_SetBit(Config, SWTS_AUTO_PACE_BIT);

                /* Set auto pace bit in TSM_SWTSn_CFG register */
#if defined(ST_5528)

                /* Write values into TSM_SWTSn_CFG register */
                merger_WriteReg(Merge_p->BaseAddress_p,
                               (TSM_SWTS_CFG_BASE + (Inst * SWTS_OFFSET)),
                                Config);
#else /* 5100,7710,7100 etc */

                /* Get trigger level */
                Config = Config |(Config_p->u.SWTS.Trigger << SWTS_REQ_TRIGGER_BIT);

 		/* Write values into TSM_SWTSn_CFG register */
#if defined(ST_7109)
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_SWTS_CFG_EXPANDED+(Inst * SWTS_OFFSET),Config);
#else
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_SWTS_CFG_EXPANDED,Config);
#endif
#endif

                /* Reset Val for updating other register */
                Config = 0;

                if (Config_p->u.SWTS.Counter.Value == STMERGE_COUNTER_AUTO_LOAD)
                {
                    /* HW Method of initialising counter */

                    /* Get auto load bit */
                    merger_SetBit(Config, CNT_AUTO_BIT);
                }
                else /* SW Method of initialising counter */
                {
                    /* Get counter value of TSM_CNT_n register */
                    Config = Config |(Config_p->u.SWTS.Counter.Value << CNT_VALUE_BIT);

                }

                if (Config_p->u.SWTS.Counter.Rate == STMERGE_COUNTER_NO_INC)
                {
                    Config = Config & CNT_RATE_MASK; /* set counter_inc to '0000' */
                }
                else
                {
                    /* Get counter rate of TSM_CNT_n register */
                    Config = Config |(Config_p->u.SWTS.Counter.Rate << CNT_RATE_BIT);

                }

                /* Write values into TSM_CNT_n register */
                merger_WriteReg(Merge_p->BaseAddress_p,(TSM_CNT_BASE +(Inst * CNT_OFFSET)),
                                Config);

            }
            else /* Using Pace Values */
            {
                /* Get pace of SWTS_CFG_n register */
                Config = Config |(Config_p->u.SWTS.Pace << SWTS_PACE_BIT);

#if defined(ST_5528)

                /* Write values into TSM_SWTSn_CFG register */
                merger_WriteReg(Merge_p->BaseAddress_p,
                               (TSM_SWTS_CFG_BASE + (Inst * SWTS_OFFSET)),
                                Config);
#else /* 5100,7710 etc */

                /* Get trigger level */
                Config = Config |(Config_p->u.SWTS.Trigger << SWTS_REQ_TRIGGER_BIT);

                /* Write values into TSM_SWTSn_CFG register */
#if defined(ST_7109)
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_SWTS_CFG_EXPANDED+(Inst * SWTS_OFFSET),Config);
#else
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_SWTS_CFG_EXPANDED,Config);
#endif
#endif

                /* Write values into TSM_CNT_n register */
                merger_WriteReg(Merge_p->BaseAddress_p,(TSM_CNT_BASE +(Inst * CNT_OFFSET)),0);
            }

            break;

        case STMERGE_ALTOUT_0:

            /* Get stream instance */
            Inst = GetInstanceFromId(STMERGE_ALTOUT_0,Id);

            Merge_p->ALTOUT[Inst] = *Config_p;

            /* Get values of TSM_STREAM_CFG_n register */
            Config = Config |((U32)Config_p->Priority << PRIORITY_BIT);

            /* TAGGING ON */
            Config = Config | (1 << ADD_TAG_BYTES_BIT);

            /* Write the value in TSM_STREAM_CFG_n register */
            Reg     = RegisterOffset(Id,STREAM_OFFSET);

            /* The register is read for getting the memory config set during Init */
            PrevVal = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg);

            Config |= (PrevVal & MEMORY_MASK_VALUE);

#if defined(ST_5525) || defined(ST_5524)
            Config = Config | (Config_p->Counter_num << COUNTER_BIT_27MHZ);
#endif

            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg,Config);

            /* Reset Val now */
            Config = 0;

            /* Get values of TSM_STREAM_SYNC_n register */
            Config = Config |((U32)Config_p->SyncLock << SYNC_BIT);

            Config = Config |((U32)Config_p->SyncDrop << DROP_BIT);

            Config = Config |((U32)Config_p->SOPSymbol << SOP_TOKEN_BIT);

            Config = Config |((U32)Config_p->PacketLength << PACKET_LENGTH_BIT);

            /* Write values into TSM_STREAM_SYNC_n register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_SYNC_BASE + Reg,Config);

#if defined(ST_7100)  || defined(ST_7109)
            /* Reset Config for updating other register */
            Config = 0;

            /* Get the values of TSM_STREAM_CFG2_n register */

            Config = Config |(Config_p->Channel_rst << CHANNEL_RST_BIT);

            Config = Config |(Config_p->Disable_mid_pkt_err << MID_PKT_ERR_BIT);

            Config = Config |(Config_p->Disable_start_pkt_err << START_PKT_ERR_BIT);

            Config = Config |(Config_p->Short_pkt_count << SHORT_PKT_COUNT_BIT);

            /* Write into register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + Reg,Config);
#endif

            /* Reset Config now & Get the values of TSM_ALT_CFG*/
            Config = 0;

            /* Check whether PTI0 or PTI1 is to be used */
            switch(Config_p->u.ALTOUT.PTISource)
            {
                case STMERGE_PTI_0:
                    merger_ResetBit(Config,PTI_ALTOUT_SELECT_BIT);
                    break;

                case STMERGE_PTI_1:
                    merger_SetBit(Config,PTI_ALTOUT_SELECT_BIT);
                    break;

            }

            if (Config_p->u.ALTOUT.Pace == STMERGE_PACE_AUTO) /* Using Counters */
            {
                /* Get auto pace bit */
                merger_SetBit(Config, PTI_ALTOUT_AUTO_PACE_BIT);

                /* Write values into TSM_ALT_CFG register */
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_PTI_ALT_OUT_CFG,Config);

                /* Reset Config now */
                Config = 0;

                if (Config_p->u.ALTOUT.Counter.Value == STMERGE_COUNTER_AUTO_LOAD)
                {
                    /* HW Method of initialising counter */
                    /* Get auto load bit */
                    merger_SetBit(Config, CNT_AUTO_BIT);
                }
                else /* SW Method of initialising counter */
                {
                    /* Get counter value of TSM_CNT_n register */
                    Config = Config |((U32)Config_p->u.ALTOUT.Counter.Value << CNT_VALUE_BIT);

                }

                if (Config_p->u.ALTOUT.Counter.Rate == STMERGE_COUNTER_NO_INC)
                {
                    Config = Config & CNT_RATE_MASK; /* set counter_inc to '0000' */
                }
                else
                {
                    /* Get counter rate of TSM_CNT_n register */
                    Config = Config |((U32)Config_p->u.ALTOUT.Counter.Rate << CNT_RATE_BIT);

                }

                /* Write values into TSM_CNT_n register */
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_ALTOUT_CNT,Config);

            }
            else /* Using Pace Values */
            {
                Config = Config |((U32)Config_p->u.ALTOUT.Pace << PTI_ALTOUT_PACE_BIT);

                /* Write values into TSM_ALT_CFG register */
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_PTI_ALT_OUT_CFG,Config);

                /* Write values into TSM_CNT_n register */
                merger_WriteReg(Merge_p->BaseAddress_p,TSM_ALTOUT_CNT,0);

            }

            break;

        case STMERGE_1394IN_0:
#ifndef  STMERGE_NO_1394_IN
            /* Get stream instance */
            Inst = GetInstanceFromId(STMERGE_1394IN_0,Id);

            Merge_p->P1394[Inst] = *Config_p;

            /* Get values of TSM_STREAM_CFG_n register */
            Config = Config |((U32)Config_p->Priority << PRIORITY_BIT);

            if (Config_p->u.P1394.Tagging)
            {
                merger_ResetBit(Config,P1394_REMOVE_TAGGING_BIT);
            }

            /* Write the value in TSM_STREAM_CFG_n register */
            Reg = RegisterOffset(Id,STREAM_OFFSET);

            /* The register is read for getting the memory config set during Init */
            PrevVal = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg);

            Config |= (PrevVal & MEMORY_MASK_VALUE);

#if defined(ST_5525) || defined(ST_5524)
            Config = Config | (Config_p->Counter_num << COUNTER_BIT_27MHZ);
#endif

            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg,Config);

            /* Reset Val now */
            Config = 0;

            /* Get values of TSM_STREAM_SYNC_n register */
            Config = Config |((U32)Config_p->SyncLock << SYNC_BIT);

            Config = Config |((U32)Config_p->SyncDrop << DROP_BIT);

            Config = Config |((U32)Config_p->SOPSymbol << SOP_TOKEN_BIT);

            Config = Config |((U32)Config_p->PacketLength << PACKET_LENGTH_BIT);

            /* Write values into TSM_STREAM_SYNC_n register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_SYNC_BASE + Reg,Config);

#if defined(ST_7100)  || defined(ST_7109)

            /* Reset Config for updating other register */
            Config = 0;

            /* Get the values of TSM_STREAM_CFG2_n register */

            Config = Config |(Config_p->Channel_rst << CHANNEL_RST_BIT);

            Config = Config |(Config_p->Disable_mid_pkt_err << MID_PKT_ERR_BIT);

            Config = Config |(Config_p->Disable_start_pkt_err << START_PKT_ERR_BIT);

            Config = Config |(Config_p->Short_pkt_count << SHORT_PKT_COUNT_BIT);

            /* Write into register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG2_BASE + Reg,Config);
#endif


            /* Reset Val now for TSM_1394_CFG */
            Config = 0;

            /* Setting In */
            merger_SetBit(Config,P1394_DIR_OUT_NO_IN);

            if (!Config_p->u.P1394.P1394ClkSrc)
            {
                merger_SetBit(Config,P1394_CLKSRC);

                Config = Config| (Config_p->u.P1394.Pace << P1394_PACE_BIT);

            }
            else
            {
                merger_ResetBit(Config,P1394_CLKSRC);
            }

            /* Write values into TSM_P1394_CFG register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_P1394_CFG,Config);

#endif /* STMERGE_NO_1394_IN */

            break;

        case STMERGE_1394OUT_0:

            /* Get stream instance */
            Inst = GetInstanceFromId(STMERGE_1394OUT_0,Id);

            Merge_p->P1394[Inst] = *Config_p;

            /* Reset Val now for TSM_1394_CFG */
            Config = 0;

            /* Setting Out */
            merger_SetBit(Config,P1394_DIR_OUT_NO_IN);

            if (!Config_p->u.P1394.P1394ClkSrc)
            {
                merger_SetBit(Config,P1394_CLKSRC);

                Config = Config| (Config_p->u.P1394.Pace << P1394_PACE_BIT);

            }
            else
            {
                merger_ResetBit(Config,P1394_CLKSRC);
            }

            if (Config_p->u.P1394.Tagging)
            {
                merger_SetBit(Config,P1394_REMOVE_TAGGING_BIT);

            }

            /* Write values into TSM_P1394_CFG register */
            merger_WriteReg(Merge_p->BaseAddress_p,TSM_P1394_CFG,Config);

            break;

    }

    merger_UnLockDriver();
    return(Error);

}/* STMERGE_SetParams */

/****************************************************************************
Name         : STMERGE_SetStatus

Description  : Reset I/P overflow,RAM overflow bits of status register.

Parameters   : SourceId: Const Id of the Source to configure

****************************************************************************/

ST_ErrorCode_t STMERGE_SetStatus(STMERGE_ObjectId_t Id)
{

    U32 StatusReg = 0,Reg = 0;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (InitSemaphore_p == NULL)
    {
        return(STMERGE_ERROR_NOT_INITIALIZED);
    }
    else
    {
        merger_LockDriver();
    }

    /* In BYPASS mode no API calls other than Init & Term are permitted */
    if (!IsValidMode(Merge_p->Mode) )
    {
        merger_UnLockDriver();
        return(STMERGE_ERROR_BYPASS_MODE);

    }

    /* Check for Valid Stream ID passed by caller */
    if (!IsValidId(Id))
    {
         merger_UnLockDriver();
         return(STMERGE_ERROR_INVALID_ID);
    }

    Reg = RegisterOffset(Id,STREAM_OFFSET);

    StatusReg = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_STA_BASE + Reg);

    if (merger_IsBitSet(StatusReg,INPUT_OVERFLOW_BIT))
    {
        /* Reset I/p FIFO overflow bit */
        merger_ResetBit(StatusReg,INPUT_OVERFLOW_BIT);
    }

    if (merger_IsBitSet(StatusReg,RAM_OVERFLOW_BIT))
    {
        /* Reset RAM overflow bit */
        merger_ResetBit(StatusReg,RAM_OVERFLOW_BIT);
    }

    /* Reset I/p Overflow & RAM Overflow bits */
    merger_WriteReg(Merge_p->BaseAddress_p,TSM_STREAM_STA_BASE + Reg , StatusReg);

    /* Signal the semaphore before deleting */
    merger_UnLockDriver();

    return(Error);

}/* STMERGE_SetStatus */

/****************************************************************************
Name         : STMERGE_Term

Description  : Terminates the driver software resource.
               Leaves TSMERGER block in state configured

Parameters   : DeviceName
               Reserved_p: Possible future usage

Return Value : ST_NO_ERROR
               STMERGE_ERROR_NOT_INITIALIZED
               ST_ERROR_UNKNOWN_DEVICE
****************************************************************************/

ST_ErrorCode_t STMERGE_Term(const ST_DeviceName_t DeviceName,
                            const void  *Reserved_p)

{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    partition_t *DriverPartition_p;

    if (InitSemaphore_p == NULL)
    {
        return(STMERGE_ERROR_NOT_INITIALIZED);
    }
    else
    {
        merger_LockDriver();
    }

    if ((DeviceName           == NULL)              ||        /* NULL Name ptr? */
        (DeviceName[0]        == '\0')              ||        /* Device Name undefined? */
        (strlen(DeviceName)   >= ST_MAX_DEVICE_NAME)||        /* string too long? */
        (strcmp(Merge_p->DeviceName,DeviceName)!= 0))         /* checks whether same device is
                                                                 asked for termination or not*/
    {
        merger_UnLockDriver();
        return(ST_ERROR_UNKNOWN_DEVICE);

    }

#if defined(STMERGE_INTERRUPT_SUPPORT)
#if defined(ST_OSLINUX)
    free_irq(Merge_p->InterruptNumber,NULL);
#else
 if(interrupt_uninstall(Merge_p->InterruptNumber,Merge_p->InterruptLevel) != 0)
    {
          /* Unable to remove the handler */
          Error = ST_ERROR_INTERRUPT_UNINSTALL;
    }
#endif
#endif

#if defined(STMERGE_INTERRUPT_SUPPORT)
#if defined(ST_OSLINUX)
    disable_irq(Merge_p->InterruptNumber);
#else
#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    if (interrupt_disable_number(Merge_p->InterruptNumber)!= 0)
#else
    if (interrupt_disable(Merge_p->InterruptLevel) != 0)
#endif
    {
         Error = ST_ERROR_INTERRUPT_UNINSTALL;
    }
#else
 if (interrupt_disable_number(Merge_p->InterruptNumber) != 0)
    {
         /* Unable to disable interrupts */
         Error = ST_ERROR_INTERRUPT_UNINSTALL;
    }
#endif
#endif
#endif  /* ST_OSLINUX */

    /* Reset all the registers */
    merger_ClearAll(Merge_p->BaseAddress_p);

    /* Copy pointer before deallocating memory */
    DriverPartition_p =(partition_t *)Merge_p->DriverPartition_p;

    /* Deallocate memory allocated for control block */
    memory_deallocate(DriverPartition_p,Merge_p);

    /* Signal the semaphore before deleting */
    merger_UnLockDriver();

    /* Now delete semaphore & assign its pointer to NULL */
    semaphore_delete(InitSemaphore_p);
    InitSemaphore_p =(semaphore_t *)NULL;

    return(Error);

} /* STMERGE_Term */

#if defined(ST_OSLINUX)
void  merger_SWTSXInjectWord(STMERGE_ObjectId_t SWTS_Port,U32 Data)
{
     int Timeout=1000;
     U32 TestBit;
     U32 Instance = 0;

     Instance = GetInstanceFromId(STMERGE_SWTS_0,SWTS_Port);
     STSYS_WriteRegDev32LE((void *)TSMergeDataPtr_p + (Instance * 0x20), Data);

     do
     {
         /* Wait until the transfer completes */
         TestBit = merger_ReadReg(Merge_p->BaseAddress_p , 0x0600);

         TestBit = TestBit & 0x80000000;
         Timeout--;

         if( !TestBit )task_delay(10);
    }
    while ( (TestBit == 0) && (Timeout > 0) );

    if( Timeout == 0 )
        printk(KERN_ALERT"Timed out \n");

}
/****************************************************************************
Name         : STMERGE_SWTSXInjectBuf

Description  : Writes a 'Data' buffer of 'Len' bytes to a SWTS 'Port'

Parameters   : Port
               Data : Data buffer
               Len : Length of the buffer

Return Value : ST_NO_ERROR
****************************************************************************/
ST_ErrorCode_t STMERGE_SWTSXInjectBuf(STMERGE_ObjectId_t SWTS_Port, U8 *Buf, U32 BufSize)
{
    /* Cast the data buffer into a 32 bit array address */
    U32* TempBuf=(U32*)Buf;

    /* Check if it is a valid SWTS_Port id */
    if (!IsValidId(SWTS_Port))
    {
        return(STMERGE_ERROR_INVALID_ID);
    }

    /* Checking if the buffer size is a multiple of 4 or not */
    if( BufSize & 0x00000003 ){
	return STMERGE_ERROR_INVALID_BYTEALIGN;
    }

    /* Check if the buffer size if greater than 0 */
    if(BufSize < 0){
        return STMERGE_ERROR_INVALID_LENGTH;
    }

    while( BufSize > 0 ){
        merger_SWTSXInjectWord(SWTS_Port, *TempBuf);
        TempBuf++;
        BufSize-=4;
    }

    return(ST_NO_ERROR);

}

/*****************************************************************************
Function Name : STMERGE_GetSWTSAddress(U32 SWTS_Port, void **Address)
  Description : Return the ioremapped address of specified port
   Parameters : Data to store the address of port
*****************************************************************************/
ST_ErrorCode_t STMERGE_GetSWTSBaseAddress(STMERGE_ObjectId_t SWTS_Port, void **Address)
{
    U32 Instance = 0;

    /* Check if it is a valid SWTS_Port id */
    if (!IsValidId(SWTS_Port))
    {
        return(STMERGE_ERROR_INVALID_ID);
    }

    Instance = GetInstanceFromId(STMERGE_SWTS_0,SWTS_Port);

    *Address = (void*)(TSMergeDataPtr_p + (Instance * 0x20));

    return ST_NO_ERROR;
}

#endif


#if defined(ST_OS20) || defined(ST_OS21)
/*****************************************************************************
Name         : merger_WriteReg()

Description  : Writes the supplied value to the given register.

Parameters   : BaseAddress, Base address of the MERGER block.
               Reg,         Register offset from base
               Value,       The value to write

Return Value : None

*****************************************************************************/

__inline void merger_WriteReg(STSYS_DU32 *Base, U32 Reg, U32 Value)
{
    U32 *Regaddr;

    Regaddr = (U32 *)((U32)Base + Reg);
    *Regaddr = Value;

}/* merger_WriteReg */

/*****************************************************************************
Name        : merger_ReadReg()

Description : Reads the value from the specified register

Parameters  : BaseAddress, Base address of the MERGER block.
              Reg ,        Register offset from base.

Return      : Value        Value read from register.
*****************************************************************************/

__inline int merger_ReadReg(STSYS_DU32 *Base, U32 RegOffset)
{
    U32 *Regaddr;
    U32 Value = 0;

    Regaddr = (U32 *)((U32)Base + RegOffset);
    Value = *Regaddr;

    return(Value);

}/* merger_ReadReg */
#endif

/******************************************************************************
Name         : RegisterOffset

Description  : Gives offset of stream X registers from base address.

Parameters   : StreamID,   Id of stream whose offset is required.
               RegOffset   Offset of stream X.

Return Value : Reg         Register Offset of stream X

*******************************************************************************/

int RegisterOffset(U32 StreamID,U32 RegOffset)
{
    U32 Reg = 0;

    switch(StreamID)
    {
        case STMERGE_ALTOUT_0:
            Reg += RegOffset;
            /* No break */

        case STMERGE_SWTS_3:
#if (STMERGE_NUM_SWTS > 3)
            Reg += RegOffset;
#endif
            /* No break */

        case STMERGE_SWTS_2:
#if (STMERGE_NUM_SWTS > 2)
            Reg += RegOffset;
#endif
            /* No break */

        case STMERGE_SWTS_1:
#if (STMERGE_NUM_SWTS > 1)
            Reg += RegOffset;
#endif
            /* No break */

        case STMERGE_SWTS_0:
            Reg += RegOffset;
            /* No break */

        case STMERGE_1394IN_0:
#ifndef STMERGE_NO_1394_IN /*On 5100,7710,1394IN is through TSIN2 */
            Reg += RegOffset;
#endif
            /* No break */

        case STMERGE_TSIN_4:
#if (STMERGE_NUM_TSIN > 4)
            Reg += RegOffset;
#endif
            /* No break */

        case STMERGE_TSIN_3:
#if (STMERGE_NUM_TSIN > 3)
            Reg += RegOffset;
#endif
            /* No break */

        case STMERGE_TSIN_2:
            Reg += RegOffset;
            /* No break */

        case STMERGE_TSIN_1:
            Reg += RegOffset;
            /* No break */

        case STMERGE_TSIN_0:
            break;

        default:
            break;
    }

    return(Reg);

}/* RegisterOffset */

/******************************************************************************
Name        : merger_ConfigureDefaultSRAM()

Description : SRAM is equally divided among all streams.

Parameters  : BaseAddr

*******************************************************************************/

void merger_ConfigureDefaultSRAM(U32 *BaseAddr)
{

    U32 Val = 0;
    U32 Offset = 0;

    while( Offset < LAST_OFFSET )
    {
        /* Starting address of stream X */
        merger_WriteReg(BaseAddr,TSM_STREAM_CFG_BASE + Offset,Val);
        Val += DEFAULT_SRAM_SIZE_PER_STREAM;

        Offset += STREAM_OFFSET;
        /* Ending address of stream X or starting address of stream X+1 */
        merger_WriteReg(BaseAddr,TSM_STREAM_CFG_BASE + Offset,Val);
    }

}/* merger_ConfigureDefaultSRAM */

/******************************************************************************
Name        : merger_ConfigureUserSRAM()

Description : Configure user defined memory for streams.

Parameters  : BaseAddr, Driver Base Address.
              SRAMMap,  SRAM Map supplied by the application.

Return Value: ST_NO_ERROR

*******************************************************************************/

ST_ErrorCode_t merger_ConfigureUserSRAM(U32 *BaseAddr,U32 *SRAMMap)
{

    U32 Slots = 0,StartAddr = 0;
    U32 *TempAddr;
    U32 MemMap[ALL_STREAMS][2] = {{0,0}};
    U32 loop = 0,count = 0;

    TempAddr = SRAMMap;

    /* Check for Valid Id & Valid Address */
    while( *SRAMMap!= 0 )
    {
        if (!IsValidId(*SRAMMap++))
        {
            return(STMERGE_ERROR_INVALID_ID);
        }

        if (!IsValidAddr(*SRAMMap))
        {
            return(STMERGE_ERROR_MERGE_MEMORY);
        }

        SRAMMap++;
    }

    SRAMMap = TempAddr;

    /* Obtain a 2 dimentional Array same as the packet allocation table(PAT) which
       MemIO block makes consisting of two arrays a src_id_array and addr_array */

    for (loop = 0; loop <  STMERGE_NUM_TSIN ;loop ++)
    {
        if (*SRAMMap == STMERGE_TSIN_0 + loop)
        {
            MemMap[count][0] = *SRAMMap++;
            MemMap[count][1] = *SRAMMap++;
        }
        else
        {
            MemMap[count][0] = STMERGE_TSIN_0 + loop;
            MemMap[count][1] = 0;
        }
        count++;
    }

#ifndef STMERGE_NO_1394_IN
    for (loop = 0 ; loop < STMERGE_NUM_P1394; loop ++)
    {
        if (*SRAMMap == STMERGE_1394IN_0 + loop)
        {
            MemMap[count][0] = *SRAMMap++;
            MemMap[count][1] = *SRAMMap++;
        }
        else
        {
            MemMap[count][0] = STMERGE_1394IN_0 + loop;
            MemMap[count][1] = 0;
        }
        count++;
    }
#endif /* STMERGE_NO_1394_IN */

    for (loop = 0; loop < STMERGE_NUM_SWTS; loop ++)
    {
        if (*SRAMMap == STMERGE_SWTS_0 + loop)
        {
            MemMap[count][0] = *SRAMMap++;
            MemMap[count][1] = *SRAMMap++;
        }
        else
        {
            MemMap[count][0] = STMERGE_SWTS_0 + loop;
            MemMap[count][1] = 0;
        }
        count++;
    }

    for (loop = 0; loop < STMERGE_NUM_ALTOUT; loop ++)
    {
        if (*SRAMMap == STMERGE_ALTOUT_0 + loop)
        {
            MemMap[count][0] = *SRAMMap++;
            MemMap[count][1] = *SRAMMap++;
        }
        else
        {
            MemMap[count][0] = STMERGE_ALTOUT_0 + loop;
            MemMap[count][1] = 0;
        }
        count++;
    }

    /* Write the starting address of each stream in TSM_STREAM_CFG_n register.This
       also defines the start & end address of each stream */

    for (loop = 0; loop < ALL_STREAMS; loop ++ )
    {

        /* Starting address for stream X */
        merger_WriteReg(BaseAddr,(TSM_STREAM_CFG_BASE + loop * STREAM_OFFSET),StartAddr );

        Slots = MemMap[loop][1] /128;

        StartAddr = StartAddr + (Slots << RAM_ALLOC_START_BIT);

        if (StartAddr > MAX_SRAM_ADDRESS )
        {
            return(STMERGE_ERROR_MERGE_MEMORY);
        }

    }

    return(ST_NO_ERROR);

}/* merger_ConfigureUserSRAM */

/****************************************************************************
Name         : merger_IsParamsSet

Description  : Checks whether register are set prior to GetParams & Connect.

Parameters   : Id,   Stream Id whose register is to be read.

Return Value : TRUE or FALSE
*****************************************************************************/

BOOL merger_IsParamsSet(U32 Id)
{
    U32 Reg,IsRegSet = 0;

    Reg = RegisterOffset(Id,STREAM_OFFSET);
    IsRegSet = merger_ReadReg(Merge_p->BaseAddress_p,TSM_STREAM_CFG_BASE + Reg);
    if (IsRegSet == 0)
    {
        return(FALSE);
    }

    return(TRUE);

}/* merger_IsParamsSet */

/****************************************************************************
Name         : IsValidId

Description  : Validate stream Id passed by caller.

Parameters   : StreamId,   Stream Id passed by caller.

Return Value : VALID      TRUE or FALSE.
******************************************************************************/

static BOOL IsValidId(U32 StreamId)
{

    BOOL VALID = TRUE;

    /* Check for Valid Stream ID passed by caller */
    switch(StreamId)
    {
        case STMERGE_TSIN_0:
        case STMERGE_TSIN_1:
        case STMERGE_TSIN_2:

#if (STMERGE_NUM_TSIN > 3)
        case STMERGE_TSIN_3:
#endif

#if (STMERGE_NUM_TSIN > 4)
        case STMERGE_TSIN_4:
#endif

        case STMERGE_SWTS_0:

#if (STMERGE_NUM_SWTS > 1)
        case STMERGE_SWTS_1:
#endif

#if (STMERGE_NUM_SWTS > 2)
        case STMERGE_SWTS_2:
#endif

#if (STMERGE_NUM_SWTS > 3)
        case STMERGE_SWTS_3:
#endif

        case STMERGE_ALTOUT_0:
#ifndef STMERGE_NO_1394_IN
        case STMERGE_1394IN_0:
#endif
        case STMERGE_1394OUT_0:
            break;

        default:
            VALID = FALSE;
            break;

    }

    return(VALID);

}/* IsValidId */

/****************************************************************************
Name         : IsValidAddr

Description  : Validate Address passed by caller.

Parameters   : Address,   Address passed by caller.

Return Value : VALID      TRUE or FALSE.

******************************************************************************/

static BOOL IsValidAddr(U32 Address)
{
    BOOL VALID = TRUE;

    /* Is Address NULL or not a multiple of 128 */
    if ((Address == 0) || ((Address % 128)!= 0))
    {
        VALID = FALSE;
    }

    return(VALID);

}/* IsValidAddr */

/****************************************************************************
Name         : IsValidMode

Description  : Validate Mode

Parameters   : Mode,

Return Value : VALID      TRUE or FALSE.
******************************************************************************/

static BOOL IsValidMode(U32 Mode)
{

    BOOL VALID = TRUE;

    /* Check for Valid Stream ID passed by caller */
    switch(Mode)
    {
        case STMERGE_NORMAL_OPERATION:
            break;

        default:
            VALID = FALSE;
            break;

    }

    return(VALID);

}/* IsValidId */

/******************************************************************************
Function Name : merger_SoftReset()
Description   : Soft reset resets all the flip flops.
Parameters    : None
******************************************************************************/

void merger_SoftReset(U32 *BaseAddress_p)
{
    /* Program TSM_SW_RESET register */
    merger_WriteReg(BaseAddress_p,TSM_SW_RESET,0);

#if defined(ST_OS21) || defined(ST_OS20)
    task_delay(10);
#else
    udelay(10);
#endif

    /* Program TSM_SW_RESET register */
    merger_WriteReg(BaseAddress_p,TSM_SW_RESET,SW_RESET_CODE_VALUE);

}

/******************************************************************************
Function Name : merger_ClearAll()
Description   : Clears all inputs to TSMerger.
Parameters    : None
******************************************************************************/

void merger_ClearAll(U32 *BaseAddress_p)
{
    int loop;
    U32 Reg = 0;

    merger_SoftReset(BaseAddress_p);

    /* All the registers should be cleared at start */
    for (loop = 0; loop < STMERGE_NUM_TSIN; loop ++)
    {
        Reg = RegisterOffset((STMERGE_TSIN_0 + loop), STREAM_OFFSET);

        /* This resets Stream X cfg register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_CFG_BASE + Reg ,0);

        /* This resets Stream X sync register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_SYNC_BASE + Reg ,0);

        /* This resets Stream X status register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_STA_BASE+ Reg ,0);

#if defined(ST_7100) || defined(ST_7109)
        /* This resets Stream X cfg2 register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_CFG2_BASE+ Reg ,0);
#endif

    }

    for(loop = 0; loop < STMERGE_NUM_SWTS; loop ++)
    {
        Reg = RegisterOffset((STMERGE_SWTS_0 + loop), STREAM_OFFSET);

        /* This resets Stream X cfg register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_CFG_BASE + Reg ,0);

        /* This resets Stream X sync register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_SYNC_BASE + Reg ,0);

        /* This resets Stream X status register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_STA_BASE+ Reg ,0);

#if defined(ST_7100) || defined(ST_7109)
        /* This resets Stream X cfg2 register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_CFG2_BASE+ Reg ,0);
#endif

        /* This resets Stream X config resgister */
        /*merger_WriteReg(BaseAddress_p,TSM_SWTS_CFG_BASE+ Reg ,0);*/

    }

    for (loop = 0; loop < STMERGE_NUM_ALTOUT; loop ++)
    {
        Reg = RegisterOffset((STMERGE_ALTOUT_0 + loop), STREAM_OFFSET);

        /* This resets Stream X cfg register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_CFG_BASE + Reg ,0);

        /* This resets Stream X sync register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_SYNC_BASE + Reg ,0);

        /* This resets Stream X status register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_STA_BASE+ Reg ,0);

#if defined(ST_7100) || defined(ST_7109)
        /* This resets Stream X cfg2 register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_CFG2_BASE+ Reg ,0);
#endif

    }

#ifndef STMERGE_NO_1394_IN
    for (loop = 0; loop < STMERGE_NUM_P1394; loop ++)
    {
        Reg = RegisterOffset((STMERGE_1394IN_0 + loop), STREAM_OFFSET);

        /* This resets Stream X cfg register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_CFG_BASE + Reg ,0);

        /* This resets Stream X sync register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_SYNC_BASE + Reg ,0);

        /* This resets Stream X status register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_STA_BASE+ Reg ,0);

#if defined(ST_7100) || defined(ST_7109)
        /* This resets Stream X cfg2 register */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_CFG2_BASE+ Reg ,0);
#endif

    }
#endif /* STMERGE_NO_1394_IN */

    /* This resets SWTS0 CNT0 register */
    merger_WriteReg(BaseAddress_p,TSM_CNT_BASE ,0);

#if  (STMERGE_NUM_SWTS > 1)
    /* This resets SWTS1 CNT1 register */
    merger_WriteReg(BaseAddress_p,TSM_CNT_BASE + CNT_OFFSET ,0);
#endif

#if  (STMERGE_NUM_SWTS > 2)
    /* This resets SWTS2 CNT2 register */
    merger_WriteReg(BaseAddress_p,TSM_CNT_BASE + CNT_OFFSET*2 ,0);
#endif

#if  (STMERGE_NUM_SWTS > 3)
    /* This resets SWTS3 CNT3 register */
    merger_WriteReg(BaseAddress_p,TSM_CNT_BASE + CNT_OFFSET*3 ,0);
#endif

    /* This resets ALTOUT CNT2 register */
    merger_WriteReg(BaseAddress_p,TSM_ALTOUT_CNT ,0);

    /* Reset Destination register */

    /* This resets PTI0 Destination register */
    merger_WriteReg(BaseAddress_p,TSM_PTI0_DEST ,0);

#if (STMERGE_NUM_PTI > 1)

    /* This resets PTI1 Destination register */
    merger_WriteReg(BaseAddress_p,TSM_PTI1_DEST ,0);

#endif

    /* This resets 1394 Destination register */
    merger_WriteReg(BaseAddress_p,TSM_P1394_DEST ,0);

    /* Reset all configuration register */

    /* This resets Altout cfg  register */
    merger_WriteReg(BaseAddress_p,TSM_PTI_ALT_OUT_CFG ,0);

    /* This resets 1394 cfg  register */
    merger_WriteReg(BaseAddress_p,TSM_P1394_CFG ,0);

#if defined(ST_5528)
    /* This resets SWTS0 cfg  register */
    merger_WriteReg(BaseAddress_p,TSM_SWTS_CFG_BASE ,0);
#else /* 7710,5100,5301 */
    /* This resets SWTS0 cfg  register */
    merger_WriteReg(BaseAddress_p,TSM_SWTS_CFG_EXPANDED ,0);
#endif

#if (STMERGE_NUM_SWTS > 1)

    /* This resets SWTS1 cfg  register */
    merger_WriteReg(BaseAddress_p,TSM_SWTS_CFG_EXPANDED + SWTS_OFFSET,0);

#endif

#if (STMERGE_NUM_SWTS > 2)

    /* This resets SWTS2 cfg  register */
    merger_WriteReg(BaseAddress_p,TSM_SWTS_CFG_EXPANDED + SWTS_OFFSET*2,0);

#endif

#if (STMERGE_NUM_SWTS > 3)

    /* This resets SWTS3 cfg  register */
    merger_WriteReg(BaseAddress_p,TSM_SWTS_CFG_EXPANDED + SWTS_OFFSET*3,0);

#endif

    /* Halt the system by setting system_halt bit */
    merger_WriteReg(BaseAddress_p,TSM_SYS_CFG,0);

}

/******************************************************************************
Function Name : merger_Connect()
Description   :
Parameters    : None
******************************************************************************/

ST_ErrorCode_t merger_Connect(U32 *BaseAddress_p,U32 SourceId, U32 DestinationId )
{
    U32 Val = 0,Src = 0,Reg;
    U32 SrcOn = 0;

    if (SourceId == STMERGE_NO_SRC) /* Disconnect specific dest from all src */
    {
        /* Check for Valid Destination ID & reset destination reg */
        switch(DestinationId)
        {
            case STMERGE_PTI_0:
                merger_WriteReg(BaseAddress_p,TSM_PTI0_DEST, RESET);
                break;

            case STMERGE_1394OUT_0:
                merger_WriteReg(BaseAddress_p,TSM_P1394_DEST, RESET);
                break;

#if (STMERGE_NUM_PTI > 1)
            case STMERGE_PTI_1:
                merger_WriteReg(BaseAddress_p,TSM_PTI1_DEST, RESET);
                break;
#endif

            default:
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_DESTINATION_ID);
        }
    }
    else if (DestinationId == STMERGE_NO_DEST) /* Disconnect specific src from all dest */
    {
        /* Check for Valid Src ID & reset src bit from all dest reg */
        switch(SourceId)
        {
            case STMERGE_ALTOUT_0:
                Val++;
                /* No break */

            case STMERGE_SWTS_3:
#if (STMERGE_NUM_SWTS > 3)
                Val++;
                /* No break */
#endif

            case STMERGE_SWTS_2:
#if (STMERGE_NUM_SWTS > 2)
                Val++;
                /* No break */
#endif

            case STMERGE_SWTS_1:
#if (STMERGE_NUM_SWTS > 1)
                Val++;
                /* No break */
#endif

            case STMERGE_SWTS_0:
                Val++;
                /* No break */

            case STMERGE_1394IN_0:
#ifndef STMERGE_NO_1394_IN
                Val++;
                /* No break */
#endif

            case STMERGE_TSIN_4:
#if (STMERGE_NUM_TSIN > 4)
                Val++;
                /* No break */
#endif

            case STMERGE_TSIN_3:
#if (STMERGE_NUM_TSIN > 3)
                Val++;
                /* No break */
#endif

            case STMERGE_TSIN_2:
                Val++;
                /* No break */

            case STMERGE_TSIN_1:
                Val++;
                /* No break */

            case STMERGE_TSIN_0:
                break;

            default:
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_SOURCE_ID);

        }


        /* Read PTI0 Dest register */
        Src = merger_ReadReg(BaseAddress_p,TSM_PTI0_DEST);

        /* Get the src information to set the src bit */
        merger_ResetBit(Src,Val);

        /* Disconnect Src from PTI0 Destination */
        merger_WriteReg(BaseAddress_p,TSM_PTI0_DEST, Src);

#if (STMERGE_NUM_PTI > 1)

        /* Read PTI1 Dest register */
        Src = merger_ReadReg(BaseAddress_p,TSM_PTI1_DEST);

        /* Get the src information to set the src bit */
        merger_ResetBit(Src,Val);

        /* Disconnect Src from PTI1 Destination */
        merger_WriteReg(BaseAddress_p,TSM_PTI1_DEST, Src);
#endif

        /* Read P1394 Dest register */
        Src = merger_ReadReg(BaseAddress_p,TSM_P1394_DEST);

        /* Get the src information to set the src bit */
        merger_ResetBit(Src,Val);

        /* Disconnect Src from P1394 Destination */
        merger_WriteReg(BaseAddress_p,TSM_P1394_DEST, Src);


        /* To reset Stream X connection,turn off gating bit */
        Reg = RegisterOffset(SourceId,STREAM_OFFSET);

        SrcOn = merger_ReadReg(BaseAddress_p,TSM_STREAM_CFG_BASE + Reg);
        merger_ResetBit(SrcOn,STREAM_ON_BIT);

        /* Reset the stream on bit of Src stream */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_CFG_BASE + Reg, SrcOn);

#if 0   /* Lock bit of status register cannot be written: Bug in H/w */

        /* Reset status register as well for resetting Lock bit */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_STA_BASE + Reg, 0);

#endif


    }
    else /* Normal connect procedure */
    {

        /* Check for Valid Stream ID passed by caller */
        if (!IsValidId(SourceId))
        {
            merger_UnLockDriver();
            return(STMERGE_ERROR_INVALID_SOURCE_ID);
        }

        /* Check for Valid Destination ID */
        switch(DestinationId)
        {
            case STMERGE_PTI_0:
            case STMERGE_1394OUT_0:

#if (STMERGE_NUM_PTI > 1)
            case STMERGE_PTI_1:
#endif
                break;

            default:
                merger_UnLockDriver();
                return(STMERGE_ERROR_INVALID_DESTINATION_ID);
        }

        /* Check for legal connection */
        if (SourceId == STMERGE_1394IN_0 && DestinationId == STMERGE_1394OUT_0)
        {
            merger_UnLockDriver();
            return(STMERGE_ERROR_ILLEGAL_CONNECTION);
        }

        /* Connection can only be made if stream parameters are set */
        if (!merger_IsParamsSet(SourceId))
        {
            merger_UnLockDriver();
            return(STMERGE_ERROR_PARAMETERS_NOT_SET);
        }


        /* A fall through switch which finds bit position of specific src
           in  Destination register */
        switch(SourceId)
        {

            case STMERGE_ALTOUT_0:
                Val++;
                /* No break */

            case STMERGE_SWTS_3:
#if (STMERGE_NUM_SWTS > 3)
                Val++;
                /* No break */
#endif

            case STMERGE_SWTS_2:
#if (STMERGE_NUM_SWTS > 2)
                Val++;
                /* No break */
#endif

            case STMERGE_SWTS_1:
#if (STMERGE_NUM_SWTS > 1)
                Val++;
                /* No break */
#endif

            case STMERGE_SWTS_0:
                Val++;
                /* No break */

            case STMERGE_1394IN_0:
#ifndef STMERGE_NO_1394_IN
                Val++;
#endif
                /* No break */

            case STMERGE_TSIN_4:
#if (STMERGE_NUM_TSIN > 4)
                Val++;
                /* No break */
#endif

            case STMERGE_TSIN_3:
#if (STMERGE_NUM_TSIN > 3)
                Val++;
                /* No break */
#endif

            case STMERGE_TSIN_2:
                Val++;
                /* No break */

            case STMERGE_TSIN_1:
                Val++;
                /* No break */

            case STMERGE_TSIN_0:
                break;
        }

        /* Get the src information to set the src bit */
        merger_SetBit(Src,Val);

        /* Write values into TSM_XXXX_DEST register */
        switch(DestinationId)
        {
            case STMERGE_PTI_0:
                Src |= merger_ReadReg(BaseAddress_p,TSM_PTI0_DEST);
                merger_WriteReg(BaseAddress_p,TSM_PTI0_DEST,Src);
                break;

#if (STMERGE_NUM_PTI > 1)
            case STMERGE_PTI_1:
                Src |= merger_ReadReg(BaseAddress_p,TSM_PTI1_DEST);
                merger_WriteReg(BaseAddress_p,TSM_PTI1_DEST,Src);
                break;
#endif

            case STMERGE_1394OUT_0:
                Src |= merger_ReadReg(BaseAddress_p,TSM_P1394_DEST);
                merger_WriteReg(BaseAddress_p,TSM_P1394_DEST,Src);
                break;

        }

        /* Get stream on BIT */
        merger_SetBit(SrcOn,STREAM_ON_BIT);

        /* StreamId register offset */
        Reg = RegisterOffset(SourceId,STREAM_OFFSET);

        SrcOn |= merger_ReadReg(BaseAddress_p,TSM_STREAM_CFG_BASE + Reg);

        /* Set the stream on bit of Src stream */
        merger_WriteReg(BaseAddress_p,TSM_STREAM_CFG_BASE + Reg, SrcOn);
    }

    return(ST_NO_ERROR);

}
/* End of stmerge.c */
