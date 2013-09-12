/*******************************************************************************

File name   : cli2c.c

Description : STI2C configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
16 Apr 2002        Created                                           HSdLM
29 Aug 2002        Add tuner support (Add I2C front)                 BS
28 Jan 2005        Add SSC1 for 7710 as I2C backend                  AC
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "testcfg.h"

#ifdef USE_I2C

#define USE_AS_FRONTEND

#include "genadd.h"
#include "stdevice.h"
#include "stddefs.h"
#include "sti2c.h"
#include "cli2c.h"
#include "stcommon.h"
#include "sttbx.h"
#include "clpio.h"
#include "clevt.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#ifndef BACK_I2C_BASE
#define BACK_I2C_BASE SSC_0_BASE_ADDRESS
#endif

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

extern ST_Partition_t *DriverPartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : I2C_Init
Description : Initialise the I2C driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL I2C_Init(void)
{
    BOOL RetOk = TRUE;
    ST_ClockInfo_t ClockInfo;
    STI2C_InitParams_t I2CInitParams;
    ST_ErrorCode_t ErrorCode;

    ST_GetClockInfo(&ClockInfo);

    /* --- Init. for Frontend --- */
#ifdef USE_I2C_FRONT
    I2CInitParams.BaseAddress           = (U32 *)SSC_1_BASE_ADDRESS;
    strcpy(I2CInitParams.PIOforSDA.PortName, PIO_FOR_SDA2_PORTNAME);
    strcpy(I2CInitParams.PIOforSCL.PortName, PIO_FOR_SCL2_PORTNAME);
    I2CInitParams.PIOforSDA.BitMask     = PIO_FOR_SDA2_BITMASK;
    I2CInitParams.PIOforSCL.BitMask     = PIO_FOR_SCL2_BITMASK;
    I2CInitParams.InterruptNumber       = SSC_1_INTERRUPT;
    I2CInitParams.InterruptLevel        = SSC_1_INTERRUPT_LEVEL;
    I2CInitParams.BaudRate              = STI2C_RATE_NORMAL;
    I2CInitParams.MasterSlave           = STI2C_MASTER;
    I2CInitParams.ClockFrequency        = ClockInfo.CommsBlock;
    I2CInitParams.GlitchWidth           = 0;
    I2CInitParams.MaxHandles            = 4;
    I2CInitParams.DriverPartition       = DriverPartition_p;
    I2CInitParams.SlaveAddress          = 0;
    strcpy(I2CInitParams.EvtHandlerName, STEVT_DEVICE_NAME);

    ErrorCode = STI2C_Init (STI2C_FRONT_DEVICE_NAME, &I2CInitParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("STI2C Front Init. failed ! error=0x%x \n", ErrorCode ));
        RetOk = FALSE;
    }
    else
    {
        STTBX_Print(("STI2C Front initialized,revision=%s\n", STI2C_GetRevision() ));
    }
#endif /* USE_I2C_FRONT */

#if defined(SAT5107)
    SetPIOBitConfiguration(3, 1, ALTERNATIVE_FUNCTION_BIRECTIONNAL);    /* Clock */
    SetPIOBitConfiguration(3, 0, ALTERNATIVE_FUNCTION_BIRECTIONNAL);    /* Data  */
    SetPIOBitConfiguration(3, 3, ALTERNATIVE_FUNCTION_BIRECTIONNAL);    /* Clock */
    SetPIOBitConfiguration(3, 2, ALTERNATIVE_FUNCTION_BIRECTIONNAL);    /* Data  */
#endif

    /* --- Init. for Backend --- */
#if defined (ST_7710)
#if defined (HDMI_I2C)
    I2CInitParams.BaseAddress           = (U32 *)SSC_1_BASE_ADDRESS;
    I2CInitParams.InterruptNumber       = SSC_1_INTERRUPT;
    I2CInitParams.InterruptLevel        = SSC_1_INTERRUPT_LEVEL;
#else
    /* Initialize the SSC0 interface to remote control receiver using I2C bus*/
    I2CInitParams.BaseAddress           = (U32 *)SSC_0_BASE_ADDRESS;
    I2CInitParams.InterruptNumber       = SSC_0_INTERRUPT;
    I2CInitParams.InterruptLevel        = SSC_0_INTERRUPT_LEVEL;
#endif /* defined (HDMI_I2C) */

#elif defined (ST_7100)|| defined (ST_7109)
    /* Initialize the SSC2 interface to remote control receiver using I2C bus*/
    I2CInitParams.BaseAddress           = (U32 *)SSC_2_BASE_ADDRESS;
    I2CInitParams.InterruptNumber       = SSC_2_INTERRUPT;
    I2CInitParams.InterruptLevel        = 1;
#elif defined (ST_7200)
    /* Initialize the SSC0 interface to remote control receiver using I2C bus*/
    I2CInitParams.BaseAddress           = (U32 *)SSC_0_BASE_ADDRESS;
    I2CInitParams.InterruptNumber       = SSC_0_INTERRUPT;
    I2CInitParams.InterruptLevel        = 1;
#else
    I2CInitParams.BaseAddress       = (U32 *)BACK_I2C_BASE;
    I2CInitParams.InterruptNumber   = SSC_0_INTERRUPT;
    I2CInitParams.InterruptLevel    = SSC_0_INTERRUPT_LEVEL;
#endif
    strcpy(I2CInitParams.PIOforSDA.PortName, PIO_FOR_SDA_PORTNAME);
    strcpy(I2CInitParams.PIOforSCL.PortName, PIO_FOR_SCL_PORTNAME);
    I2CInitParams.PIOforSDA.BitMask = PIO_FOR_SDA_BITMASK;
    I2CInitParams.PIOforSCL.BitMask = PIO_FOR_SCL_BITMASK;
    I2CInitParams.MasterSlave       = STI2C_MASTER;

#ifdef ST_5516 /* for sti2c release 1.5.0A0 */
    I2CInitParams.BaudRate          = STI2C_RATE_FASTMODE;
#else
    I2CInitParams.BaudRate          = STI2C_RATE_NORMAL;      /* Fast is STI2C_RATE_FASTMODE */
#endif
    I2CInitParams.ClockFrequency    = ClockInfo.CommsBlock;
    I2CInitParams.GlitchWidth       = 0;
    I2CInitParams.MaxHandles        = 4;
    I2CInitParams.DriverPartition   = DriverPartition_p;
    I2CInitParams.SlaveAddress      = 0; /* ignored as the I2C is set as MASTER */
    strcpy(I2CInitParams.EvtHandlerName, STEVT_DEVICE_NAME);
    ErrorCode = STI2C_Init(STI2C_BACK_DEVICE_NAME, &I2CInitParams);

    if (ErrorCode != ST_NO_ERROR)
    {
        RetOk = FALSE;
        STTBX_Print(("STI2C Back Init failed ! error=0x%x\n", ErrorCode));
    }
    else
    {
        STTBX_Print(("STI2C Back initialized,\trevision=%s\n", STI2C_GetRevision() ));
    }
    return(RetOk);
} /* end I2C_Init() */

/*******************************************************************************
Name        : I2C_Term
Description : Terminate the I2C driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void I2C_Term(void)
{
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;
    STI2C_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;
    ErrorCode = STI2C_Term(STI2C_BACK_DEVICE_NAME, &TermParams);
    if (ErrorCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("STI2C_Term(%s): Warning !! Still open handle\n", STI2C_BACK_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STI2C_Term(STI2C_BACK_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Print(("STI2C_Term(%s): ok\n", STI2C_BACK_DEVICE_NAME));
    }
    else
    {
        STTBX_Print(("STI2C_Term(%s): error 0x%0x\n", STI2C_BACK_DEVICE_NAME, ErrorCode));
    }

#ifdef USE_I2C_FRONT
    TermParams.ForceTerminate = FALSE;
    ErrorCode = STI2C_Term(STI2C_FRONT_DEVICE_NAME, &TermParams);
    if (ErrorCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("STI2C_Term(%s): Warning !! Still open handle\n", STI2C_FRONT_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STI2C_Term(STI2C_FRONT_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Print(("STI2C_Term(%s): ok\n", STI2C_FRONT_DEVICE_NAME));
    }
    else
    {
        STTBX_Print(("STI2C_Term(%s): error 0x%0x\n", STI2C_FRONT_DEVICE_NAME, ErrorCode));
    }
#endif
} /* end I2C_Term() */

#endif /* USE_I2C */
/* End of cli2c.c */
