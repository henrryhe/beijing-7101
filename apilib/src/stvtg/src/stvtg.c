/************************************************************************
 File  Name   : stvtg.c

Description : VTG driver module standard API functions source file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
04 Mar 1999        Created                                          PL
 ...
30 Aou 2000        STi7015/7020 support added                       HSdLM
16 Nov 2000        Correct returned capability                      HSdLM
 *                 Improve error tracking
18 Jan 2001        New function STVTG_GetVtgId and                  HSdLM
 *                 new init parameters.
21 Mar 2001        Maxopen 1 -> 3, get DencCellId, add              HSdLM
 *                 STDENC_Close in Term() (DDTS 6803),
 *                 set CKG, C656 and DHDO base addresses
28 Jun 2001        Handle new DeviceType STVTG_DEVICE_TYPE_VFE      HSdLM
 *                 Set prefix stvtg_ on exported symbols
 *                 Make Unit Validity checking macro global
04 Sep 2001        Fix Init issue with type VFE : no                HSdLM
 *                 InterruptEventName needed. Remove useless
 *                 routine parameters (Open/Term).
08 Oct 2001        Use compilation flags related to device type     HSdLM
09 Oct 2001        Use STINTMR for VFE                              HSdLM
16 Apr 2002        New DeviceType for STi7020                       HSdLM
16 Apr 2003        New DeviceType for STi5528                       HSdLM
21 Jan 2004        Add new CLK (VIDPIX_2XCLK) on 5528               AC
13 Sep 2004        Add ST40/OS21 support                            MH
************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
/* Include system files only if not in Kernel mode */
#include <stdlib.h>
#include <string.h>
#endif

#ifdef  ST_OSLINUX
#include "stevt.h"
#include "vfe_reg.h"
#else
#include "stlite.h"
#endif  /* ST_OSLINUX */

#include "sttbx.h"

#include "stdevice.h"
#include "vtg_drv.h"
#include "stvtg.h"
#include "vtg_rev.h"
#include "vtg_ihal.h"

#include "stdenc.h"
#ifdef STVTG_USE_CLKRV
#include "stclkrv.h"
#endif


/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

#define INVALID_DEVICE_INDEX (-1)
#define C656_ST7015_OFFSET     0x00006300 /* same for STi7020 */
#define DHDO_ST7015_OFFSET     0x00006200 /* same for STi7020 */

#ifdef ST_OSWINCE
#define SYSCFG_SIZE 0x1000
#endif
#if defined(ST_OSLINUX)
#define STVTG_MAPPING_WIDTH                 0x400
#define CKG_WIDTH                           0x1000
#define SYSCFG_WIDTH                        0x400
#endif /** ST_OSLINUX  **/
#if defined(ST_7200)
#define SD_TVOUT_TOPGLUE_BASE_ADDRESS     ST7200_SD_TVOUT_TOPGLUE_BASE_ADDRESS
#define HD_TVOUT_MAIN_GLUE_BASE_ADDRESS	  ST7200_HD_TVOUT_MAIN_GLUE_BASE_ADDRESS
#define HD_TVOUT_AUX_GLUE_BASE_ADDRESS	  ST7200_HD_TVOUT_AUX_GLUE_BASE_ADDRESS
#elif defined(ST_7111)
#define SD_TVOUT_TOPGLUE_BASE_ADDRESS     ST7111_SD_TVOUT_TOPGLUE_BASE_ADDRESS
#define HD_TVOUT_MAIN_GLUE_BASE_ADDRESS	  ST7111_HD_TVOUT_MAIN_GLUE_BASE_ADDRESS
#define HD_TVOUT_AUX_GLUE_BASE_ADDRESS	  ST7111_HD_TVOUT_AUX_GLUE_BASE_ADDRESS
#elif defined(ST_7105)
#define SD_TVOUT_TOPGLUE_BASE_ADDRESS     ST7105_SD_TVOUT_TOPGLUE_BASE_ADDRESS
#define HD_TVOUT_MAIN_GLUE_BASE_ADDRESS	  ST7105_HD_TVOUT_MAIN_GLUE_BASE_ADDRESS
#define HD_TVOUT_AUX_GLUE_BASE_ADDRESS	  ST7105_HD_TVOUT_AUX_GLUE_BASE_ADDRESS
#endif
/* Private variables (static) --------------------------------------------- */

static stvtg_Device_t DeviceArray[STVTG_MAX_DEVICE];
static semaphore_t *InstancesAccessControl_p;      /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;

/* Global Variables --------------------------------------------------------- */

/* need to be global to enable use of macro IsValidHandle(Handle) from outside this file */
stvtg_Unit_t stvtg_UnitArray[STVTG_MAX_UNIT];

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

#ifdef ST_OSWINCE
/* EnterCriticalSection & LeaveCriticalSection are WinCE system calls*/
#define EnterCriticalSection myEnterCriticalSection
#define LeaveCriticalSection myLeaveCriticalSection
#endif

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(  stvtg_Device_t *           const Device_p
                           , const STVTG_InitParams_t * const InitParams_p
                           , const ST_DeviceName_t      DeviceName
                          );
static ST_ErrorCode_t Open(void);
static ST_ErrorCode_t Close(stvtg_Unit_t * const Unit_p);
static ST_ErrorCode_t Term(  stvtg_Device_t *           const Device_p
                          );
static ST_ErrorCode_t RegisterEvents(  const ST_DeviceName_t EvtHandlerName
                                     , stvtg_Device_t *      const Device_p
                                     , const ST_DeviceName_t DeviceName
                                    );

/*
******************************************************************************
Private Functions
******************************************************************************
*/

/*******************************************************************************
Name        : EnterCriticalSection
Description : Used to protect critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnterCriticalSection(void)
{
    static BOOL InstancesAccessControlInitialized = FALSE;

    STOS_TaskLock();

    if (!InstancesAccessControlInitialized)
    {
        InstancesAccessControlInitialized = TRUE;
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        InstancesAccessControl_p = STOS_SemaphoreCreateFifo(NULL,1);
    }

    STOS_TaskUnlock();

    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(InstancesAccessControl_p);
} /* End of EnterCriticalSection() */

/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    /* Release the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreSignal(InstancesAccessControl_p);
} /* End of LeaveCriticalSection() */

/*******************************************************************************
Name        : Init
Description : VTG API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_INTERRUPT_INSTALL,
 *            STVTG_ERROR_EVT_REGISTER, STVTG_ERROR_EVT_SUBSCRIBE
 *            STVTG_ERROR_DENC_OPEN, STVTG_ERROR_CLKRV_OPEN, STVTG_ERROR_DENC_ACCESS
*******************************************************************************/
static ST_ErrorCode_t Init(  stvtg_Device_t * const Device_p
                           , const STVTG_InitParams_t * const InitParams_p
                           , const ST_DeviceName_t DeviceName
                          )
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#ifdef STVTG_USE_CLKRV
    const STCLKRV_OpenParams_t ClkrvOpenParams;
#endif /* STVTG_USE_CLKRV */
    const STDENC_OpenParams_t DencOpenParams;
    STDENC_Capability_t DencCapability;
    STVTG_VtgId_t MatchingVtgId;

    /* Test initialisation parameters and exit if some are invalid. */
    switch (InitParams_p->DeviceType)
    {
        case STVTG_DEVICE_TYPE_DENC :
            break;

#if defined(STVTG_VFE) || defined(STVTG_VOS) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE :           /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
            if (   (InitParams_p->Target.VtgCell.BaseAddress_p == NULL)
                || (strlen(InitParams_p->Target.VtgCell.InterruptEventName) > ST_MAX_DEVICE_NAME - 1)
                || ((((const char *) InitParams_p->Target.VtgCell.InterruptEventName)[0]) == '\0'))
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            break;
#endif
#if defined(STVTG_VFE) || defined (STVTG_VFE2) || defined(STVTG_VOS)
        case STVTG_DEVICE_TYPE_VFE2:        /* Sti5528 - Sti5100/5 - Stb5301 */
            if (InitParams_p->Target.VtgCell2.BaseAddress_p == NULL)
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            break;

        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
            if (InitParams_p->Target.VtgCell3.BaseAddress_p == NULL)
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            break;

        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
            if (InitParams_p->Target.VtgCell3.BaseAddress_p == NULL)
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            break;
       case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            if (InitParams_p->Target.VtgCell3.BaseAddress_p == NULL)
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            break;

#endif
        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }
    /* InitParams exploitation and other actions */

    ErrorCode = stvtg_IHALGetVtgId( InitParams_p, &MatchingVtgId);
    if ( ErrorCode != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (InitParams_p->DeviceType)
    {
        case STVTG_DEVICE_TYPE_DENC :
            ErrorCode = STDENC_Open(  InitParams_p->Target.DencName
                                    , &DencOpenParams
                                    , (STDENC_Handle_t * const)&(Device_p->DencHandle));
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STDENC_GetCapability( InitParams_p->Target.DencName, &DencCapability);
            }
            if (ErrorCode == ST_NO_ERROR)
            {
                Device_p->DencCellId     = DencCapability.CellId;
                Device_p->DencDeviceType = DencCapability.DeviceType;
            }
            else
            {
                ErrorCode = STVTG_ERROR_DENC_OPEN;
            }
            break;
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
        case STVTG_DEVICE_TYPE_VFE :
            Device_p->BaseAddress_p    = (void *) (  ((U8 *) InitParams_p->Target.VtgCell.DeviceBaseAddress_p)
                                                   + ((U32)  InitParams_p->Target.VtgCell.BaseAddress_p)
                                                  );
            break;
        case STVTG_DEVICE_TYPE_VFE2 :
            Device_p->BaseAddress_p    = (void *) (  ((U8 *) InitParams_p->Target.VtgCell2.DeviceBaseAddress_p)
                                                   + ((U32)  InitParams_p->Target.VtgCell2.BaseAddress_p)
                                                  );
            Device_p->CKGBaseAddress_p = (void*) (   ((U8 *) InitParams_p->Target.VtgCell2.DeviceBaseAddress_p)
                                                   + ((U32)  CKG_BASE_ADDRESS)
                                                  );
            break;
#endif /* #ifdef STVTG_VFE || VFE2*/
#ifdef STVTG_VOS
        case STVTG_DEVICE_TYPE_VTG_CELL_7015 : /* no break */
        case STVTG_DEVICE_TYPE_VTG_CELL_7020 :
            /* Useful registers base address for VTG programming is real address in memory,
            * that is address of the register map + address of VTG registers inside register map */
            Device_p->BaseAddress_p     = (void *) (  ((U8 *) InitParams_p->Target.VtgCell.DeviceBaseAddress_p)
                                                    + ((U32)  InitParams_p->Target.VtgCell.BaseAddress_p)
                                                   );
            Device_p->CKGBaseAddress_p  =  (void *) (  ((U8 *) InitParams_p->Target.VtgCell.DeviceBaseAddress_p)
                                                    + ( (U32) CKG_BASE_ADDRESS) );
            Device_p->C656BaseAddress_p = (void *) ( ((U8 *) InitParams_p->Target.VtgCell.DeviceBaseAddress_p)
                                                    + ((U32)  C656_ST7015_OFFSET)
                                                   );
            Device_p->DHDOBaseAddress_p = (void *) ( ((U8 *) InitParams_p->Target.VtgCell.DeviceBaseAddress_p)
                                                    + ((U32)  DHDO_ST7015_OFFSET)
                                                   );

          break;
        case STVTG_DEVICE_TYPE_VTG_CELL_7710 :
        case STVTG_DEVICE_TYPE_VTG_CELL_7100 :
            Device_p->BaseAddress_p     = (void *) (  ((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    + ((U32)  InitParams_p->Target.VtgCell3.BaseAddress_p));

            Device_p->CKGBaseAddress_p  = (void *)  (  ((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    +  ((U32) CKG_BASE_ADDRESS));
            Device_p->C656BaseAddress_p = (void *)  (  ((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    + ((U32) VOS_BASE_ADDRESS));
            Device_p->DHDOBaseAddress_p = (void *)  (  ((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    + ((U32) VOS_BASE_ADDRESS));
#ifndef ST_7710
            Device_p->SYSCFGBaseAddress_p = (void *)  (  ((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                            + ((U32)CFG_BASE_ADDRESS));
#endif

            #ifdef STVTG_USE_CLKRV
                ErrorCode = STCLKRV_Open( InitParams_p->Target.VtgCell3.ClkrvName
                                    , &ClkrvOpenParams
                                    , (STCLKRV_Handle_t * const)&(Device_p->ClkrvHandle));

                if (ErrorCode != ST_NO_ERROR)
                {
                    ErrorCode = STVTG_ERROR_CLKRV_OPEN;
                }
            #endif
            break;
        case STVTG_DEVICE_TYPE_VTG_CELL_7200 :
            Device_p->BaseAddress_p     = (void *) (  ((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    + ((U32)  InitParams_p->Target.VtgCell3.BaseAddress_p));

            Device_p->CKGBaseAddress_p  = (void *)  (  ((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    +  ((U32) CKG_BASE_ADDRESS));
            Device_p->C656BaseAddress_p = (void *)  (  ((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    + ((U32) VOS_BASE_ADDRESS));
            Device_p->DHDOBaseAddress_p = (void *)  (  ((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    + ((U32) VOS_BASE_ADDRESS));
            #if defined(ST_7200) || defined(ST_7111) || defined(ST_7105)
            /* SD TVout Base Address*/
            Device_p->SDGlueBaseAddress_p = (void *)  (((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    + ((U32) SD_TVOUT_TOPGLUE_BASE_ADDRESS));

            /* Top Glue Main Base Address*/
            Device_p->HDGlueMainBaseAddress_p = (void *) (((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    + ((U32) HD_TVOUT_MAIN_GLUE_BASE_ADDRESS));

            /* Top Glue Aux Base Address*/
            Device_p->HDGlueAuxBaseAddress_p = (void *) (((U8 *) InitParams_p->Target.VtgCell3.DeviceBaseAddress_p)
                                                    + ((U32) HD_TVOUT_AUX_GLUE_BASE_ADDRESS));
            #endif
			#ifdef STVTG_USE_CLKRV
                ErrorCode = STCLKRV_Open( InitParams_p->Target.VtgCell3.ClkrvName
                                    , &ClkrvOpenParams
                                    , (STCLKRV_Handle_t * const)&(Device_p->ClkrvHandle));

                if (ErrorCode != ST_NO_ERROR)
                {
                    ErrorCode = STVTG_ERROR_CLKRV_OPEN;
                }
            #endif
            break;

#endif /* #ifdef STVTG_VOS */
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

#ifdef  ST_OSLINUX

  	/* Mapping VTG Registers */
	if ( ErrorCode == ST_NO_ERROR)
	{

         Device_p->VtgMappedWidth = (U32)(STVTG_MAPPING_WIDTH);
         Device_p->VtgMappedBaseAddress_p = STLINUX_MapRegion( (void *) (Device_p->BaseAddress_p), (U32) (Device_p->VtgMappedWidth), "VTG");

        if ( Device_p->VtgMappedBaseAddress_p )
	    {
            /** affecting VTG Base Adress with mapped Base Adress **/
            Device_p->BaseAddress_p = Device_p->VtgMappedBaseAddress_p;
	    }
        else
        {
             ErrorCode =  STVTG_ERROR_MAP_VTG;
        }
    }

	/* Mapping Clock Generator registers */
	if ( ErrorCode == ST_NO_ERROR)
	{
         Device_p->CkgMappedWidth  =  (U32)(CKG_WIDTH);
         Device_p->VtgCKGMappedBaseAddress_p  = STLINUX_MapRegion( (void *) (Device_p->CKGBaseAddress_p), (U32) (Device_p->CkgMappedWidth), "CKGVTG");

        if ( Device_p->VtgCKGMappedBaseAddress_p )
	    {
            /** affecting CKG Base Adress with mapped Base Adress **/
            Device_p->CKGBaseAddress_p = Device_p->VtgCKGMappedBaseAddress_p;
	    }
        else
        {
             ErrorCode =  STVTG_ERROR_MAP_CKG;
        }
    }

#if !(defined(ST_7200)|| defined(ST_7111)|| defined (ST_7105))
    /* Mapping SYSCFG (PLL & fs )Registers */
	if ( ErrorCode == ST_NO_ERROR)
	{
         Device_p->SYSCFGMappedWidth  =  (U32)(SYSCFG_WIDTH);
         Device_p->SYSCFGMappedBaseAddress_p  = STLINUX_MapRegion( (void *) (Device_p->SYSCFGBaseAddress_p), (U32) (Device_p->SYSCFGMappedWidth), "SYSCFG");

        if ( Device_p->SYSCFGMappedBaseAddress_p )
	    {
            /** affecting SYS CFG Base Adress with mapped Base Adress **/
            Device_p->SYSCFGBaseAddress_p = Device_p->SYSCFGMappedBaseAddress_p;
	    }
        else
        {
             ErrorCode =  STVTG_ERROR_MAP_SYS;
        }
    }
#endif
#endif  /*  ST_OSLINUX */



    if ( ErrorCode == ST_NO_ERROR)
    {

        Device_p->VtgId      = MatchingVtgId;
        Device_p->DeviceType = InitParams_p->DeviceType;
        Device_p->MaxOpen    = InitParams_p->MaxOpen;
#ifdef WA_PixClkExtern
        Device_p->Upsample   = FALSE;
#endif /* #ifdef WA_PixClkExtern */

        ErrorCode = RegisterEvents(InitParams_p->EvtHandlerName, Device_p, DeviceName);
        if ( ErrorCode == ST_NO_ERROR)
        {
            /* Make HAL Device type dependant initializations */
            ErrorCode = stvtg_IHALInit(Device_p, InitParams_p);
        }
        if ( ErrorCode == ST_NO_ERROR)
        {
            /* Setup accessing mode data semaphore */
            Device_p->ModeCtrlAccess_p = STOS_SemaphoreCreateFifo(NULL,1);
        }
    }
    return(ErrorCode);
} /* End of Init() */

/*******************************************************************************
Name        : Open
Description : VTG actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t Open(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* OpenParams exploitation and other actions */

    return(ErrorCode);
} /* End of Open() */

/*******************************************************************************
Name        : Close
Description : VTG specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_NO_ERROR
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t Close(stvtg_Unit_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    /* reset the Clock configuration */
#if defined (STVTG_VFE2)
    stvtg_HALSetPixelClockRst(Unit_p->Device_p);
#endif
    /* Wait pending configuration finish before closing instance */
    /* then release mutex for other instances */

    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    return(ErrorCode);
} /* End of Close() */

/*******************************************************************************
Name        : Term
Description : VTG specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Return      : ST_NO_ERROR, ST_ERROR_INTERRUPT_UNINSTALL,
 *            STVTG_ERROR_EVT_UNREGISTER, STVTG_ERROR_DENC_OPEN
*******************************************************************************/
static ST_ErrorCode_t Term(  stvtg_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrorCode;
    /* TermParams exploitation and other actions */
    ErrorCode = stvtg_IHALTerm(Device_p);

    /* Don't leave instance semi-terminated: terminate as much as possible */
    if (STEVT_Close(Device_p->EvtHandle) != ST_NO_ERROR)
    {
        /* overwrites previous ErrorCode, only last error will be known by caller */
        ErrorCode = STVTG_ERROR_EVT_UNREGISTER;
    }
    if (Device_p->DeviceType == STVTG_DEVICE_TYPE_DENC)
    {
        if (STDENC_Close(Device_p->DencHandle) != ST_NO_ERROR)
        {
            /* overwrites previous ErrorCode, only last error will be known by caller */
            ErrorCode = STVTG_ERROR_DENC_OPEN;
        }
    }
    STOS_SemaphoreDelete(NULL,Device_p->ModeCtrlAccess_p);

    #ifdef STVTG_USE_CLKRV
        ErrorCode = STCLKRV_Close((STCLKRV_Handle_t)(Device_p->ClkrvHandle));

        if (ErrorCode != ST_NO_ERROR)
        {
            ErrorCode = STVTG_ERROR_CLKRV_CLOSE;
        }
    #endif

#ifdef  ST_OSLINUX
    /** VTG region **/
    STLINUX_UnmapRegion(Device_p->VtgMappedBaseAddress_p, (U32) (Device_p->VtgMappedWidth));
    /** CKG region **/
    STLINUX_UnmapRegion(Device_p->VtgCKGMappedBaseAddress_p, (U32) (Device_p->CkgMappedWidth));
    /** SYSCFG region **/
    STLINUX_UnmapRegion(Device_p->SYSCFGMappedBaseAddress_p, (U32) (Device_p->SYSCFGMappedWidth));
#endif  /* ST_OSLINUX */

    return(ErrorCode);
} /* End of Term() */


/*******************************************************************************
Name        : RegisterEvents
Description : Register VTGn events.
Parameters  : EvtHandlerName (IN) : Name of the Evt handler
 *            DeviceName (IN)     : Name of current VTG device
 *            Device_p (IN/OUT)   : memory location on stvtg_Device_t data.
Assumptions : parameters are OK
Limitations :
Return :      ST_NO_ERROR, STVTG_ERROR_EVT_REGISTER
*******************************************************************************/
static ST_ErrorCode_t RegisterEvents(  const ST_DeviceName_t EvtHandlerName
                                     , stvtg_Device_t * const Device_p
                                     , const ST_DeviceName_t DeviceName
                                    )
{
    STEVT_OpenParams_t STEVT_OpenParams;
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;

    ErrorCode = STEVT_Open(EvtHandlerName, &STEVT_OpenParams, &(Device_p->EvtHandle));

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Register STVTG_VSYNC_EVT with RegistrantName */
        ErrorCode = STEVT_RegisterDeviceEvent(Device_p->EvtHandle,
                        DeviceName, /* cast : work-around for DDTS GNBvd08060 */
                        STVTG_VSYNC_EVT,
                        &(Device_p->EventID[STVTG_VSYNC_ID]));

        if (ErrorCode != ST_NO_ERROR) /* Registering failed but STEVT_Open succeeded : close */
        {
            if (STEVT_Close(Device_p->EvtHandle) != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error in STEVT_Close(%lu)", Device_p->EvtHandle));
            }
        }
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = STVTG_ERROR_EVT_REGISTER;
    }
    return(ErrorCode);
} /* End of RegisterEvents() */

/*******************************************************************************
Name        : GetIndexOfDeviceNamed
Description : Get the index in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
Assumptions :
Limitations :
Returns     : the index if the name was found, INVALID_DEVICE_INDEX otherwise
*******************************************************************************/
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName)
{
    S32 WantedIndex = INVALID_DEVICE_INDEX, Index = 0;

    do
    {
        /* Device initialised: check if name is matching */
        if (strcmp(DeviceArray[Index].DeviceName, WantedName) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STVTG_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() */

/*
******************************************************************************
Public Functions
******************************************************************************
*/

/***************************************************************
Name : STVTG_GetRevision
Description : Retrieve the driver revision
Parameters : none
Return : driver revision
****************************************************************/
ST_Revision_t STVTG_GetRevision(void)
{

    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return(VTG_Revision);
} /* End of STVTG_GetRevision() */

/***************************************************************
Name : STVTG_GetCapability
Description : Retrieve the capabilities of the driver
Parameters : DeviceName (IN) : Name of the device to analyse
             Capability_p (IN/OUT) : @ for giving back information
Return : ST_NO_ERROR, ST_ERROR_UNKNOWN_DEVICE or ST_ERROR_BAD_PARAMETER
****************************************************************/
ST_ErrorCode_t STVTG_GetCapability(   const ST_DeviceName_t DeviceName
                                    , STVTG_Capability_t *  const Capability_p
                                  )
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                        /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!FirstInitDone)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill capability structure */
            Capability_p->TimingMode          = DeviceArray[DeviceIndex].CapabilityBank1;
            Capability_p->TimingModesAllowed2 = DeviceArray[DeviceIndex].CapabilityBank2;
            Capability_p->UserModeCapable     = FALSE;
        }
    }
    LeaveCriticalSection();

    return (ErrorCode);
} /* End of STVTG_GetCapability() */

/*
--------------------------------------------------------------------------------
Initialise VTG API
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVTG_Init(  const ST_DeviceName_t      DeviceName
                          , const STVTG_InitParams_t * const InitParams_p
                         )
{
    S32 Index = 0;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||                        /* There must be parameters */
        (InitParams_p->MaxOpen > STVTG_MAX_OPEN) ||      /* No more than MAX_OPEN open handles supported */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if ( !FirstInitDone)
    {
        for (Index = 0; Index < STVTG_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }
        for (Index = 0; Index < STVTG_MAX_UNIT; Index++)
        {
            stvtg_UnitArray[Index].UnitValidity = 0;
        }
        /* Process this only once */
        FirstInitDone = TRUE;
    }

    /* Check if device already initialised and return error if so */
    if (GetIndexOfDeviceNamed(DeviceName) != INVALID_DEVICE_INDEX)
    {
        /* Device name found */
        ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Look for a non-initialised device and return error if none is available */
        Index = 0;
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STVTG_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STVTG_MAX_DEVICE)
        {
            /* All devices initialised */
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            ErrorCode = Init(&DeviceArray[Index], InitParams_p, DeviceName);

            if (ErrorCode == ST_NO_ERROR)
            {
                /* Device available and successfully initialised: register device name */
                strncpy(DeviceArray[Index].DeviceName, DeviceName,sizeof(DeviceArray[Index].DeviceName));
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialized", DeviceArray[Index].DeviceName));
            }
        } /* End exists device not initialised */
    } /* End device not already initialised */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STVTG_Init() */


/*
--------------------------------------------------------------------------------
Open a VTG
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVTG_Open(  const ST_DeviceName_t      DeviceName
                          , const STVTG_OpenParams_t * const OpenParams_p
                          , STVTG_Handle_t *           const Handle_p
                         )
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                        /* There must be parameters ! */
        (Handle_p == NULL) ||                            /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (! FirstInitDone)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Look for a free unit and check all opened units to check if MaxOpen is not reached */
            UnitIndex = STVTG_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STVTG_MAX_UNIT; Index++)
            {
                if ((stvtg_UnitArray[Index].UnitValidity == STVTG_VALID_UNIT) && (stvtg_UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (stvtg_UnitArray[Index].UnitValidity != STVTG_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxOpen) || (UnitIndex >= STVTG_MAX_UNIT))
            {
                /* None of the units is free or MaxOpen reached */
                ErrorCode = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STVTG_Handle_t) &stvtg_UnitArray[UnitIndex];
                stvtg_UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex]; /* 'inherits' device characteristics */

                /* API specific actions after opening */
                ErrorCode = Open();

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    stvtg_UnitArray[UnitIndex].UnitValidity = STVTG_VALID_UNIT;
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle opened on device '%s'", DeviceName));
                }
            } /* End found unit unused */
        } /* End device valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STVTG_Open() */

/*
--------------------------------------------------------------------------------
Close a VTG
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVTG_Close(STVTG_Handle_t Handle)
{
    stvtg_Unit_t *Unit_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    EnterCriticalSection();

    if (! FirstInitDone)
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        if (!IsValidHandle(Handle))
        {
            ErrorCode = ST_ERROR_INVALID_HANDLE;
        }
        else
        {
            Unit_p = (stvtg_Unit_t *) Handle;

            /* API specific actions before closing */
            ErrorCode = Close(Unit_p);

            /* Close only if no errors */
            if (ErrorCode == ST_NO_ERROR)
            {
                Unit_p->UnitValidity = 0;
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
            }
        }
    }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STVTG_Close() */

/*
--------------------------------------------------------------------------------
Terminate VTG API
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVTG_Term(   const ST_DeviceName_t      DeviceName
                           , const STVTG_TermParams_t * const TermParams_p
                         )
{
    stvtg_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex = 0;
    BOOL Found = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                        /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (! FirstInitDone)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if NOT so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Check if there is still 'open' on this device */
            /*    UnitIndex = 0;
            Found = FALSE;*/
            Unit_p = stvtg_UnitArray;
            while ((UnitIndex < STVTG_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVTG_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    Unit_p = stvtg_UnitArray;
                    UnitIndex = 0;
                    while (UnitIndex < STVTG_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVTG_VALID_UNIT))
                        {
                            /* Found an open instance: close it ! */
                            ErrorCode = Close(Unit_p);
                            if (ErrorCode != ST_NO_ERROR)
                            {
                                /* If error: don't care, force close to force terminate... */
                                ErrorCode = ST_NO_ERROR;
                            }
                            /* Un-register opened handle whatever the error */
                            Unit_p->UnitValidity = 0;

                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
                        }
                        Unit_p++;
                        UnitIndex++;
                    }
                } /* End ForceTerminate: closed all opened handles */
                else
                {
                    /* Can't term if there are handles still opened, and ForceTerminate not set */
                    ErrorCode = ST_ERROR_OPEN_HANDLE;
                }
            } /* End found handle not closed */

            /* Terminate if OK */
            if (ErrorCode == ST_NO_ERROR)
            {
                /* API specific terminations */
                ErrorCode = Term(&DeviceArray[DeviceIndex]);
                /* Don't leave instance semi-terminated: terminate as much as possible */
                /* free device */
                DeviceArray[DeviceIndex].DeviceName[0] = '\0';

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */
    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STVTG_Term() */

/*************************************************************************
 Name : STVTG_GetVtgId
 Description : Gives back the VtgId associated to a VTG base address
              , that is the  offset of the VTG registers within the register map
 Parameters : BaseAddress_p (IN) : base address of the VTG
              VtgId_p (IN/OUT)   : memory location to give back information
 Return : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_UNKNOWN_DEVICE
**************************************************************************/
ST_ErrorCode_t STVTG_GetVtgId(  const ST_DeviceName_t  DeviceName
                              , STVTG_VtgId_t *        const VtgId_p
                             )
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((VtgId_p == NULL) ||
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (! FirstInitDone)
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if NOT so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            *VtgId_p = DeviceArray[DeviceIndex].VtgId;
        }
    }
    return(ErrorCode);
} /* End of STVTG_GetVtgId() function */

/***************************************************************
Name : stvtg_SignalVSYNC
Description : Notify VSYNC event to each opened VTG
Parameters : Type : Top or Bottom VSYNC
Return : none
****************************************************************/
void stvtg_SignalVSYNC(STVTG_VSYNC_t Type)
{
    stvtg_Unit_t *Unit_p;
    U16 Tmp;

    for (Unit_p = stvtg_UnitArray, Tmp= 0; (Tmp < STVTG_MAX_UNIT); Unit_p++, Tmp++)
    {
        if (Unit_p->UnitValidity == STVTG_VALID_UNIT)
        {
            STEVT_Notify(  Unit_p->Device_p->EvtHandle
                         , Unit_p->Device_p->EventID[STVTG_VSYNC_ID]
                         , &Type
                        );
        }
    }
} /* End of stvtg_SignalVSYNC() */


/* ----------------------------- End of stvtg.c ------------------------------ */




