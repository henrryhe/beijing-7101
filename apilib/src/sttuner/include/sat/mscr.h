
/* ----------------------------------------------------------------------------
File Name: mscr.h

Description: 

    SCR driver, instance and API structure.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 6-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: 
    
   date: 17-August-2001
version: 3.1.1
 author: GJP
comment: update SubAddr to U16

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_MSCR_H
#define __STTUNER_SAT_MSCR_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */
#include "sttuner.h"
#include "ioarch.h"

    
/* -------------------------------------------------------------------------
    STTUNER_scr_dbase_t
   ------------------------------------------------------------------------- */
typedef U32 SCR_Handle_t;



typedef struct SCR_Capability_s
{ 
   U8                                          Number_of_SCR ;
   U8                                          Number_of_LNB ;
   STTUNER_SCR_Mode_t                          SCR_Mode ;
   BOOL                                        LegacySupport ;
   U16   SCRLNB_LO_Frequencies[8];
   U16   SCRBPFFrequencies[8];
   STTUNER_ScrType_t          Scr_App_Type;
    STTUNER_SCRBPF_t          SCRBPF   ;        /* SCR indexing***/
   STTUNER_SCRLNB_Index_t    LNBIndex ;       /* LNBIndex***/ 
   BOOL SCREnable; 
}
SCR_Capability_t ;
        
typedef struct  SCR_Config_s
{
STTUNER_SCR_Mode_t            LNBSCR_Mode;   /* Set SCR params manually or autodetection*/
STTUNER_SCRLNB_Index_t              LNBIndex;   /* From which LNB to get RF signal*/
STTUNER_SCRBPF_t                 SCRBPF;    /* RF should route on this SCR*/
}
SCR_Config_t ;

typedef struct
    {
        IOARCH_Handle_t                     IOHandle;  /* already open handle for STTUNER_IOARCH_ReadWrite I/O */
        ST_Partition_t                      *MemoryPartition;
        STTUNER_SCRLNB_Index_t              LNBIndex ;
	STTUNER_SCRBPF_t                    SCRBPF ;
	STTUNER_SCR_Mode_t                  SCR_Mode;
	
	STTUNER_ScrType_t                   Scr_App_Type;
	U8                                  Number_of_SCR ;
        U8                                  Number_of_LNB ;
	U16   SCRLNB_LO_Frequencies[8];  /* In MHz*/
        U16   SCRBPFFrequencies[8];  /* In MHz*/
        BOOL SCREnable; 
    }
    SCR_InitParams_t;

/* support type: SCR driver termination parameters */
    typedef struct
    {
        BOOL ForceTerminate;        /* force driver termination (don't wait) */
    }
    SCR_TermParams_t;


    /* support type: LNB driver close parameters */
    typedef struct
    {
        STTUNER_Da_Status_t Status;
    }
    SCR_CloseParams_t;
    
    typedef struct
    {
        STTUNER_Handle_t TopLevelHandle;
    }
    SCR_OpenParams_t;
    


typedef struct STTUNER_SCR_dbase_s
    {

        STTUNER_ScrType_t ID;     /* enumerated hardware ID */

        /* ---------- API to SCR ---------- */
        
        ST_ErrorCode_t (*scr_Init)(ST_DeviceName_t *DeviceName, SCR_InitParams_t *InitParams);
        ST_ErrorCode_t (*scr_Term)(ST_DeviceName_t *DeviceName, SCR_TermParams_t *TermParams);
        ST_ErrorCode_t (*scr_Open) (ST_DeviceName_t *DeviceName, SCR_OpenParams_t  *OpenParams, SCR_Handle_t  *Handle, DEMOD_Handle_t DemodHandle, SCR_Capability_t *Capability);
        ST_ErrorCode_t (*scr_Close)(SCR_Handle_t  Handle, SCR_CloseParams_t *CloseParams);
        /*ST_ErrorCode_t (*SCR_GetConfig)(SCR_Handle_t Handle, SCR_Config_t *Config);
        ST_ErrorCode_t (*SCR_SetConfig)(SCR_Handle_t Handle, SCR_Config_t *Config);*/
        ST_ErrorCode_t (*scr_IsLocked)(SCR_Handle_t  Handle, DEMOD_Handle_t DemodHandle, BOOL  *IsLocked);
	ST_ErrorCode_t (*scr_SetFrequency)(STTUNER_Handle_t Handle, DEMOD_Handle_t DemodHandle, ST_DeviceName_t *DeviceName, U32  InitialFrequency,  U8 LNBIndex, U8 SCRBPF );
        ST_ErrorCode_t (*scr_Off)(SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle, U8 SCRBPF);

       /* access device specific low-level functions */
        /*ST_ErrorCode_t (*SCR_ioctl)(SCR_Handle_t Handle, U32 Function, void *InParams, void *OutParams );*/
    }
    STTUNER_scr_dbase_t ;
    
    typedef struct STTUNER_scr_instance_s
{
  ST_ErrorCode_t                  Error;      /* last reported driver error  */
  STTUNER_scr_dbase_t  		  *Driver;     /* which contain to function pointer for SCR*/    
  IOARCH_Handle_t             IOHandle;   /* IO handle */
  SCR_Handle_t                    DrvHandle;  /* handle to instance of driver*/
 }
 STTUNER_scr_instance_t;


/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_MSCR_H */


/* End of mscr.h */
