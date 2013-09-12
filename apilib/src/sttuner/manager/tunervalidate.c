/* ----------------------------------------------------------------------------
File Name: validate.c

Description:

     This module handles the management functions for the STAPI level
    validation (device neutral) of parameters


Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 24-July-2001
version: 3.1.0
 author: GJP
comment:

Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   
/* C libs */
#include <string.h>
#endif
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* IO control */
#include "ioreg.h"      /* IO control via register access functions */


/* private variables ------------------------------------------------------- */

/* private functions ------------------------------------------------------- */


/* ----------------------------------------------------------------------------
Name: VALIDATE_InitParams()

Description:
    validates STTUNER initalization parameters common to all devices
    (specific device init params are checked later by the device in question)

Parameters:
    DeviceName  STTUNER global instance name device name as set during initialization.
   *InitParams  pointer to initialization parameters.

Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    ST_ERROR_NO_MEMORY,             memory allocation fail.

See Also:
    STTUNER_Term()
---------------------------------------------------------------------------- */
ST_ErrorCode_t VALIDATE_InitParams(STTUNER_InitParams_t *InitParams_p)
{
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
    const char *identity = "STTUNER validate.c VALIDATE_InitParams()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* check InitParams_p itself */
    Error = STTUNER_Util_CheckPtrNull(InitParams_p);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail Initparams\n", identity));
#endif
        return(Error);
    }


    /* device type */
    switch(InitParams_p->Device)
    {
        case STTUNER_DEVICE_SATELLITE:
        case STTUNER_DEVICE_CABLE:
        case STTUNER_DEVICE_TERR:
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail Device\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
        /*break;*/ /*this will never be reached*/
    }


    /* band lists */
    if (InitParams_p->Max_BandList < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail Max_BandList\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (InitParams_p->Max_ThresholdList < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail Max_ThresholdList\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (InitParams_p->Max_ScanList < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail Max_ScanList\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }


    /* check STEVT init() name passed to STTUNER */
    if ( (strlen(InitParams_p->EVT_DeviceName) >= ST_MAX_DEVICE_NAME) ||
         (strlen(InitParams_p->EVT_DeviceName) == 0)                  ||
         (       InitParams_p->EVT_DeviceName  == NULL) )
    {
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail EVT_DeviceName\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* check STEVT name for STEVT_open() */
    if ( (strlen(InitParams_p->EVT_RegistrantName) >= ST_MAX_DEVICE_NAME) ||
         (strlen(InitParams_p->EVT_RegistrantName) == 0)                  ||
         (       InitParams_p->EVT_RegistrantName  == NULL) )
    {
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail EVT_RegistrantName\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }


    Error = STTUNER_Util_CheckPtrNull(InitParams_p->DriverPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail DriverPartition\n", identity));
#endif
        return(Error);
    }


    /* demod type */
    switch(InitParams_p->DemodType)
    {
        case STTUNER_DEMOD_NONE:
        case STTUNER_DEMOD_STV0299:
        case STTUNER_DEMOD_STX0288:
        case STTUNER_DEMOD_STV0399:
        case STTUNER_DEMOD_STB0899:
        case STTUNER_DEMOD_STV0360:
        case STTUNER_DEMOD_STV0361:
        case STTUNER_DEMOD_STV0362:
        case STTUNER_DEMOD_STV0297:
        case STTUNER_DEMOD_STV0297J:
        case STTUNER_DEMOD_STV0297E:
        case STTUNER_DEMOD_STV0498:
        case STTUNER_DEMOD_STB0370VSB:
        case STTUNER_DEMOD_STB0370QAM:
        case STTUNER_DEMOD_STV0372:
        case STTUNER_DEMOD_STB0900:
        
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail DemodType\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
        /*break;*/ /*this will never be reached*/
    }

    /* tuner type */
    switch(InitParams_p->TunerType)
    {
        case STTUNER_TUNER_NONE:
	        case STTUNER_TUNER_VG1011:
	        case STTUNER_TUNER_TUA6100:
	        case STTUNER_TUNER_EVALMAX:
	        case STTUNER_TUNER_S68G21:
	        case STTUNER_TUNER_VG0011:  /* STEM tuner */
	        case STTUNER_TUNER_HZ1184:  /* STEM tuner */
	        case STTUNER_TUNER_MAX2116: /* STEM tuner */
	        case STTUNER_TUNER_DSF8910: /*satellite*/
	        case STTUNER_TUNER_DTT7572: /* terrestrial */
	        case STTUNER_TUNER_DTT7578: /* terrestrial */
	        case STTUNER_TUNER_DTT7592: /* terrestrial */
	        case STTUNER_TUNER_TDLB7:   /* terrestrial */
	        case STTUNER_TUNER_TDQD3:   /* terrestrial */
	        case STTUNER_TUNER_DTT7300X:   /* terrestrial */
	        case STTUNER_TUNER_ENG47402G1: /* terrestrial */
	        case STTUNER_TUNER_EAL2780: /* terrestrial */
	        case STTUNER_TUNER_TDA6650: /* terrestrial */
	        case STTUNER_TUNER_TDM1316: /* terrestrial */
	        case STTUNER_TUNER_TDBE2:   /* terrestrial */
	        case STTUNER_TUNER_ED5058:  /* terrestrial */
	        case STTUNER_TUNER_MIVAR:   /* terrestrial */
	        case STTUNER_TUNER_TDED4:   /* terrestrial */
	        case STTUNER_TUNER_DTT7102: /* terrestrial */
	        case STTUNER_TUNER_TECC2849PG: /* terrestrial */
	        case STTUNER_TUNER_TDCC2345: /* terrestrial */
	        case STTUNER_TUNER_RF4000: /* terrestrial COFDM */
	        case STTUNER_TUNER_TDTGD108:/* LG terrestrial tuner PLL-infineon*/
	        case STTUNER_TUNER_MT2266:/* Microtune MT2266 terrestrial tuner */
	        case STTUNER_TUNER_MT2131:/* Microtune MT2266 terrestrial tuner */
	        case STTUNER_TUNER_TDBE1:
	        case STTUNER_TUNER_TDDE1:
	        case STTUNER_TUNER_SP5730:
	        case STTUNER_TUNER_MT2030:
	        case STTUNER_TUNER_MT2040:
	        case STTUNER_TUNER_MT2050:
	        case STTUNER_TUNER_MT2060:
	        case STTUNER_TUNER_DCT7040:
	        case STTUNER_TUNER_DCT7050:
	        case STTUNER_TUNER_DCT7710:
	        case STTUNER_TUNER_DCF8710:
	        case STTUNER_TUNER_DCF8720:
	        case STTUNER_TUNER_MACOETA50DR:
	        case STTUNER_TUNER_CD1516LI:
	        case STTUNER_TUNER_DF1CS1223:
	        case STTUNER_TUNER_SHARPXX:
	        case STTUNER_TUNER_TDBE1X016A:
	        case STTUNER_TUNER_TDBE1X601:
	        case STTUNER_TUNER_TDEE4X012A:
	        case STTUNER_TUNER_DCT7045:
	        case STTUNER_TUNER_TDQE3:
	        case STTUNER_TUNER_DCF8783:
	        case STTUNER_TUNER_DCT7045EVAL:
	        case STTUNER_TUNER_DCT70700:
	        case STTUNER_TUNER_TDCHG:
	        case STTUNER_TUNER_TDCJG:
	        case STTUNER_TUNER_TDCGG:
                case STTUNER_TUNER_MT2011:/*Cable J83 B,A,C*/
	        case STTUNER_TUNER_STB6000: /*satellite*/
        case STTUNER_TUNER_DTT7600:/* terrestrial VSB */
        
        case STTUNER_TUNER_TDVEH052F:/* terrestrial VSB and QAM and NTSC  */
        case STTUNER_TUNER_DTVS223: /* Terrestrial VSB Samsung */
        case STTUNER_TUNER_TD1336: /* Analog NTSC, Dig ATSC,QAM :Philips*/
        case STTUNER_TUNER_FQD1236: /* Analog NTSC, Dig ATSC,QAM :Philips*/
        case STTUNER_TUNER_T2000: /* Analog NTSC, Dig ATSC,QAM :Philips*/
        case STTUNER_TUNER_STB6100:  /* satellite */
        case STTUNER_TUNER_DTT761X:/* terrestrial VSB and QAM and NTSC  */
        case STTUNER_TUNER_DTT768XX:/* terrestrial VSB and QAM and NTSC  */
        break;

        default:
		#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail TunerType\n", identity));
		#endif
        return(ST_ERROR_BAD_PARAMETER);
        /*break;*/ /*this will never be reached*/
    }


    /* DemodIO */
    switch(InitParams_p->DemodIO.Route)
    {
        case STTUNER_IO_DIRECT:
        case STTUNER_IO_REPEATER:
        case STTUNER_IO_PASSTHRU:
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail DemodIO.Route\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
        /*break;*/ /*this will never be reached*/
    }

    /* TunerIO */
    switch(InitParams_p->TunerIO.Route)
    {
        case STTUNER_IO_DIRECT:
        case STTUNER_IO_REPEATER:
        case STTUNER_IO_PASSTHRU:
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
        STTBX_Print(("%s fail TunerIO.Route\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
        /*break;*/ /*this will never be reached*/
    }


    return(Error);

}   /* VALIDATE_InitParams() */



/* ----------------------------------------------------------------------------
Name: VALIDATE_IsUniqueName()

Description:

Parameters:

Return Value:

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t VALIDATE_IsUniqueName(ST_DeviceName_t DeviceName)
{
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
    const char *identity = "STTUNER validate.c VALIDATE_IsUniqueName()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t    *Inst;
    U32 index;

    Inst = STTUNER_GetDrvInst();

    for(index = 0; index < STTUNER_MAX_HANDLES; index++)
    {
        if( strcmp(Inst[index].Name, DeviceName) == 0 )
        {
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
            STTBX_Print(("%s name '%s' is NOT unique\n", identity, DeviceName));
#endif
            return(ST_ERROR_BAD_PARAMETER);
        }
    }
#ifdef STTUNER_DEBUG_MODULE_VALIDATE
    STTBX_Print(("%s name '%s' is unique\n", identity, DeviceName));
#endif

    return(Error);
}   /* VALIDATE_IsUniqueName() */



/* End of validate.c */
