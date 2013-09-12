/*******************************************************************************

File name : wce_sdio.h

Description : Header for WCE API for Serial Debug I/O. 
			Implement fonction to Get and Print string.

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                 Name
----               ------------                                 ----
29 may 2005         Created                                      OLGN

*******************************************************************************/

#if !defined WCE_SDIO_H_INCLUDED_
#define WCE_SDIO_H_INCLUDED_

#include <WinBase.h>
#include <stdlib.h>	
#include "sh4_base_reg.h"
#include "sh4_scif.h"
#include "bsp_cfg.h"



#include "serial_io.h"

#define MAX_SERIALBUF_SIZE MAX_PATH

#endif // WCE_SDIO_H_INCLUDED_