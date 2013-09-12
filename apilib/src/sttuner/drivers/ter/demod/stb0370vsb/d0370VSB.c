/* ----------------------------------------------------------------------------
File Name: d0370VSB.c

Description:

    stb0370 8VSB demod driver.


Copyright (C) 2004-2006 STMicroelectronics

History:

   date:
version:
 author:
comment:

Reference:


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

/* LLA */


#include "370_VSB_init.h"    /* register mappings for the stv0360 */
#include "370_VSB_drv.h"    /* misc driver functions */
#include "d0370VSB.h"      /* header for this file */
#include "370_VSB_map.h"

#include "tioctl.h"     /* data structure typedefs for all the ter ioctl functions */


        #define DELAGC_0_VALUE 255
	#define DELAGC_1_VALUE 255
	#define DELAGC_6_VALUE 128
	#define WBAGC_1_VALUE 2

 U16  STB0370_VSBAddress[STB0370_VSB_NBREGS] =
{
0xf000,
0xf001,
0xf002,
0xf003,
0xf004,
0xf005,
0xf006,
0xf007,
0xf008,
0xf640,
0xf641,
0xf610,
0xf611,
0xf612,
0xf613,
0xf614,
0xf615,
0xf616,
0xf617,
0xf618,
0xf619,
0xf61a,
0xf61b,
0xf61c,
0xf680 ,
0xf681,
0xf682,
0xf010,
0xf011,
0xf012,
0xf013,
0xf014,
0xf015,
0xf016,
0xf017,
0xf018,
0xf019,
0xf01a,
0xf01b,
0xf01c,
0xf01d,
0xf01e,
0xf01f,
0xf020,
0xf021,
0xf022,
0xf023,
0xf024,
0xf025,
0xf026,
0xf027,
0xf028,
0xf029,
0xf02a,
0xf02b,
0xf02c,
0xf02d,
0xf02e,
0xf02f,
0xf030,
0xf031,
0xf032,
0xf033,
0xf034,
0xf035,
0xf036,
0xf037,
0xf038,
0xf039,
0xf03a,
0xf03b,
0xf03c,
0xf03d,
0xf03e,
0xf03f,
0xf040,
0xf041,
0xf042,
0xf043,
0xf044,
0xf045,
0xf046,
0xf047,
0xf048,
0xf049,
0xf04a,
0xf04b,
0xf04c,
0xf04d,
0xf04e,
0xf04f,
0xf050,
0xf051,
0xf052,
0xf053,
0xf054,
0xf055,
0xf056,
0xf057,
0xf058,
0xf059,
0xf05a,
0xf05b,
0xf05c,
0xf05d,
0xf05e,
0xf05f,
0xf060,
0xf061,
0xf062,
0xf063,
0xf064,
0xf065,
0xf066,
0xf067,
0xf068,
0xf069,
0xf06a,
0xf06b,
0xf06c,
0xf06d,
0xf06e,
0xf06f,
0xf070,
0xf071,
0xf072,
0xf073,
0xf074,
0xf075,
0xf076,
0xf077,
0xf078,
0xf079,
0xf07a,
0xf07b,
0xf07c,
0xf07d,
0xf07e,
0xf07f,
0xf080,
0xf081,
0xf082,
0xf083,
0xf084,
0xf085,
0xf086,
0xf087,
0xf088,
0xf089,
0xf08a,
0xf08b,
0xf08c,
0xf08d,
0xf08e,
0xf08f,
0xf090,
0xf091,
0xf092,
0xf093,
0xf094,
0xf095,
0xf096,
0xf097,
0xf098,
0xf099,
0xf09a,
0xf09b,
0xf09c,
0xf09d,
0xf09e,
0xf09f,
0xf0a0,
0xf0a1,
0xf0a2,
0xf0a3,
0xf0a4,
0xf0a5,
0xf0a6,
0xf0a7,
0xf0a8,
0xf0a9,
0xf0aa ,
0xf0ab,
0xf0ac ,
0xf0ad ,
0xf0ae ,
0xf0af ,
0xf0b0,
0xf0b1,
0xf0b2,
0xf0b3,
0xf0b4 ,
0xf0b5 ,
0xf0b6 ,
0xf0b7,
0xf0b8,
0xf0b9,
0xf0ba,
0xf0bb,
0xf0bc,
0xf0bd ,
0xf0be  ,
0xf0bf ,
0xf0c0,
0xf0c1,
0xf0c2,
0xf0c3,
0xf0c4,
0xf0c5,
0xf0c6,
0xf0c7 ,
0xf0c8 ,
0xf0c9 ,
0xf0ca ,
0xf0cb ,
0xf0cc ,
0xf0cd ,
0xf0ce,
0xf0cf ,
0xf0d0 ,
0xf0d1 ,
0xf0d2 ,
0xf0d3 ,
0xf0d4 ,
0xf0d5,
0xf0d6 ,
0xf0d7 ,
0xf0d8,
0xf422,
0xf423,
0xf428,
0xf432

};
U8  STB0370_VSBDefVal[STB0370_VSB_NBREGS]=
{
0x10,	/*	0xf000*/
0x00, /*          0xf001*/
0x32, /*          0xf002*/
0x07, /*          0xf003*/
0x0a, /*          0xf004*/
0xcd, /*          0xf005*/
0x58, /*          0xf006*/
0x18, /*          0xf007*/
0x00, /*          0xf008*/
0x02, /*          0xf640*/
0x00, /*          0xf641*/
0xc7, /*          0xf610*/
0x08, /*          0xf611*/
0x00, /*          0xf612*/
0x00, /*          0xf613*/
0x00, /*          0xf614*/
0x03, /*          0xf615*/
0x00, /*          0xf616*/
0x00, /*          0xf617*/
0x00, /*          0xf618*/
0x08, /*          0xf619*/
0x00, /*          0xf61a*/
0x03, /*          0xf61b*/
0xb4, /*          0xf61c*/
0x00, /*          0xf680*/
0xe0, /*          0xf681*/
0x00, /*          0xf682*/
0x00, /*          0xf010*/
0x00, /*          0xf011*/
0x80, /*          0xf012*/
0x00, /*          0xf013*/
0x72, /*          0xf014*/
0xf0, /*          0xf015*/
0xff, /*          0xf016*/
0x07, /*          0xf017*/
0x00, /*          0xf018*/
0x08, /*          0xf019*/
0x80, /*          0xf01a*/
0x03, /*          0xf01b*/
0x04, /*          0xf01c*/
0x01, /*          0xf01d*/
0x80, /*          0xf01e*/
0x13, /*          0xf01f*/
0xce, /*          0xf020*/
0xb7, /*          0xf021*/
0xc6, /*          0xf022*/
0x03, /*          0xf023*/
0xe2, /*          0xf024*/
0x5e, /*          0xf025*/
0x05, /*          0xf026*/
0xb3, /*          0xf027*/
0xba, /*          0xf028*/
0x3f, /*          0xf029*/
0x00, /*          0xf02a*/
0x00, /*          0xf02b*/
0x7d, /*          0xf02c*/
0x20, /*          0xf02d*/
0xf4, /*          0xf02e*/
0x10, /*          0xf02f*/
0xd8, /*          0xf030*/
0x80, /*          0xf031*/
0x80, /*          0xf032*/
0x20, /*          0xf033*/
0x78, /*          0xf034*/
0x20, /*          0xf035*/
0xe4, /*          0xf036*/
0x05, /*          0xf037*/
0x05, /*          0xf038*/
0x7f, /*          0xf039*/
0xbb, /*          0xf03a*/
0xff, /*          0xf03b*/
0x7d, /*          0xf03c*/
0x25, /*          0xf03d*/
0x04, /*          0xf03e*/
0x00, /*          0xf03f*/
0x77, /*          0xf040*/
0x01, /*          0xf041*/
0x00, /*          0xf042*/
0x14, /*          0xf043*/
0x4c, /*          0xf044*/
0x68, /*          0xf045*/
0x2f, /*          0xf046*/
0x02, /*          0xf047*/
0x02, /*          0xf048*/
0x08, /*          0xf049*/
0x08, /*          0xf04a*/
0x06, /*          0xf04b*/
0x39, /*          0xf04c*/
0x05, /*          0xf04d*/
0x00, /*          0xf04e*/
0x95, /*          0xf04f*/
0xec, /*          0xf050*/
0x05, /*          0xf051*/
0xb6, /*          0xf052*/
0x4c, /*          0xf053*/
0x01, /*          0xf054*/
0x00, /*          0xf055*/
0x77, /*          0xf056*/
0xc5, /*          0xf057*/
0x00, /*          0xf058*/
0xc2, /*          0xf059*/
0x14, /*          0xf05a*/
0x00, /*          0xf05b*/
0x00, /*          0xf05c*/
0xff, /*          0xf05d*/
0xff, /*          0xf05e*/
0xff, /*          0xf05f*/
0x0f, /*          0xf060*/
0x90, /*          0xf061*/
0x00, /*          0xf062*/
0x00, /*          0xf063*/
0x8e, /*          0xf064*/
0x1a, /*          0xf065*/
0xc8, /*          0xf066*/
0x00, /*          0xf067*/
0xff, /*          0xf068*/
0xff, /*          0xf069*/
0x3f, /*          0xf06a*/
0x5a, /*          0xf06b*/
0x70, /*          0xf06c*/
0x00, /*          0xf06d*/
0xe1, /*          0xf06e*/
0x37, /*          0xf06f*/
0xf1, /*          0xf070*/
0x00, /*          0xf071*/
0x00, /*          0xf072*/
0x40, /*          0xf073*/
0x08, /*          0xf074*/
0x08, /*          0xf075*/
0x10, /*          0xf076*/
0x08, /*          0xf077*/
0x40, /*          0xf078*/
0x20, /*          0xf079*/
0x10, /*          0xf07a*/
0x20, /*          0xf07b*/
0x20, /*          0xf07c*/
0xca, /*          0xf07d*/
0xca, /*          0xf07e*/
0xd8, /*          0xf07f*/
0x53, /*          0xf080*/
0x75, /*          0xf081*/
0x0d, /*          0xf082*/
0x77, /*          0xf083*/
0x97, /*          0xf084*/
0x08, /*          0xf085*/
0x34, /*          0xf086*/
0x22, /*          0xf087*/
0x30, /*          0xf088*/
0xb8, /*          0xf089*/
0x00, /*          0xf08a*/
0x00, /*          0xf08b*/
0x00, /*          0xf08c*/
0x00, /*          0xf08d*/
0x00, /*          0xf08e*/
0x00, /*          0xf08f*/
0x00, /*          0xf090*/
0x00, /*          0xf091*/
0x00, /*          0xf092*/
0x89, /*          0xf093*/
0x32, /*          0xf094*/
0x2a, /*          0xf095*/
0xec, /*          0xf096*/
0x05, /*          0xf097*/
0x0a, /*          0xf098*/
0x00, /*          0xf099*/
0x0b, /*          0xf09a*/
0x00, /*          0xf09b*/
0x00, /*          0xf09c*/
0xbe, /*          0xf09d*/
0xc8, /*          0xf09e*/
0x24, /*          0xf09f*/
0x00, /*          0xf0a0*/
0x00, /*          0xf0a1*/
0x30, /*          0xf0a2*/
0x41, /*          0xf0a3*/
0x44, /*          0xf0a4*/
0x04, /*          0xf0a5*/
0x00, /*          0xf0a6*/
0x00, /*          0xf0a7*/
0x01, /*          0xf0a8*/
0x00, /*          0xf0a9*/
0x9f, /*          0xf0aa*/
0x27, /*          0xf0ab*/
0xc3, /*          0xf0ac*/
0x06, /*          0xf0ad*/
0xe3, /*          0xf0ae*/
0xe0, /*          0xf0af*/
0x02, /*          0xf0b0*/
0xe0, /*          0xf0b1*/
0x00, /*          0xf0b2*/
0x80, /*          0xf0b3*/
0x30, /*          0xf0b4*/
0xa1, /*          0xf0b5*/
0x01, /*          0xf0b6*/
0xf0, /*          0xf0b7*/
0x0e, /*          0xf0b8*/
0xb7, /*          0xf0b9*/
0x00, /*          0xf0ba*/
0x16, /*          0xf0bb*/
0x87, /*          0xf0bc*/
0xc7, /*          0xf0bd*/
0x00, /*          0xf0be*/
0x20, /*          0xf0bf*/
0x9b, /*          0xf0c0*/
0x00, /*          0xf0c1*/
0x1b, /*          0xf0c2*/
0x1b, /*          0xf0c3*/
0x00, /*          0xf0c4*/
0xe0, /*          0xf0c5*/
0x06, /*          0xf0c6*/
0x3f, /*          0xf0c7*/
0x00, /*          0xf0c8*/
0xf6, /*          0xf0c9*/
0x70, /*          0xf0ca*/
0xe3, /*          0xf0cb*/
0x43, /*          0xf0cc*/
0xa0, /*          0xf0cd*/
0x00, /*          0xf0ce*/
0x93, /*          0xf0cf*/
0x06, /*          0xf0d0*/
0x5e, /*          0xf0d1*/
0x50, /*          0xf0d2*/
0xa3, /*          0xf0d3*/
0x83, /*          0xf0d4*/
0x70, /*          0xf0d5*/
0x01, /*          0xf0d6*/
0x00, /*          0xf0d7*/
0x00, /*          0xf0d8*/
0xFF, /*          0xf422*/
0xFF, /*          0xf423*/
0x80, /*          0xf428*/
0x02       /*     0xf432*/

};








/* Private types/constants ------------------------------------------------ */



/* Device capabilities */
#define MAX_AGC                         4095
#define MAX_SIGNAL_QUALITY              100
#define MAX_BER                         200000
#define STCHIP_HANDLE(x) ((STCHIP_InstanceData_t *)x)

/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;



/* instance chain, the default boot value is invalid, to catch errors */
static D0370VSB_InstanceData_t *InstanceChainTop = (D0370VSB_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */

int FE_370VSB_PowOf2(int number);

/* API */
ST_ErrorCode_t demod_d0370VSB_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0370VSB_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0370VSB_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0370VSB_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_d0370VSB_GetTunerInfo    (DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p);
ST_ErrorCode_t demod_d0370VSB_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0370VSB_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0370VSB_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0370VSB_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0370VSB_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0370VSB_StandByMode        (DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);
 ST_ErrorCode_t demod_d0370VSB_ScanFrequency   (DEMOD_Handle_t  Handle,
                                            U32  InitialFrequency,
                                            U32  SymbolRate,
                                            U32  MaxOffset,
                                            U32  TunerStep,
                                            U8   DerotatorStep,
                                            BOOL *ScanSuccess,
                                            U32  *NewFrequency,
                                            U32  Mode,
                                            U32  Guard,
                                            U32  Force,
                                            U32  Hierarchy,
                                            U32  Spectrum,
                                            U32  FreqOff,
                                            U32  ChannelBW,
                                            S32  EchoPos);
ST_ErrorCode_t demod_d0370VSB_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);

/* I/O API */
ST_ErrorCode_t demod_d0370VSB_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                  STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* local functions --------------------------------------------------------- */

D0370VSB_InstanceData_t *d0370VSB_GetInstFromHandle(DEMOD_Handle_t Handle);

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0370VSB_Install()

Description:
    install a terrestrial device driver(8VSB) into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0370VSB_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c STTUNER_DRV_DEMOD_STB0370VSB_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s installing ter:demod:STB0370VSB...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STB0370VSB;

    /* map API */
    Demod->demod_Init = demod_d0370VSB_Init;
    Demod->demod_Term = demod_d0370VSB_Term;

    Demod->demod_Open  = demod_d0370VSB_Open;
    Demod->demod_Close = demod_d0370VSB_Close;

    Demod->demod_IsAnalogCarrier  = NULL;
    Demod->demod_GetTunerInfo     = demod_d0370VSB_GetTunerInfo;
    Demod->demod_GetSignalQuality = demod_d0370VSB_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0370VSB_GetModulation;
    Demod->demod_GetAGC           = demod_d0370VSB_GetAGC;
    Demod->demod_GetMode          = NULL;
    Demod->demod_GetGuard         = NULL;
    Demod->demod_GetFECRates      = demod_d0370VSB_GetFECRates;
    Demod->demod_IsLocked         = demod_d0370VSB_IsLocked ;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = NULL;
    Demod->demod_ScanFrequency    = demod_d0370VSB_ScanFrequency;
    Demod->demod_StandByMode      = demod_d0370VSB_StandByMode;
    Demod->demod_ioaccess = demod_d0370VSB_ioaccess;
    Demod->demod_ioctl    = demod_d0370VSB_ioctl;
    Demod->demod_GetRFLevel	  = NULL;

    InstanceChainTop = NULL;


     Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0370VSB_UnInstall()

Description:
    uninstall a terrestrial device driver(8VSB) into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0370VSB_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c STTUNER_DRV_DEMOD_STV0360_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Demod->ID != STTUNER_DEMOD_STB0370VSB)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s uninstalling ter:demod:STV0360...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_NO_DRIVER;

    /* unmap API */
    Demod->demod_Init = NULL;
    Demod->demod_Term = NULL;

    Demod->demod_Open  = NULL;
    Demod->demod_Close = NULL;

    Demod->demod_IsAnalogCarrier  = NULL;
    Demod->demod_GetTunerInfo     = NULL;
    Demod->demod_GetSignalQuality = NULL;
    Demod->demod_GetModulation    = NULL;
    Demod->demod_GetAGC           = NULL;
    Demod->demod_GetMode          = NULL;
    Demod->demod_GetFECRates      = NULL;
    Demod->demod_GetGuard         = NULL;
    Demod->demod_IsLocked         = NULL;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = NULL;
    Demod->demod_ScanFrequency    = NULL;
    Demod->demod_StandByMode      = NULL;
    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;
    Demod->demod_GetRFLevel	  = NULL;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("<"));
#endif


       STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0370VSB_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370VSB_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0370VSB_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
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

    InstanceNew->ExternalClock       = InitParams->ExternalClock;
    InstanceNew->TSOutputMode        = InitParams->TSOutputMode;
    InstanceNew->SerialDataMode      = InitParams->SerialDataMode;
    InstanceNew->SerialClockSource   = InitParams->SerialClockSource;
    InstanceNew->FECMode             = InitParams->FECMode;
    InstanceNew->ClockPolarity       = InitParams->ClockPolarity;
    InstanceNew->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    InstanceNew->DeviceMap.Registers = STB0370_VSB_NBREGS;
    InstanceNew->DeviceMap.Fields    = STB0370_VSB_NBFIELDS;
    InstanceNew->DeviceMap.Mode      = IOREG_MODE_SUBADR_16; /* NEW as of 3.4.0: i/o addressing mode to use */
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->DeviceMap.DefVal= (U32 *)&STB0370_VSBDefVal[0];
    /* Allocate  memory for register mapping */
    Error = STTUNER_IOREG_Open(&InstanceNew->DeviceMap);
     if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail setup new register database\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }



#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0370VSB_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370VSB_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
            STTBX_Print(("<-- ]\n"));
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

 /* Added for removing memory related problem in API test
                 memory for register mapping */
           Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
           if (Error != ST_NO_ERROR)
            {
	   #ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
            STTBX_Print(("%s fail deallocate register database\n", identity));
	   #endif
           SEM_UNLOCK(Lock_InitTermOpenClose);
           return(Error);
        }
           STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
                STTBX_Print(("\n%s fail no free handle before end of list\n", identity));
#endif
                SEM_UNLOCK(Lock_InitTermOpenClose);
                return(STTUNER_ERROR_INITSTATE);
        }
        else
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }

    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370VSB_InstanceData_t     *Instance;
    U8 ChipID = 0;


    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* else now got pointer to free (and valid) data block */



    ChipID = STTUNER_IOREG_GetRegister( &(Instance->DeviceMap), Instance->IOHandle, R0370_ID);
    if ( (ChipID & 0x10) !=  0x10)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, ChipID));
#endif
        /* Term LLA */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s FE_360_Term()\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x10));
#endif
     }

 /* reset all chip registers */
    Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,STB0370_VSBDefVal,STB0370_VSBAddress);

    	/**************** QAM AGC Settings *****************/
    STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,F0370_STANDBY_VSB,1);
    STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,F0370_STANDBY_QAM,0);


    STTUNER_IOREG_SetRegister(&(Instance->DeviceMap), Instance->IOHandle,R0370_DELAGC_00,DELAGC_0_VALUE);
    STTUNER_IOREG_SetRegister(&(Instance->DeviceMap), Instance->IOHandle,R0370_DELAGC_11,DELAGC_1_VALUE);
    STTUNER_IOREG_SetRegister(&(Instance->DeviceMap), Instance->IOHandle,R0370_DELAGC_66,DELAGC_6_VALUE);
    STTUNER_IOREG_SetRegister(&(Instance->DeviceMap), Instance->IOHandle,R0370_WBAGC_11,WBAGC_1_VALUE);


    STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,F0370_STANDBY_VSB,0);
    STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle,F0370_STANDBY_QAM,1);

    /* Set serial/parallel data mode */
    if (Instance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
    {
        Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_SERIES, 0x01);
 	Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_SWAP, 0x01);

    }
    else
    {
        Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_SERIES, 0x00);
        Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_SWAP, 0x00);

    }


    /*set data clock polarity inversion mode (rising/falling)*/
    switch(Instance->ClockPolarity)
    {
       case STTUNER_DATA_CLOCK_INVERTED:
    	 Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_POLARITY, 0x01);
    	 break;
       case STTUNER_DATA_CLOCK_NONINVERTED:
    	 Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_POLARITY, 0x00);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_DEFAULT:
    	default:
    	break;
    }


    /* Set capabilties */
    Capability->FECAvail        = STTUNER_FEC_ALL;  /* direct mapping to STTUNER_FECRate_t    */
    Capability->ModulationAvail = STTUNER_MOD_ALL;  /* direct mapping to STTUNER_Modulation_t */
    Capability->AGCControl      = FALSE;
    Capability->SymbolMin       =  1000000; /*   1 MegaSymbols/sec */
    Capability->SymbolMax       = 50000000; /*  50 MegaSymbols/sec */

    Capability->BerMax           = MAX_BER;
    Capability->SignalQualityMax = MAX_SIGNAL_QUALITY;
    Capability->AgcMax           = MAX_AGC;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s no EVALMAX tuner driver, F0360_IAGC=0\n", identity));
#endif


    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370VSB_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = d0370VSB_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }


    /* indicate instance is closed */
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_GetTunerInfo()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_GetTunerInfo(DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_GetTunerInfo()";
#endif
    ST_ErrorCode_t       Error = ST_NO_ERROR;
    D0370VSB_InstanceData_t *Instance;
    STTUNER_Modulation_t CurModulation;
    STTUNER_FECRate_t    CurFECRates;
    /*STTUNER_Spectrum_t   CurSpectrum;*/
    U32                  CurFrequency,CurBitErrorRate;
    S32 CurSignalQuality;
     /* private driver instance data */
    Instance = d0370VSB_GetInstFromHandle(Handle);

    CurModulation = STTUNER_MOD_8VSB;

    CurFECRates = STTUNER_FEC_2_3;

    CurFrequency = TunerInfo_p->Frequency;

    /* Read noise estimations for C/N and BER */
    CurSignalQuality = FE_370_VSB_GetCarrierToNoiseRatio(&(Instance->DeviceMap), Instance->IOHandle);
    CurBitErrorRate=FE_370_VSB_GetBitErrorRate(&(Instance->DeviceMap), Instance->IOHandle);
    #ifndef ST_OSLINUX
     STOS_TaskLock();
     #endif
    CurSignalQuality = (CurSignalQuality * 10)/36; /*signal qaulity in % calculated considering 36dB as 100% signal-quality,. x*100/36 *10 as value should be devided by 10 */
    /*If it reaches maximum value then force it to max value*/
    if ( CurSignalQuality > MAX_SIGNAL_QUALITY)
    {
      	CurSignalQuality = MAX_SIGNAL_QUALITY;
    }
    TunerInfo_p->FrequencyFound      = CurFrequency;
    TunerInfo_p->SignalQuality       = CurSignalQuality;
    TunerInfo_p->BitErrorRate        = CurBitErrorRate;
    TunerInfo_p->ScanInfo.Modulation = CurModulation;
    TunerInfo_p->ScanInfo.FECRates   = CurFECRates;
    /*TunerInfo_p->ScanInfo.Spectrum   = CurSpectrum;*//*for removing compilation warnings*/
    /*Inst[Instance->TopLevelHandle].TunerInfoError = TunerError;)*/
    Error = Instance->DeviceMap.Error;
    #ifndef ST_OSLINUX
    STOS_TaskUnlock();
    #endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_GetSignalQuality()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370VSB_InstanceData_t *Instance;
       /* private driver instance data */
    Instance = d0370VSB_GetInstFromHandle(Handle);

     /* Read noise estimations for C/N and BER */
    *SignalQuality_p = FE_370_VSB_GetCarrierToNoiseRatio(&(Instance->DeviceMap), Instance->IOHandle);
    *Ber=FE_370_VSB_GetBitErrorRate(&(Instance->DeviceMap), Instance->IOHandle);
    *SignalQuality_p = (*SignalQuality_p * 10)/36; /*signal qaulity in % calculated considering 36dB as 100% signal-quality,. x*100/36 *10 as value should be devided by 10 */

       /*If it reaches maximum value then force it to max value*/
    if ( *SignalQuality_p > MAX_SIGNAL_QUALITY)
    {
    	*SignalQuality_p = MAX_SIGNAL_QUALITY;
    }
    Error = Instance->DeviceMap.Error;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_GetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_GetModulation()";
#endif
    STTUNER_Modulation_t CurModulation;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    CurModulation = STTUNER_MOD_8VSB;

    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_GetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_GetFECRates()";
#endif
    STTUNER_FECRate_t CurFecRate;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    CurFecRate = STTUNER_FEC_2_3;
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_IsLocked()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370VSB_InstanceData_t *Instance;
    U32 LockedStatus =0;

    /* private driver instance data */
    Instance = d0370VSB_GetInstFromHandle(Handle);

    LockedStatus = STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle, R0370_DEMSTATUS);
  if(LockedStatus == 0x5A)
    {
  	*IsLocked =1;
    }
    else
    {
	*IsLocked =0;
     }
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
#endif
  Error = Instance->DeviceMap.Error;
  return(Error);
}




/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_ScanFrequency   (DEMOD_Handle_t Handle,
                                            U32  InitialFrequency,
                                            U32  SymbolRate,
                                            U32  MaxOffset,
                                            U32  TunerStep,
                                            U8   DerotatorStep,
                                            BOOL *ScanSuccess,
                                            U32  *NewFrequency,
                                            U32  Mode,
                                            U32  Guard,
                                            U32  Force,
                                            U32  Hierarchy,
                                            U32  IQMode,
                                            U32  FreqOff,
                                            U32  ChannelBW,
                                            S32  EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_ScanFrequency()";
#endif
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    FE_370_VSB_Error_t        error = FE_VSB_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0370VSB_InstanceData_t        *Instance;
  
    FE_370_VSB_SearchParams_t       Search;
    FE_370_VSB_SearchResult_t       Result;
	
    /* private driver instance data */
    Instance = d0370VSB_GetInstFromHandle(Handle);

    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Terr.Tuner;

    Search.Frequency = InitialFrequency;
    Search.SymbolRate = SymbolRate;
    Search.SearchRange = 0;
    Search.IQMode  = IQMode;

    Search.FistWatchdog = ST_GetClocksPerSecond() *5 ; /*get number of ticks per 10 seconds*/
    Search.DefaultWatchdog = ST_GetClocksPerSecond() ;/*get number of ticks per  seconds*/
    Search.LOWPowerTh = 90;
    Search.UPPowerTh = 130;

    /** error checking is done here for the fix of the bug GNBvd20315 **/
     
    error=FE_370_VSB_Search(&(Instance->DeviceMap),Instance->IOHandle, &Search, &Result,Instance->TopLevelHandle);
    if(error == FE_VSB_BAD_PARAMETER)
    {
       Error= ST_ERROR_BAD_PARAMETER;
 	 #ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
           STTBX_Print(("%s fail, scan not done: bad parameter(s) to FE_370_VSB_Search() == FE_BAD_PARAMETER\n", identity ));
	 #endif
       return Error;
    }

    else if (error == FE_VSB_SEARCH_FAILED)
    {
    	Error= ST_NO_ERROR;
    	#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
           STTBX_Print(("%s  FE_370_VSB_Search() == FE_VSB_SEARCH_FAILED\n", identity ));
	#endif
	return Error;

    }
    

     *ScanSuccess = Result.Locked;
     if (*ScanSuccess == TRUE)
     {
     	*NewFrequency = Result.Frequency;
     	#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s NewFrequency=%u\t", identity, *NewFrequency));
        STTBX_Print(("%s ScanSuccess=%u\n",  identity, *ScanSuccess));
        #endif
     	/* return(ST_NO_ERROR);*/

     }
     	 return(ST_NO_ERROR);


}



/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370VSB_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = d0370VSB_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = STTUNER_IOREG_GetRegister(&(Instance->DeviceMap),Instance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register */
            Error =  STTUNER_IOREG_SetRegister(&(Instance->DeviceMap),Instance->IOHandle,
                  ((TERIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((TERIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif

    return(Error);

}



/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_ioaccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0370VSB_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0370VSB.c demod_d0370VSB_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    IOARCH_Handle_t ThisIOHandle;
    D0370VSB_InstanceData_t *Instance;

    Instance = d0370VSB_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* this (demod) drivers I/O handle */
    ThisIOHandle = Instance->IOHandle;

    /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s write passthru\n", identity));
#endif
        Error = STTUNER_IOARCH_ReadWrite(ThisIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    else    /* repeater */
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
        STTBX_Print(("%s write repeater\n", identity));
#endif

        Error = STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_STOP_ENABLE_1, 1);
        Error = STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_I2CT_ON_1, 1);
        /* send callers data using their address. (nb: subaddress == 0)
         Function 'STTUNER_IOARCH_ReadWriteNoRep' is called as calling the normal 'STTUNER_IOARCH_ReadWrite'
        function would cause it to call the redirection function which is THIS function, and around we
        would go forever. */
        Error = STTUNER_IOARCH_ReadWriteNoRep(IOHandle, Operation, SubAddr, Data, TransferSize, Timeout);

    }

    return(Error);
}




/* ----------------------------------------------------------------------------
Name: demod_d0370VSB_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0370VSB_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB
   const char *identity = "STTUNER d0288.c demod_d0370VSB_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0370VSB_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = d0370VSB_GetInstFromHandle(Handle);


    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_STANDBY_AD10, 0);
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_STANDBY_TUNER, 0);
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_STANDBY_VSB, 0);
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_STANDBY_NCO, 0);
       }


       if(Error==ST_NO_ERROR)
       {
          Instance->StandBy_Flag = 0 ;
       }

       break;
       case STTUNER_STANDBY_POWER_MODE :
       if(Instance->StandBy_Flag == 0)
       {
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_STANDBY_AD10, 1);
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_STANDBY_TUNER, 1);
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_STANDBY_VSB, 1);
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0370_STANDBY_NCO, 1);
          if(Error==ST_NO_ERROR)
          {
             Instance->StandBy_Flag = 1 ;

          }
       }
       break ;
	   /* Switch statement */

    }


    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

D0370VSB_InstanceData_t *d0370VSB_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB_HANDLES
   const char *identity = "STTUNER d0370VSB.c d0370VSB_GetInstFromHandle()";
#endif
    D0370VSB_InstanceData_t *Instance;

    Instance = (D0370VSB_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0370VSB_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}




/* End of d0370VSB.c */
