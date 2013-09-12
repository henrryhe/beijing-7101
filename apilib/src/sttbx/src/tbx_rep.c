/*******************************************************************************

File name   : tbx_rep.c

Description : Source of Report and Print feature of toolbox API

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
10 Mar 1999         Created                                          HG
11 Aug 1999         Change Semaphores structure (now one
                    semaphore for each I/O device)                   HG
20 Aug 1999         Suppressed STTBX_REPORT definition
                    Added print functions STTBX_Print(())
                    and STTBX_DirectPrint(())                        HG
10 Jan 2000         Added 2 user specific report levels              HG
05 Oct 2001         Decrease stack use                               HS
16 Nov 2001         Fix DDTS GNBvd09866                              HS
29 Jan 2002         Fix DDTS GNBvd10496 "STTBX_Print does not work   HS
 *                  from multiple threads"
*******************************************************************************/

 #if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */

/* Includes ----------------------------------------------------------------- */

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#ifdef ST_OS20
#include <debug.h>
#endif /* ST_OS20 */
#if defined(ST_OS21) && !defined(ST_OSWINCE)
#include <os21debug.h>
#endif /* ST_OS21 && !ST_OSWINCE*/
#include "stddefs.h"
#include "stlite.h"

#undef STTBX_REPORT
#undef STTBX_PRINT
#include "sttbx.h"

#include "tbx_out.h"
#include "tbx_init.h"
#include "tbx_filt.h"

/* Private Constants -------------------------------------------------------- */

#define REPORT_FILE_NAME_LENGTH   11  /* length of the file name in the report */
#define REPORT_LINE_NUMBER_LENGTH 4   /* length of the line number in the report */
#define REPORT_PROMPT_LENGTH      10  /* length of the prompt corresponding to the report level */

/* Private Types ------------------------------------------------------------ */

/* Private Variables (static)------------------------------------------------ */

static char             GlobalReportFileName_p[REPORT_FILE_NAME_LENGTH];
static STTBX_ModuleID_t GlobalReportID;
static U32              GlobalReportLine;

/* Global Variables --------------------------------------------------------- */
semaphore_t *sttbx_LockPrintTable_p;
STTBX_ModuleID_t sttbx_GlobalPrintID;

/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
void sttbx_ReportFileLine(const STTBX_ModuleID_t ModuleID, const char * const FileName_p, const U32 Line);
void sttbx_ReportMessage(const STTBX_ReportLevel_t ReportLevel, const char *const Format_p, ...);
void sttbx_Print(const char *const Format_p, ...);
void sttbx_DirectPrint(const char *const Format_p, ...);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : sttbx_NewPrintSlot
Description : give new memory slot for print
Parameters  : PrintTableIndex_p : pointer to be filled by index in table reserved
 *                                for new print
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_NewPrintSlot(U8 * const PrintTableIndex_p)
{
    sttbx_PrintTable_t **Tmp_p = sttbx_PrintTable_p; /* make it not NULL */
    sttbx_PrintTable_t **OldPrintTable_p;
    U8 SlotIndex=0;

    /* get next free slot */
    semaphore_wait(sttbx_LockPrintTable_p);
    for (SlotIndex=0; SlotIndex < sttbx_PrintTableSize; SlotIndex ++)
    {
        if (sttbx_PrintTable_p[SlotIndex]->IsSlotFree)
        {
            sttbx_PrintTable_p[SlotIndex]->IsSlotFree = FALSE;
            break;
        }
    }

    if (SlotIndex >= STTBX_PRINT_TABLE_INIT_SIZE ) /* sttbx_PrintTableSize is always >= STTBX_PRINT_TABLE_INIT_SIZE */
    {
        if (SlotIndex == sttbx_PrintTableSize) /* failed to found a free slot : extend table */
        {
            /* re-allocate with greater size print table and copy content to new @ */

            Tmp_p = (sttbx_PrintTable_t **)memory_allocate(sttbx_GlobalCPUPartition_p, sizeof(sttbx_PrintTable_t*)*(sttbx_PrintTableSize + 1));
            if (Tmp_p != NULL)
            {
                /* copy content from sttbx_PrintTable_p to Tmp_p to keep sttbx_PrintTableSize old values */
                memcpy(Tmp_p, sttbx_PrintTable_p, sizeof(sttbx_PrintTable_t*)*(sttbx_PrintTableSize));
                OldPrintTable_p = sttbx_PrintTable_p;
                sttbx_PrintTable_p = Tmp_p;
                sttbx_PrintTableSize ++;
                sttbx_PrintTable_p[SlotIndex] = (sttbx_PrintTable_t *)memory_allocate(sttbx_GlobalCPUPartition_p, sizeof(sttbx_PrintTable_t));
                memory_deallocate(sttbx_GlobalCPUPartition_p, OldPrintTable_p);
            }
            else
            {
                /* allocation failed : use last slot, and crunch previous print ...*/
                SlotIndex = sttbx_PrintTableSize - 1;
                if (sttbx_GlobalCurrentOutputDevice == STTBX_DEVICE_DCU )
                {
                    debugmessage("STTBX failed to allocate new memory for print ! Messages can be lost. \n");
                }
            }
        }
        if (Tmp_p != NULL)
        {
            sttbx_PrintTable_p[SlotIndex]->IsSlotFree = FALSE;
            sttbx_PrintTable_p[SlotIndex]->String = (char *)memory_allocate(sttbx_GlobalCPUPartition_p, STTBX_MAX_LINE_SIZE);
            sttbx_PrintTable_p[SlotIndex]->String[0] = '\0';
        }
    }
    semaphore_signal(sttbx_LockPrintTable_p);
    *PrintTableIndex_p = SlotIndex;
}

/*******************************************************************************
Name        : sttbx_FreePrintSlot
Description : free memory slot for print
Parameters  : PrintTableIndex : index in table of slot to free
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_FreePrintSlot(const U8 PrintTableIndex)
{
    sttbx_PrintTable_p[PrintTableIndex]->String[0]='\0';
    if (PrintTableIndex >= STTBX_PRINT_TABLE_INIT_SIZE )
    {
        memory_deallocate(sttbx_GlobalCPUPartition_p, sttbx_PrintTable_p[PrintTableIndex]->String);
    }
    sttbx_PrintTable_p[PrintTableIndex]->IsSlotFree = TRUE;
}

/*******************************************************************************
Name        : sttbx_ReportFileLine
Description : Output the file and line passed as parameter to the current output device
Parameters  : file name and line number
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_ReportFileLine(const STTBX_ModuleID_t ModuleID, const char * const FileName_p, const U32 Line)
{
    U32 FileNameLength;

    /* Save value for use in sttbx_ReportMessage */
    GlobalReportID = ModuleID;
    GlobalReportLine = Line;

    /* Limit the file name to REPORT_FILE_NAME_LENGTH characters. If longer (for instance when containing
       path information), cut to the left to keep the file name */
    FileNameLength = (U32) strlen(FileName_p);
    if (FileNameLength >= REPORT_FILE_NAME_LENGTH)
    {
        strncpy(GlobalReportFileName_p, FileName_p + FileNameLength - REPORT_FILE_NAME_LENGTH, REPORT_FILE_NAME_LENGTH);
    }
    else
    {
        strncpy(GlobalReportFileName_p + REPORT_FILE_NAME_LENGTH - FileNameLength, FileName_p, FileNameLength);
        /* Fill begining of string with spaces */
        while (FileNameLength < REPORT_FILE_NAME_LENGTH)
        {
            GlobalReportFileName_p[REPORT_FILE_NAME_LENGTH - FileNameLength - 1] = ' ';
            FileNameLength++;
        }
    }

    /* Data will be output in sttbx_ReportMessage() */
}


/*******************************************************************************
Name        : sttbx_ReportMessage
Description : Output the specified message to the current output device
Parameters  : report level and <printf> like format and arguments
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_ReportMessage(const STTBX_ReportLevel_t ReportLevel, const char *const Format_p, ...)
{
    va_list Argument;
    char *CurString_p;
    STTBX_Device_t Device;
    U8 PrintTableIndex;

/* This is a temporary solution to remove warnings, it will be reviewed */
    const char*  line_p = (const char*)"%s\r\n";
    void*        var_p  = (void*) &line_p;

    /* Determine output device */
    Device = sttbx_GlobalCurrentOutputDevice;
    sttbx_ApplyReportFiltersOnOutputDevice(ReportLevel, GlobalReportID, &Device);
    sttbx_ApplyRedirectionsOnDevice(&Device);

    /* Exit if no output device is selected */
    if (Device == STTBX_DEVICE_NULL)
    {
        return;
    }

    /* take new index */
    sttbx_NewPrintSlot(&PrintTableIndex);

    /* Display file name (cut) and line on the current output device for STTBX_Report */
    CurString_p = sttbx_PrintTable_p[PrintTableIndex]->String;
    sprintf(CurString_p, "%*.*s, %0*lu ", REPORT_FILE_NAME_LENGTH,
            REPORT_FILE_NAME_LENGTH, GlobalReportFileName_p, REPORT_LINE_NUMBER_LENGTH, (long int) GlobalReportLine);

    /* Display formated prompt corresponding to the report level */
    /* Be careful: those prompts should not be longer than REPORT_PROMPT_LENGTH,
       they would be truncated otherwise. */
    CurString_p += REPORT_FILE_NAME_LENGTH + REPORT_LINE_NUMBER_LENGTH + 3;
    switch(ReportLevel)
    {
        case STTBX_REPORT_LEVEL_FATAL :
            strcpy(CurString_p, "   Fatal> ");
            break;

        case STTBX_REPORT_LEVEL_ERROR :
            strcpy(CurString_p, "   Error> ");
            break;

        case STTBX_REPORT_LEVEL_WARNING :
            strcpy(CurString_p, " Warning> ");
            break;

        case STTBX_REPORT_LEVEL_ENTER_LEAVE_FN :
            strcpy(CurString_p, "InOut Fn> ");
            break;

        case STTBX_REPORT_LEVEL_INFO :
            strcpy(CurString_p, "    Info> ");
            break;

        case STTBX_REPORT_LEVEL_USER1 :
            strcpy(CurString_p, "   User1> ");
            break;

        case STTBX_REPORT_LEVEL_USER2 :
            strcpy(CurString_p, "   User2> ");
            break;

        default:
            /* Unknown Report Level: display ???? */
            strcpy(CurString_p, " ?????? > ");
            break;
    }

    /* Translate arguments into a character buffer */
    va_start(Argument, Format_p);
    /* REPORT_PROMPT_LENGTH is the length of the prompt corresponding to the
       report level: start format characters after the prompt */
    CurString_p += REPORT_PROMPT_LENGTH;
    vsprintf(CurString_p, Format_p, Argument);
    va_end(Argument);

    /* Display this buffer on the determined device */
    sttbx_Output(Device,(char* const) (*(char** const*)var_p), sttbx_PrintTable_p[PrintTableIndex]->String);


    /* Free sttbx_PrintTable_p index used before leaving : */
    sttbx_FreePrintSlot(PrintTableIndex);
}


/*******************************************************************************
Name        : sttbx_Print
Description : Output the specified message to the appropriate device (care about
              filters and redirections)
Parameters  : <printf> like format and arguments
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_Print(const char *const Format_p, ...)
{
    STTBX_ModuleID_t ModuleID = sttbx_GlobalPrintID;
    va_list Argument;
    STTBX_Device_t Device;
    U8 PrintTableIndex;
/*#ifdef IRDETOCA	
    extern semaphore_t *gpsema_Uart;
#endif*/
    /* Determine output device */
    Device = sttbx_GlobalCurrentOutputDevice;
/*#ifdef IRDETOCA	
    semaphore_wait ( gpsema_Uart );
#endif*/
    sttbx_ApplyPrintFiltersOnOutputDevice(ModuleID, &Device);
    sttbx_ApplyRedirectionsOnDevice(&Device);

    /* Exit if no output device is selected */
    if (Device == STTBX_DEVICE_NULL)
    {
/*#ifdef IRDETOCA
	semaphore_signal ( gpsema_Uart );
#endif*/	
        return;
    }

    /* take new index */
    sttbx_NewPrintSlot(&PrintTableIndex);

    /* Translate arguments into a character buffer */
    va_start(Argument, Format_p);
    vsprintf(sttbx_PrintTable_p[PrintTableIndex]->String, Format_p, Argument);
    va_end(Argument);

    /* Exit if no character to print */
    if (sttbx_PrintTable_p[PrintTableIndex]->String[0] != '\0')
    {
        /* Output buffer on the determined device*/
        sttbx_StringOutput(Device, sttbx_PrintTable_p[PrintTableIndex]->String);
    }

    /* Free sttbx_PrintTable_p index used before leaving : */
    sttbx_FreePrintSlot(PrintTableIndex);
/*#ifdef IRDETOCA
	semaphore_signal ( gpsema_Uart );
#endif*/	
}



/*******************************************************************************
Name        : sttbx_DirectPrint
Description : Output the specified message to the appropriate device (do not care
              about filters nor about redirections)
Parameters  : <printf> like format and arguments
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
void sttbx_DirectPrint(const char *const Format_p, ...)
{
    va_list Argument;
    STTBX_Device_t Device;
    U8 PrintTableIndex;

    /* Determine output device */
    Device = sttbx_GlobalCurrentOutputDevice;
    /* No filter */
    /* No redirection */

    /* Exit if no output device is selected */
    if (Device == STTBX_DEVICE_NULL)
    {
        return;
    }

    /* take new index */
    sttbx_NewPrintSlot(&PrintTableIndex);

    /* Translate arguments into a character buffer */
    va_start(Argument, Format_p);
    vsprintf(sttbx_PrintTable_p[PrintTableIndex]->String, Format_p, Argument);
    va_end(Argument);

    /* Exit if no character to print */
    if (sttbx_PrintTable_p[PrintTableIndex]->String[0] != '\0')
    {
        /* Output buffer on the determined device*/
        sttbx_StringOutput(Device, sttbx_PrintTable_p[PrintTableIndex]->String);
    }

    /* Free sttbx_PrintTable_p index used before leaving : */
    sttbx_FreePrintSlot(PrintTableIndex);
}



#endif  /* #ifndef ST_OSLINUX */
/* End of tbx_rep.c */


