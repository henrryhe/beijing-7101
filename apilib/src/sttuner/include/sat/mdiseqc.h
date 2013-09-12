
/* ----------------------------------------------------------------------------
File Name: mdiseqc.h

Description: 

Copyright (C) 2004-2005 STMicroelectronics

History:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_MDISEQC_H
#define __STTUNER_SAT_MDISEQC_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */
#include "sttuner.h"
#include "ioarch.h"

    
/* -------------------------------------------------------------------------
    STTUNER_diseqc_dbase_t
   ------------------------------------------------------------------------- */
typedef U32 DISEQC_Handle_t;


typedef struct DISEQC_Capability_s
{ 
   STTUNER_DISEQC_Mode_t                          DISEQC_Mode ;
   STTUNER_DISEQC_Version_t                       DISEQC_VER ;
}DISEQC_Capability_t ;
      

typedef struct
    {
        IOARCH_Handle_t                IOHandle;  /* already open handle for STTUNER_IOARCH_ReadWrite I/O */
        ST_Partition_t                 *MemoryPartition;
        STTUNER_Init_Diseqc_t          Init_Diseqc; /*  DiseqC2 init param for 5100 */
        STTUNER_IOParam_t              DiseqcIO; 	/* configure diseqc driver IO */
    }
    DISEQC_InitParams_t;

/* support type: SCR driver termination parameters */
    typedef struct
    {
        BOOL ForceTerminate;        /* force driver termination (don't wait) */
    }
    DISEQC_TermParams_t;


    /* support type: LNB driver close parameters */
    typedef struct
    {
        STTUNER_Da_Status_t Status;
    }
    DISEQC_CloseParams_t;
    
    typedef struct
    {
        STTUNER_Handle_t TopLevelHandle;
    }
    DISEQC_OpenParams_t;
    


typedef struct STTUNER_DISEQC_dbase_s
    {

        STTUNER_DiSEqCType_t ID;     /* enumerated hardware ID */

        /* ---------- API to SCR ---------- */
        
        ST_ErrorCode_t (*diseqc_Init)(ST_DeviceName_t *DeviceName, DISEQC_InitParams_t *InitParams);
        ST_ErrorCode_t (*diseqc_Term)(ST_DeviceName_t *DeviceName, DISEQC_TermParams_t *TermParams);
        ST_ErrorCode_t (*diseqc_Open) (ST_DeviceName_t *DeviceName, DISEQC_OpenParams_t  *OpenParams, DISEQC_Handle_t  *Handle,DISEQC_Capability_t *Capability);
        ST_ErrorCode_t (*diseqc_Close)(DISEQC_Handle_t  Handle, DISEQC_CloseParams_t *CloseParams);
        ST_ErrorCode_t (*diseqc_SendReceive)(DISEQC_Handle_t,STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
						STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket);
        }STTUNER_diseqc_dbase_t ;
    
typedef struct STTUNER_diseqc_instance_s
{
  ST_ErrorCode_t                  Error;      /* last reported driver error  */
  STTUNER_diseqc_dbase_t  	  *Driver;     /* which contain to function pointer for SCR*/    
  IOARCH_Handle_t                 IOHandle;   /* IO handle */
  DISEQC_Handle_t                 DrvHandle;  /* handle to instance of driver*/
 }
 STTUNER_diseqc_instance_t;

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* STTUNER_SAT_MDISEQC_H */


/* End of mdiseqc.h */
