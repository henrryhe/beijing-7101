/*******************************************************************************

File name   : clpwm.c

Description : STPWM configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
16 Apr 2002        Created                                          HSdLM
14 Oct 2002        Add support for 5517                             HSdLM
06 Nov 2003        Add support for espresso                         CL
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "testcfg.h"

#ifdef USE_PWM

#define USE_AS_FRONTEND

#include "genadd.h"
#include "stdevice.h"
#include "stddefs.h"
#include "stpwm.h"
#include "clpwm.h"
#include "clpio.h"

#ifdef ST_OSLINUX
#include "compat.h"
/*#else*/
#endif

#include "sttbx.h"
/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#if defined(mb231)  || defined(mb282b) || defined(mb295) || defined(mb314) \
|| defined(mb361)   || defined(mb382)  || defined(mb376) || defined(mb390) \
|| defined(espresso)|| defined(mb391)  || defined(mb400) || defined(mb424) \
|| defined(mb421)   || defined(mb411)  || defined(mb519)
#define PWM_CLKRV_VCXO_CHANNEL  STPWM_CHANNEL_0 /* on these boards, DENC chroma is linked to PWM0 */

#elif defined(mb275) || defined(mb275_64) || defined(mb290)
#define PWM_CLKRV_VCXO_CHANNEL  STPWM_CHANNEL_1 /* on these boards, DENC chroma is linked to PWM1 */

#endif

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : PWM_SetConfigurationForChroma
Description : Configure the PWM to have chroma
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL PWM_SetConfigurationForChroma(void)
{
    STPWM_OpenParams_t OpenParams;
    U32 MarkValue, ReturnMark;
    BOOL RetOk = TRUE;
    ST_ErrorCode_t ErrorCode;
    STPWM_Handle_t PWMHandleForChroma;
    char PWMDeviceName[5];
    strcpy(PWMDeviceName, STPWM_DEVICE_NAME);

    STTBX_Print(("Configuring PWM: PWM %d ", PWM_CLKRV_VCXO_CHANNEL));
    OpenParams.C4.PrescaleFactor= STPWM_PRESCALE_MAX;
    OpenParams.C4.ChannelNumber = PWM_CLKRV_VCXO_CHANNEL;
    OpenParams.PWMUsedFor =STPWM_Timer;
    ErrorCode = STPWM_Open(PWMDeviceName, &OpenParams, &PWMHandleForChroma);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* error handling */
        STTBX_Print(("Open failed ! Error 0x%0x\n", ErrorCode));
        RetOk = FALSE;
    }
    else
    {
        STTBX_Print(("Open Ok, "));

#if defined(mb382)
        /* Set special PWM ratio for mb382. */
        /* STPWM_MARK_MAX/2 is not suitable for this board to get colour on NTSC display. (checked with P. Ley) */
        MarkValue = 0x6E;
#else
        /* Set PWM Mark to Space Ratio */ MarkValue = STPWM_MARK_MAX / 2;
#endif
        ErrorCode = STPWM_SetRatio(PWMHandleForChroma, MarkValue);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* error handling */
            STTBX_Print(("SetRatio failed ! Error 0x%0x\n", ErrorCode));
            RetOk = FALSE;
        }
        else
        {
            ErrorCode = STPWM_GetRatio( PWMHandleForChroma, &ReturnMark);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* error handling */
                STTBX_Print(("GetRatio failed ! Error 0x%0x\n", ErrorCode));
                RetOk = FALSE;
            }
            else
            {
                STTBX_Print(("Mark Value %d.\n", ReturnMark));
            }

        } /* SetRatio OK */

        ErrorCode = STPWM_Close(PWMHandleForChroma);
        if (ErrorCode == ST_NO_ERROR)
        {
            STTBX_Print(("STPWM_Close(PWMHandleForChroma): ok\n"));
        }
        else
        {
            STTBX_Print(("STPWM_Close(PWMHandleForChroma): failed ! ErrorCode = 0x%0x\n", ErrorCode));
        }

    } /* Open() OK */
    return(RetOk);
} /* end PWM_SetConfigurationForChroma() */

/*******************************************************************************
Name        : PWM_Init
Description : Initialise the PWM driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL PWM_Init(void)
{
    BOOL RetOk = TRUE;
    STPWM_InitParams_t PWMInitParams_s;

    /* Initialize the sub-ordinate PWM Driver
       for Channel 0 only (VCXO fine control) */
    PWMInitParams_s.Device                       = STPWM_C4;
    PWMInitParams_s.C4.BaseAddress               = (U32 *) PWM_BASE_ADDRESS;
    strcpy(PWMInitParams_s.C4.PIOforPWM0.PortName, PIO_FOR_PWM0_PORTNAME);
    PWMInitParams_s.C4.PIOforPWM0.BitMask        = PIO_FOR_PWM0_BITMASK;
    strcpy(PWMInitParams_s.C4.PIOforPWM1.PortName, PIO_FOR_PWM1_PORTNAME);
    PWMInitParams_s.C4.PIOforPWM1.BitMask        = PIO_FOR_PWM1_BITMASK;
    strcpy(PWMInitParams_s.C4.PIOforPWM2.PortName, PIO_FOR_PWM2_PORTNAME);
    PWMInitParams_s.C4.PIOforPWM2.BitMask        = PIO_FOR_PWM2_BITMASK;
    PWMInitParams_s.C4.InterruptNumber           = PWM_CAPTURE_INTERRUPT;
#ifdef WA_GNBvd54182
    PWMInitParams_s.C4.InterruptLevel            = 14;
#else
    PWMInitParams_s.C4.InterruptLevel            = PWM_INTERRUPT_LEVEL;
#endif
    if ((STPWM_Init(STPWM_DEVICE_NAME, &PWMInitParams_s)) != ST_NO_ERROR)
    {
        RetOk = FALSE;
        STTBX_Print(("STPWM_Init() failed !\n"));
    }
    else
    {
        STTBX_Print(("STPWM initialized, \trevision=%s\n", STPWM_GetRevision() ));
        RetOk = PWM_SetConfigurationForChroma();
    }

    return(RetOk);
} /* end PWM_Init() */


/*******************************************************************************
Name        : PWM_Term
Description : Terminate the PWM driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void PWM_Term(void)
{
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;
    STPWM_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;
    ErrorCode = STPWM_Term(STPWM_DEVICE_NAME, &TermParams);
    if (ErrorCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("STPWM_Term(%s): Warning !! Still open handle\n", STPWM_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STPWM_Term(STPWM_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Print(("STPWM_Term(%s): ok\n", STPWM_DEVICE_NAME));
    }
    else
    {
        STTBX_Print(("STPWM_Term(%s): error 0x%0x\n", STPWM_DEVICE_NAME, ErrorCode));
    }
} /* end PWM_Term() */


#endif /* USE_PWM */
/* End of clpwm.c */
