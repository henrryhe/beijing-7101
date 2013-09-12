/*****************************************************************************

File name   : vtg_treg.h

Description : VTG register description for test

COPYRIGHT (C) ST-Microelectronics 1999.

Date               Modification                 Name
----               ------------                 ----
08-Dec-99          Created from output_reg.h    FC
27-Avr-01          Define as static array       XD

*****************************************************************************/

#ifndef __VTG_TREG_H
#define __VTG_TREG_H

/* Includes --------------------------------------------------------------- */

#include "dumpreg.h"
#include "vtg_reg.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Registers description */
/*------------------------ VTG1 Registers -----------------------*/
static const Register_t VtgReg[] =
{
    {"VTG1_HVDRIVE",                      	/* H-Vsync output enable  */
     VTG1,
     VTG_HVDRIVE,
     0x3,
     0x3,
     0,
     REG_32B,
     0
    },
    {"VTG1_CLKLN",                      	/* Line length (Pixclk) */
     VTG1,
     VTG_CLKLN,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG1_HDO",                      		/* HSync start relative to Href (Pixclk) */
     VTG1,
     VTG_HDO,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG1_HDS",                      		/* Hsync end relative to Href (Pixclk) */
     VTG1,
     VTG_HDS,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG1_HLFLN",                      	/* Half-Lines per Field */
     VTG1,
     VTG_HLFLN,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG1_VDO",                      		/* Vertical Drive Output start relative to Vref (half lines) */
     VTG1,
     VTG_VDO,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG1_VDS",                      		/* Vertical Drive Output end relative to Vref (half lines) */
     VTG1,
     VTG_VDS,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG1_VTGMOD",                      	/* VTG Mode */
     VTG1,
     VTG_VTGMOD,
     0x3ff,
     0x3ff,
     0,
     REG_32B,
     0
    },
    {"VTG1_VTIMER",                      	/* Vsync Timer (Cycles) */
     VTG1,
     VTG_VTIMER,
     0xffffff,
     0xffffff,
     0,
     REG_32B,
     0
    },
    {"VTG1_DRST",                      		/* VTG Reset */
     VTG1,
     VTG_DRST,
     0x0,
     0x1,
     0,
     REG_32B|REG_SPECIAL,
     0
    },
    {"VTG1_RG1",                      		/* Start of range which generates a bottom field (in slave mode) */
     VTG1,
     VTG_RG1,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG1_RG2",                      		/* End of range which generates a bottom field (in slave mode) */
     VTG1,
     VTG_RG2,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG1_ITM",                      		/* Interrupt Mask */
     VTG1,
     VTG_ITM,
     0x3c,
     0x3c,
     0,
     REG_32B,
     0
    },
    {"VTG1_ITS",                      		/* Interrupt Status */
     VTG1,
     VTG_ITS,
     0x3c,
     0x0,
     0,
     REG_32B|REG_SPECIAL,
     0
    },
    {"VTG1_STA",                      		/* Status register */
     VTG1,
     VTG_STA,
     0x3c,
     0x0,
     0,
     REG_32B,
     0
    },
    /*------------------------ VTG2 Registers -----------------------*/
    {"VTG2_HVDRIVE",                      	/* H/Vsync input/output  */
     VTG2,
     VTG_HVDRIVE,
     0x3,
     0x3,
     0,
     REG_32B,
     0
    },
    {"VTG2_CLKLN",                      	/* Line length (Pixclk) */
     VTG2,
     VTG_CLKLN,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG2_HDO",                      		/* HSync start relative to Href (Pixclk) */
     VTG2,
     VTG_HDO,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG2_HDS",                      		/* Hsync end relative to Href (Pixclk) */
     VTG2,
     VTG_HDS,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG2_HLFLN",                      	/* Half-Lines per Field */
     VTG2,
     VTG_HLFLN,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG2_VDO",                      		/* Vertical Drive Output start relative to Vref (half lines) */
     VTG2,
     VTG_VDO,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG2_VDS",                      		/* Vertical Drive Output end relative to Vref (half lines) */
     VTG2,
     VTG_VDS,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG2_VTGMOD",                      	/* VTG Mode */
     VTG2,
     VTG_VTGMOD,
     0x3ff,
     0x3ff,
     0,
     REG_32B,
     0
    },
    {"VTG2_VTIMER",                      	/* Vsync Timer (Cycles) */
     VTG2,
     VTG_VTIMER,
     0xffffff,
     0xffffff,
     0,
     REG_32B,
     0
    },
    {"VTG2_DRST",                      		/* VTG Reset */
     VTG2,
     VTG_DRST,
     0x0,
     0x1,
     0,
     REG_32B|REG_SPECIAL,
     0
    },
    {"VTG2_RG1",                      		/* Start of range which generates a bottom field (in slave mode) */
     VTG2,
     VTG_RG1,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG2_RG2",                      		/* End of range which generates a bottom field (in slave mode) */
     VTG2,
     VTG_RG2,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"VTG2_ITM",                      		/* Interrupt Mask */
     VTG2,
     VTG_ITM,
     0x3c,
     0x3c,
     0,
     REG_32B,
     0
    },
    {"VTG2_ITS",                      		/* Interrupt Status */
     VTG2,
     VTG_ITS,
     0x3c,
     0x0,
     0,
     REG_32B|REG_SPECIAL,
     0
    },
    {"VTG2_STA",                      		/* Status register */
     VTG2,
     VTG_STA,
     0x3c,
     0x0,
     0,
     REG_32B,
     0
    }
};

/* Nbr of registers */
#define VTG_REGISTERS (sizeof(VtgReg)/sizeof(Register_t))



#ifdef __cplusplus
}
#endif

#endif /* __VTG_TREG_H */
/* ------------------------------- End of file ---------------------------- */


