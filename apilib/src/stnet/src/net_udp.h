/******************************************************************************

File name   : net_udp.h

Description : UDP related info

COPYRIGHT (C) 2007 STMicroelectronics

******************************************************************************/
#ifndef __NET_UDP_H
#define __NET_UDP_H

/* Includes ---------------------------------------------------------------- */


#include "stddefs.h"
#include "stnet.h"
#include "net.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ----------------------------------------------------------- */



/* Private Constants ------------------------------------------------------- */

#define STNETi_UDP_DEFAULT_PORT         1234
#define STNETi_TS_NB_PACKETS_IN_UDP     7
#define STNETi_TS_PAYLOAD_SIZE_IN_UDP   (STNETi_TS_PACKET_SIZE * STNETi_TS_NB_PACKETS_IN_UDP)


/* Private Variables ------------------------------------------------------- */


/* Private Macros ---------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif
/* EOF --------------------------------------------------------------------- */

