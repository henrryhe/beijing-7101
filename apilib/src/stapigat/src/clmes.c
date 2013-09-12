/*******************************************************************************

File name   : clmes.c

Description : STMES configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
10 Jul 2007        Created                                          OG + DG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#if defined(DVD_SECURED_CHIP)

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "clmes.h"
#include "stmes.h"
#include "stos.h"
#include "sttbx.h"
#include "stdevice.h"
#include "stddefs.h"
#include "stsys.h"
#include "testtool.h"
#include "clavmem.h"

/* Private Types ------------------------------------------------------------ */

typedef struct MESSecureRegion_s
{
    U32 RegionAddrStart; /* Start address of the secure region */
    U32 RegionAddrStop;  /* End address of the secure region   */
} MES_SecureRegion_t;

/* Private Constants -------------------------------------------------------- */

#if defined(ST_OS21) && !defined(ST_OSWINCE)
    #define ST7109_MES_LMI_SYS_INTERRUPT	    OS21_INTERRUPT_MES_LMI_SYS
    #define ST7109_MES_LMI_VID_INTERRUPT	    OS21_INTERRUPT_MES_LMI_VID
#elif defined(ST_OSWINCE)
    #define ST7109_MES_LMI_SYS_INTERRUPT        IRQ_MES_LMI_SYS
    #define ST7109_MES_LMI_VID_INTERRUPT	    IRQ_MES_LMI_VID
#endif

#define MES_SYS_INT_MASK        0xb9500008
#define MES_SYS_VIOLATED_ADD    0xb950000c
#define MES_SYS_VIOLATED_SRC    0xb9500010
#define MES_SYS_STATUS          0xb9500004
#define MES_VID_INT_MASK        0xb9508008
#define MES_VID_VIOLATED_ADD    0xb950800c
#define MES_VID_VIOLATED_SRC    0xb9508010
#define MES_VID_STATUS          0xb9508004

#define LX_CODE_START (LX_VIDEO_CODE_MEMORY_BASE)
#define LX_CODE_STOP  (LX_VIDEO_CODE_MEMORY_BASE+LX_VIDEO_CODE_MEMORY_SIZE+LX_AUDIO_CODE_MEMORY_SIZE-1)


/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define MES_Read32(Address_p)           STSYS_ReadRegDev32LE((void *)(Address_p))
#define MES_Write32(Address_p, Val)     STSYS_WriteRegDev32LE((void *)(Address_p), (Val))

#define MES_ACCESS_MODE_STRING(CpuMode) ((CpuMode) == ACCESS_ALL               ? "ACCESS_ALL" : \
                                         (CpuMode) == ACCESS_NO_STORE_INSECURE ? "ACCESS_NO_STORE_INSECURE" : \
                                         (CpuMode) == ACCESS_NO_SECURE         ? "ACCESS_NO_SECURE" : \
                                         (CpuMode) == ACCESS_LOAD_SECURE_ONLY  ? "ACCESS_LOAD_SECURE_ONLY" : \
                                         "UNRECOGNISED CPU MODE")

#define MES_IS_ERROR_CODE(Val)          (((Val)>=STMES_ERROR_ALREADY_INITIALISED)&&((Val)<STMES_ERROR_MAX))

#define MES_ERROR_STRING(MesError)      ((MesError) == STMES_ERROR_ALREADY_INITIALISED ? "STMES_ERROR_ALREADY_INITIALISED" : \
                                         (MesError) == STMES_ERROR_NOT_INITIALISED     ? "STMES_ERROR_NOT_INITIALISED" : \
                                         (MesError) == STMES_ERROR_ADDR_RANGE          ? "STMES_ERROR_ADDR_RANGE" : \
                                         (MesError) == STMES_ERROR_ADDR_ALIGN          ? "STMES_ERROR_ADDR_ALIGN" : \
                                         (MesError) == STMES_ERROR_MES_INCORRECT       ? "STMES_ERROR_MES_INCORRECT" : \
                                         (MesError) == STMES_ERROR_SID_INCORRECT       ? "STMES_ERROR_SID_INCORRECT" : \
                                         (MesError) == STMES_ERROR_CPUMODE_INCORRECT   ? "STMES_ERROR_CPUMODE_INCORRECT" : \
                                         (MesError) == STMES_ERROR_NOT_SUPPORTED       ? "STMES_ERROR_NOT_SUPPORTED" : \
                                         (MesError) == STMES_ERROR_NO_REGIONS          ? "STMES_ERROR_NO_REGIONS" : \
                                         (MesError) == STMES_ERROR_BUSY                ? "STMES_ERROR_BUSY" : \
                                         "UNRECONISED STMES ERROR CODE")

/* Private Function prototypes ---------------------------------------------- */
static STOS_INTERRUPT_DECLARE(MesITHandler, Param);
static BOOL MES_SetSecureRegions(STTST_Parse_t *pars_p, char *result_sym_p);
static BOOL MES_EnableInterrupts(STTST_Parse_t *pars_p, char *result_sym_p);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : MES_Init
Description : Initialize the MES driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL MES_Init(void)
{
    BOOL RetOk = TRUE;
    STMES_InitParams_t InitParams;

	STOS_InterruptLock();

	/* Mount the MES IRQ for LMI_SYS */
	if(STOS_InterruptInstall(ST7109_MES_LMI_SYS_INTERRUPT,
                             0,
                             STOS_INTERRUPT_CAST(MesITHandler),
                             "MES_SYS_IRQ_NAME",
                             (char *)"MES_SYS") != STOS_SUCCESS)
    {
        RetOk = FALSE;
        STTBX_Print(("Failed to mount MES IT\r\n"));
    }
    else
	{
	    /* Interrupt installed, enable it */
    	if (STOS_InterruptEnable(ST7109_MES_LMI_SYS_INTERRUPT, 0) != STOS_SUCCESS)
        {
            RetOk = FALSE;
            STTBX_Print(("Failed to enable MES IT\r\n"));
        }
    }

	/* Mount the MES IRQ for LMI_VID */
	if(STOS_InterruptInstall(ST7109_MES_LMI_VID_INTERRUPT,
                             0,
                             STOS_INTERRUPT_CAST(MesITHandler),
                             "MES_VID_IRQ_NAME",
                             (char *)"MES_VID") != STOS_SUCCESS)
    {
        RetOk = FALSE;
        STTBX_Print(("Failed to mount MES IT\r\n"));
    }
    else
	{
	    /* Interrupt installed, enable it */
    	if (STOS_InterruptEnable(ST7109_MES_LMI_VID_INTERRUPT, 0) != STOS_SUCCESS)
        {
            RetOk = FALSE;
            STTBX_Print(("Failed to enable MES IT\r\n"));
        }
    }

	STOS_InterruptUnlock();

    if (RetOk)
    {
    	/* Initialize STMES */
    	InitParams.uInterruptFlags = SEC_TO_INSEC_INT | INSEC_TO_SEC_INT | ENCRYPT_ALL_INT;

    	if(STMES_Init(&InitParams) != ST_NO_ERROR)
    	{
    	    RetOk = FALSE;
    	}
    }

    if (!RetOk)
    {
        STTBX_Print(("STMES Init failed!"));
    }
    else
    {
        ST_Revision_t STMES_Revision = STMES_GetVersion();
        STTBX_Print(("STMES initialized,\trevision=%s\n", STMES_Revision));
    }

    return(RetOk);
} /* End of MES_Init() */

/*******************************************************************************
Name        : MES_Term
Description : Terminate the MES driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void MES_Term(void)
{
    /* Disable and uninstall MES interrupts */
    if (STOS_InterruptDisable(ST7109_MES_LMI_SYS_INTERRUPT, 0))
    {
        STTBX_Report(("Failed to disable MES interrupt\n"));
        return;
    }

    STOS_InterruptLock();

    if (((STOS_InterruptUninstall(ST7109_MES_LMI_SYS_INTERRUPT, 0, (char *)"MES_SYS")) != STOS_SUCCESS)
      || ((STOS_InterruptUninstall(ST7109_MES_LMI_SYS_INTERRUPT, 0, (char *)"MES_VID")) != STOS_SUCCESS))
    {
        STTBX_Report(("Failed to detach handler for MES interrupts\n"));
    }

    STOS_InterruptUnlock();

    if(STMES_Term() != ST_NO_ERROR)
    {
        STTBX_Print(("STMES_Term(): FAILED!\r\n"));
    }
    else
    {
        STTBX_Print(("STMES_Term(): ok\r\n"));
    }
} /* End of MES_Term() */

/*******************************************************************************
Name        : MesSysITHandler
Description : MES SYS violation ISR
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static STOS_INTERRUPT_DECLARE(MesITHandler, Param)
{
    U32 TempVariable1, TempVariable2, TempVariable3;

    if ((char*)Param == "MES_SYS")
    {
        /* MES SYS violated address */
        TempVariable1 = MES_Read32(MES_SYS_VIOLATED_ADD);
        /* MES SYS violated source */
        TempVariable2 = MES_Read32(MES_SYS_VIOLATED_SRC);
        /* MES SYS status */
        TempVariable3 = MES_Read32(MES_SYS_STATUS);
    }

    if ((char*)Param == "MES_VID")
    {
        /* MES VID violated address */
        TempVariable1 = MES_Read32(MES_VID_VIOLATED_ADD);
        /* MES VID violated source */
        TempVariable2 = MES_Read32(MES_VID_VIOLATED_SRC);
        /* MES VID status */
        TempVariable3 = MES_Read32(MES_VID_STATUS);
    }

	/*assert(0);*/

    STOS_INTERRUPT_EXIT(STOS_SUCCESS);
} /* End of MesITHandler() */

/*******************************************************************************
Name        : MES_SetSecureRegions
Description : Initialize the secure regions
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/
static BOOL MES_SetSecureRegions(STTST_Parse_t *pars_p, char *result_sym_p)
{
#if defined(ST_OS21) && !defined(ST_OSWINCE)
    MES_SecureRegion_t   SecureRegions[]=
        {
            {LX_CODE_START,     LX_CODE_STOP },     /* Both LXs must be in a secure region */
            {PARTITION2_START,  PARTITION2_STOP},   /* Partition2 must be in a secure region */
            {PARTITION1_START,  PARTITION1_STOP},   /* Partition1 (LMI_VID) must be in a secure region */
        };

#elif defined(ST_OSWINCE)
    MES_SecureRegion_t   SecureRegions[]=
        {
            { LX_CODE_START, LX_CODE_STOP },     /* Both LXs must be in a secure region */
            {PARTITION2_START, PARTITION2_STOP}, /* Partition2 must be in a secure region */
            {PARTITION1_START, PARTITION1_STOP}, /* Partition1 (LMI_VID) must be in a secure region */
        };
#endif /* defined(ST_OS21) && !defined(ST_OSWINCE) */

    U32                 NbOfSecureRegions = sizeof(SecureRegions)/sizeof(SecureRegions[0]);
    U32                 RegionIndex       = 0;
    ST_ErrorCode_t      ErrorCode         = ST_NO_ERROR;
    MODES_t             CpuMode;

    UNUSED_PARAMETER(pars_p);

    /***********
     * LMI_SYS *
     ***********/
    /* Get the SH4 CPU access mode to LMI_SYS */
    CpuMode = STMES_GetSourceMode(SID_SH4_CPU, SYS_MES);

    /* Check if function returned an error */
    if( MES_IS_ERROR_CODE(CpuMode) )
    {
        /* Function failed */
        STTBX_Print(("Failed to get SH4 CPU access mode for LMI_SYS, got error code 0x%.8X (%s)\r\n",
                     (ST_ErrorCode_t)CpuMode, MES_ERROR_STRING((ST_ErrorCode_t)CpuMode)));
        goto FAILURE_EXIT;
    }

    /* Check if the SH4 CPU is in the default mode */
    if(CpuMode != ACCESS_NO_SECURE)
    {
        STTBX_Print(("Unexpected SH4 CPU access mode for LMI_SYS (expected %s, got %s)\r\n",
                     MES_ACCESS_MODE_STRING(ACCESS_NO_SECURE), MES_ACCESS_MODE_STRING(CpuMode)));
        goto FAILURE_EXIT;
    }
    else
    {
        STTBX_Print(("SH4 access mode for LMI_SYS: %s\r\n", MES_ACCESS_MODE_STRING(CpuMode)));
    }

    /***********
     * LMI_VID *
     ***********/
    /* Get the SH4 CPU access mode to LMI_VID */
    CpuMode = STMES_GetSourceMode(SID_SH4_CPU, VID_MES);

    /* Check if function returned an error */
    if( MES_IS_ERROR_CODE(CpuMode) )
    {
        /* Function failed */
        STTBX_Print(("Failed to get SH4 CPU access mode for LMI_VID, got error code 0x%.8X (%s)\r\n",
                     (ST_ErrorCode_t)CpuMode, MES_ERROR_STRING((ST_ErrorCode_t)CpuMode)));
        goto FAILURE_EXIT;
    }

    /* Check if the SH4 CPU is in the default mode */
    if(CpuMode != ACCESS_NO_SECURE)
    {
        STTBX_Print(("Unexpected SH4 CPU access mode for LMI_VID (expected %s, got %s)\r\n",
                     MES_ACCESS_MODE_STRING(ACCESS_NO_SECURE), MES_ACCESS_MODE_STRING(CpuMode)));
        goto FAILURE_EXIT;
    }
    else
    {
        STTBX_Print(("SH4 access mode for LMI_VID: %s\r\n", MES_ACCESS_MODE_STRING(CpuMode)));
    }

    /**********************
     * Set Secure regions *
     **********************/
    /* Secure the regions listed in the SecureRegions table */
    for(RegionIndex=0; RegionIndex < NbOfSecureRegions; RegionIndex++)
    {
        ErrorCode = STMES_SetSecureRange( STAVMEM_VirtualToDevice((void*)SecureRegions[RegionIndex].RegionAddrStart, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                          STAVMEM_VirtualToDevice((void*)SecureRegions[RegionIndex].RegionAddrStop, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                          SID_SH4_CPU);

        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Print(("Failed to declare the secure range (0x%.8X,0x%.8X), got error code 0x%.8X (%s)\r\n",
                         SecureRegions[RegionIndex].RegionAddrStart, SecureRegions[RegionIndex].RegionAddrStop,
                         ErrorCode, MES_ERROR_STRING(ErrorCode)));
            goto FAILURE_EXIT;
        }
        else
        {
            STTBX_Print(("MES secure region %d set: 0x%.8x -> 0x%.8x\r\n", RegionIndex + 1,
                         SecureRegions[RegionIndex].RegionAddrStart, SecureRegions[RegionIndex].RegionAddrStop));
        }
    }

    /*********************
     * Set access rights *
     *********************/
    /* Authorize the Video LX to access both secure and non-secure regions */
    ErrorCode = STMES_SetCPUAccess(SID_VIDEO_LX, ACCESS_ALL);

    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Failed to authorise VIDEO_LX CPU access to all memory, got error code 0x%.8X (%s)\r\n",
                     ErrorCode, MES_ERROR_STRING(ErrorCode)));
        goto FAILURE_EXIT;
    }

    /* Authorize the Audio LX to access both secure and non-secure regions */
    ErrorCode = STMES_SetCPUAccess(SID_AUDIO_LX, ACCESS_ALL);

    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Failed to authorise AUDIO_LX CPU access to all memory, got error code 0x%.8X (%s)\r\n",
                     ErrorCode, MES_ERROR_STRING(ErrorCode)));
        goto FAILURE_EXIT;
    }

    STTST_AssignInteger(result_sym_p, ST_NO_ERROR, FALSE);
    return FALSE; /* means no error */

FAILURE_EXIT:
    STTST_AssignInteger(result_sym_p, ST_NO_ERROR, FALSE);
    return TRUE; /* means error */
} /* end of function MES_SetSecureRegions() */

/*******************************************************************************
Name        : MES_EnableInterrupts
Description : Enable MES interrupts
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL MES_EnableInterrupts(STTST_Parse_t *pars_p, char *result_sym_p)

{
    UNUSED_PARAMETER(pars_p);

    MES_Write32( MES_SYS_INT_MASK, 0x3);
    MES_Write32( MES_VID_INT_MASK, 0x3);

    STTBX_Print(("MES interrupts enabled.\r\n"));

    STTST_AssignInteger(result_sym_p, ST_NO_ERROR, FALSE);
    return FALSE; /* means no error */
} /* end of function MES_EnableInterrupts() */


/*******************************************************************************
Name        : MES_RegisterCmd
Description : Initialize the MES driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL MES_RegisterCmd(void)
{
    BOOL RetErr = FALSE;

    /*  Register the STMES commands */
    RetErr |= STTST_RegisterCommand( "MES_SET_SECURE_REGIONS",  MES_SetSecureRegions,
                                     ": initializes the secure regions");
    RetErr |= STTST_RegisterCommand( "MES_ENABLE_INTERRUPTS",  MES_EnableInterrupts,
                                     ": enables MES interrupts");
    return( RetErr == FALSE ? TRUE : FALSE);

} /* end MES_RegisterCmd */

#endif /* DVD_SECURED_CHIP */
/* End of clmes.c */
