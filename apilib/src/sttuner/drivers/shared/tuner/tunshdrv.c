/* ----------------------------------------------------------------------------
File Name: tunshdrv.c

Description:

    shared tuner driver


Copyright (C) 2005-2006 STMicroelectronics

History:

   date:
version:
 author:
comment:



Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */
#ifndef ST_OSLINUX
  
#include <string.h>
#endif
#include "stlite.h"     /* Standard includes */

/* STAPI */

#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#include "chip.h"
#include "shioctl.h"     /* data structure typedefs for all the the ter ioctl functions */
#include "tunshdrv.h"      /* header for this file */
#ifdef STTUNER_DRV_SHARED_TUN_MT2060
#include "MT2060.h"
#endif

#ifdef STTUNER_DRV_SHARED_TUN_MT2131
#include "MT2131.h"
#endif

/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;

static BOOL Installed_TD1336    = FALSE;


static BOOL Installed_FQD1236    = FALSE;



static BOOL Installed_T2000    = FALSE;


static BOOL Installed_DTT7600    = FALSE;


static BOOL Installed_TDVEH052F    = FALSE;
static BOOL Installed_DTT761X    = FALSE;

static BOOL Installed_MT2060    = FALSE;

static BOOL Installed_MT2131   = FALSE;
static BOOL Installed_DTT768XX    = FALSE;

#define WAIT_N_MS_SHTUNER(X)     STOS_TaskDelayUs(X * 1000)

#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))
/* instance chain, the default boot value is invalid, to catch errors */
static TUNSHDRV_InstanceData_t *InstanceChainTop = (TUNSHDRV_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */

/* API */

ST_ErrorCode_t tuner_tunshdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams);
ST_ErrorCode_t tuner_tunshdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams);

ST_ErrorCode_t tuner_tunshdrv_Open (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Handle_t *Handle);
ST_ErrorCode_t tuner_tunshdrv_Close(TUNER_Handle_t  Handle, TUNER_CloseParams_t *CloseParams);

ST_ErrorCode_t tuner_tunshdrv_SetFrequency (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency);
ST_ErrorCode_t tuner_tunshdrv_GetStatus    (TUNER_Handle_t Handle, TUNER_Status_t *Status);
ST_ErrorCode_t tuner_tunshdrv_IsTunerLocked(TUNER_Handle_t Handle, BOOL *Locked);
ST_ErrorCode_t tuner_tunshdrv_SetBandWidth (TUNER_Handle_t Handle, U32 BandWidth, U32 *NewBandWidth);


/* I/O API */
ST_ErrorCode_t tuner_tunshdrv_ioaccess(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle,
    STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);


/* access device specific low-level functions */
ST_ErrorCode_t tuner_tunshdrv_ioctl(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);
#ifdef STTUNER_DRV_SHARED_TUN_TD1336
ST_ErrorCode_t tuner_tunshdrv_Open_TD1336(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_SHARED_TUN_FQD1236
ST_ErrorCode_t tuner_tunshdrv_Open_FQD1236(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_SHARED_TUN_T2000
ST_ErrorCode_t tuner_tunshdrv_Open_T2000(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_SHARED_TUN_DTT7600
ST_ErrorCode_t tuner_tunshdrv_Open_DTT7600(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif

#ifdef STTUNER_DRV_SHARED_TUN_MT2060
ST_ErrorCode_t tuner_tunshdrv_Open_MT2060(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif

#ifdef STTUNER_DRV_SHARED_TUN_MT2131
	ST_ErrorCode_t tuner_tunshdrv_Open_MT2131(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif

#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
ST_ErrorCode_t tuner_tunshdrv_Open_TDVEH052F(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif

#ifdef STTUNER_DRV_SHARED_TUN_DTT761X
ST_ErrorCode_t tuner_tunshdrv_Open_DTT761X(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
ST_ErrorCode_t tuner_tunshdrv_Open_DTT768XX(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif

TUNSHDRV_InstanceData_t *TUNSHDRV_GetInstFromHandle(TUNER_Handle_t Handle);
U32 SharedTunerGetStepsize(TUNSHDRV_InstanceData_t *Instance);
extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];

ST_ErrorCode_t STTUNER_DRV_TUNER_TUNSHDRV_Install(  STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType);
ST_ErrorCode_t STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType);

#ifdef STTUNER_DRV_SHARED_TUN_TD1336
U16 TD1336_Address[TD1336_NBREGS]=
{

0x0000,
0x0001,
0x0002,
0x0003,
0x0004
};

U8 TD1336_DefVal[TD1336_NBREGS]=
{0x2E,0x7C,0xC0,0x1a,0x00}  ;
#endif
#ifdef STTUNER_DRV_SHARED_TUN_FQD1236
U16  FQD1236_Address[FQD1236_NBREGS]=
{


0x0000,
0x0001,
0x0002,
0x0003,
0x0004
};

U8  FQD1236_DefVal[FQD1236_NBREGS]=
{0x06,0x50,0xDE,0xA0,0x00}  ;
#endif
#ifdef STTUNER_DRV_SHARED_TUN_T2000
U16  T2000_Address[T2000_NBREGS]=
{


0x0000,
0x0001,
0x0002,
0x0003,
0x0004
};

U8  T2000_DefVal[T2000_NBREGS]={0x0006,0x0050,0x0085,0x000C,0x0000};
#endif
#ifdef STTUNER_DRV_SHARED_TUN_DTT7600
U16  DTT7600_Address[DTT7600_NBREGS]=
{


0x0000,
0x0001,
0x0002,
0x0003,
0x0004
};


U8  DTT7600_DefVal[DTT7600_NBREGS]=

{
0x1A,
0x30,
0x8E,
0x0A,
0x00
};
#endif

#ifdef STTUNER_DRV_SHARED_TUN_MT2060


U8  MT2060_DefVal[MT2060_NBREGS]=

{
	0x3F,            /* reg[0x01] <- 0x3F */
        0x74,            /* reg[0x02] <- 0x74 */
        0x80,            /* reg[0x03] <- 0x80 (FM1CA = 1) */
        0x08,            /* reg[0x04] <- 0x08 */
        0x93,            /* reg[0x05] <- 0x93 */
        0x88,            /* reg[0x06] <- 0x88 (RO) */
        0x80,            /* reg[0x07] <- 0x80 (RO) */
        0x60,            /* reg[0x08] <- 0x60 (RO) */
        0x20,            /* reg[0x09] <- 0x20 */
        0x1E,            /* reg[0x0A] <- 0x1E */
        0x31,            /* reg[0x0B] <- 0x31 (VGAG = 1) */
        0xFF,            /* reg[0x0C] <- 0xFF */
        0x80,            /* reg[0x0D] <- 0x80 */
        0xFF ,          /* reg[0x0E] <- 0xFF */
        0x00,            /* reg[0x0F] <- 0x00 */
        0x2C,            /* reg[0x10] <- 0x2C (HW def = 3C) */
        0x42           /* reg[0x11] <- 0x42 */
};
#endif


#ifdef STTUNER_DRV_SHARED_TUN_MT2131
U8 MT2131_DefVal[MT2131_NBREGS]=
	{

	     0x50,            /* reg[0x01] <- 0x50 */
        0x00,            /* reg[0x02] <- 0x00 */
        0x50,            /* reg[0x03] <- 0x50 */
        0x80,            /* reg[0x04] <- 0x80 */
        0x00,            /* reg[0x05] <- 0x00 */
        0x49,            /* reg[0x06] <- 0x49 */
        0xFA,            /* reg[0x07] <- 0xFA */
        0x88,            /* reg[0x08] <- 0x88 */
        0x08,            /* reg[0x09] <- 0x08 */
        0x77,            /* reg[0x0A] <- 0x77 */
        0x41,            /* reg[0x0B] <- 0x41 */
        0x04,            /* reg[0x0C] <- 0x04 */
        0x00,            /* reg[0x0D] <- 0x00 */
        0x00,            /* reg[0x0E] <- 0x00 */
        0x00,            /* reg[0x0F] <- 0x00 */
        0x32,            /* reg[0x10] <- 0x32 */
        0x7F,            /* reg[0x11] <- 0x7F */
        0xDA,            /* reg[0x12] <- 0xDA */
        0x4C,            /* reg[0x13] <- 0x4C */
        0x00,            /* reg[0x14] <- 0x00 */
        0x10,            /* reg[0x15] <- 0x10 */
        0xAA,            /* reg[0x16] <- 0xAA */
        0x78,            /* reg[0x17] <- 0x78 */
        0x80,            /* reg[0x18] <- 0x80 */
        0xFF,            /* reg[0x19] <- 0xFF */
        0x68,            /* reg[0x1A] <- 0x68 */
        0xA0,            /* reg[0x1B] <- 0xA0 */
        0xFF,            /* reg[0x1C] <- 0xFF */
        0xDD,            /* reg[0x1D] <- 0xDD */
        0x00,            /* reg[0x1E] <- 0x00 */
        0x00            /* reg[0x1F] <- 0x00 */

	};


#endif

#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
U16  TDVEH052F_Address[TDVEH052F_NBREGS]=
{


0x0000,
0x0001,
0x0002,
0x0003,
0x0004
};

/*default values are not tested*/
U8  TDVEH052F_DefVal[TDVEH052F_NBREGS]=

{
0x1A,
0x30,
0x8E,
0x0A,
0x00
};
#endif

#ifdef STTUNER_DRV_SHARED_TUN_DTT761X
U16  DTT761X_Address[DTT761X_NBREGS]=
{


0x0000,
0x0001,
0x0002,
0x0003,
0x0000,
0x0001,
0x0002,
0x0003,
0x0004
};

/*default values are not tested*/
U8  DTT761X_DefVal[DTT761X_NBREGS]=

{
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00
};
#endif

#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
U16  DTT768XX_Address[DTT768XX_NBREGS]=
{


0x0000,
0x0001,
0x0002,
0x0003,
0x0002,
0x0003,
0x0004
};


U8  DTT768XX_DefVal[DTT768XX_NBREGS]=

{
0x06,0x50,0x85,0x39,0x13,0x80,0x00
};
#endif

/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_TD1336_Install()


Description:
    install a shared device driver(TD1336 -- Philips).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_TD1336
ST_ErrorCode_t STTUNER_DRV_TUNER_TD1336_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_Install(Tuner, STTUNER_TUNER_TD1336) );
}
#endif
/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_FQD1236_Install()


Description:
    install a shared device driver(FQD1236 -- Philips).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_FQD1236
ST_ErrorCode_t STTUNER_DRV_TUNER_FQD1236_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_Install(Tuner, STTUNER_TUNER_FQD1236) );
}
#endif
/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_T2000_Install()


Description:
    install a shared device driver(T2000 -- TMM).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_T2000
ST_ErrorCode_t STTUNER_DRV_TUNER_T2000_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_Install(Tuner, STTUNER_TUNER_T2000) );
}
#endif


/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_DTT7600_Install()


Description:
    install a shared device driver(DTT7600 -- TMM).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_DTT7600
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7600_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_Install(Tuner, STTUNER_TUNER_DTT7600) );
}
#endif



/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_MT2060_Install()


Description:
    install a shared device driver(MT2060 -- TMM).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_MT2060
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2060_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_Install(Tuner, STTUNER_TUNER_MT2060) );
}
#endif




/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_MT2131_Install()


Description:
    install a shared device driver(MT2131 -- TMM).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_MT2131
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2131_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_Install(Tuner, STTUNER_TUNER_MT2131) );
}
#endif



/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_TDVEH052F_Install()


Description:
    install a shared device driver(TDVEH052F -- TMM).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
ST_ErrorCode_t STTUNER_DRV_TUNER_TDVEH052F_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_Install(Tuner, STTUNER_TUNER_TDVEH052F) );
}
#endif


/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_DTT761X_Install()


Description:
    install a shared device driver(DTT761X -- TMM).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_DTT761X
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT761X_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_Install(Tuner, STTUNER_TUNER_DTT761X) );
}
#endif


/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_DTT768XX_Install()


Description:
    install a shared device driver(DTT768XX -- TMM).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT768XX_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_Install(Tuner, STTUNER_TUNER_DTT768XX) );
}
#endif


/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_TD1336_UnInstall()

Description:
    uninstall a shared device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_TD1336
ST_ErrorCode_t STTUNER_DRV_TUNER_TD1336_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(Tuner, STTUNER_TUNER_TD1336));
}
#endif
/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_FQD1236_UnInstall()

Description:
    uninstall a shared device driver(FQD1236 -Philips).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_FQD1236
ST_ErrorCode_t STTUNER_DRV_TUNER_FQD1236_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(Tuner, STTUNER_TUNER_FQD1236));
}
#endif
/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_T2000_UnInstall()

Description:
    uninstall a shared device driver(FQD1236 -Philips).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_T2000
ST_ErrorCode_t STTUNER_DRV_TUNER_T2000_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(Tuner, STTUNER_TUNER_T2000));
}
#endif

/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_DTT7600_UnInstall()

Description:
    uninstall a shared device driver(DTT7600 ).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_DTT7600
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7600_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(Tuner, STTUNER_TUNER_DTT7600));
}
#endif

/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_MT2060_UnInstall()

Description:
    uninstall a shared device driver(MT2060 ).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_MT2060
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2060_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(Tuner, STTUNER_TUNER_MT2060));
}
#endif



/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_MT2131_UnInstall()

Description:
    uninstall a shared device driver(MT2060 ).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_MT2131
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2131_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(Tuner, STTUNER_TUNER_MT2131));
}
#endif


/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_TDVEH052F_UnInstall()

Description:
    uninstall a shared device driver(TDVEH052F ).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
ST_ErrorCode_t STTUNER_DRV_TUNER_TDVEH052F_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(Tuner, STTUNER_TUNER_TDVEH052F));
}
#endif

/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_DTT761X_UnInstall()

Description:
    uninstall a shared device driver(TDVEH052F ).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_DTT761X
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT761X_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(Tuner, STTUNER_TUNER_DTT761X));
}
#endif


/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_DTT768XX_UnInstall()

Description:
    uninstall a shared device driver(DTT768XX ).

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
ST_ErrorCode_t STTUNER_DRV_TUNER_DTT768XX_UnInstall (STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(Tuner, STTUNER_TUNER_DTT768XX));
}
#endif







/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_TUNER_TUNSHDRV_Install()

Description:
    install a shared device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_TUNER_TUNSHDRV_Install(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c STTUNER_DRV_TUNER_TUNSHDRV_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

   
    
    if (Installed == FALSE)
    {
        InstanceChainTop = NULL;

        Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);

        Installed = TRUE;
    }
    

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s installing sat:tuner:none...", identity));
#endif

     switch(TunerType)
    {

#ifdef STTUNER_DRV_SHARED_TUN_TD1336
        case STTUNER_TUNER_TD1336:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s installing sat:tuner:TD1336...", identity));
#endif
            if (Installed_TD1336 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TD1336;
            Tuner->tuner_Open = tuner_tunshdrv_Open_TD1336;
            Installed_TD1336 = TRUE;
            break;
         #endif
            #ifdef STTUNER_DRV_SHARED_TUN_FQD1236
             case STTUNER_TUNER_FQD1236:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s installing sat:tuner:FQD1236...", identity));
#endif
            if (Installed_FQD1236 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
                STTBX_Print(("%s fail already installed\n",identity));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_FQD1236;
            Tuner->tuner_Open = tuner_tunshdrv_Open_FQD1236;
            Installed_FQD1236 = TRUE;
            break;
            #endif
            #ifdef STTUNER_DRV_SHARED_TUN_T2000
           case STTUNER_TUNER_T2000:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s installing sat:tuner:FQD1236...", identity));
#endif
            if (Installed_T2000 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
                STTBX_Print(("%s fail already installed\n",identity));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_T2000;
            Tuner->tuner_Open = tuner_tunshdrv_Open_T2000;
            Installed_T2000 = TRUE;
            break;
            #endif

#ifdef STTUNER_DRV_SHARED_TUN_DTT7600
           case STTUNER_TUNER_DTT7600:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s installing :tuner:DTT7600...", identity));
#endif
            if (Installed_DTT7600 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
                STTBX_Print(("%s fail already installed\n",identity));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }

            Tuner->ID = STTUNER_TUNER_DTT7600;
            Tuner->tuner_Open = tuner_tunshdrv_Open_DTT7600;
            Installed_DTT7600 = TRUE;
            break;
            #endif

#ifdef STTUNER_DRV_SHARED_TUN_MT2060
           case STTUNER_TUNER_MT2060:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s installing :tuner:MT2060...", identity));
#endif
            if (Installed_MT2060 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
                STTBX_Print(("%s fail already installed\n",identity));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }

            Tuner->ID = STTUNER_TUNER_MT2060;
            Tuner->tuner_Open = tuner_tunshdrv_Open_MT2060;
            Installed_MT2060 = TRUE;
            break;
            #endif


#ifdef STTUNER_DRV_SHARED_TUN_MT2131
           case STTUNER_TUNER_MT2131:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s installing :tuner:MT2131...", identity));
#endif
            if (Installed_MT2131 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
                STTBX_Print(("%s fail already installed\n",identity));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }

            Tuner->ID = STTUNER_TUNER_MT2131;
            Tuner->tuner_Open = tuner_tunshdrv_Open_MT2131;
            Installed_MT2131 = TRUE;
            break;
            #endif



#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
           case STTUNER_TUNER_TDVEH052F:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s installing sat:tuner:TDVEH052F...", identity));
#endif
            if (Installed_TDVEH052F == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
                STTBX_Print(("%s fail already installed\n",identity));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }

            Tuner->ID = STTUNER_TUNER_TDVEH052F;
            Tuner->tuner_Open = tuner_tunshdrv_Open_TDVEH052F;
            Installed_TDVEH052F = TRUE;
            break;
            #endif
#ifdef STTUNER_DRV_SHARED_TUN_DTT761X
           case STTUNER_TUNER_DTT761X:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s installing sat:tuner:DTT761X...", identity));
#endif
            if (Installed_DTT761X == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
                STTBX_Print(("%s fail already installed\n",identity));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }

            Tuner->ID = STTUNER_TUNER_DTT761X;
            Tuner->tuner_Open = tuner_tunshdrv_Open_DTT761X;
            Installed_DTT761X = TRUE;
            break;
            #endif
            
            #ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
           case STTUNER_TUNER_DTT768XX:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s installing :tuner:DTT768XX...", identity));
#endif
            if (Installed_DTT768XX == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
                STTBX_Print(("%s fail already installed\n",identity));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }

            Tuner->ID = STTUNER_TUNER_DTT768XX;
            Tuner->tuner_Open = tuner_tunshdrv_Open_DTT768XX;
            Installed_DTT768XX = TRUE;
            break;
            #endif
            
         default:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s incorrect tuner index", identity));
#endif
            return(ST_ERROR_UNKNOWN_DEVICE);
            break;
     }

    /* map API */
    Tuner->tuner_Init  = tuner_tunshdrv_Init;
    Tuner->tuner_Term  = tuner_tunshdrv_Term;
    Tuner->tuner_Close = tuner_tunshdrv_Close;

    Tuner->tuner_SetFrequency  = tuner_tunshdrv_SetFrequency;
    Tuner->tuner_GetStatus     = tuner_tunshdrv_GetStatus;
    Tuner->tuner_IsTunerLocked = tuner_tunshdrv_IsTunerLocked;
    Tuner->tuner_SetBandWidth  = tuner_tunshdrv_SetBandWidth;

    Tuner->tuner_ioaccess = tuner_tunshdrv_ioaccess;

    Tuner->tuner_ioctl    = tuner_tunshdrv_ioctl;

   

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("ok\n"));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_TUNER_NONE_UnInstall()

Description:
    uninstall a shared device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_TUNER_TUNSHDRV_UnInstall(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STLNB tunshdrv.c STTUNER_DRV_TUNER_TUNSHDRV_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    switch(TunerType)
    {
    	#ifdef STTUNER_DRV_SHARED_TUN_TD1336
        case STTUNER_TUNER_TD1336:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s uninstalling ter:tuner:TD1336\n", identity));
#endif
            Installed_TD1336 = FALSE;
            break;
           #endif
            #ifdef STTUNER_DRV_SHARED_TUN_FQD1236
         case STTUNER_TUNER_FQD1236:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s uninstalling ter:tuner:FQD1236\n", identity));
#endif
            Installed_FQD1236 = FALSE;
            break;
             #endif
            #ifdef STTUNER_DRV_SHARED_TUN_T2000
            case STTUNER_TUNER_T2000:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s uninstalling ter:tuner:T2000\n", identity));
#endif
            Installed_T2000 = FALSE;
            break;
             #endif

             #ifdef STTUNER_DRV_SHARED_TUN_DTT7600
            case STTUNER_TUNER_DTT7600:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s uninstalling ter:tuner:DTT7600\n", identity));
#endif
            Installed_DTT7600 = FALSE;
            break;
             #endif

#ifdef STTUNER_DRV_SHARED_TUN_MT2060
            case STTUNER_TUNER_MT2060:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s uninstalling ter:tuner:MT2060\n", identity));
#endif
            Installed_MT2060 = FALSE;
            break;
             #endif


#ifdef STTUNER_DRV_SHARED_TUN_MT2131
            case STTUNER_TUNER_MT2131:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s uninstalling ter:tuner:MT2131\n", identity));
#endif
            Installed_MT2131 = FALSE;
            break;
             #endif


        #ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
            case STTUNER_TUNER_TDVEH052F:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s uninstalling ter:tuner:TDVEH052F\n", identity));
#endif
            Installed_TDVEH052F = FALSE;
            break;
             #endif
  #ifdef STTUNER_DRV_SHARED_TUN_DTT761X
            case STTUNER_TUNER_DTT761X:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s uninstalling ter:tuner:DTT761X\n", identity));
#endif
            Installed_DTT761X = FALSE;
            break;
#endif

#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
            case STTUNER_TUNER_DTT768XX:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s uninstalling ter:tuner:DTT768XX\n", identity));
#endif
            Installed_DTT768XX = FALSE;
            break;
             #endif


      default:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s incorrect tuner index", identity));
#endif
            return(ST_ERROR_UNKNOWN_DEVICE);
            break;
     }

    Tuner->ID = STTUNER_NO_DRIVER;

    /* unmap API */
    Tuner->tuner_Init  = NULL;
    Tuner->tuner_Term  = NULL;
    Tuner->tuner_Open  = NULL;
    Tuner->tuner_Close = NULL;

    Tuner->tuner_SetFrequency  = NULL;
    Tuner->tuner_GetStatus     = NULL;
    Tuner->tuner_IsTunerLocked = NULL;
    Tuner->tuner_SetBandWidth  = NULL;
    Tuner->tuner_ioaccess = NULL;
    Tuner->tuner_ioctl    = NULL;

    if(
    (Installed_TD1336 == FALSE)&&
    (Installed_FQD1236 == FALSE)&&
    (Installed_T2000 == FALSE)&&
    (Installed_DTT7600 == FALSE)&&
    (Installed_MT2060 == FALSE)&&
    (Installed_MT2131 == FALSE)&&
    (Installed_TDVEH052F == FALSE)&&
    (Installed_DTT761X == FALSE)&&
    (Installed_DTT768XX == FALSE))
     {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("<"));
#endif


       STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);


#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print((">"));
#endif
       InstanceChainTop = (TUNSHDRV_InstanceData_t *)0x7ffffffe;
       Installed = FALSE;
     }

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: tuner_tunshdrv_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunshdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_shdrv_Init()";
#endif

    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail no driver installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( TUNSHDRV_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);
    }

    /* slot into chain */
    if (InstanceChainTop == NULL)
    {
        InstanceNew->InstanceChainPrev = NULL; /* no previous instance */
        InstanceChainTop = InstanceNew;
    }
    else    /* tag onto last data block in chain */
    {
        Instance = InstanceChainTop;

        while(Instance->InstanceChainNext != NULL)
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }
        Instance->InstanceChainNext     = (void *)InstanceNew;
        InstanceNew->InstanceChainPrev  = (void *)Instance;
    }

    InstanceNew->DeviceName          = DeviceName;
    InstanceNew->TopLevelHandle      = STTUNER_MAX_HANDLES;
    InstanceNew->IOHandle            = InitParams->IOHandle;
    InstanceNew->MemoryPartition     = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
    InstanceNew->TunerType           = InitParams->TunerType;
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;;

    switch(InstanceNew->TunerType)
    {

        #ifdef STTUNER_DRV_SHARED_TUN_TD1336
        case STTUNER_TUNER_TD1336:
            InstanceNew->PLLType = TUNER_PLL_TD1336;
            break;
         #endif
         #ifdef STTUNER_DRV_SHARED_TUN_FQD1236
        case STTUNER_TUNER_FQD1236:
            InstanceNew->PLLType = TUNER_PLL_FQD1236;
            break;
            #endif
            #ifdef STTUNER_DRV_SHARED_TUN_T2000
         case STTUNER_TUNER_T2000:
            InstanceNew->PLLType = TUNER_PLL_T2000;
            break;
            #endif

            #ifdef STTUNER_DRV_SHARED_TUN_DTT7600
         case STTUNER_TUNER_DTT7600:
            InstanceNew->PLLType = TUNER_PLL_DTT7600;
            break;
            #endif
 #ifdef STTUNER_DRV_SHARED_TUN_MT2060
         case STTUNER_TUNER_MT2060:
            InstanceNew->PLLType = TUNER_PLL_MT2060;
            break;
            #endif

#ifdef STTUNER_DRV_SHARED_TUN_MT2131
         case STTUNER_TUNER_MT2131:
            InstanceNew->PLLType = TUNER_PLL_MT2131;
            break;
            #endif
              #ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
         case STTUNER_TUNER_TDVEH052F:
            InstanceNew->PLLType = TUNER_PLL_TDVEH052F;
            break;
            #endif
 #ifdef STTUNER_DRV_SHARED_TUN_DTT761X
         case STTUNER_TUNER_DTT761X:
            InstanceNew->PLLType = TUNER_PLL_DTT761X;
            break;
#endif

 #ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
         case STTUNER_TUNER_DTT768XX:
            InstanceNew->PLLType = TUNER_PLL_DTT768XX;
            break;
            #endif
            default:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s incorrect tuner index", identity));
#endif
            return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes) for  tuner ID=%d\n", identity, InstanceNew->DeviceName, InstanceNew, sizeof( TUNSHDRV_InstanceData_t ), InstanceNew->TunerType ));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: tuner_tunshdrv_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunshdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(TermParams);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("Looking (%s)", Instance->DeviceName));
#endif
    while(1)
    {
        if ( strcmp((char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("]\n"));
#endif
            /* found so now xlink prev and next(if applicable) and deallocate memory */
            InstancePrev = Instance->InstanceChainPrev;
            InstanceNext = Instance->InstanceChainNext;

            /* if instance to delete is first in chain */
            if (Instance->InstanceChainPrev == NULL)
            {
                InstanceChainTop = InstanceNext;        /* which would be NULL if last block to be term'd */
                if (InstanceNext != NULL)
                {
                InstanceNext->InstanceChainPrev = NULL; /* now top of chain, no previous instance */
                }
            }
            else
            {   /* safe to set value for prev instaance (because there IS one) */
                InstancePrev->InstanceChainNext = InstanceNext;
            }

            /* if there is a next block in the chain */
            if (InstanceNext != NULL)
            {
                InstanceNext->InstanceChainPrev = InstancePrev;
            }
            /*Deallocate memory for ioreg */
#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
            Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
           if (Error != ST_NO_ERROR)
            {
	   #ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s fail deallocate register database\n", identity));
	   #endif
           SEM_UNLOCK(Lock_InitTermOpenClose);
           return(Error);
           }
       }
#endif

            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);
            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance->TunerRegVal);
            
         #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
        STOS_MemoryDeallocate(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition, IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream);
        #endif 

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
                STTBX_Print(("\n%s fail no free handle before end of list\n", identity));
#endif
                SEM_UNLOCK(Lock_InitTermOpenClose);
                return(STTUNER_ERROR_INITSTATE);
        }
        else
        {
            Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        }

    }


#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: tuner_tunshdrv_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunshdrv_Open (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print((" found ok\n"));
#endif

    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}




/* ----------------------------------------------------------------------------
Name: tuner_tunshdrv_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunshdrv_Close(TUNER_Handle_t Handle, TUNER_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_Close()";
#endif

    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;

    Instance = TUNSHDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}




/* ----------------------------------------------------------------------------
Name: tuner_tunshdrv_SetFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunshdrv_SetFrequency (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_SetFrequency()";
#endif

    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;
    #ifdef STTUNER_DRV_SHARED_TUN_FQD1236
     U32 P0_1_2,AL2,P4;
     #endif
     #ifdef STTUNER_DRV_SHARED_TUN_DTT7600
     U32 atc,al;
     #endif
    
    #if defined (STTUNER_DRV_SHARED_TUN_DTT7600) || defined (STTUNER_DRV_SHARED_TUN_FQD1236)|| defined (STTUNER_DRV_SHARED_TUN_TD1336)|| defined (STTUNER_DRV_SHARED_TUN_T2000)|| defined (STTUNER_DRV_SHARED_TUN_TDVEH052F)|| defined (STTUNER_DRV_SHARED_TUN_DTT761X) || defined (STTUNER_DRV_SHARED_TUN_DTT768XX) 
    U32 divider,value1,value2;
    #endif
     #if defined (STTUNER_DRV_SHARED_TUN_DTT7600) || defined (STTUNER_DRV_SHARED_TUN_FQD1236)|| defined (STTUNER_DRV_SHARED_TUN_TD1336)|| defined (STTUNER_DRV_SHARED_TUN_T2000)|| defined (STTUNER_DRV_SHARED_TUN_TDVEH052F)|| defined (STTUNER_DRV_SHARED_TUN_DTT761X) || defined (STTUNER_DRV_SHARED_TUN_DTT768XX) ||defined (STTUNER_DRV_SHARED_TUN_MT2060) || defined (STTUNER_DRV_SHARED_TUN_MT2131)
    U32 frequency;
    #endif
 #if defined (STTUNER_DRV_SHARED_TUN_MT2060) || defined (STTUNER_DRV_SHARED_TUN_MT2131)
U32 mt_ferror=0,mt_id=0;
   
#endif

    Instance = TUNSHDRV_GetInstFromHandle(Handle);


   if(!Instance->Error)
      {
      	switch(Instance->TunerType)
		{
			#ifdef STTUNER_DRV_SHARED_TUN_TD1336
			case STTUNER_TUNER_TD1336:

				frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (SharedTunerGetStepsize(Instance)  / 10);

				ChipSetFieldImage(FTD1336_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);
				ChipSetFieldImage(FTD1336_N_LSB,((divider ) & 0xFF),Instance->TunerRegVal);


				if 	(Frequency <= 164000)

					ChipSetFieldImage(FTD1336_SP,0x01,Instance->TunerRegVal);

				else if (Frequency <= 458000)

					ChipSetFieldImage(FTD1336_SP,0x02,Instance->TunerRegVal);
				else if (Frequency <= 865000)

					ChipSetFieldImage(FTD1336_SP,0x04,Instance->TunerRegVal);
              #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
              if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	          {
				Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RTD1336_DIV1,Instance->TunerRegVal,4);
			   }
			    #endif
			    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
               if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
              {
              	Error = ChipSetRegisters(Instance->IOHandle,RTD1336_DIV1,Instance->TunerRegVal,4); 	 
               }
               #endif
				if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif

        				return(Error);
    				  }
    				 divider =0;
    				 frequency =0;

				value1=ChipGetFieldImage(FTD1336_N_MSB,Instance->TunerRegVal)<<8;
		 		value2=ChipGetFieldImage(FTD1336_N_LSB,Instance->TunerRegVal);

				 divider=value1+value2;

				frequency = (divider*SharedTunerGetStepsize(Instance) )/1000 - Instance->Status.IntermediateFrequency;
				*NewFrequency = Instance->Status.Frequency = frequency;
			        break;
			       #endif
			       #ifdef STTUNER_DRV_SHARED_TUN_FQD1236
			   case STTUNER_TUNER_FQD1236:
			   	frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (SharedTunerGetStepsize(Instance)  / 10);


				ChipSetFieldImage(FFQD1236_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);
				ChipSetFieldImage(FFQD1236_N_LSB,((divider ) & 0xFF),Instance->TunerRegVal);

				if 	(Frequency <= 55000)

					ChipSetFieldImage(FFQD1236_P0_1_2,0x01,Instance->TunerRegVal);

				else if (Frequency <= 442000)

					ChipSetFieldImage(FFQD1236_P0_1_2,0x02,Instance->TunerRegVal);

				else if (Frequency <= 804000)

					ChipSetFieldImage(FFQD1236_P0_1_2,0x04,Instance->TunerRegVal);


				/*Assuming this tuner to be digital tuner now*/
				AL2=0;
				P4=0;

				P0_1_2=ChipGetFieldImage(FFQD1236_P0_1_2,Instance->TunerRegVal);



				ChipSetFieldImage(FFQD1236_AL0_P4,P4,Instance->TunerRegVal);


				ChipSetFieldImage(FFQD1236_AL1,0x0,Instance->TunerRegVal);


				ChipSetFieldImage(FFQD1236_AL2,0x0,Instance->TunerRegVal);


				ChipSetFieldImage(FFQD1236_ATC,0x0,Instance->TunerRegVal);


				ChipSetFieldImage(FFQD1236_T,0x0,Instance->TunerRegVal);

              #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
             if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	          {
				Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RFQD1236_DIV1,Instance->TunerRegVal,4);
			   }
			   #endif
			   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
              if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
              { 				
             Error = ChipSetRegisters(Instance->IOHandle,RFQD1236_DIV1,Instance->TunerRegVal,4);
               }
             #endif
				
				if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif

        				return(Error);
    				  }
				WAIT_N_MS_SHTUNER(10);


				ChipSetFieldImage(FFQD1236_P0_1_2,0,Instance->TunerRegVal);
				ChipSetFieldImage(FFQD1236_AL0_P4,0,Instance->TunerRegVal);
				ChipSetFieldImage(FFQD1236_AL1,0x1,Instance->TunerRegVal);
				ChipSetFieldImage(FFQD1236_AL2,AL2,Instance->TunerRegVal);
				ChipSetFieldImage(FFQD1236_ATC,0x1,Instance->TunerRegVal);
				ChipSetFieldImage(FFQD1236_T,0x3,Instance->TunerRegVal);




              #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
              if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	            { 
				Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RFQD1236_DIV1,Instance->TunerRegVal,4);
              }
              #endif
              #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
               if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
              {
              Error = ChipSetRegisters(Instance->IOHandle,RFQD1236_DIV1,Instance->TunerRegVal,4);
              }
              #endif
              	
				if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif

        				return(Error);
    				  }
    				 divider =0;
    				 frequency =0;


				value1=ChipGetFieldImage(FFQD1236_N_MSB,Instance->TunerRegVal)<<8;
		 		value2=ChipGetFieldImage(FFQD1236_N_LSB,Instance->TunerRegVal);

				 divider=value1+value2;

				frequency = (divider*SharedTunerGetStepsize(Instance) )/1000 - Instance->Status.IntermediateFrequency;
				*NewFrequency = Instance->Status.Frequency = frequency;
			        break;
			        #endif
			      #ifdef STTUNER_DRV_SHARED_TUN_T2000
			     case STTUNER_TUNER_T2000:
			   	frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (SharedTunerGetStepsize(Instance)  / 10);


				ChipSetFieldImage(FT2000_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);
				ChipSetFieldImage(FT2000_N_LSB,((divider ) & 0xFF),Instance->TunerRegVal);

				if (frequency <= 201000) /*Check is it Frequency or frequency*/
				{

					ChipSetFieldImage(FT2000_BS,0,Instance->TunerRegVal);
					ChipSetFieldImage(FT2000_P_3_2,0x03,Instance->TunerRegVal);

					if(frequency <=180000)
					{

						ChipSetFieldImage(FT2000_C,0,Instance->TunerRegVal);

					}
					else if(frequency <= 195000)
					{

						ChipSetFieldImage(FT2000_C,1,Instance->TunerRegVal);

					}
					else
					{

						ChipSetFieldImage(FT2000_C,2,Instance->TunerRegVal);

					}
				}
				else if (frequency <= 472000)
				{

					ChipSetFieldImage(FT2000_BS,1,Instance->TunerRegVal);
					ChipSetFieldImage(FT2000_P_3_2,0x02,Instance->TunerRegVal);

					if(frequency <=345000)
					{

						ChipSetFieldImage(FT2000_C,0,Instance->TunerRegVal);

					}
					else if(frequency <= 420000)
					{

						ChipSetFieldImage(FT2000_C,1,Instance->TunerRegVal);

					}
					else if(frequency <= 460000)
					{

						ChipSetFieldImage(FT2000_C,2,Instance->TunerRegVal);

					}
					else
					{

						ChipSetFieldImage(FT2000_C,3,Instance->TunerRegVal);

					}

				}
				else if (frequency <= 902000)
				{

					ChipSetFieldImage(FT2000_BS,2,Instance->TunerRegVal);
					ChipSetFieldImage(FT2000_P_3_2,0x01,Instance->TunerRegVal);

					if(frequency <=514000)
					{

						ChipSetFieldImage(FT2000_C,0,Instance->TunerRegVal);

					}
					else if(frequency <= 745000)
					{

						ChipSetFieldImage(FT2000_C,1,Instance->TunerRegVal);

					}
					else if(frequency <= 820000)
					{

						ChipSetFieldImage(FT2000_C,2,Instance->TunerRegVal);

					}
					else
					{

						ChipSetFieldImage(FT2000_C,3,Instance->TunerRegVal);

					}

				}
 #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{             
				Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,0,Instance->TunerRegVal,4);
    }
 #endif
 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	  Error = ChipSetRegisters(Instance->IOHandle,0,Instance->TunerRegVal,4);
   	}
 #endif
	
				if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif

        				return(Error);
    				  }
    				 divider =0;
    				 frequency =0;


				value1=ChipGetFieldImage(FT2000_N_MSB,Instance->TunerRegVal)<<8;
		 		value2=ChipGetFieldImage(FT2000_N_LSB,Instance->TunerRegVal);

				 divider=value1+value2;
				frequency = (divider*SharedTunerGetStepsize(Instance) )/1000 - Instance->Status.IntermediateFrequency;
				*NewFrequency = Instance->Status.Frequency = frequency;
			        break;
			        #endif

			    /***************************/

			    #ifdef STTUNER_DRV_SHARED_TUN_DTT7600
			case STTUNER_TUNER_DTT7600:

				frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (SharedTunerGetStepsize(Instance)  / 10);

				ChipSetFieldImage(FDTT7600_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);

				ChipSetFieldImage(FDTT7600_N_LSB,((divider) & 0xFF),Instance->TunerRegVal);

				if 	(Frequency <= 147000)
				ChipSetFieldImage(FDTT7600_P,0x09,Instance->TunerRegVal);
				else if (Frequency <= 417000)
				ChipSetFieldImage(FDTT7600_P,0x0A,Instance->TunerRegVal);
				else if (Frequency <= 863000)
				ChipSetFieldImage(FDTT7600_P,0x0C,Instance->TunerRegVal);


				atc=ChipGetFieldImage(FDTT7600_ATC,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT7600_ATC,0,Instance->TunerRegVal);
				al=ChipGetFieldImage(FDTT7600_AL,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT7600_AL,0,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT7600_T,0,Instance->TunerRegVal);
 #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{        
				Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTT7600_P_DIV1,Instance->TunerRegVal,4);
    }
 #endif
 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 	
   					Error = ChipSetRegisters(Instance->IOHandle,RDTT7600_P_DIV1,Instance->TunerRegVal,4);
   }
 #endif   
   	   
				WAIT_N_MS_SHTUNER(10);

				ChipSetFieldImage(FDTT7600_AL,al,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT7600_ATC,atc,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT7600_T,0x3,Instance->TunerRegVal);

#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
				Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTT7600_P_DIV1,Instance->TunerRegVal,4);
    }
#endif
#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   {  
   	    Error = ChipSetRegisters(Instance->IOHandle,RDTT7600_P_DIV1,Instance->TunerRegVal,4);
   	}
#endif
				if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif

        				return(Error);
    				  }


    				 divider =0;
    				 frequency =0;


				value1=ChipGetFieldImage(FDTT7600_N_MSB,Instance->TunerRegVal)<<8;
		 		value2=ChipGetFieldImage(FDTT7600_N_LSB,Instance->TunerRegVal);

				 divider=value1+value2;


				frequency = (divider*SharedTunerGetStepsize(Instance) )/1000 - Instance->Status.IntermediateFrequency;
				*NewFrequency = Instance->Status.Frequency = frequency;
				break;
			#endif



#ifdef STTUNER_DRV_SHARED_TUN_MT2060

			case STTUNER_TUNER_MT2060:



frequency = Frequency *1000;
if ( (frequency < 50000000) || (frequency > 1100000000) )
		mt_ferror= 1;
#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
 mt_id=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle,PART_REV);
  }
#endif
#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
     mt_id=ChipGetOneRegister(Instance->IOHandle,PART_REV);
   }
 #endif
				 if ( (mt_id != 0x63 ) || mt_ferror) {
 				#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        				STTBX_Print(("%s fail, MT2060 tuner  I2C Acknowledhgement Error\n", identity));
				 #endif 							/* Not really, but only way to get the Panel "red */
					 break;
				 }
#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
				 STTUNER_IOREG_SetRegister(&(Instance->DeviceMap), Instance->IOHandle,MISC_CTRL_3,0x33);
    }
 #endif
#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	 ChipSetOneRegister(Instance->IOHandle,MISC_CTRL_3,0x33);
   }
 #endif
 
				 Instance->FirstIF=Instance->FirstIF*1e6; /* desired 1st IF frequency    */
				 Instance->Status.IntermediateFrequency=Instance->Status.IntermediateFrequency*1000; /* IF output center frequency  */
				Instance->ChannelBW=(Instance->ChannelBW*1000000)+375000;/* IF output bandwidth + guard */


MT2060_ChangeFreq(Instance,frequency);


				*NewFrequency = Frequency;/*Update with a good parameter*/
				break;
	#endif




#ifdef STTUNER_DRV_SHARED_TUN_MT2131

			case STTUNER_TUNER_MT2131:


frequency = Frequency *1000;
if ( (frequency < 50000000) || (frequency > 1100000000) )
		mt_ferror= 1;
#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
 mt_id=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle,MT2131_PART_REV);
  }
#endif
#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	mt_id=ChipGetOneRegister(Instance->IOHandle,MT2131_PART_REV);
  }
#endif	   
				 if ( (mt_id != 0x3e && mt_id != 0x3f) || mt_ferror) {
 				#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        				STTBX_Print(("%s fail, MT2131 tuner  I2C Acknowledhgement Error\n", identity));
				 #endif 							/* Not really, but only way to get the Panel "red */
					 break;
				 }


			MT2131_SetParam(Instance,MT2131_RCVR_MODE,2);
			 MT2131_SetParam(Instance,MT2131_OUTPUT_FREQ,(/*hTuner->IF*1000*/Instance->Status.IntermediateFrequency*1000));
			 MT2131_SetParam(Instance,MT2131_IF1_CENTER,(1217500000));
			 MT2131_ChangeFreq(Instance,frequency,(8750000));

				*NewFrequency = Frequency;/*Update with a good parameter*/

			break;
	#endif


			          #ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
			case STTUNER_TUNER_TDVEH052F:

				frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (SharedTunerGetStepsize(Instance)  / 10);

				ChipSetFieldImage(FTDVEH052F_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);

				ChipSetFieldImage(FTDVEH052F_N_LSB,((divider) & 0xFF),Instance->TunerRegVal);

				if 	(Frequency <= 165000)
				ChipSetFieldImage(FTDVEH052F_P3,0x1,Instance->TunerRegVal);
				else if (Frequency <= 450000)
				ChipSetFieldImage(FTDVEH052F_P3,0x02,Instance->TunerRegVal);
				else if (Frequency <= 864000)
				ChipSetFieldImage(FTDVEH052F_P3,0x04,Instance->TunerRegVal);
#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
				Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RTDVEH052F_DIV1,Instance->TunerRegVal,4);
    }
  #endif
  
#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   					Error = ChipSetRegisters(Instance->IOHandle,RTDVEH052F_DIV1,Instance->TunerRegVal,4);
  }  
  #endif


    				 divider =0;
    				 frequency =0;


				value1=ChipGetFieldImage(FTDVEH052F_N_MSB,Instance->TunerRegVal)<<8;
		 		value2=ChipGetFieldImage(FTDVEH052F_N_LSB,Instance->TunerRegVal);

				 divider=value1+value2;


				frequency = (divider*SharedTunerGetStepsize(Instance) )/1000 - Instance->Status.IntermediateFrequency;
				*NewFrequency = Instance->Status.Frequency = frequency;
				break;
			#endif

    #ifdef STTUNER_DRV_SHARED_TUN_DTT761X
			case STTUNER_TUNER_DTT761X:
				
				frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (SharedTunerGetStepsize(Instance)/ 10);
			
				ChipSetFieldImage(FDTT761X_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);
				ChipSetFieldImage(FDTT761X_N_LSB,((divider ) & 0xFF),Instance->TunerRegVal);
				ChipSetFieldImage(FDTT761X_ZERO1,0,Instance->TunerRegVal);
				
				ChipSetFieldImage(FDTT761X_N_MSB_2,((divider >> 8) & 0x7F),Instance->TunerRegVal);
				ChipSetFieldImage(FDTT761X_N_LSB_2,((divider ) & 0xFF),Instance->TunerRegVal);
				ChipSetFieldImage(FDTT761X_ZERO_2,0,Instance->TunerRegVal);  
				
				
				ChipSetFieldImage(FDTT761X_OS,0x0,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT761X_P5,0x1,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT761X_P4,0x1,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT761X_P3,0x1,Instance->TunerRegVal);

				if 	    (Frequency <= 147000)
				{
					ChipSetFieldImage(FDTT761X_P2_0,0x1,Instance->TunerRegVal);
				}
				else if (Frequency <= 417000)
				{
					ChipSetFieldImage(FDTT761X_P2_0,0x2,Instance->TunerRegVal);
				}
				else if (Frequency <= 863000)
				{
					ChipSetFieldImage(FDTT761X_P2_0,0x4,Instance->TunerRegVal);
				}
#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{		
			  STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTT761X_P_DIV1,Instance->TunerRegVal,4); 
	}
#endif
#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   			  ChipSetRegisters(Instance->IOHandle,RDTT761X_P_DIV1,Instance->TunerRegVal,4); 	
  }
#endif

				WAIT_N_MS_SHTUNER(10);
				ChipSetFieldImage(FDTT761X_T21_2,1,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT761X_T0_2, 1,Instance->TunerRegVal);
#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
				STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTT761X_P_DIV1_2,Instance->TunerRegVal,4); 
    }
 #endif
#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	ChipSetRegisters(Instance->IOHandle,RDTT761X_P_DIV1_2,Instance->TunerRegVal,4); 
  }
  #endif
 	
				
				divider =0;
    		frequency =0;


				value1=ChipGetFieldImage(FDTT761X_N_MSB,Instance->TunerRegVal)<<8;
		 		value2=ChipGetFieldImage(FDTT761X_N_LSB,Instance->TunerRegVal);

				 divider=value1+value2;


				frequency = (divider*SharedTunerGetStepsize(Instance) )/1000 - Instance->Status.IntermediateFrequency;
				*NewFrequency = Instance->Status.Frequency = frequency;		

				break;
			#endif



 #ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
			case STTUNER_TUNER_DTT768XX:

				frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (SharedTunerGetStepsize(Instance)  / 10);

				ChipSetFieldImage(FDTT768xx_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);

				ChipSetFieldImage(FDTT768xx_N_LSB,((divider) & 0xFF),Instance->TunerRegVal);
				ChipSetFieldImage(FDTT768xx_SL10,0x3,Instance->TunerRegVal);

				if 	(Frequency <= 147000)
				
				{
				ChipSetFieldImage(FDTT768xx_BS10,0x0,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT768xx_P30 ,0x9,Instance->TunerRegVal);
			}
				else if (Frequency <= 417000)
				{
			ChipSetFieldImage(FDTT768xx_BS10,0x1,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT768xx_P30 ,0xc,Instance->TunerRegVal);
			}
				else if (Frequency <= 863000)
				{
			ChipSetFieldImage(FDTT768xx_BS10,0x2,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT768xx_P30 ,0x5,Instance->TunerRegVal);
			}


				
 #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{        
				Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTT768xx_P_DIV1,Instance->TunerRegVal,6);
    }
 #endif
 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 	
   					Error = ChipSetRegisters(Instance->IOHandle,RDTT768xx_P_DIV1,Instance->TunerRegVal,6);
   					
   }
 #endif   
   	   
			
    				 divider =0;
    				 frequency =0;


				value1=ChipGetFieldImage(FDTT768xx_N_MSB,Instance->TunerRegVal)<<8;
		 		value2=ChipGetFieldImage(FDTT768xx_N_LSB,Instance->TunerRegVal);


           
				 divider=value1+value2;


				frequency = (divider*SharedTunerGetStepsize(Instance) )/1000 - Instance->Status.IntermediateFrequency;
				*NewFrequency = Instance->Status.Frequency = frequency;
				break;
			#endif


			default:
			/*Do nothing now. but return error value later on */
			        break;
	}
      }


#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s ok\n", identity));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: tuner_tunshdrv_GetStatus()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunshdrv_GetStatus(TUNER_Handle_t Handle, TUNER_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_GetStatus()";
#endif
     TUNSHDRV_InstanceData_t *Instance;
     ST_ErrorCode_t Error= ST_NO_ERROR;

    Instance = TUNSHDRV_GetInstFromHandle(Handle);

    switch (Instance->TunerType)
    {
        #ifdef STTUNER_DRV_SHARED_TUN_TD1336
        case STTUNER_TUNER_TD1336:

 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
        break;
        #endif
        #ifdef STTUNER_DRV_SHARED_TUN_FQD1236
        case STTUNER_TUNER_FQD1236:

 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
        break;
        #endif
        #ifdef STTUNER_DRV_SHARED_TUN_T2000
        case STTUNER_TUNER_T2000:

 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
        break;
	#endif

	#ifdef STTUNER_DRV_SHARED_TUN_DTT7600
        case STTUNER_TUNER_DTT7600:

 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
        break;
	#endif
	#ifdef STTUNER_DRV_SHARED_TUN_MT2060
        case STTUNER_TUNER_MT2060:

 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
        break;
	#endif

#ifdef STTUNER_DRV_SHARED_TUN_MT2131
        case STTUNER_TUNER_MT2131:

 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
        break;
	#endif

	#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
        case STTUNER_TUNER_TDVEH052F:

 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
        break;
	#endif
	#ifdef STTUNER_DRV_SHARED_TUN_DTT761X
        case STTUNER_TUNER_DTT761X:

 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
        break;
	#endif
#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
        case STTUNER_TUNER_DTT768XX:

 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
        break;
	#endif
        default:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s WARNING - no TUNER specified\n",identity));
#endif
        break;
    }

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s GetStatus (%d) ok\n", identity, Handle));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: tuner_tunshdrv_IsTunerLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunshdrv_IsTunerLocked(TUNER_Handle_t Handle, BOOL *Locked)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_IsTunerLocked()";
#endif

    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;

    Instance = TUNSHDRV_GetInstFromHandle(Handle);

/*????Need to read tuner to get the correct update of the tuner status*/
    switch(Instance->TunerType)
    {
    	#ifdef STTUNER_DRV_SHARED_TUN_TD1336
        case STTUNER_TUNER_TD1336:
    		*Locked = FALSE;    /* Assume not locked */
#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
    		Error = STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RTD1336_STATUS,1,Instance->TunerRegVal);
    }
 #endif	
 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	    		Error = ChipGetRegisters(Instance->IOHandle,RTD1336_STATUS,1,Instance->TunerRegVal);
  }
#endif	           
   	         
 
    		
    		if( Error != ST_NO_ERROR)
    		 {
		  #ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        		STTBX_Print(("%s fail, error=%d\n", identity, Error));
		  #endif

        	   return(Error);
    		 }
    		*Locked = ChipGetFieldImage(FTD1336_FL,Instance->TunerRegVal);
    	        break;
    	    #endif
    	        #ifdef STTUNER_DRV_SHARED_TUN_FQD1236
    	case STTUNER_TUNER_FQD1236:
    		*Locked = FALSE;    /* Assume not locked */
  #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
    		Error = STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RFQD1236_STATUS,1,Instance->TunerRegVal);
    }
  #endif
  #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	Error = ChipGetRegisters(Instance->IOHandle,RFQD1236_STATUS,1,Instance->TunerRegVal);
  }
#endif	 
  
    		if( Error != ST_NO_ERROR)
    		 {
		  #ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        		STTBX_Print(("%s fail, error=%d\n", identity, Error));
		  #endif

        	   return(Error);
    		 }
    		*Locked=ChipGetFieldImage(FFQD1236_FL,Instance->TunerRegVal);

    	         break;
    	     #endif
    	         #ifdef STTUNER_DRV_SHARED_TUN_T2000
    	case STTUNER_TUNER_T2000:
    		*Locked = FALSE;    /* Assume not locked */
#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{ 
    		Error = STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RT2000_STATUS,1,Instance->TunerRegVal);
    }
#endif
#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	Error = ChipGetRegisters(Instance->IOHandle,RT2000_STATUS,1,Instance->TunerRegVal);
  }
#endif	
    		if( Error != ST_NO_ERROR)
    		{
		  #ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        		STTBX_Print(("%s fail, error=%d\n", identity, Error));
		  #endif

        	   return(Error);
    		}
    		*Locked=ChipGetFieldImage(FT2000_FL,Instance->TunerRegVal);

    	         break;
    	         #endif
    	         /***********************/

    	         #ifdef STTUNER_DRV_SHARED_TUN_DTT7600
        case STTUNER_TUNER_DTT7600:
    		*Locked = FALSE;    /* Assume not locked */
 #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
    		 Error =  STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTT7600_STATUS,1,Instance->TunerRegVal);
   }
 #endif
 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	    		 Error =  ChipGetRegisters(Instance->IOHandle,RDTT7600_STATUS,1,Instance->TunerRegVal);
  }
#endif	 
 
 		if (Error == ST_NO_ERROR)
    		{
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   		 STTBX_Print(("%s Error during reading tuner  ...\n", identity));
		#endif
    		}

    	     	 *Locked=ChipGetFieldImage(FDTT7600_FL,Instance->TunerRegVal);

    	        break;
    	        #endif


 #ifdef STTUNER_DRV_SHARED_TUN_MT2060
        case STTUNER_TUNER_MT2060:
    		*Locked = FALSE;    /* Assume not locked */

    		    *Locked = MT2060_GetLocked(Instance);
    	        break;
    	        #endif

 #ifdef STTUNER_DRV_SHARED_TUN_MT2131
        case STTUNER_TUNER_MT2131:
    		*Locked = FALSE;    /* Assume not locked */

    		    *Locked = MT2131_GetLocked(Instance);
    	        break;
    	        #endif



    	        #ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
        case STTUNER_TUNER_TDVEH052F:
    		*Locked = FALSE;    /* Assume not locked */
   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
    		 Error =  STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RTDVEH052F_STATUS,1,Instance->TunerRegVal);
 	}
  #endif
  #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	    		 Error =  ChipGetRegisters(Instance->IOHandle,RTDVEH052F_STATUS,1,Instance->TunerRegVal);

  }
#endif
 		if (Error == ST_NO_ERROR)
    		{
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   		 STTBX_Print(("%s Error during reading tuner  ...\n", identity));
		#endif
    		}

    	     	 *Locked=ChipGetFieldImage(FTDVEH052F_FL,Instance->TunerRegVal);

    	        break;
    	        #endif

#ifdef STTUNER_DRV_SHARED_TUN_DTT761X
        case STTUNER_TUNER_DTT761X:
    		*Locked = FALSE;    /* Assume not locked */
  #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
    		 Error =  STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTT761X_STATUS,1,Instance->TunerRegVal);
   }
  #endif
  #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	    		 Error =  ChipGetRegisters(Instance->IOHandle,RDTT761X_STATUS,1,Instance->TunerRegVal);
  }
#endif	           
   	        
 		
 		if (Error == ST_NO_ERROR)
    		{
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   		 STTBX_Print(("%s Error during reading tuner  ...\n", identity));
		#endif
    		}

    	     	 *Locked=ChipGetFieldImage(FDTT761X_AFC,Instance->TunerRegVal);

    	        break;
    	        #endif

 #ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
        case STTUNER_TUNER_DTT768XX:
    		*Locked = FALSE;    /* Assume not locked */
 #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
    		 Error =  STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTT768xx_STATUS,1,Instance->TunerRegVal);
   }
 #endif
 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	    		 Error =  ChipGetRegisters(Instance->IOHandle,RDTT768xx_STATUS,1,Instance->TunerRegVal);
  }
#endif	 
 
 		if (Error == ST_NO_ERROR)
    		{
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   		 STTBX_Print(("%s Error during reading tuner  ...\n", identity));
		#endif
    		}

    	     	 *Locked=ChipGetFieldImage(FDTT768xx_FL,Instance->TunerRegVal);

    	        break;
    	        #endif



    	         /**********************/
        default:
        /*Do nothing now*/
               break;
    }

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s Locked: ", identity));
    if (*Locked == TRUE)
    {
        STTBX_Print(("Yes\n"));
    }
    else
    {
        STTBX_Print(("no\n"));
    }
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: tuner_tunshdrv_SetBandWidth()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunshdrv_SetBandWidth(TUNER_Handle_t Handle, U32 BandWidth, U32 *NewBandWidth)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_SetBandWidth()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s SetBandWidth (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: tuner_tunshdrv_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunshdrv_ioctl(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_ioctl()";
#endif
    ST_ErrorCode_t Error=ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;
    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
    U8 Buffer[30];
    U32 Size;
    U8  SubAddress;
    #endif

    Instance = TUNSHDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s tuner driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function)
    {
        case STTUNER_IOCTL_RAWIO: /* raw I/O to device */
					      #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
					      if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					   	    {
					            Error =  STTUNER_IOARCH_ReadWrite( Instance->IOHandle,
					                                                 ((SHIOCTL_IOARCH_Params_t *)InParams)->Operation,
					                                                 ((SHIOCTL_IOARCH_Params_t *)InParams)->SubAddr,
					                                                 ((SHIOCTL_IOARCH_Params_t *)InParams)->Data,
					                                                 ((SHIOCTL_IOARCH_Params_t *)InParams)->TransferSize,
					                                                 ((SHIOCTL_IOARCH_Params_t *)InParams)->Timeout );
					         }
					      #endif
					      #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
					      SubAddress = (U8)(((SHIOCTL_IOARCH_Params_t *)InParams)->SubAddr & 0xFF);
					      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
					     { 
 					       	switch(((SHIOCTL_IOARCH_Params_t *)InParams)->Operation)
 					  	       {
							  	   case STTUNER_IO_SA_READ_NS:
							            Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, ((SHIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
				                       Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((SHIOCTL_IOARCH_Params_t *)InParams)->Data,((SHIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((SHIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							
							     case STTUNER_IO_SA_READ:
							           Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, ((SHIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size); /* fix for cable (297 chip) */
							            /* fallthrough (no break;) */
							     case STTUNER_IO_READ:
							           Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((SHIOCTL_IOARCH_Params_t *)InParams)->Data,((SHIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((SHIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
				                case STTUNER_IO_SA_WRITE_NS:
							            Buffer[0] = SubAddress;
							            memcpy( (Buffer + 1), ((SHIOCTL_IOARCH_Params_t *)InParams)->Data, ((SHIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
							            Error = STI2C_WriteNoStop(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, ((SHIOCTL_IOARCH_Params_t *)InParams)->TransferSize+1, ((SHIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     case STTUNER_IO_SA_WRITE:				         
							           Buffer[0] = SubAddress;
								        memcpy( (Buffer + 1), ((SHIOCTL_IOARCH_Params_t *)InParams)->Data, ((SHIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
								        Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, ((SHIOCTL_IOARCH_Params_t *)InParams)->TransferSize+1, ((SHIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     case STTUNER_IO_WRITE:
							            Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((SHIOCTL_IOARCH_Params_t *)InParams)->Data, ((SHIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((SHIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     default:
							            Error = STTUNER_ERROR_ID;
							            break;
 				   
 				               }
				        }
				       #endif	 
     break;

     default:
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }



#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s function %d called\n", identity, Function));
#endif
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: tuner_tunshdrv_ioaccess()

Description:
    we get called with the instance of
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tunshdrv_ioaccess(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_ioaccess()";
#endif

    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;
    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
    U8 Buffer[30];
    U32 Size;
    U8  SubAddress;
    #endif

    Instance = TUNSHDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

        /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
    	 #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
      if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	    {
          Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
         }
      #endif
      #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
       SubAddress = (U8)(SubAddr & 0xFF);
					      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
					     { 
 					       	switch(Operation)
 					  	       {
							  	   case STTUNER_IO_SA_READ_NS:
							            Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, Timeout, &Size);
				                       Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data,TransferSize, Timeout, &Size);
							            break;
							
							     case STTUNER_IO_SA_READ:
							           Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, Timeout, &Size); /* fix for cable (297 chip) */
							            /* fallthrough (no break;) */
							     case STTUNER_IO_READ:
							           Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data,TransferSize, Timeout, &Size);
							            break;
				                case STTUNER_IO_SA_WRITE_NS:
							            Buffer[0] = SubAddress;
							            memcpy( (Buffer + 1), Data, TransferSize);
							            Error = STI2C_WriteNoStop(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, TransferSize+1, Timeout, &Size);
							            break;
							     case STTUNER_IO_SA_WRITE:				         
							           Buffer[0] = SubAddress;
								        memcpy( (Buffer + 1), Data, TransferSize);
								        Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, TransferSize+1, Timeout, &Size);
							            break;
							     case STTUNER_IO_WRITE:
							            Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data, TransferSize, Timeout, &Size);
							            break;
							     default:
							            Error = STTUNER_ERROR_ID;
							            break;
 				   
 				               }
				        }
	    #endif   
    }
    else    /* repeater */
    {
        Error = ST_ERROR_FEATURE_NOT_SUPPORTED; /* not supported for this tuner */
    }


#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
    STTBX_Print(("%s called\n", identity));
#endif
    return(Error);
}


/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_TD1336()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_TD1336
ST_ErrorCode_t tuner_tunshdrv_Open_TD1336(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_Open_TD1336()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;
     int i;
     Error = tuner_tunshdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSHDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_TD1336)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TD1336 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

     Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  TD1336_NBREGS, sizeof( U8 ));



for ( i=0;i<TD1336_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=TD1336_DefVal[i];

	}



   Instance->Status.IntermediateFrequency =44000;
  #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM) 
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = TD1336_NBREGS;
   Instance->DeviceMap.Fields = TD1336_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   Instance->DeviceMap.WrStart =RTD1336_DIV1;
   Instance->DeviceMap.WrSize = 4;
   Instance->DeviceMap.RdStart = RTD1336_STATUS;
   Instance->DeviceMap.RdSize =1;
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   /*Check for error condition*/
if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
   Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,TD1336_DefVal,TD1336_Address);
  }
   #endif
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = TD1336_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields = TD1336_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RTD1336_DIV1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RTD1336_STATUS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,TD1336_NBREGS+1,sizeof(U8));
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	Error = ChipSetRegisters(Instance->IOHandle,0,TD1336_DefVal,TD1336_NBREGS);
  }
   #endif
   
   
    /* now safe to unlock semaphore */
  SEM_UNLOCK(Lock_InitTermOpenClose);

  if (Error == ST_NO_ERROR)
   {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
    return(Error);
}
#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_FQD1236()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_FQD1236
ST_ErrorCode_t tuner_tunshdrv_Open_FQD1236(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_Open_FQD1236()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;
    int i;

    Error = tuner_tunshdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSHDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_FQD1236)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_FQD1236 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }


    Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  FQD1236_NBREGS, sizeof( U8 ));



for ( i=0;i<FQD1236_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=FQD1236_DefVal[i];

	}


   Instance->Status.IntermediateFrequency =44000;
   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = FQD1236_NBREGS;
   Instance->DeviceMap.Fields = FQD1236_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   Instance->DeviceMap.WrStart =RFQD1236_DIV1;
   Instance->DeviceMap.WrSize = 4;
   Instance->DeviceMap.RdStart = RFQD1236_STATUS;
   Instance->DeviceMap.RdSize =1;
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   /*Check for error condition*/
  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
  Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,FQD1236_DefVal,FQD1236_Address);
  }
  #endif
 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
  IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = FQD1236_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields = FQD1236_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RFQD1236_DIV1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RFQD1236_STATUS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,FQD1236_NBREGS+1,sizeof(U8));
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	Error = ChipSetRegisters(Instance->IOHandle,0,FQD1236_DefVal,FQD1236_NBREGS);
  }
#endif	
   
    /* now safe to unlock semaphore */
  SEM_UNLOCK(Lock_InitTermOpenClose);

  if (Error == ST_NO_ERROR)
   {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
    return(Error);
}
#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tunshdrv_Open_T2000()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_SHARED_TUN_T2000
ST_ErrorCode_t tuner_tunshdrv_Open_T2000(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tunshdrv_Open_T2000()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;
    int   i;

    Error = tuner_tunshdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSHDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_T2000)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_T2000 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

      Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  T2000_NBREGS, sizeof( U8 ));



for ( i=0;i<T2000_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=T2000_DefVal[i];

	}

   Instance->Status.IntermediateFrequency =44000;
   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = T2000_NBREGS;
   Instance->DeviceMap.Fields = T2000_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   Instance->DeviceMap.WrStart =RT2000_DIV1;
   Instance->DeviceMap.WrSize = 4;
   Instance->DeviceMap.RdStart = RT2000_STATUS;
   Instance->DeviceMap.RdSize =1;
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   /*Check for error condition*/
  if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
  Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,T2000_DefVal,T2000_Address);
}
 #endif
 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = T2000_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields = T2000_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RT2000_DIV1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RT2000_STATUS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,T2000_NBREGS+1,sizeof(U8));
     
      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	Error = ChipSetRegisters(Instance->IOHandle,0,T2000_DefVal,T2000_NBREGS);
  }
  #endif
    /* now safe to unlock semaphore */
  SEM_UNLOCK(Lock_InitTermOpenClose);

  if (Error == ST_NO_ERROR)
   {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
    return(Error);
}
#endif

/***********************************/

/* ----------------------------------------------------------------------------
Name:   tuner_tunshdrv_Open_DTT7600()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */


#ifdef STTUNER_DRV_SHARED_TUN_DTT7600
ST_ErrorCode_t tuner_tunshdrv_Open_DTT7600(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
	int i;
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tuntdrv_Open_DTT7600()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;


    Error = tuner_tunshdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSHDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_DTT7600)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DTT7600 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
     Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  DTT7600_NBREGS, sizeof( U8 ));


for ( i=0;i<DTT7600_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=DTT7600_DefVal[i];

	}



   Instance->Status.IntermediateFrequency =44000;
   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = DTT7600_NBREGS;
   Instance->DeviceMap.Fields = DTT7600_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   Instance->DeviceMap.WrStart =RDTT7600_P_DIV1;
   Instance->DeviceMap.WrSize = 4;
   Instance->DeviceMap.RdStart = RDTT7600_STATUS;
   Instance->DeviceMap.RdSize =1;
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   /*Check for error condition*/
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
  Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,DTT7600_DefVal,DTT7600_Address);
}
#endif
#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = DTT7600_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields = DTT7600_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RDTT7600_P_DIV1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RDTT7600_STATUS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,DTT7600_NBREGS+1,sizeof(U8));
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	Error = ChipSetRegisters(Instance->IOHandle,0,DTT7600_DefVal,DTT7600_NBREGS);
  }
#endif	        
    /* now safe to unlock semaphore */
  SEM_UNLOCK(Lock_InitTermOpenClose);

  if (Error == ST_NO_ERROR)
   {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
    return(Error);
}

#endif




/* ----------------------------------------------------------------------------
Name:   tuner_tunshdrv_Open_MT2060()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */


#ifdef STTUNER_DRV_SHARED_TUN_MT2060
ST_ErrorCode_t tuner_tunshdrv_Open_MT2060(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
	int i;
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tuntdrv_Open_MT2060()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;


    Error = tuner_tunshdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSHDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_MT2060)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MT2060 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
     Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  MT2060_NBREGS, sizeof( U8 ));


for ( i=0;i<MT2060_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=MT2060_DefVal[i];

	}



Instance->Status.IntermediateFrequency =36000;
Instance->FirstIF=1220;
#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = MT2060_NBREGS;
   Instance->DeviceMap.Fields = MT2060_NBFIELDS;
   Instance->DeviceMap.Mode = /*IOREG_MODE_NOSUBADR*/IOREG_MODE_SUBADR_8_NS_MICROTUNE;
   Instance->DeviceMap.WrStart =LO1C_1;
   Instance->DeviceMap.WrSize = 17;
   Instance->DeviceMap.RdStart = PART_REV;
   Instance->DeviceMap.RdSize =18;

   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));

 #endif
 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = MT2060_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields = MT2060_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode = /*IOREG_MODE_NOSUBADR*/IOREG_MODE_SUBADR_8_NS_MICROTUNE;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =LO1C_1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 17;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = PART_REV;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =18;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,MT2060_NBREGS+1,sizeof(U8));
#endif
   Instance->ChannelBW = STTUNER_CHAN_BW_8M;
   Instance->MT2060_Info.tuner_id = Instance->TunerRegVal[PART_REV];
   Instance->MT2060_Info.version = VERSION;

   /*Instance->MT2131_Info.f_in = 65000000UL;
   Instance->MT2131_Info.f_LO = UHF_DEFAULT_FREQ;
   Instance->MT2131_Info.f_bw = OUTPUT_BW;
   Instance->MT2131_Info.band = MT2266_UHF_BAND;*/



   Instance->MT2060_Info.AS_Data.f_ref = REF_FREQ;
   Instance->MT2060_Info.AS_Data.f_in = 651000000;
   Instance->MT2060_Info.AS_Data.f_if1_Center = IF1_CENTER;
   Instance->MT2060_Info.AS_Data.f_if1_bw = IF1_BW;
   Instance->MT2060_Info.AS_Data.f_out = 43750000;
   Instance->MT2060_Info.AS_Data.f_out_bw = 6750000;
   Instance->MT2060_Info.AS_Data.f_zif_bw = ZIF_BW;
   Instance->MT2060_Info.AS_Data.f_LO1_Step = LO1_STEP_SIZE;
   Instance->MT2060_Info.AS_Data.f_LO2_Step = TUNE_STEP_SIZE;
   Instance->MT2060_Info.AS_Data.maxH1 = MAX_HARMONICS_1;
   Instance->MT2060_Info.AS_Data.maxH2 = MAX_HARMONICS_2;
   Instance->MT2060_Info.AS_Data.f_min_LO_Separation = MIN_LO_SEP;
   Instance->MT2060_Info.AS_Data.f_if1_Request = IF1_FREQ;
   Instance->MT2060_Info.AS_Data.f_LO1 = 1871000000;
   Instance->MT2060_Info.AS_Data.f_LO2 = 1176250000;
   Instance->MT2060_Info.f_IF1_actual=Instance->MT2060_Info.AS_Data.f_LO1 - Instance->MT2060_Info.AS_Data.f_in;
   Instance->MT2060_Info.AS_Data.f_LO1_FracN_Avoid = LO1_FRACN_AVOID;
   Instance->MT2060_Info.AS_Data.f_LO2_FracN_Avoid = LO2_FRACN_AVOID;



  Instance->MT2060_Info.num_regs = MT2060_NBREGS;






   Error= MT2060_ReInit(Instance);















  SEM_UNLOCK(Lock_InitTermOpenClose);

  if (Error == ST_NO_ERROR)
   {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
    return(Error);
}

#endif





/* ----------------------------------------------------------------------------
Name:   tuner_tunshdrv_Open_MT2131()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */


#ifdef STTUNER_DRV_SHARED_TUN_MT2131
ST_ErrorCode_t tuner_tunshdrv_Open_MT2131(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
	int i;
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tuntdrv_Open_MT2131()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;


    Error = tuner_tunshdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSHDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_MT2131)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MT2131 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
     Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  MT2131_NBREGS, sizeof( U8 ));


for ( i=0;i<MT2131_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=MT2131_DefVal[i];

	}

   Instance->Status.IntermediateFrequency =36000;
   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = MT2131_NBREGS;
   Instance->DeviceMap.Fields = MT2131_NBFIELDS;
   Instance->DeviceMap.Mode = /*IOREG_MODE_NOSUBADR*/IOREG_MODE_SUBADR_8_NS_MICROTUNE;
   Instance->DeviceMap.WrStart =MT2131_LO1C_1;
   Instance->DeviceMap.WrSize = MT2131_NBREGS-1;
   Instance->DeviceMap.RdStart = MT2131_PART_REV;
   Instance->DeviceMap.RdSize =MT2131_NBREGS;

      Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));

   #endif
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = MT2131_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields = MT2131_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode = /*IOREG_MODE_NOSUBADR*/IOREG_MODE_SUBADR_8_NS_MICROTUNE;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =MT2131_LO1C_1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = MT2131_NBREGS-1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = MT2131_PART_REV;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =MT2131_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,MT2131_NBREGS+1,sizeof(U8));
   #endif
   Instance->ChannelBW = STTUNER_CHAN_BW_8M;
   Instance->MT2131_Info.tuner_id = Instance->TunerRegVal[MT2131_PART_REV];
   Instance->MT2131_Info.version = VERSION;
   Instance->MT2131_Info.f_Ref = REF_FREQ;
   Instance->MT2131_Info.f_Step=Instance->StepSize = 50000;  /* kHz -> Hz */
   /*Instance->MT2131_Info.f_in = 65000000UL;
   Instance->MT2131_Info.f_LO = UHF_DEFAULT_FREQ;
   Instance->MT2131_Info.f_bw = OUTPUT_BW;
   Instance->MT2131_Info.band = MT2266_UHF_BAND;*/



   Instance->MT2131_Info.AS_Data.f_ref = REF_FREQ;
   Instance->MT2131_Info.AS_Data.f_in = 65000000;
   Instance->MT2131_Info.AS_Data.f_if1_Center = IF1_CENTER;
   Instance->MT2131_Info.AS_Data.f_if1_bw = IF1_BW;
   Instance->MT2131_Info.AS_Data.f_out = 44000000;
   Instance->MT2131_Info.AS_Data.f_out_bw = 6750000;
   Instance->MT2131_Info.AS_Data.f_zif_bw = ZIF_BW;
   Instance->MT2131_Info.AS_Data.f_LO1_Step = LO1_STEP_SIZE;
   Instance->MT2131_Info.AS_Data.f_LO2_Step = TUNE_STEP_SIZE;
   Instance->MT2131_Info.AS_Data.maxH1 = MAX_HARMONICS_1;
   Instance->MT2131_Info.AS_Data.maxH2 = MAX_HARMONICS_2;
   Instance->MT2131_Info.AS_Data.f_min_LO_Separation = MIN_LO_SEP;
   Instance->MT2131_Info.AS_Data.f_if1_Request = IF1_FREQ;
   Instance->MT2131_Info.AS_Data.f_LO1 = 1285000610;
   Instance->MT2131_Info.AS_Data.f_LO2 = 1176000976;
   Instance->MT2131_Info.f_IF1_actual=Instance->MT2131_Info.AS_Data.f_LO1 - Instance->MT2131_Info.AS_Data.f_in;
   Instance->MT2131_Info.AS_Data.f_LO1_FracN_Avoid = LO1_FRACN_AVOID;
   Instance->MT2131_Info.AS_Data.f_LO2_FracN_Avoid = LO2_FRACN_AVOID;



  Instance->MT2131_Info.num_regs = MT2131_NBREGS;
    Instance->MT2131_Info.f_IF1_actual =Instance->MT2131_Info.AS_Data.f_LO1 - Instance->MT2131_Info.AS_Data.f_in;
     Instance->MT2131_Info.rcvr_mode = MT2131_DIGITAL_CABLE;
      Instance->MT2131_Info.CTrim_sum = 0;
       Instance->MT2131_Info.VGA_gain_mode = MT2131_VGA_AUTO;
        Instance->MT2131_Info.LNAgain_mode = MT2131_LNA_AUTO;




    Error= MT2131_ReInit(Instance);




  SEM_UNLOCK(Lock_InitTermOpenClose);

  if (Error == ST_NO_ERROR)
   {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
    return(Error);
}

#endif







/* ----------------------------------------------------------------------------
Name:   tuner_tunshdrv_Open_TDVEH052F()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */


#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
ST_ErrorCode_t tuner_tunshdrv_Open_TDVEH052F(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
	int i;
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tuntdrv_Open_TDVEH052F()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;


    Error = tuner_tunshdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSHDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_TDVEH052F)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDVEH052F for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
     Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  TDVEH052F_NBREGS, sizeof( U8 ));


for ( i=0;i<TDVEH052F_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=TDVEH052F_DefVal[i];

	}
   Instance->Status.IntermediateFrequency =44000;
 #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)  
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = TDVEH052F_NBREGS;
   Instance->DeviceMap.Fields =TDVEH052F_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   Instance->DeviceMap.WrStart =RTDVEH052F_DIV1;
   Instance->DeviceMap.WrSize = 4;
   Instance->DeviceMap.RdStart = RTDVEH052F_STATUS;
   Instance->DeviceMap.RdSize =1;
    Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
   Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,TDVEH052F_DefVal,TDVEH052F_Address);
  }
  #endif
  #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E)
  IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = TDVEH052F_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields =TDVEH052F_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RTDVEH052F_DIV1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RTDVEH052F_STATUS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,TDVEH052F_NBREGS+1,sizeof(U8));

   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	Error = ChipSetRegisters(Instance->IOHandle,0,TDVEH052F_DefVal,TDVEH052F_NBREGS);
  }
#endif	           
   	          
   
    /* now safe to unlock semaphore */
  SEM_UNLOCK(Lock_InitTermOpenClose);

  if (Error == ST_NO_ERROR)
   {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
    return(Error);
}

#endif



/* ----------------------------------------------------------------------------
Name:   tuner_tunshdrv_Open_DTT761X()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */


#ifdef STTUNER_DRV_SHARED_TUN_DTT761X
ST_ErrorCode_t tuner_tunshdrv_Open_DTT761X(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
	int i;
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tuntdrv_Open_DTT761X()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;


    Error = tuner_tunshdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSHDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_DTT761X)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DTT761X for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
     Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  DTT761X_NBREGS, sizeof( U8 ));


for ( i=0;i<DTT761X_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=DTT761X_DefVal[i];

	}



   Instance->Status.IntermediateFrequency =44000;
   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = DTT761X_NBREGS;
   Instance->DeviceMap.Fields =DTT761X_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   Instance->DeviceMap.WrStart =RDTT761X_P_DIV1;
   Instance->DeviceMap.WrSize = 5;
   Instance->DeviceMap.RdStart = RDTT761X_STATUS;
   Instance->DeviceMap.RdSize =1;
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{

  Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,DTT761X_DefVal,DTT761X_Address);
  }
  #endif
  
  #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = DTT761X_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields =DTT761X_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RDTT761X_P_DIV1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 5;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RDTT761X_STATUS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,DTT761X_NBREGS+1,sizeof(U8));
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	Error = ChipSetRegisters(Instance->IOHandle,0,DTT761X_DefVal,DTT761X_NBREGS);
  }
  #endif
   
    /* now safe to unlock semaphore */
  SEM_UNLOCK(Lock_InitTermOpenClose);

  if (Error == ST_NO_ERROR)
   {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
    return(Error);
}

#endif


/* ----------------------------------------------------------------------------
Name:   tuner_tunshdrv_Open_DTT768XX()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */


#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
ST_ErrorCode_t tuner_tunshdrv_Open_DTT768XX(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
	int i;
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
   const char *identity = "STTUNER tunshdrv.c tuner_tuntdrv_Open_DTT768XX()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNSHDRV_InstanceData_t *Instance;


    Error = tuner_tunshdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNSHDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_DTT768XX)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DTT768XX for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
     Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  DTT768XX_NBREGS, sizeof( U8 ));


for ( i=0;i<DTT768XX_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=DTT768XX_DefVal[i];

	}



   Instance->Status.IntermediateFrequency =44000;
   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = DTT768XX_NBREGS;
   Instance->DeviceMap.Fields = DTT768XX_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   Instance->DeviceMap.WrStart =RDTT768xx_P_DIV1;
   Instance->DeviceMap.WrSize = 6;
   Instance->DeviceMap.RdStart = RDTT768xx_STATUS;
   Instance->DeviceMap.RdSize =1;
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   /*Check for error condition*/
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
  Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,DTT768XX_DefVal,DTT768XX_Address);
}
#endif
#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) || defined(STTUNER_DRV_CAB_STV0297E) 
 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = DTT768XX_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields = DTT768XX_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RDTT768xx_P_DIV1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 6;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RDTT768xx_STATUS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,DTT768XX_NBREGS+1,sizeof(U8));
   /*IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(Instance->MemoryPartition,DTT768XX_NBREGS+1,sizeof(U8));*/
     
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   { 
   	
   	Error = ChipSetRegisters(Instance->IOHandle,0,DTT768XX_DefVal,DTT768XX_NBREGS);
  }
#endif	        
    /* now safe to unlock semaphore */
  SEM_UNLOCK(Lock_InitTermOpenClose);

  if (Error == ST_NO_ERROR)
   {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
    return(Error);
}

#endif






/***********************************/
/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


TUNSHDRV_InstanceData_t *TUNSHDRV_GetInstFromHandle(TUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV_HANDLES
   const char *identity = "STTUNER tunshdrv.c TUNSHDRV_GetInstFromHandle()";
#endif
    TUNSHDRV_InstanceData_t *Instance;

    Instance = (TUNSHDRV_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}


/*****************************************************
**FUNCTION	::	SharedTunerGetStepsize
**ACTION	::	Get tuner astep size
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/

U32 SharedTunerGetStepsize(TUNSHDRV_InstanceData_t *Instance)
{
	U32 Stepsize = 0;
	#if defined (STTUNER_DRV_SHARED_TUN_DTT7600) || defined (STTUNER_DRV_SHARED_TUN_FQD1236)|| defined (STTUNER_DRV_SHARED_TUN_TD1336)|| defined (STTUNER_DRV_SHARED_TUN_T2000)|| defined (STTUNER_DRV_SHARED_TUN_TDVEH052F)|| defined (STTUNER_DRV_SHARED_TUN_DTT761X) || defined (STTUNER_DRV_SHARED_TUN_DTT768XX)
	U8 u8 = 0;
	#endif


	switch(Instance->TunerType)
	  {
	  	#ifdef STTUNER_DRV_SHARED_TUN_TD1336
	      case STTUNER_TUNER_TD1336:
		  u8 = ChipGetFieldImage(FTD1336_RS,Instance->TunerRegVal) ;
		  switch(u8)
		    {

			case 0:
			    Stepsize = 50000; /*50 KHz */
			     break;
			case 1:
			    Stepsize = 31250; /*31.25 KHz */
			    break;
			case 3:
			    Stepsize = 62500; /* 62,5 KHz */
			    break;

			default:
			    break;
		     }

		    break;
		    #endif
		    #ifdef STTUNER_DRV_SHARED_TUN_FQD1236

		case STTUNER_TUNER_FQD1236:

		   u8=ChipGetFieldImage(FFQD1236_RS,Instance->TunerRegVal);

		   switch(u8)
			{

			case 0:
			  Stepsize = 50000; /*50KHz */
			  break;
		        case 1:
			  Stepsize = 31250; /*125KHz */
			  break;
			case 2:
			  Stepsize = 166667; /* 166.667KHz */
			   break;
			case 3:
			  Stepsize = 62500; /* 62500KHz */
			  break;

			default:
			 break;
			}
		     break;
		  #endif
		  #ifdef STTUNER_DRV_SHARED_TUN_T2000
		  case STTUNER_TUNER_T2000:

		   u8=ChipGetFieldImage(FT2000_R,Instance->TunerRegVal);

		   switch(u8)
			{

			  case 13:
			    Stepsize = 50000; /*50KHz */
			    break;
			  case 4:
			    Stepsize = 125000; /*125KHz */
			    break;
			  case 19:
			    Stepsize = 166667; /* 166.667KHz */
			    break;
			  case 5:
			    Stepsize = 62500; /* 62500KHz */
			    break;
			   default:
			    break;
			}
		     break;
		     #endif
		     /***************************/

		     #ifdef STTUNER_DRV_SHARED_TUN_DTT7600
	      case STTUNER_TUNER_DTT7600:

		 u8=ChipGetFieldImage(FDTT7600_RS,Instance->TunerRegVal);
		  switch(u8)
		    {

			case 0:
			  Stepsize = 50000; /*50KHz */
			  break;
			 case 1:
			  Stepsize = 31250; /*31.25KHz */
			  break;
			 case 2:
			  Stepsize = 166667; /* 166.667KHz */
			  break;
			 case 3:
			  Stepsize = 62500; /* 62500KHz */
			  break;
		     }
		    break;
		    #endif

		          #ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
	      case STTUNER_TUNER_TDVEH052F:

		 u8=ChipGetFieldImage(FTDVEH052F_R,Instance->TunerRegVal);
		  switch(u8)
		    {

			case 0:
			  Stepsize = 50000; /*50KHz */
			  break;
			 case 1:
			  Stepsize = 31250; /*31.25KHz */
			  break;
			 case 2:
			  Stepsize = 166667; /* 166.667KHz */
			  break;
			 case 3:
			  Stepsize = 62500; /* 62500KHz */
			  break;
		     }
		    break;
		    #endif

    #ifdef STTUNER_DRV_SHARED_TUN_DTT761X
	      case STTUNER_TUNER_DTT761X:

		 u8=ChipGetFieldImage(FDTT761X_RS,Instance->TunerRegVal);
		  switch(u8)
		    {

			case 0:
			  Stepsize = 50000; /*50KHz */
			  break;
			 case 1:
			  Stepsize = 31250; /*31.25KHz */
			  break;
			 case 2:
			  Stepsize = 166667; /* 166.667KHz */
			  break;
			 case 3:
			  Stepsize = 62500; /* 62500KHz */
			  break;
		     }
		    break;
		    #endif

  #ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
	      case STTUNER_TUNER_DTT768XX:

		 u8=ChipGetFieldImage(FDTT768xx_R40,Instance->TunerRegVal);
		  switch(u8)
		    {

					case 0x3:
					Stepsize = 250000;
					break;
				
					case 0x4:
					Stepsize = 125000;
					break;
				
					case 0x5:
					Stepsize = 62500;
				   	 break;
			
					case 0x6:
					Stepsize = 31250;
					break;
				
					case  0xb:
					Stepsize = 200000;
					break;
				
					case 0xc:
					Stepsize = 100000;
					break;
				
					case 0xd:
					Stepsize = 50000;
					break;
				
					case 0xe:
					Stepsize = 25000;
					break;
				
					case 0x13:
					Stepsize = 166000;
					break;
				
					case 0x14:
					Stepsize = 83330;
					break;
				
					case 0x15:
					Stepsize = 41670;
					break;
				
					case 0x16:
					Stepsize = 20830;
					break;
				
					case 0x1b:
					Stepsize = 142800;
					break;
				
					case 0x1c:
					Stepsize = 71420;
					break;
				
					case 0x1d:
					Stepsize = 35710;
					break;
				
					case 0x1e:
					Stepsize = 17850;
					
					default:
					 break;
		     }
		    break;
		    #endif
		     /**************************/

		default:
		/*Doing nothing now. But return error value later on*/
		    break;
		}
	return Stepsize;
}
/* End of tunshdrv.c */
