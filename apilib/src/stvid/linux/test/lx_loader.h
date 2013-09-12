/*******************************************************************************

File name   : Lx_loader.h

Description : LX Firmware loader commands header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
16 Feb 2005        Created                                           CL
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __LXLOAD_CMD_H
#define __LXLOAD_CMD_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)

#ifndef CPUPLUG_BASE_ADDRESS
    #if defined(ST_DELTAMU_GREEN_HE)
        #if defined(ARCHITECTURE_ST20)
            #define CPUPLUG_BASE_ADDRESS 0x43A40000
        #else /* ARCHITECTURE_ST40 */
            #define CPUPLUG_BASE_ADDRESS 0xA3A40000
        #endif /* defined(ARCHITECTURE_ST20)*/

    #else /* not defined(ST_DELTAMU_GREEN_HE)*/

        #if defined(ARCHITECTURE_ST20)
            #define CPUPLUG_BASE_ADDRESS 0x43A12000
        #else /* ARCHITECTURE_ST40 */
            #define CPUPLUG_BASE_ADDRESS 0xA3A12000
        #endif /* defined(ARCHITECTURE_ST20)*/

    #endif /*defined(ST_DELTAMU_GREEN_HE)*/
#endif /* not CPUPLUG_BASE_ADDRESS */

#endif /* defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */

#ifndef MME_AUDIO_TRANSPORT_NAME
    #define MME_AUDIO_TRANSPORT_NAME "ups"
#endif /* not MME_AUDIO_TRANSPORT_NAME */

#ifndef MME_AUDIO1_TRANSPORT_NAME
#define MME_AUDIO1_TRANSPORT_NAME "TransportAudio1"
#endif /* not MME_AUDIO1_TRANSPORT_NAME */

#ifndef MME_AUDIO2_TRANSPORT_NAME
#define MME_AUDIO2_TRANSPORT_NAME "TransportAudio2"
#endif /* not MME_AUDIO2_TRANSPORT_NAME */

#ifndef MME_VIDEO_TRANSPORT_NAME
    #define MME_VIDEO_TRANSPORT_NAME "ups"
#endif /* not MME_VIDEO_TRANSPORT_NAME */

#ifndef MME_VIDEO1_TRANSPORT_NAME
#define MME_VIDEO1_TRANSPORT_NAME "TransportVideo1"
#endif /* not MME_VIDEO1_TRANSPORT_NAME */

#ifndef MME_VIDEO2_TRANSPORT_NAME
#define MME_VIDEO2_TRANSPORT_NAME "TransportVideo2"
#endif /* not MME_VIDEO2_TRANSPORT_NAME */

#if !defined(ST_OSLINUX) || defined(MODULE)
void LxVideoBootSetup(void * RegBaseAddress, void * CodeAddress, BOOL debug_enable);
void LxVideoReboot(void * RegBaseAddress, BOOL debug_enable);
void LxAudioBootSetup(void * RegBaseAddress, void * CodeAddress, BOOL debug_enable);
void LxAudioReboot(void * RegBaseAddress, BOOL debug_enable);
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void LxBootSetup(S32 lx_number, BOOL debug_enable);
void LxReboot(S32 lx_number, BOOL debug_enable);

BOOL LxLoaderInitCmds(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LXLOAD_CMD_H */

/* End of ttx_cmd.h */
