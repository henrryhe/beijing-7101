/*******************************************************************************

File name   : tbx_filt.c

Description : Filtering and redirection feature source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
13 Jan 2000        Created                                          HG
05 Feb 2002        Development.                                     HS
*******************************************************************************/
#if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <string.h>
#include "stddefs.h"
#include "sttbx.h"
#include "tbx_filt.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Table of the input filters. Value with weight more than 1 means no filter */
static STTBX_Device_t InputFilters[STTBX_NB_OF_INPUT_TYPES][STTBX_MAX_NB_OF_MODULES];

/* Table of the output filters for print. Value with weight more than 1 means no filter */
static STTBX_Device_t PrintFilters[STTBX_NB_OF_PRINT_TYPES][STTBX_MAX_NB_OF_MODULES];

/* Table of the output filters for report. Value with weight more than 1 means no filter */
static STTBX_Device_t ReportFilters[STTBX_NB_OF_REPORT_TYPES][STTBX_MAX_NB_OF_MODULES];

/* Table of the redirections. Value with weight more than 1 means no redirection */
static STTBX_Device_t Redirections[STTBX_NB_OF_DEVICES];

/* Weight more than 1 is detected with: (((x) & (x - 1)) != 0) */

/* Table of the redirections. Value with weight more than 1 means no redirection */
static STTBX_ModuleID_t ModuleIndexTable[STTBX_MAX_NB_OF_MODULES];

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Returns n when given 2^(n-1)  (up to n=12: returns 12 if n>12) */
#define DeviceToIndex(x) (((x) <= 2) ? (x) :            \
((((x) /   8) <= 2)?(((x) /   8) +  3):                 \
((((x) /  64) <= 2)?(((x) /  64) +  6):                 \
((((x) / 512) <= 2)?(((x) / 512) +  9):                 \
12                                                      \
))))


/* Private Function prototypes ---------------------------------------------- */

static U8 GetModuleIDIndex(STTBX_ModuleID_t ModuleID);
void sttbx_InitFiltersTables(void);
#ifndef STTBX_FILTER
void sttbx_SetInputFilter(const STTBX_InputFilter_t * const Filter_p, const STTBX_Device_t Device);
void sttbx_SetOutputFilter(const STTBX_OutputFilter_t * const Filter_p, const STTBX_Device_t Device);
void sttbx_SetRedirection(const STTBX_Device_t FromDevice, const STTBX_Device_t ToDevice);
#endif

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : GetModuleIDIndex
Description : give back index in ModuleIndexTable matching given ModuleID,
 *            index to be used in InputFilters, PrintFilters and ReportFilters
 *            tables
Parameters  : ModuleID
Assumptions : STTBX_MAX_NB_OF_MODULES is not reached
Limitations : STTBX_MAX_NB_OF_MODULES
Returns     : Index
*******************************************************************************/
static U8 GetModuleIDIndex(STTBX_ModuleID_t ModuleID)
{
    U8 i=0;

    /* check ModuleID is known, otherwise create a index for it */
    while (    (ModuleIndexTable[i] != 0)
            && (ModuleIndexTable[i] != ModuleID)
            && (i < STTBX_MAX_NB_OF_MODULES)
          )
    {
           i++;
    }
    if (ModuleIndexTable[i] == 0)
    {
        ModuleIndexTable[i] = ModuleID;
    }
    return(i);
} /* end of GetModuleIDIndex() */


/*******************************************************************************
Name        : sttbx_ApplyInputFiltersOnInputDevice
Description : Eventually apply filters on input device
Parameters  : Pointer on an input device to filter
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void sttbx_ApplyInputFiltersOnInputDevice(const STTBX_ModuleID_t ModuleID, STTBX_Device_t *const Device_p)
{
    U32 Tmp;

    /* Apply filters of input device if set */
    Tmp = InputFilters[0][GetModuleIDIndex(ModuleID)];
    if ((Tmp & (Tmp - 1)) == 0)
    {
        /* Valid filter is set: change device */
        *Device_p = Tmp;
    }
}

/*******************************************************************************
Name        : sttbx_ApplyPrintFiltersOnOutputDevice
Description : Eventually apply filters on output device
Parameters  : Pointer on an output device to filter
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void sttbx_ApplyPrintFiltersOnOutputDevice(const STTBX_ModuleID_t ModuleID, STTBX_Device_t * const Device_p)
{
    U32 Tmp;

    /* Apply filers of output device if set */
    Tmp = PrintFilters[0][GetModuleIDIndex(ModuleID)];
    if ((Tmp & (Tmp - 1)) == 0)
    {
        /* Valid filter is set: change device */
        *Device_p = Tmp;
    }
}

/*******************************************************************************
Name        : sttbx_ApplyReportFiltersOnOutputDevice
Description : Eventually apply filters on output device
Parameters  : Pointer on an output device to filter
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void sttbx_ApplyReportFiltersOnOutputDevice(const STTBX_ReportLevel_t Level, const STTBX_ModuleID_t ModuleID, STTBX_Device_t * const Device_p)
{
    U32 Tmp;

    /* Apply filers of output device if set */
    Tmp = ReportFilters[Level][GetModuleIDIndex(ModuleID)];
    if ((Tmp & (Tmp - 1)) == 0)
    {
        /* Valid filter is set: change device */
        *Device_p = Tmp;
    }
}

/*******************************************************************************
Name        : sttbx_ApplyRedirectionsOnDevice
Description : Redirect device
Parameters  : Pointer on a device to redirect
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void sttbx_ApplyRedirectionsOnDevice(STTBX_Device_t * const Device_p)
{
    U32 Tmp;

    /* Apply redirection */
    Tmp  = Redirections[DeviceToIndex(*Device_p)];
    if ((Tmp & (Tmp - 1)) == 0)
    {
        /* Valid redirection  is set: change device */
        *Device_p = Tmp;
    }
}


/*******************************************************************************
Name        : sttbx_InitFiltersTables
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void sttbx_InitFiltersTables(void)
{
    U32 i,j;

    /* do not use memset */
    for (j = 0; j < STTBX_MAX_NB_OF_MODULES; j++)
    {
        for (i = 0; i < STTBX_NB_OF_INPUT_TYPES; i ++)
        {
            InputFilters[i][j] = STTBX_NO_FILTER;
        }
        for (i = 0; i < STTBX_NB_OF_PRINT_TYPES; i ++)
        {
            PrintFilters[i][j] = STTBX_NO_FILTER;
        }
        for (i = 0; i < STTBX_NB_OF_REPORT_TYPES; i ++)
        {
            ReportFilters[i][j] = STTBX_NO_FILTER;
        }
        ModuleIndexTable[j] = 0;
    }
    for (i = 0; i < STTBX_NB_OF_DEVICES; i ++)
    {
        Redirections[i] = STTBX_NO_FILTER;
    }
} /* end of sttbx_InitFiltersTables() */


/*******************************************************************************
Name        : sttbx_SetInputFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void sttbx_SetInputFilter(const STTBX_InputFilter_t * const Filter_p, const STTBX_Device_t Device)
{
    InputFilters[0][GetModuleIDIndex(Filter_p->ModuleID)] = Device;
} /* end of sttbx_SetInputFilter() */

/*******************************************************************************
Name        : sttbx_SetOutputFilter
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void sttbx_SetOutputFilter(const STTBX_OutputFilter_t * const Filter_p, const STTBX_Device_t Device)
{
    U8 Level, Index;

    Index = GetModuleIDIndex(Filter_p->ModuleID);

    if  ((Filter_p->OutputType == STTBX_OUTPUT_PRINT) || (Filter_p->OutputType == STTBX_OUTPUT_ALL))
    {
        PrintFilters[0][Index] = Device;
    }
    if  ((Filter_p->OutputType == STTBX_OUTPUT_REPORT) || (Filter_p->OutputType == STTBX_OUTPUT_ALL))
    {
        for (Level = 0; Level < STTBX_NB_OF_REPORT_LEVEL ; Level ++)
        {
            ReportFilters[Level][Index] = Device;
        }
    }
    switch (Filter_p->OutputType)
    {
        case STTBX_OUTPUT_ALL: /* done yet */
            break;
        case STTBX_OUTPUT_PRINT: /* done yet */
            break;
        case STTBX_OUTPUT_REPORT: /* done yet */
            break;
        case STTBX_OUTPUT_REPORT_FATAL:
            ReportFilters[STTBX_REPORT_LEVEL_FATAL][Index] = Device;
            break;
        case STTBX_OUTPUT_REPORT_ERROR:
            ReportFilters[STTBX_REPORT_LEVEL_ERROR][Index] = Device;
            break;
        case STTBX_OUTPUT_REPORT_WARNING:
            ReportFilters[STTBX_REPORT_LEVEL_WARNING][Index] = Device;
            break;
        case STTBX_OUTPUT_REPORT_ENTER_LEAVE_FN:
            ReportFilters[STTBX_REPORT_LEVEL_ENTER_LEAVE_FN][Index] = Device;
            break;
        case STTBX_OUTPUT_REPORT_INFO:
            ReportFilters[STTBX_REPORT_LEVEL_INFO][Index] = Device;
            break;
        case STTBX_OUTPUT_REPORT_USER1:
            ReportFilters[STTBX_REPORT_LEVEL_USER1][Index] = Device;
            break;
        case STTBX_OUTPUT_REPORT_USER2:
            ReportFilters[STTBX_REPORT_LEVEL_USER2][Index] = Device;
            break;
        default :
            break;
    }
} /* end of sttbx_SetOutputFilter() */

/*******************************************************************************
Name        : sttbx_SetRedirection
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void sttbx_SetRedirection(const STTBX_Device_t FromDevice, const STTBX_Device_t ToDevice)
{
    Redirections[DeviceToIndex(FromDevice)] = ToDevice;
} /* end of sttbx_SetRedirection() */

#endif  /* #ifndef ST_OSLINUX */

/* End of tbx_filt.c */
