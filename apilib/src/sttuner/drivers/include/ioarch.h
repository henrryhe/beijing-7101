
/*---------------------------------------------------------------
File Name: ioarch.h

Description: 

Copyright (C) 1999-2002 STMicroelectronics

History:

   date: 14-June-2001
version: 3.1.0
 author: GJP
comment: new
    
   date: 17-August-2001
version: 3.1.1
 author: GJP
comment: update SubAddr to U16

   date: 4-April-2002
version: 3.4.0
 author: GJP
comment: Add function to support changing default target function and routing.

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_IOARCH_H
#define __STTUNER_IOARCH_H


/* includes --------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */
#include "sti2c.h"
#include "sttuner.h"

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* number of I/O handles (at least one per driver, eg. for 1 instance of sat 4 are required */
#define STTUNER_IOARCH_MAX_HANDLES (STTUNER_MAX_HANDLES * 4)

#ifndef STTUNER_MINIDRIVER
/* IO operation required */
typedef enum
{
    STTUNER_IO_READ,        /* Read only */
    STTUNER_IO_WRITE,       /* Write only */

    STTUNER_IO_SA_READ,     /* Read with prefix subaddress (for devices that support it) */
    STTUNER_IO_SA_WRITE,    /* Write with prefix subaddress */

    STTUNER_IO_SA_READ_NS,  /* Read Sub Add with WriteNoStop Cde */
    STTUNER_IO_SA_WRITE_NS  /* */
   
    
}
STTUNER_IOARCH_Operation_t;
#endif
#ifdef STTUNER_MINIDRIVER
typedef enum
{
    STTUNER_IO_READ,        /* Read only */
    STTUNER_IO_WRITE,       /* Write only */

    STTUNER_IO_SA_READ,     /* Read with prefix subaddress (for devices that support it) */
    STTUNER_IO_SA_WRITE,    /* Write with prefix subaddress */

    STTUNER_IO_SA_READ_NS,  /* Read Sub Add with WriteNoStop Cde */
    STTUNER_IO_SA_WRITE_NS,  /* */
    
    STTUNER_IO_SA_LNB_READ,    
    STTUNER_IO_SA_LNB_WRITE,
       
    STTUNER_IO_LNB_READ,    
    STTUNER_IO_LNB_WRITE  
 
    
}
STTUNER_IOARCH_Operation_t;
#endif

typedef U32 IOARCH_Handle_t;    /* handle returned by IO driver open */
typedef U32 IOARCH_ExtHandle_t; /* handle to Instance of function being called */


/* IO repeater/passthru function format */
typedef ST_ErrorCode_t (*STTUNER_IOARCH_RedirFn_t)(IOARCH_ExtHandle_t hInstance, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);
typedef ST_ErrorCode_t (*STTUNER_IOARCH_RepeaterFn_t)(IOARCH_ExtHandle_t hInstance,BOOL REPEATER_STATUS);

/* hInstance is the Handle to the Instance of the driver being CALLED (not the caller) from a call to its Open() function
   IOHandle is the CALLERS handle to an already open I/O connection */


/*  */
typedef struct
{
    U32 PlaceHolder;
} STTUNER_IOARCH_InitParams_t;


/*  */
typedef struct
{
    U32 PlaceHolder;
} STTUNER_IOARCH_TermParams_t;


/*  */
typedef struct
{
    STTUNER_IORoute_t  IORoute;     /* where IO will go */
    STTUNER_IODriver_t IODriver;    /* which IO to use */
    ST_DeviceName_t    DriverName;  /* driver name (for Init/Open)  */
    U32                Address;     /* and final destination        */

    IOARCH_ExtHandle_t       *hInstance;        /* pointer to instance of driver being called */
    STTUNER_IOARCH_RedirFn_t  TargetFunction;   /* if IODriver is STTUNER_IO_REPEATER or  STTUNER_IO_PASSTHRU 
                                                 then specify function to call instead of STTUNER_IOARCH_ReadWrite */
    STTUNER_IOARCH_RepeaterFn_t RepeaterFn;
    U32 BaudRate;
} STTUNER_IOARCH_OpenParams_t;


/*  */
typedef struct
{
    U32 PlaceHolder;
} STTUNER_IOARCH_CloseParams_t;



/* functions --------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_IOARCH_Init(      STTUNER_IOARCH_InitParams_t *InitParams); /* pull out arch specific info */
ST_ErrorCode_t STTUNER_IOARCH_Term(const STTUNER_IOARCH_TermParams_t *TermParams);

ST_ErrorCode_t STTUNER_IOARCH_Open (IOARCH_Handle_t *Handle, STTUNER_IOARCH_OpenParams_t  *OpenParams);
ST_ErrorCode_t STTUNER_IOARCH_Close(IOARCH_Handle_t  Handle, STTUNER_IOARCH_CloseParams_t *CloseParams);

/* normal read/write to IO */
ST_ErrorCode_t STTUNER_IOARCH_ReadWrite(IOARCH_Handle_t Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* read/write to IO turning off repeater/passthru redirection (passthru only included as a precaution) */
ST_ErrorCode_t STTUNER_IOARCH_ReadWriteNoRep(IOARCH_Handle_t Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* read/write to IO turning off repeater/passthru redirection (passthru only included as a precaution) */
ST_ErrorCode_t STTUNER_IOARCH_ReadWriteNoRep(IOARCH_Handle_t Handle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* NEW as of 3.4.0: change target function associated with handle (only makes sense for repeater and passthrough modes) */
ST_ErrorCode_t STTUNER_IOARCH_ChangeTarget(IOARCH_Handle_t Handle, STTUNER_IOARCH_RedirFn_t TargetFunction, IOARCH_ExtHandle_t *hInstance);

/* NEW as of 3.4.0: change IO routing associated with handle */
ST_ErrorCode_t STTUNER_IOARCH_ChangeRoute(IOARCH_Handle_t Handle, STTUNER_IORoute_t IORoute);

/* Function added for I2c access to avoid multiple level of recursion by doing direct I2C write in this function*/
ST_ErrorCode_t STTUNER_IOARCH_DirectToI2C(IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout );

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_IOARCH_H */

/* End of ioarch.h */
