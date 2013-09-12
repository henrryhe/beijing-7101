/*******************************************************************************

File name   : dac_out.c

Description : VOUT driver module for DACs out of ST40GX1

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
14 Sep 2001        Created                                          HSdLM
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#if !defined (ST_OSLINUX)
#include <string.h>
#endif

#include "stsys.h"

#include "vout_drv.h"
#include "dac_out.h"

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

#define DENC_DAC_IF_CONTROL_COMP 0x20

/* Private variables (static) --------------------------------------------- */

/* Global Variables --------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */

#define stvout_rd32(a)     STSYS_ReadRegDev32LE((void*)(a))
#define stvout_wr32(a,v)   STSYS_WriteRegDev32LE((void*)(a),(v))

/* Private Function prototypes -------------------------------------------- */

/*
******************************************************************************
Exported Functions
******************************************************************************
*/

/*************************************************************************
Name : stvout_HalSetDACSource
Description : Switch DAC source from DENC outputs
Parameters : Device_p (IN) : pointer on VOUT device to access
Assumptions : pointer on device is checked yet (not NULL, and ->OutputType is OK)
Limitations :
Return : none
**************************************************************************/
void stvout_HalSetDACSource(const stvout_Device_t * const Device_p)
{
    U32 DencDacInterfaceControl;
    void *RegAddress_p;

    RegAddress_p = (void *) ( (U8 *)Device_p->DeviceBaseAddress_p + (U32)Device_p->BaseAddress_p);

    DencDacInterfaceControl = stvout_rd32(RegAddress_p);
    switch ( Device_p->OutputType)
    {
        case STVOUT_OUTPUT_ANALOG_RGB : /* no break */
        case STVOUT_OUTPUT_ANALOG_YUV :
            DencDacInterfaceControl &= ~(U32)DENC_DAC_IF_CONTROL_COMP;
            break;
        case STVOUT_OUTPUT_ANALOG_YC :  /* no break */
        case STVOUT_OUTPUT_ANALOG_CVBS :
            DencDacInterfaceControl |= (U32)DENC_DAC_IF_CONTROL_COMP;
            break;
        default: /* see assumptions */
            break;
    }
    stvout_wr32(RegAddress_p, DencDacInterfaceControl);
} /* end stvout_HalSetDACSource() */

/* end of file */
