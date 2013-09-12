/*****************************************************************************

File name   :  stata.h

Description :  header module for atapi interface.

COPYRIGHT (C) ST-Microelectronics 1999-2000.

Date               Modification                          Name
----               ------------                          ----
 02/11/00          Created                               FR
 25 May 01         Rename statapi.h in stata.h           FQ

*****************************************************************************/
/* --- prevents recursive inclusion --------------------------------------- */
#ifndef __STATA_H
#define __STATA_H

/* --- allows C compiling with C++ compiler ------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* --- includes ----------------------------------------------------------- */
#include <partitio.h>
#include "stddefs.h"

/* --- defines ------------------------------------------------------------ */
#define STATA_DRIVER_ID       0x0012
#define STATA_DRIVER_BASE     (STATA_DRIVER_ID << 16)

#define SECTOR_SIZE 512
#define MULTIPLE_SETTING 16
#define ATA_BLOCK_SIZE (SECTOR_SIZE*MULTIPLE_SETTING)

/* --- variables ---------------------------------------------------------- */

/* --- enumerations ------------------------------------------------------- */
/* =======================================================================
   Defines the standard ata error messages
   ======================================================================== */
typedef enum
{
    STATA_FUNCTION_NOT_IMPLEMENTED =  STATA_DRIVER_BASE,
    STATA_HARD_RESET_FAILURE,
    STATA_ERROR_TIMEOUT,
    STATA_DEVICE_FAULT,
    STATA_COMMAND_ABORTED,
    STATA_WRONG_ADDRESS,
    STATA_ERROR_UNDEFINED,
    STATA_DEVICE_FAILED
};

/* =======================================================================
   Defines the main parametes of video decoder
   ======================================================================== */
typedef struct
{
    U32                   InterruptNumber;
    U32                   InterruptLevel;
    partition_t*          CPUPartition_p;
    U32                   CallBackEvents;
    void (*CallBackFunction)(U32 Type, U32 Value);
}   STATA_InitParams_t;

/* --- prototypes of functions -------------------------------------------- */
ST_Revision_t   STATA_GetRevision          ( void );
ST_ErrorCode_t  STATA_Init                 ( STATA_InitParams_t *InitParams_p );
ST_ErrorCode_t  STATA_Term                 ( void );
ST_ErrorCode_t  STATA_PerformDiagnostic    ( void );
ST_ErrorCode_t  STATA_IdentifyDevice       ( U16 *Info );
ST_ErrorCode_t  STATA_WriteMultiple        ( U32 Source     , U32 LBA, U32 BlockNumber);
ST_ErrorCode_t  STATA_ReadMultiple         ( U32 Destination, U32 LBA, U32 BlockNumber, U8 SizeOfABlock);
/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STATA_H */


