/******************************************************************************

File name   : stspi.h

Description : Interface definition for driver code for SPI (SSC) interface

COPYRIGHT (C) STMicroelectronics 2004

******************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __STSPI_H
#define __STSPI_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"         /* Standard definitions */
#include "stpio.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Constants ----------------------------------------------------------------*/

#define STSPI_DRIVER_ID                331
#define STSPI_DRIVER_BASE              (STSPI_DRIVER_ID << 16)

/* External return codes */
#define STSPI_ERROR_BASE               STSPI_DRIVER_BASE
#define STSPI_ERROR_PIO                (STSPI_ERROR_BASE + 1)
#define STSPI_ERROR_NO_FREE_SSC        (STSPI_ERROR_BASE + 2)
#define STSPI_ERROR_EVT_REGISTER       (STSPI_ERROR_BASE + 3)
#define STSPI_ERROR_DATAWIDTH          (STSPI_ERROR_BASE + 4)

/* Others */
#define STSPI_RATE_NORMAL              100000
#define STSPI_RATE_FASTMODE            400000
#define STSPI_TIMEOUT_INFINITY         0x0000


/* Types --------------------------------------------------------------------*/

typedef U32 STSPI_Handle_t;

typedef enum STSPI_MasterSlave_e
{
    STSPI_MASTER,
    STSPI_SLAVE
} STSPI_MasterSlave_t;

enum
{
   STSPI_TRANSMIT_NOTIFY_EVT  = STSPI_DRIVER_BASE,
   STSPI_RECEIVE_NOTIFY_EVT,
   STSPI_TRANSMIT_BYTE_EVT,
   STSPI_RECEIVE_BYTE_EVT,
   STSPI_TRANSMIT_COMPLETE_EVT,
   STSPI_RECEIVE_COMPLETE_EVT
};
typedef struct STSPI_Params_s
{
    BOOL  MSBFirst;
    BOOL  ClkPhase;
    BOOL  Polarity;
    U8    DataWidth;
    U32   BaudRate;
    U32   GlitchWidth;
} STSPI_Params_t;

typedef struct STSPI_InitParams_s
{

    U32                  *BaseAddress;
    STPIO_PIOBit_t       PIOforSCL;
    STPIO_PIOBit_t       PIOforMTSR;
    STPIO_PIOBit_t       PIOforMRST;
    U32                  InterruptNumber;
    U32                  InterruptLevel;
    STSPI_MasterSlave_t  MasterSlave;
    U32                  ClockFrequency;
    U32                  BusAccessTimeout;
    U32                  MaxHandles;
    ST_Partition_t       *DriverPartition;
    STSPI_Params_t       *DefaultParams;

#ifdef STSPI_MASTER_SLAVE
    ST_DeviceName_t      EvtHandlerName;
#endif

} STSPI_InitParams_t;


typedef struct STSPI_OpenParams_s
{
    STPIO_PIOBit_t       PIOforCS;
} STSPI_OpenParams_t;



typedef struct STSPI_TermParams_s
{
    BOOL   ForceTerminate;
} STSPI_TermParams_t;


/* Functions ----------------------------------------------------------------*/

ST_ErrorCode_t STSPI_Close (STSPI_Handle_t           Handle);

ST_ErrorCode_t STSPI_GetParams( STSPI_Handle_t       Handle,
                                STSPI_Params_t       *Params );

ST_Revision_t  STSPI_GetRevision( void );

ST_ErrorCode_t STSPI_Init ( const ST_DeviceName_t    Name,
                            const STSPI_InitParams_t *InitParams);

ST_ErrorCode_t STSPI_Open ( const ST_DeviceName_t    Name,
                            const STSPI_OpenParams_t *OpenParams,
                            STSPI_Handle_t           *Handle);


ST_ErrorCode_t STSPI_Read ( STSPI_Handle_t           Handle,
                            U16                      *Buffer,
                            U32                      MaxLen,
                            U32                      Timeout,
                            U32                      *ActLen);

ST_ErrorCode_t STSPI_SetParams( STSPI_Handle_t       Handle,
                                const STSPI_Params_t *Params );

ST_ErrorCode_t STSPI_Term ( const ST_DeviceName_t    Name,
                            const STSPI_TermParams_t *Params);


ST_ErrorCode_t STSPI_Write ( STSPI_Handle_t          Handle,
                             U16                    *Buffer,
                             U32                     NumberToWrite,
                             U32                     Timeout,
                             U32                     *ActLen);


#ifdef __cplusplus
}
#endif

#endif
