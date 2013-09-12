/*****************************************************************************

File name   : sdin_treg.h

Description : SD input register description for test

COPYRIGHT (C) ST-Microelectronics 1999.

Date               Modification                 Name
----               ------------                 ----
26-Aug-99          Created                      FC
30-Nov-99	   Updated		        FC
27-Avr-01          define as static array       XD

*****************************************************************************/

#ifndef __SDIN_TREG_H
#define __SDIN_TREG_H

/* Includes --------------------------------------------------------------- */

#include "dumpreg.h"
#include "sdin_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Registers description */
static const Register_t SdinReg[] =
{
    {"SDIN1_RFP",                      	/* Reconstruction luma frame pointer */
     SDIN1,
     SDIN_RFP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"SDIN1_RCHP",                      /* Reconstruction chroma frame pointer */
     SDIN1,
     SDIN_RCHP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"SDIN1_PFP",                      	/* Presentation luma frame pointer */
     SDIN1,
     SDIN_PFP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"SDIN1_PCHP",                      /* Presentation chroma frame pointer */
     SDIN1,
     SDIN_PCHP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"SDIN1_PFLD",                      /* Presentation field Main display */
     SDIN1,
     SDIN_PFLD,
     0xf,
     0xf,
     0,
     REG_32B,
     0
    },
    {"SDIN1_ANC",                      	/* Ancilliary data buffer begin */
     SDIN1,
     SDIN_ANC,
     0x3ffff00,
     0x3ffff00,
     0,
     REG_32B,
     0
    },
    {"SDIN1_DFW",                      	/* Input frame width */
     SDIN1,
     SDIN_DFW,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"SDIN1_PFW",                      	/* Presentation frame width */
     SDIN1,
     SDIN_PFW,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"SDIN1_PFH",                      	/* Presentation frame height */
     SDIN1,
     SDIN_PFH,
     0x7f,
     0x7f,
     0,
     REG_32B,
     0
    },
    {"SDIN1_DMOD",                      /* Reconstruction memory mode */
     SDIN1,
     SDIN_DMOD,
     0x30,
     0x30,
     0,
     REG_32B,
     0
    },
    {"SDIN1_PMOD",                      /* Presentation memory mode */
     SDIN1,
     SDIN_PMOD,
     0x30,
     0x30,
     0,
     REG_32B,
     0
    },
    {"SDIN1_ITM",                      	/* Interrupt mask */
     SDIN1,
     SDIN_ITM,
     0x3,
     0x3,
     0,
     REG_32B,
     0
    },
    {"SDIN1_ITS",                      	/* Interrupt status */
     SDIN1,
     SDIN_ITS,
     0x3,
     0x0,
     0,
     REG_32B|REG_SPECIAL,				/* Reset on read */
     0
    },
    {"SDIN1_STA",                      	/* Status register */
     SDIN1,
     SDIN_STA,
     0x3,
     0x0,
     0,
     REG_32B,
     0
    },
    {"SDIN2_RFP",                      	/* Reconstruction luma frame pointer */
     SDIN2,
     SDIN_RFP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"SDIN2_RCHP",                      /* Reconstruction chroma frame pointer */
     SDIN2,
     SDIN_RCHP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"SDIN2_PFP",                      	/* Presentation luma frame pointer */
     SDIN2,
     SDIN_PFP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"SDIN2_PCHP",                      /* Presentation chroma frame pointer */
     SDIN2,
     SDIN_PCHP,
     0x3fffe00,
     0x3fffe00,
     0,
     REG_32B,
     0
    },
    {"SDIN2_PFLD",                      /* Presentation field Main display */
     SDIN2,
     SDIN_PFLD,
     0xf,
     0xf,
     0,
     REG_32B,
     0
    },
    {"SDIN2_ANC",                      	/* Ancilliary data buffer begin */
     SDIN2,
     SDIN_ANC,
     0x3ffff00,
     0x3ffff00,
     0,
     REG_32B,
     0
    },
    {"SDIN2_DFW",                      	/* Input frame width */
     SDIN2,
     SDIN_DFW,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"SDIN2_PFW",                      	/* Presentation frame width */
     SDIN2,
     SDIN_PFW,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"SDIN2_PFH",                      	/* Presentation frame height */
     SDIN2,
     SDIN_PFH,
     0x7f,
     0x7f,
     0,
     REG_32B,
     0
    },
    {"SDIN2_DMOD",                      /* Reconstruction memory mode */
     SDIN2,
     SDIN_DMOD,
     0x30,
     0x30,
     0,
     REG_32B,
     0
    },
    {"SDIN2_PMOD",                      /* Presentation memory mode */
     SDIN2,
     SDIN_PMOD,
     0x30,
     0x30,
     0,
     REG_32B,
     0
    },
    {"SDIN2_ITM",                      	/* Interrupt mask */
     SDIN2,
     SDIN_ITM,
     0x3,
     0x3,
     0,
     REG_32B,
     0
    },
    {"SDIN2_ITS",                      	/* Interrupt status */
     SDIN2,
     SDIN_ITS,
     0x3,
     0x0,
     0,
     REG_32B|REG_SPECIAL,				/* Reset on read */
     0
    },
    {"SDIN2_STA",                      	/* Status register */
     SDIN2,
     SDIN_STA,
     0x3,
     0x0,
     0,
     REG_32B,
     0
    },
#if defined (ST_7015)
    {"SDIN1_CTRL",                      /* Control register for SD pixel port 0 */
     SDIN1,
     SDIN_CTRL,
     0x7f,
     0x7f,
     0,
     REG_32B,
     0
    },
    {"SDIN1_LL",                        /* Sets lower pixel count limit to start search for HSync */
     SDIN1,
     SDIN_LL,
     SDIN_LL_MASK,
     SDIN_LL_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN1_UL",						/* Upper pixel count limit to start search for HSync */
     SDIN1,
     SDIN_UL,
     SDIN_UL_MASK,
     SDIN_UL_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN1_LNCNT",						/* Line count at most recent VSync */
     SDIN1,
     SDIN_LNCNT,
     SDIN_LNCNT_MASK,
     0,
     0,
     REG_32B,
     0
    },
    {"SDIN1_HOFF",						/* Horizontal offset from HSync to HSyncout */
     SDIN1,
     SDIN_HOFF,
     SDIN_HOFF_MASK,
     SDIN_HOFF_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN1_VLOFF",						/* Vertical lines of offset from VSync to VSyncout */
     SDIN1,
     SDIN_VLOFF,
     SDIN_VLOFF_MASK,
     SDIN_VLOFF_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN1_VHOFF",						/* Horizontal offset from VSync to VSyncout */
     SDIN1,
     SDIN_VHOFF,
     SDIN_VHOFF_MASK,
     SDIN_VHOFF_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN1_HBLANK",					/* Horizontal blanking following trailing edge of Vsync */
     SDIN1,
     SDIN_HBLANK,
     SDIN_HBLANK_MASK,
     SDIN_HBLANK_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN1_VBLANK",					/* Vertical blanking following trailing edge of Vsync */
     SDIN1,
     SDIN_VBLANK,
     SDIN_VBLANK_MASK,
     SDIN_VBLANK_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN1_LENGTH",					/* Ancillary Data Capture Length */
     SDIN1,
     SDIN_LENGTH,
     SDIN_LENGTH_MASK,
     SDIN_LENGTH_MASK,
     0,
     REG_32B,
     0
    },
#elif defined (ST_7020)
    {"SDIN1_CTL",                      /* Control register for SD pixel port 0 */
     SDIN1,
     SDIN_CTL,
     0x7ff,
     0x7ff,
     0,
     REG_32B,
     0
    },
    {"SDIN1_TFO",
     SDIN1,
     SDIN_TFO,
     0x07FF1FFF,
     0x07FF1FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN1_TFS",
     SDIN1,
     SDIN_TFS,
     0x07FF1FFF,
     0x07FF1FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN1_BFO",
     SDIN1,
     SDIN_BFO,
     0x07FF1FFF,
     0x07FF1FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN1_BFS",
     SDIN1,
     SDIN_BFS,
     0x07FF1FFF,
     0x07FF1FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN1_VSD",
     SDIN1,
     SDIN_VSD,
     0x07FF07FF,
     0x07FF07FF,
     0,
     REG_32B,
     0
    },
    {"SDIN1_HSD",
     SDIN1,
     SDIN_HSD,
     0x07FF07FF,
     0x07FF07FF,
     0,
     REG_32B,
     0
    },
    {"SDIN1_HLL",
     SDIN1,
     SDIN_HLL,
     0x00000FFF,
     0x00000FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN1_HLFLN",
     SDIN1,
     SDIN_HLFLN,
     0x00000FFF,
     0x00000FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN1_APS",
     SDIN1,
     SDIN_APS,
     0x00000FFF,
     0x00000FFF,
     0,
     REG_32B,
     0
    },
#endif /* ST_7020 */
#if defined (ST_7015)
    {"SDIN2_CTRL",                      /* Control register for SD pixel port 1 */
     SDIN2,
     SDIN_CTRL,
     0x7f,
     0x7f,
     0,
     REG_32B,
     0
    },
    {"SDIN2_LL",                        /* Sets lower pixel count limit to start search for HSync */
     SDIN2,
     SDIN_LL,
     SDIN_LL_MASK,
     SDIN_LL_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN2_UL",						/* Upper pixel count limit to start search for HSync */
     SDIN2,
     SDIN_UL,
     SDIN_UL_MASK,
     SDIN_UL_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN2_LNCNT",						/* Line count at most recent VSync */
     SDIN2,
     SDIN_LNCNT,
     SDIN_LNCNT_MASK,
     0x0,
     0,
     REG_32B,
     0
    },
    {"SDIN2_HOFF",						/* Horizontal offset from HSync to HSyncout */
     SDIN2,
     SDIN_HOFF,
     SDIN_HOFF_MASK,
     SDIN_HOFF_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN2_VLOFF",						/* Vertical lines of offset from VSync to VSyncout */
     SDIN2,
     SDIN_VLOFF,
     SDIN_VLOFF_MASK,
     SDIN_VLOFF_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN2_VHOFF",						/* Horizontal offset from VSync to VSyncout */
     SDIN2,
     SDIN_VHOFF,
     SDIN_VHOFF_MASK,
     SDIN_VHOFF_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN2_HBLANK",					/* Horizontal blanking following trailing edge of Vsync */
     SDIN2,
     SDIN_HBLANK,
     SDIN_HBLANK_MASK,
     SDIN_HBLANK_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN2_VBLANK",					/* Vertical blanking following trailing edge of Vsync */
     SDIN2,
     SDIN_VBLANK,
     SDIN_VBLANK_MASK,
     SDIN_VBLANK_MASK,
     0,
     REG_32B,
     0
    },
    {"SDIN2_LENGTH",					/* Ancillary Data Capture Length */
     SDIN2,
     SDIN_LENGTH,
     SDIN_LENGTH_MASK,
     SDIN_LENGTH_MASK,
     0,
     REG_32B,
     0
    }
#elif defined (ST_7020)
    {"SDIN2_CTL",                      /* Control register for SD pixel port 0 */
     SDIN2,
     SDIN_CTL,
     0x7ff,
     0x7ff,
     0,
     REG_32B,
     0
    },
    {"SDIN2_TFO",
     SDIN2,
     SDIN_TFO,
     0x07FF1FFF,
     0x07FF1FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN2_TFS",
     SDIN2,
     SDIN_TFS,
     0x07FF1FFF,
     0x07FF1FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN2_BFO",
     SDIN2,
     SDIN_BFO,
     0x07FF1FFF,
     0x07FF1FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN2_BFS",
     SDIN2,
     SDIN_BFS,
     0x07FF1FFF,
     0x07FF1FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN2_VSD",
     SDIN2,
     SDIN_VSD,
     0x07FF07FF,
     0x07FF07FF,
     0,
     REG_32B,
     0
    },
    {"SDIN2_HSD",
     SDIN2,
     SDIN_HSD,
     0x07FF07FF,
     0x07FF07FF,
     0,
     REG_32B,
     0
    },
    {"SDIN2_HLL",
     SDIN2,
     SDIN_HLL,
     0x00000FFF,
     0x00000FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN2_HLFLN",
     SDIN2,
     SDIN_HLFLN,
     0x00000FFF,
     0x00000FFF,
     0,
     REG_32B,
     0
    },
    {"SDIN2_APS",
     SDIN2,
     SDIN_APS,
     0x00000FFF,
     0x00000FFF,
     0,
     REG_32B,
     0
    }
#endif
};

/* Nbr of registers */
#define SDIN_REGISTERS (sizeof(SdinReg)/sizeof(Register_t))



#ifdef __cplusplus
}
#endif

#endif /* __SDIN_TREG_H */
/* ------------------------------- End of file ---------------------------- */


