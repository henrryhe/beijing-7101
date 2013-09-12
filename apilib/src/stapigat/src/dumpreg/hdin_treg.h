/*****************************************************************************

File name   : hdin_treg.h

Description : SD input register description for test

COPYRIGHT (C) ST-Microelectronics 1999.

Date               Modification                 Name
----               ------------                 ----
01-Sep-99          Created                      FC
30-Nov-99	   Updated		       	FC
27-Avr-01          define as static array       XD

*****************************************************************************/

#ifndef __HDIN_TREG_H
#define __HDIN_TREG_H

/* Includes --------------------------------------------------------------- */

#include "dumpreg.h"
#include "hdin_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Registers description */
static const Register_t HdinReg[] =
{
    {"HDIN_RFP",                      	/* Reconstruction luma frame pointer */
     HDIN,
     HDIN_RFP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"HDIN_RCHP",                      /* Reconstruction chroma frame pointer */
     HDIN,
     HDIN_RCHP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"HDIN_PFP",                      	/* Presentation luma frame pointer */
     HDIN,
     HDIN_PFP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"HDIN_PCHP",                      /* Presentation chroma frame pointer */
     HDIN,
     HDIN_PCHP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"HDIN_PFLD",                      /* Presentation field Main display */
     HDIN,
     HDIN_PFLD,
     0xf,
     0xf,
     0,
     REG_32B,
     0
    },
    {"HDIN_DFW",                      	/* Input frame width */
     HDIN,
     HDIN_DFW,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"HDIN_PFW",                      	/* Presentation frame width */
     HDIN,
     HDIN_PFW,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"HDIN_PFH",                      	/* Presentation frame height */
     HDIN,
     HDIN_PFH,
     0x7f,
     0x7f,
     0,
     REG_32B,
     0
    },
    {"HDIN_DMOD",                      /* Reconstruction memory mode */
     HDIN,
     HDIN_DMOD,
     0x7C,
     0x7C,
     0,
     REG_32B,
     0
    },
    {"HDIN_PMOD",                      /* Presentation memory mode */
     HDIN,
     HDIN_PMOD,
     0xFC,
     0xFC,
     0,
     REG_32B,
     0
    },
    {"HDIN_ITM",                      	/* Interrupt mask */
     HDIN,
     HDIN_ITM,
     0x3,
     0x3,
     0,
     REG_32B,
     0
    },
    {"HDIN_ITS",                      	/* Interrupt status */
     HDIN,
     HDIN_ITS,
     0x3,
     0x0,
     0,
     REG_32B|REG_SPECIAL,				/* Reset on read */
     0
    },
    {"HDIN_STA",                      	/* Status register */
     HDIN,
     HDIN_STA,
     0x3,
     0x0,
     0,
     REG_32B,
     0
    },
    {"HDIN_CTRL",                      		/* Control register for HD pixel port */
     HDIN,
     HDIN_CTRL,
     0x1ff,
     0x1ff,
     0,
     REG_32B,
     0
    },
    {"HDIN_HSIZE",                        	/* Nbr of samples per input line */
     HDIN,
     HDIN_HSIZE,
     HDIN_HSIZE_MASK,
     HDIN_HSIZE_MASK,
     0,
     REG_32B,
     0
    },
    {"HDIN_TOP_VEND",                        /* Last line of the top field/frame */
     HDIN,
     HDIN_TOP_VEND,
     HDIN_TOP_VEND_MASK,
     HDIN_TOP_VEND_MASK,
     0,
     REG_32B,
     0
    },
    {"HDIN_BOT_VEND",                        /* Last line of the bottom field */
     HDIN,
     HDIN_BOT_VEND,
     HDIN_BOT_VEND_MASK,
     HDIN_BOT_VEND_MASK,
     0,
     REG_32B,
     0
    },
    {"HDIN_HBLANK",                     	/* Time between HSync and active input video */
     HDIN,
     HDIN_HBLANK,
     HDIN_HBLANK_MASK,
     HDIN_HBLANK_MASK,
     0,
     REG_32B,
     0
    },
    {"HDIN_HOFF",                        	/* Time between HSync and rising edge of HSyncOut */
     HDIN,
     HDIN_HOFF,
     HDIN_HOFF_MASK,
     HDIN_HOFF_MASK,
     0,
     REG_32B,
     0
    },
    {"HDIN_VBLANK",                        	/* Vertical offset for active video */
     HDIN,
     HDIN_VBLANK,
     HDIN_VBLANK_MASK,
     HDIN_VBLANK_MASK,
     0,
     REG_32B,
     0
    },
    {"HDIN_VLOFF",                        	/* Verical offset for VSyncOut */
     HDIN,
     HDIN_VLOFF,
     HDIN_VLOFF_MASK,
     HDIN_VLOFF_MASK,
     0,
     REG_32B,
     0
    },
    {"HDIN_VHOFF",                        	/* Horizontal offset for VSyncOut */
     HDIN,
     HDIN_VHOFF,
     HDIN_VHOFF_MASK,
     HDIN_VHOFF_MASK,
     0,
     REG_32B,
     0
    },
    {"HDIN_LL",                        		/* Lower horizontal limit */
     HDIN,
     HDIN_LL,
     HDIN_LL_MASK,
     HDIN_LL_MASK,
     0,
     REG_32B,
     0
    },
    {"HDIN_UL",                        		/* Upper horizontal limit */
     HDIN,
     HDIN_UL,
     HDIN_UL_MASK,
     HDIN_UL_MASK,
     0,
     REG_32B,
     0
    },
    {"HDIN_LNCNT",                        	/* Line count */
     HDIN,
     HDIN_LNCNT,
     HDIN_LNCNT_MASK,
     0x0,
     0,
     REG_32B,
     0
    }
};

/* Nbr of registers */
#define HDIN_REGISTERS (sizeof(HdinReg)/sizeof(Register_t))



#ifdef __cplusplus
}
#endif

#endif /* __HDIN_TREG_H */
/* ------------------------------- End of file ---------------------------- */


