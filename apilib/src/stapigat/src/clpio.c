/*******************************************************************************

File name   : clpio.c

Description : PIO configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
31 Jan 2002        Created                                           HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>

#ifdef ST_OSLINUX
    #include "compat.h"
    #define debugmessage(x)
#else
    #ifdef ST_OS20
        #include <debug.h>
    #endif /* ST_OS20 */

    #ifdef ST_OS21
        #include <os21debug.h>
    #endif /* ST_OS21 */
#endif

#include "testcfg.h"

#ifdef USE_PIO

#define USE_AS_FRONTEND

#include "genadd.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stpio.h"
#include "clpio.h"
#include "stsys.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

extern ST_Partition_t *DriverPartition_p;

#if defined(ST_5514) || defined (ST_5516) || defined (ST_5517) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
ST_DeviceName_t PioDeviceName[] =
{
    STPIO_0_DEVICE_NAME,
    STPIO_1_DEVICE_NAME,
    STPIO_2_DEVICE_NAME,
    STPIO_3_DEVICE_NAME,
    STPIO_4_DEVICE_NAME,
    STPIO_5_DEVICE_NAME,
    ""
};
#elif defined(ST_5528) || defined (ST_7200)
ST_DeviceName_t PioDeviceName[] =
{
    STPIO_0_DEVICE_NAME,
    STPIO_1_DEVICE_NAME,
    STPIO_2_DEVICE_NAME,
    STPIO_3_DEVICE_NAME,
    STPIO_4_DEVICE_NAME,
    STPIO_5_DEVICE_NAME,
    STPIO_6_DEVICE_NAME,
    STPIO_7_DEVICE_NAME,
    ""
};
#else
ST_DeviceName_t PioDeviceName[] =
{
    STPIO_0_DEVICE_NAME,
    STPIO_1_DEVICE_NAME,
    STPIO_2_DEVICE_NAME,
    STPIO_3_DEVICE_NAME,
    STPIO_4_DEVICE_NAME,
    ""
};
#endif


/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : PIO_Init
Description : Initialise the PIO driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL PIO_Init(void)
{
#ifdef ST_OSLINUX
#define ST5528_PIO0_INTERRUPT		0
#define ST5528_PIO1_INTERRUPT		1
#define ST5528_PIO2_INTERRUPT		2
#define ST5528_PIO3_INTERRUPT		3
#define ST5528_PIO4_INTERRUPT		4
#define ST5528_PIO5_INTERRUPT		5
#define ST5528_PIO6_INTERRUPT		6
#define ST5528_PIO7_INTERRUPT		7
#endif

    BOOL RetOk = TRUE;
    ST_ErrorCode_t ErrorCode;
    STPIO_InitParams_t PioInitParams[NUMBER_PIO];
    char Msg[256];
    U8 i;

    /* Setup PIO global parameters */
    PioInitParams[0].BaseAddress = (U32 *)PIO_0_BASE_ADDRESS;
    PioInitParams[0].InterruptNumber    = PIO_0_INTERRUPT;
    PioInitParams[0].InterruptLevel     = PIO_0_INTERRUPT_LEVEL;

    PioInitParams[1].BaseAddress = (U32 *)PIO_1_BASE_ADDRESS;
    PioInitParams[1].InterruptNumber    = PIO_1_INTERRUPT;
    PioInitParams[1].InterruptLevel     = PIO_1_INTERRUPT_LEVEL;

    PioInitParams[2].BaseAddress = (U32 *)PIO_2_BASE_ADDRESS;
    PioInitParams[2].InterruptNumber    = PIO_2_INTERRUPT;
    PioInitParams[2].InterruptLevel     = PIO_2_INTERRUPT_LEVEL;

    PioInitParams[3].BaseAddress = (U32 *)PIO_3_BASE_ADDRESS;
    PioInitParams[3].InterruptNumber    = PIO_3_INTERRUPT;
    PioInitParams[3].InterruptLevel     = PIO_3_INTERRUPT_LEVEL;

#if (NUMBER_PIO > 4)
    PioInitParams[4].BaseAddress = (U32 *)PIO_4_BASE_ADDRESS;
    PioInitParams[4].InterruptNumber    = PIO_4_INTERRUPT;
    PioInitParams[4].InterruptLevel     = PIO_4_INTERRUPT_LEVEL;
#endif

#if (NUMBER_PIO > 5)
    PioInitParams[5].BaseAddress = (U32 *)PIO_5_BASE_ADDRESS;
    PioInitParams[5].InterruptNumber    = PIO_5_INTERRUPT;
    PioInitParams[5].InterruptLevel     = PIO_5_INTERRUPT_LEVEL;
#endif

#if (NUMBER_PIO > 6)
    PioInitParams[6].BaseAddress = (U32*)PIO_6_BASE_ADDRESS;
    PioInitParams[6].InterruptNumber    = PIO_6_INTERRUPT;
    PioInitParams[6].InterruptLevel     = PIO_6_INTERRUPT_LEVEL;
#endif

#if (NUMBER_PIO > 7)
    PioInitParams[7].BaseAddress = (U32*)PIO_7_BASE_ADDRESS;
    PioInitParams[7].InterruptNumber    = PIO_7_INTERRUPT;
    PioInitParams[7].InterruptLevel     = PIO_7_INTERRUPT_LEVEL;
#endif

    /* Initialize all pio devices */
    for (i = 0; i < NUMBER_PIO; i++)
    {
        PioInitParams[i].DriverPartition = DriverPartition_p;
        ErrorCode = STPIO_Init(PioDeviceName[i], &PioInitParams[i]);
        if ( ErrorCode != ST_NO_ERROR)
        {
            RetOk = FALSE;
            sprintf(Msg,"Error: %s init failed ! Error 0x%0x\n", PioDeviceName[i], ErrorCode);
            debugmessage(Msg);
        }
    }
    if (RetOk)
    {
        sprintf(Msg,"PIO 0-%d initialized,\trevision=%s\n", NUMBER_PIO-1, STPIO_GetRevision());
        debugmessage(Msg);
    }
    return(RetOk);
} /* end PIO_Init() */


/*******************************************************************************
Name        : PIO_Term
Description : Terminate the PIO driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void PIO_Term(void)
{
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;
    STPIO_TermParams_t TermParams;
    char Msg[256];
    U8 i;

    for (i = 0; i < NUMBER_PIO; i++)
    {
        TermParams.ForceTerminate = FALSE;
        ErrorCode = STPIO_Term(PioDeviceName[i], &TermParams);
        if (ErrorCode == ST_ERROR_OPEN_HANDLE)
        {
            sprintf(Msg,"STPIO_Term(%s): Warning !! Still open handle\n", PioDeviceName[i]);
            debugmessage(Msg);
            TermParams.ForceTerminate = TRUE;
            ErrorCode = STPIO_Term(PioDeviceName[i], &TermParams);
        }
        if (ErrorCode == ST_NO_ERROR)
        {
            sprintf(Msg,"STPIO_Term(%s): ok\n", PioDeviceName[i]);
        }
        else
        {
            sprintf(Msg,"STPIO_Term(%s): error 0x%0x\n", PioDeviceName[i], ErrorCode);
        }
        debugmessage(Msg);
    }
} /* end PIO_Term() */


#if defined(SAT5107)
BOOL GetPIOBaseAddress(char PIONumber, int* PIOBaseAddress)
{
 BOOL    error;

 error = FALSE;
 switch (PIONumber)
  {
    case 0 :
      *PIOBaseAddress = PIO_0_BASE_ADDRESS;
      break;
    case 1 :
      *PIOBaseAddress = PIO_1_BASE_ADDRESS;
      break;
    case 2 :
      *PIOBaseAddress = PIO_2_BASE_ADDRESS;
      break;
    case 3 :
      *PIOBaseAddress = PIO_3_BASE_ADDRESS;
      break;
    #if defined(STI5100) || defined(ST_5301)
      case 4 :
        *PIOBaseAddress = PIO_4_BASE_ADDRESS;
        break;
      case 5 :
        *PIOBaseAddress = PIO_5_BASE_ADDRESS;
        break;
    #endif /* STI5100 || ST_5301 */
    default :
      error = TRUE;
      break;
  }
  return(error);
}


/*------------------------------------------------------------------------------
 Name    : SetPIOBitConfiguration
 Purpose : Sets the configuration for a done bit of a done PIO
 In      : BitNumber : the bit number (0 to 7)
           PIONumber : the PIO number
           PIOBitConfiguration : the bit configuration
 Out     : error code -> true for error, false for OK.
 Note    : -
------------------------------------------------------------------------------*/
BOOL SetPIOBitConfiguration(char PIONumber, char BitNumber,
                               PIOConfiguration_t PIOBitConfiguration)
{
  int      PIOBaseAddress;
  BOOL     error;

  error = GetPIOBaseAddress(PIONumber, &PIOBaseAddress);
  if (error == FALSE)
  {
    switch (PIOBitConfiguration)
    {
      case BIRECTIONNAL :
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC0),
                          (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_CLEAR_PnC1),
                          (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_CLEAR_PnC2),
                          (0x1 << BitNumber));
        break;

      case OUTPUT :
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_CLEAR_PnC0),
                          (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC1),
                          (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_CLEAR_PnC2),
                          (0x1 << BitNumber));
        break;

      case INPUT :
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_CLEAR_PnC0),
                          (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_CLEAR_PnC1),
                          (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC2),
                          (0x1 << BitNumber));
        break;

      case ALTERNATIVE_FUNCTION_OUTPUT :
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_CLEAR_PnC0),
                          (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC1),
                          (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC2),
                          (0x1 << BitNumber));
        break;

      case ALTERNATIVE_FUNCTION_BIRECTIONNAL :
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC0),
                          (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC1),
                          (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC2),
                          (0x1 << BitNumber));
        break;
      default :
        error = TRUE;
        break;
    }
  }
  return(error);
}
#endif

#endif /* USE_PIO */
/* End of clpio.c */
