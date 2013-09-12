/*****************************************************************************

File Name :  denc_ra.h

Description: Register access interface header file for Denc cell

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                      Name
----               ------------                                      ----
06 Feb 2001        Created                                           HSdLM
22 Jun 2001        To prevent issues with endianness, 16, 24 and     HSdLM
 *                 32 bits access routines have been removed.
******************************************************************************/

/* Prevent recursive inclusion */
#ifndef __DENC_RA_H
#define __DENC_RA_H

/* Includes ---------------------------------------------------------------- */
#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ---------------------------------------------------------- */

typedef struct
{
    ST_ErrorCode_t (*ReadReg8)   (const void *          const Device_p,
                                  const U32             Addr,
                                  U8 *                  const Value_p);
    ST_ErrorCode_t (*WriteReg8)  (const void *          const Device_p,
                                  const U32             Addr,
                                  const U8              Value );
} stdenc_RegFunctionTable_t;


/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif


#endif /* __DENC_RA_H */

/* End of denc_ra.h */
