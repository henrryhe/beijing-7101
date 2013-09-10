/*******************************************************************************

File name   : sttbx.h

Description : Public Header of toolbox API

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
10 Mar 1999         Created                                          HG
29 Apr 1999         Added STTBX_InputPollChar                        HG
11 Aug 1999         Implement UART input/output                      HG
20 Aug 1999         Suppressed STTBX_Output(()) function
                    Added print functions STTBX_Print(())
                    and STTBX_DirectPrint(())
                    Added STTBX_PRINT definition                     HG
10 Jan 2000         Added 2 user specific report levels              HG
13 Jan 2000         Added filtering and redirection
                    Added STTBX_FILTER definition                    HG
05 Oct 2001         Make pointer to string to be printed 'const'     HS
28 Jan 2002         DDTS GNBvd10174 "wrapper around STAPI component  HS
 *                  headers".
10 Dec 2002         DDTS GNBvd15739 "Multiline macros should be      HS
 *                  replaced by empty braces"
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STTBX_H
#define __STTBX_H

#if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */


/* The STTBX_MODULE_ID should be defined in the makefile */
/* and set to the module ID ; if not done, zero is taken */
/* and I/O will be done using the default devices.       */
/* It is used by the Toolbox to filter the data.         */
#ifndef STTBX_MODULE_ID
    #define STTBX_MODULE_ID 0
#endif


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#ifndef STTBX_NO_UART
#include "stuart.h"
#endif /* #ifndef STTBX_NO_UART  */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STTBX_DRIVER_ID       28
#define STTBX_DRIVER_BASE     (STTBX_DRIVER_ID << 16)

enum
{              /* max of 32 chars: | */
    STTBX_ERROR_FUNCTION_NOT_IMPLEMENTED = STTBX_DRIVER_BASE
};


/* Max number of modules that the toolbox can recognise. */
#define STTBX_MAX_NB_OF_MODULES 36

/* Defines the supported input/output devices */
typedef enum STTBX_Device_e
{
    /* Value should have a weight (number of bits set) 1 or 0 */
    STTBX_DEVICE_NULL   =  0,   /* No input device */
    STTBX_DEVICE_DCU    =  1,   /* DCU: input from keyboard and output on screen */
    STTBX_DEVICE_UART   =  2,   /* UART: input and output on UART serial port */
    STTBX_DEVICE_UART2  =  4,   /* UART: input and output on UART serial port */
    STTBX_DEVICE_I1284  =  8,   /* I1284: input and output on I1284 parallel port */

    /* Add new devices above here and update value below...*/
    STTBX_NB_OF_DEVICES =  5,   /* Number of STTBX_DEVICE_XXX elements above */
    STTBX_NO_FILTER     =  3    /* Any value with weight (number of bits set) more than 1 */
} STTBX_Device_t;


/* Defines the levels of the report (type of message) */
typedef enum STTBX_ReportLevel_e
{                                   /* Use the given level to indicate: */
    STTBX_REPORT_LEVEL_FATAL = 0,   /* Imminent non recoverable system failure */
    STTBX_REPORT_LEVEL_ERROR,       /* Serious error, though recoverable */
    STTBX_REPORT_LEVEL_WARNING,     /* Warnings of unusual occurences */
    STTBX_REPORT_LEVEL_ENTER_LEAVE_FN, /* Entrance or exit of a function, and parameter display */
    STTBX_REPORT_LEVEL_INFO,        /* Status and other information - normal though */
    STTBX_REPORT_LEVEL_USER1,       /* User specific */
    STTBX_REPORT_LEVEL_USER2,       /* User specific */

    /* Keep as last one (Internal use only) */
    STTBX_NB_OF_REPORT_LEVEL        /* Last element has a value corresponding to the number of elements by default ! */
} STTBX_ReportLevel_t;


/* Defines the various types of inputs that can be filtered */
typedef enum STTBX_InputType_e
{
    STTBX_INPUT_ALL             /* All inputs together */

    /* Keep as last one (Internal use only) */
              /* Last element has a value corresponding to the number of elements by default ! */
} STTBX_InputType_t;


/* Defines the various types of outputs that can be filtered */
typedef enum STTBX_OutputType_e
{
    STTBX_OUTPUT_ALL,           /* All outputs together */
    STTBX_OUTPUT_PRINT,         /* Only STTBX_Print */
    STTBX_OUTPUT_REPORT,        /* Only STTBX_Report (all report levels) */
    STTBX_OUTPUT_REPORT_FATAL,  /* Only STTBX_Report with report level fatal */
    STTBX_OUTPUT_REPORT_ERROR,  /* Only STTBX_Report with report level error */
    STTBX_OUTPUT_REPORT_WARNING, /* Only STTBX_Report with report level warning */
    STTBX_OUTPUT_REPORT_ENTER_LEAVE_FN, /* Only STTBX_Report with report level enter/leave function */
    STTBX_OUTPUT_REPORT_INFO,   /* Only STTBX_Report with report level info */
    STTBX_OUTPUT_REPORT_USER1,  /* Only STTBX_Report with report level user1 */
    STTBX_OUTPUT_REPORT_USER2,  /* Only STTBX_Report with report level user2 */

    /* Keep as last one (Internal use only) */
    STTBX_NB_OF_OUTPUTS         /* Last element has a value corresponding to the number of elements by default ! */
} STTBX_OutputType_t;


/* Exported Types ----------------------------------------------------------- */


typedef U32 STTBX_ModuleID_t;

typedef struct STTBX_Capability_s
{
    U32 NotUsed;
} STTBX_Capability_t;

typedef struct STTBX_InitParams_s
{
    ST_Partition_t *CPUPartition_p;     /* partition for internal memory allocation */
    STTBX_Device_t  SupportedDevices;    /* OR of the devices the toolbox should support for this init */
    STTBX_Device_t  DefaultOutputDevice; /* Default output device */
    STTBX_Device_t  DefaultInputDevice;  /* Default input device */
    ST_DeviceName_t UartDeviceName;     /* Name of an initialised UART driver, to use as the resp. device if
                                           its support is required. If not, this parameter is disregarded. */
    ST_DeviceName_t Uart2DeviceName;    /* Name of an initialised UART driver, to use as the resp. device if
                                           its support is required. If not, this parameter is disregarded. */
    ST_DeviceName_t I1284DeviceName;    /* Name of an initialised I1284 driver, to use as the resp. device if
                                           its support is required. If not, this parameter is disregarded. */
} STTBX_InitParams_t;

typedef struct STTBX_TermParams_s
{
    U32 NotUsed;
} STTBX_TermParams_t;


typedef struct STTBX_InputFilter_s
{
    STTBX_ModuleID_t    ModuleID;
    STTBX_InputType_t   InputType;
} STTBX_InputFilter_t;

typedef struct STTBX_OutputFilter_s
{
    STTBX_ModuleID_t    ModuleID;
    STTBX_OutputType_t  OutputType;
} STTBX_OutputFilter_t;

typedef struct STTBX_BuffersConfigParams_s
{
    BOOL Enable;
    BOOL KeepOldestWhenFull;
} STTBX_BuffersConfigParams_t;


/* Exported Variables ------------------------------------------------------- */

#define STTBX_REPORT
#define STTBX_PRINT

/* Functions and variables NOT TO BE USED by users of the API --------------- */
#ifdef STTBX_INPUT
    BOOL sttbx_InputPollChar(const STTBX_ModuleID_t ModuleID, char * const Value_p);
    void sttbx_InputChar(const STTBX_ModuleID_t ModuleID, char * const Value_p);
    U16 sttbx_InputStr(const STTBX_ModuleID_t ModuleID, char * const Buffer_p, const U16 BufferSize);
#endif /* STTBX_INPUT */
#ifdef STTBX_REPORT
    extern semaphore_t * sttbx_LockReport_p;
    void sttbx_ReportFileLine(const STTBX_ModuleID_t ModuleID, const char * const FileName_p, const U32 Line);
    void sttbx_ReportMessage(const STTBX_ReportLevel_t ReportLevel, const char *const Format_p, ...);
#endif /* STTBX_REPORT */
#ifdef STTBX_PRINT
    extern semaphore_t * sttbx_GlobalPrintIdMutex_p;
    extern STTBX_ModuleID_t sttbx_GlobalPrintID;
    void sttbx_Print(const char *const Format_p, ...);
    void sttbx_DirectPrint(const char *const Format_p, ...);
#endif /* STTBX_PRINT */
#ifdef STTBX_FILTER
    void sttbx_SetRedirection(const STTBX_Device_t FromDevice, const STTBX_Device_t ToDevice);
    void sttbx_SetInputFilter(const STTBX_InputFilter_t * const Filter_p, const STTBX_Device_t Device);
    void sttbx_SetOutputFilter(const STTBX_OutputFilter_t * const Filter_p, const STTBX_Device_t Device);
#endif /* STTBX_FILTER */


/* Exported Macros ---------------------------------------------------------- */

#ifdef STTBX_INPUT
    #define STTBX_InputPollChar(x) sttbx_InputPollChar(STTBX_MODULE_ID, (x))
    #define STTBX_InputChar(x) sttbx_InputChar(STTBX_MODULE_ID, (x))
    #define STTBX_InputStr(x, y) sttbx_InputStr(STTBX_MODULE_ID, (x), (y))
#else /* not STTBX_INPUT */
    /* STTBX_InputPollChar should return FALSE when undefined but
       returns 0 instead to avoid a warning when not using the value */
    #define STTBX_InputPollChar(x) 0
    #define STTBX_InputChar(x) { }
    #define STTBX_InputStr(x, y) 0
#endif /* not STTBX_INPUT */


#ifdef STTBX_REPORT
/*  #ifdef ST_OSWINCE 
    ST-CD: added STTBX_Report support 
  #define STTBX_Report(x)			_WCE_Report_Level x 
 #else*/
    #define STTBX_Report(x)                                                 \
            {                                                               \
              semaphore_wait(sttbx_LockReport_p);                            \
              /* sttbx_ReportFileLine has to be called here
                otherwise FILE and LINE would always be the same */         \
              sttbx_ReportFileLine(STTBX_MODULE_ID, __FILE__, __LINE__);    \
              sttbx_ReportMessage x;                                        \
              semaphore_signal(sttbx_LockReport_p);                          \
            }
/*  #endif   */         
#else /* not STTBX_REPORT */
    #define STTBX_Report(x) { }
#endif /* not STTBX_REPORT */


#ifdef STTBX_PRINT
/*  #ifdef ST_OSWINCE
     ST-PC: added STTBX_Print support 
    #define STTBX_Print(x)			_WCE_Printf x
  #else */
    #define STTBX_Print(x)                                                  \
            {                                                               \
              semaphore_wait(sttbx_GlobalPrintIdMutex_p);                    \
              sttbx_GlobalPrintID = STTBX_MODULE_ID;                        \
              sttbx_Print x;                                                \
              semaphore_signal(sttbx_GlobalPrintIdMutex_p);                  \
            }
    #define STTBX_DirectPrint(x) sttbx_DirectPrint x
/*  #endif  */  
#else /* not STTBX_PRINT */
    #define STTBX_Print(x) { }
    #define STTBX_DirectPrint(x) { }
#endif /* not STTBX_PRINT */


#ifdef STTBX_FILTER
    #define STTBX_SetRedirection(x, y) sttbx_SetRedirection((x), (y))
    /* Set device to STTBX_NO_FILTER disables the filter */
    #define STTBX_SetInputFilter(x, y) sttbx_SetInputFilter((x), (y))
    /* Set device to STTBX_NO_FILTER disables the filter */
    #define STTBX_SetOutputFilter(x, y) sttbx_SetOutputFilter((x), (y))
#else /* not STTBX_FILTER */
    #define STTBX_SetRedirection(x, y) { }
    #define STTBX_SetInputFilter(x, y) { }
    #define STTBX_SetOutputFilter(x, y) { }
#endif /* not STTBX_FILTER */

/* Exported Functions ------------------------------------------------------- */

ST_Revision_t STTBX_GetRevision(void);
ST_ErrorCode_t STTBX_GetCapability(const ST_DeviceName_t DeviceName, STTBX_Capability_t * const Capability_p);
ST_ErrorCode_t STTBX_Init(const ST_DeviceName_t DeviceName, const STTBX_InitParams_t * const InitParams_p);
ST_ErrorCode_t STTBX_Term(const ST_DeviceName_t DeviceName, const STTBX_TermParams_t * const TermParams_p);

ST_ErrorCode_t STTBX_SetBuffersConfig(const STTBX_BuffersConfigParams_t * const BuffersConfigParam_p);


void STTBX_WaitMicroseconds(U32 Microseconds);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif  /* #ifndef ST_OSLINUX */
#endif /* #ifndef __STTBX_H */

/* End of sttbx.h */

