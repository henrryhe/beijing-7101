/*******************************************************************************

File name   : ondenc.h

Description : hal vbi on denc header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
06 Jul 2000        Created                                           JG
12 Dec 2000        Improve error tracking                            HSdLM
22 Feb 2001        Use STDENC mutual exclusion register accesses     HSdLM
04 Jul 2001        Non-API exported symbols begin with stvbi_        HSdLM
 *                 New function to check Closed Caption dynamic
 *                 parameters
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VBI_ON_DENC_H
#define __VBI_ON_DENC_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stvbi.h"
#include "vbi_drv.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define OK(ErrorCode)    (ErrorCode == ST_NO_ERROR)
#define VbiEc(ErrorCode) (ErrorCode == ST_NO_ERROR ? ST_NO_ERROR : STVBI_ERROR_DENC_ACCESS)

/* Exported Functions ------------------------------------------------------- */

        /* teletext */
ST_ErrorCode_t stvbi_HALSetTeletextConfigurationOnDenc(              VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALSetTeletextDynamicParamsOnDenc(              VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALDisableTeletextOnDenc(                       VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALEnableTeletextOnDenc(                        VBI_t * const Vbi_p);

        /* Closed caption */
ST_ErrorCode_t stvbi_HALDisableClosedCaptionOnDenc(                  VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALEnableClosedCaptionOnDenc(                   VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALSetClosedCaptionConfigurationOnDenc(         VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALCheckClosedCaptionDynamicParamsOnDenc( const VBI_t * const Vbi_p, const STVBI_DynamicParams_t * const DynamicParams_p);
ST_ErrorCode_t stvbi_HALSetClosedCaptionDynamicParamsOnDenc(         VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALWriteClosedCaptionDataOnDenc(                VBI_t * const Vbi_p, const U8* const Data_p);

        /* Cgms */
ST_ErrorCode_t stvbi_HALDisableCgmsOnDenc(                           VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALEnableCgmsOnDenc(                            VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALWriteCgmsDataOnDenc(                         VBI_t * const Vbi_p, const U8* const Data_p);

        /* Vps */
ST_ErrorCode_t stvbi_HALDisableVpsOnDenc(                            VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALEnableVpsOnDenc(                             VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALWriteVpsDataOnDenc(                          VBI_t * const Vbi_p, const U8* const Data_p);

        /* Wss */
ST_ErrorCode_t stvbi_HALDisableWssOnDenc(                            VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALEnableWssOnDenc(                             VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALWriteWssDataOnDenc(                          VBI_t * const Vbi_p, const U8* const Data_p);

        /* Copy Protection */
#ifdef COPYPROTECTION
ST_ErrorCode_t stvbi_HALInitCopyProtectionOnDenc(                    VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALWriteCopyProtectionDataOnDenc(               VBI_t * const Vbi_p, const U8* const Data_p);
ST_ErrorCode_t stvbi_HALSetCopyProtectionDynamicParamsOnDenc(        VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALEnableCopyProtectionOnDenc(                  VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HALDisableCopyProtectionOnDenc(                 VBI_t * const Vbi_p);
#endif /* #ifdef COPYPROTECTION */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VBI_ON_DENC_H */

/* End of ondenc.h */
