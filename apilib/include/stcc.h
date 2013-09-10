/*******************************************************************************

File name   : stcc.h

Description : CC module header file

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
26 June 2001       Created                                          BS
02 Jan  2002       Fix DDTS GNBvd10143 writing wrapper around STAPI HSdLM
16 Apr  2002       Fix DDTS GNBvd13032 "STCC driver ID clashes .."  HSdLM
15 May  2002       Add STCC_FORMAT_MODE_EIA608                      M Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STCC_H
#define __STCC_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STCC_DRIVER_ID    136 /* Driver id (API doc number), in decimal */
#define STCC_DRIVER_BASE  (STCC_DRIVER_ID << 16)

enum
{
    STCC_ERROR_DECODER_RUNNING = STCC_DRIVER_BASE,
    STCC_ERROR_DECODER_STOPPED,
    STCC_ERROR_VBI_UNKNOWN,
    STCC_ERROR_VBI_ACCESS,
    STCC_ERROR_EVT_REGISTER,
    STCC_ERROR_EVT_UNREGISTER,
    STCC_ERROR_EVT_SUBSCRIBE,
    STCC_ERROR_TASK_CALL
};

enum
{
    STCC_DATA_TO_BE_PRESENTED_EVT = STCC_DRIVER_BASE
};

typedef enum STCC_FormatMode_e
{
    STCC_FORMAT_MODE_DETECT          = 0x00,
    STCC_FORMAT_MODE_DTVVID21        = 0x01,
    STCC_FORMAT_MODE_EIA708          = 0x02,
    STCC_FORMAT_MODE_DVS157          = 0x04,
    STCC_FORMAT_MODE_UDATA130        = 0x08,
    STCC_FORMAT_MODE_EIA608          = 0x10,
    STCC_FORMAT_MODE_DVB             = 0x20
} STCC_FormatMode_t;

typedef enum STCC_OutputMode_e
{
    STCC_OUTPUT_MODE_NONE            = 0,
    STCC_OUTPUT_MODE_DENC            = 1,
    STCC_OUTPUT_MODE_EVENT           = 2,
    STCC_OUTPUT_MODE_DTV_BMP         = 4,
    STCC_OUTPUT_MODE_UART_DECODED    = 8  /* debug or test mode */
} STCC_OutputMode_t;

/* Exported Types ----------------------------------------------------------- */

typedef struct STCC_Data_s
{
    U8                  Values[2];
    STCC_FormatMode_t   FormatMode;
    U32                 Field;
} STCC_Data_t;

typedef U32 STCC_Handle_t;

typedef struct STCC_InitParams_s
{
    ST_Partition_t *        CPUPartition_p;
    U32                     MaxOpen;
    ST_DeviceName_t         EvtHandlerName;
    ST_DeviceName_t         VBIName;
    ST_DeviceName_t         VIDName;
    ST_DeviceName_t         VTGName;
} STCC_InitParams_t;

typedef struct STCC_OpenParams_s
{
    U32 Dummy;
} STCC_OpenParams_t;

typedef struct STCC_TermParams_s
{
    BOOL ForceTerminate;
} STCC_TermParams_t;


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t STCC_Close(const STCC_Handle_t Handle);
ST_ErrorCode_t STCC_GetFormatMode(const STCC_Handle_t Handle,
                        STCC_FormatMode_t * const FormatMode_p);
ST_ErrorCode_t STCC_GetOutputMode(STCC_Handle_t const Handle,
                        STCC_OutputMode_t * const OutputMode_p);
ST_Revision_t STCC_GetRevision(void);
ST_ErrorCode_t STCC_Init(const ST_DeviceName_t DeviceName,
                        const STCC_InitParams_t * const InitParams_p);
ST_ErrorCode_t STCC_Open(const ST_DeviceName_t DeviceName,
                        const STCC_OpenParams_t * const OpenParams_p,
                        STCC_Handle_t * const Handle_p);
ST_ErrorCode_t STCC_SetFormatMode(const STCC_Handle_t Handle,
                        const STCC_FormatMode_t FormatMode);
ST_ErrorCode_t STCC_SetOutputMode(STCC_Handle_t const Handle,
                        const STCC_OutputMode_t OutputMode);
ST_ErrorCode_t STCC_Term(const ST_DeviceName_t DeviceName,
                        const STCC_TermParams_t * const TermParams_p);
ST_ErrorCode_t STCC_Stop(const STCC_Handle_t Handle);
ST_ErrorCode_t STCC_Start(const STCC_Handle_t Handle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STCC_H */

/* End of stcc.h */
