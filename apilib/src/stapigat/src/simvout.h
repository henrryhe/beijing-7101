/*******************************************************************************

File name   : simvout.h

Description : header file of STVOUT driver simulator source file.

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
07 Mar 2002        Created                                           HS
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __SIMVOUT_H
#define __SIMVOUT_H


/* Includes ----------------------------------------------------------------- */

#include "stdenc.h"
#include "testtool.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */

typedef enum VoutSimOutput_e
{
    VOUTSIM_OUTPUT_RGB,
    VOUTSIM_OUTPUT_YUV,
    VOUTSIM_OUTPUT_YC,
    VOUTSIM_OUTPUT_CVBS
} VoutSimOutput_t;

typedef enum VoutSimSelectOut_e
{
    VOUTSIM_SELECT_OUT_MAIN,
    VOUTSIM_SELECT_OUT_AUX
} VoutSimSelectOut_t ;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

void VoutSimDencOut( const STDENC_Handle_t     DencHnd,
                     const STDENC_DeviceType_t DencType,
                     const VoutSimOutput_t     Output,
                     const VoutSimSelectOut_t  SelectOut);
void VoutSimSetViewport(const BOOL IsNTSC, const VoutSimSelectOut_t SelectOut);

BOOL VoutSimDencOutCmd( STTST_Parse_t *pars_p, char *result_sym_p );
BOOL VoutSimSetViewportCmd( STTST_Parse_t *pars_p, char *result_sym_p );
BOOL VoutSim_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __SIMVOUT_H */

/* End of simvout.h */
