/*****************************************************************************
File name   :  OS21Interrupt.c

Description :  Operating system independence file.

COPYRIGHT (C) STMicroelectronics 2007.

Date               Modification                                          Name
----               ------------                                          ----
08/06/2007         Created                                               WA
*****************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "stsys.h"
#include "sttbx.h"
#include "stos.h"



/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

/* --- Global variables ------------------------------------------------ */

/* --- Prototype ------------------------------------------------------- */

/* --- Externals ------------------------------------------------------- */



/*******************************************************************************
Name        : STOS_InterruptInstallConfigurable
Description : 
Parameters  : 
Assumptions : None
Limitations :
Returns     : STOS_SUCCESS/STOS_FAILURE
*******************************************************************************/
STOS_Error_t STOS_InterruptInstallConfigurable( U32 InterruptNumber,
                                    U32 InterruptLevel,
                                    STOS_INTERRUPT_PROTOTYPE(Fct),
                                    STOS_InterruptParam_t * InterruptParam_p,
                                    char * IrqName,
                                    void * Params)
{
     /* Remove warnings */
    UNUSED_PARAMETER(InterruptParam_p);
    UNUSED_PARAMETER(InterruptLevel);

    return (interrupt_install_WinCE(interrupt_handle(InterruptNumber),
                                    Fct,
                                    Params,
                                    IrqName) == OS21_SUCCESS ? STOS_SUCCESS : STOS_FAILURE);
}

/*******************************************************************************
Name        : STOS_InterruptInstall
Description : Install interrupt
Parameters  : 
Assumptions : None
Limitations :
Returns     : STOS_SUCCESS/STOS_FAILURE
*******************************************************************************/
STOS_Error_t STOS_InterruptInstall( U32 InterruptNumber,
                                    U32 InterruptLevel,
                                    STOS_INTERRUPT_PROTOTYPE(Fct),
                                    char * IrqName,
                                    void * Params)
{
    return (STOS_InterruptInstallConfigurable(InterruptNumber,
                                    InterruptLevel,
                                    Fct,
                                    NULL,
                                    IrqName,
                                    Params) );
}

/*******************************************************************************
Name        : STOS_InterruptUninstall
Description : free interrupt
Parameters  : interrupt number, level and parameters
Assumptions : None
Limitations :
Returns     : STOS_SUCCESS/STOS_FAILURE
*******************************************************************************/
STOS_Error_t STOS_InterruptUninstall(U32 InterruptNumber, U32 InterruptLevel, void * Params)
{
    /* Remove warnings */
    UNUSED_PARAMETER(Params);
    UNUSED_PARAMETER(InterruptLevel);

    return (interrupt_uninstall(InterruptNumber, InterruptLevel) == OS21_SUCCESS ? STOS_SUCCESS : STOS_FAILURE);
}

/* End of winceinterrupt.c */
