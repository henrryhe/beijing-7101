/******************************************************************************

File name : bereg.h

Description : Back end init used registers

COPYRIGHT (C) STMicroelectronics 2001.

Date        	Modification                 		Name
----            ------------                 		----
27 Sep 99       Created                      		HG
12 Jan 00	Adapted for STi7020			FC
16 Jan 01	Add used register addresses		FM
******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BACKEND_REGISTERS_H
#define __BACKEND_REGISTERS_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------ */

/* Base address for config registers  -------------------------------------- */

/* Register offsets */
/* Comes from vali7020/include/video_reg.h */
#define VID_SRS		0x90			/* Soft Reset */

/* Comes from vali7020/include/vos_reg.h */
#define DSPCFG     	DSPCFG_BASE_ADDRESS
#define DSPCFG_CLK	0x00			/* Clocks Configuration */
#define VOS_CLK_INVCLK	0x08			/* Clocks Configuration */

/* Comes from vali7020/include/audio_reg.h */
#define MMDSP		MMDSP_BASE_ADDRESS	/* MMDSP block base address */
#define AUD_PLLPCM	0x48

/* FPGA register */
#define FPGA_7020_PAGE  0x08                    /* SDRAM page seen from ST20 */

/* C++ support */
#ifdef __cplusplus
}
#endif
 
#endif /* #ifndef __BACKEND_REGISTERS_H */
 
/* End of bereg.h */
