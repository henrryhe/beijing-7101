/*****************************************************************************

File name   : vos_treg.h

Description : Video Output Stage register description for test

COPYRIGHT (C) ST-Microelectronics 1999.

Date               Modification                 Name
----               ------------                 ----
01-Sep-99          Created                      FC
08-Dec-99          Renamed to vos_reg.h,	FC
	      	   removed VTG registers
27-Avr-01          Define as static array       XD

*****************************************************************************/

#ifndef __DSPCFG_TREG_H
#define __DSPCFG_TREG_H

/* Includes --------------------------------------------------------------- */

#include "dumpreg.h"
#include "vos_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Registers description */
static const Register_t VosReg[] =
{
    /*---------------------- DSPCFG Registers -------------------*/
    {"DSPCFG_CLK",                      	/* Clocks Configuration */
     DSPCFG,
     DSPCFG_CLK,
     0x3F,
     0x3F,
     0,
     REG_32B,
     0
    },
    {"DSPCFG_DIG",                      	/* Digital Output Configuration */
     DSPCFG,
     DSPCFG_DIG,
     0xf,
     0xf,
     0,
     REG_32B,
     0
    },
    {"DSPCFG_ANA",                      	/* Analog Output Configuration */
     DSPCFG,
     DSPCFG_ANA,
     0x7,
     0x7,
     0,
     REG_32B,
     0
    },
    {"DSPCFG_CAPT",                      	/* Capture Port Configuration */
     DSPCFG,
     DSPCFG_CAPT,
     0x1,
     0x1,
     0,
     REG_32B,
     0
    },
    {"DSPCFG_TST",                      	/* Test configuration register */
     DSPCFG,
     DSPCFG_TST,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"DSPCFG_SVM",                      	/* SVM Static Value */
     DSPCFG,
     DSPCFG_SVM,
     0xff,
     0xff,
     0,
     REG_32B,
     0
    },
    {"DSPCFG_REF",                      	/* RGB DAC reference value */
     DSPCFG,
     DSPCFG_REF,
     0x3ff,
     0x3ff,
     0,
     REG_32B,
     0
    },
    {"DSPCFG_DAC",                      	/* HD DAC Configuration */
     DSPCFG,
     DSPCFG_DAC,
     0x1ffff,
     0x1ffff,
     0,
     REG_32B,
     0
    },


    /*---------------------- DHDO Registers ----------------------*/
    {"DHDO_ACFG",                      		/* Analog output configuration */
     DHDO,
     DHDO_ACFG,
     0x7,
     0x7,
     0,
     REG_32B,
     0
    },
    {"DHDO_ABROAD",                      	/* Sync pulse width */
     DHDO,
     DHDO_ABROAD,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"DHDO_BBROAD",                      	/* Start of broad pulse */
     DHDO,
     DHDO_BBROAD,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"DHDO_CBROAD",                      	/* End of broad pulse */
     DHDO,
     DHDO_CBROAD,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"DHDO_BACTIVE",                      	/* Start of Active video */
     DHDO,
     DHDO_BACTIVE,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"DHDO_CACTIVE",                      	/* End of Active video */
     DHDO,
     DHDO_CACTIVE,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"DHDO_BROADL",                      	/* Broad Length */
     DHDO,
     DHDO_BROADL,
     0x7ff,
     0x7ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_BLANKL",                      	/* Blanking Length */
     DHDO,
     DHDO_BLANKL,
     0x7ff,
     0x7ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_ACTIVEL",                      	/* Active Length */
     DHDO,
     DHDO_ACTIVEL,
     0x7ff,
     0x7ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_ZEROL",                      	/* Zero Level */
     DHDO,
     DHDO_ZEROL,
     0x03ff03ff,
     0x03ff03ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_MIDL",                      		/* Sync pulse Mid Level */
     DHDO,
     DHDO_MIDL,
     0x03ff03ff,
     0x03ff03ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_HIGHL",                      	/* Sync pulse High Level */
     DHDO,
     DHDO_HIGHL,
     0x03ff03ff,
     0x03ff03ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_MIDLOW",                      	/* Sync pulse Mid-Low Level */
     DHDO,
     DHDO_MIDLOW,
     0x03ff03ff,
     0x03ff03ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_LOWL",                      		/* Broad pulse Low Level */
     DHDO,
     DHDO_LOWL,
     0x03ff03ff,
     0x03ff03ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_COLOR",                      	/* RGB2YCbCr Mode */
     DHDO,
     DHDO_COLOR,
     0x3,
     0x3,
     0,
     REG_32B,
     0
    },
    {"DHDO_YMLT",                      		/* Luma multiplication factor */
     DHDO,
     DHDO_YMLT,
     0x3ff,
     0x3ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_CMLT",                      		/* Chroma multiplication factor */
     DHDO,
     DHDO_CMLT,
     0x3ff,
     0x3ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_YOFF",                      		/* Luma offset */
     DHDO,
     DHDO_YOFF,
     0x3ff,
     0x3ff,
     0,
     REG_32B,
     0
    },
    {"DHDO_COFF",                      		/* Chroma offset */
     DHDO,
     DHDO_COFF,
     0x3ff,
     0x3ff,
     0,
     REG_32B,
     0
    },
    /*---------------------- C656 Registers ----------------------*/
    {"C656_BACT",                      		/* Begin of active line in pixels */
     C656,
     C656_BACT,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"C656_EACT",                      		/* End of active line in pixels */
     C656,
     C656_EACT,
     0xfff,
     0xfff,
     0,
     REG_32B,
     0
    },
    {"C656_BLANKL",                      	/* Blank Length in lines */
     C656,
     C656_BLANKL,
     0x7ff,
     0x7ff,
     0,
     REG_32B,
     0
    },
    {"C656_ACTL",                      		/* Active Length in lines */
     C656,
     C656_ACTL,
     0x7ff,
     0x7ff,
     0,
     REG_32B,
     0
    },
    {"C656_PAL",                      		/* PAL/notNTSC Mode */
     C656,
     C656_PAL,
     0x1,
     0x1,
     0,
     REG_32B,
     0
    },

/* Main Output Scan Velocity Modulator */
    {"SVM_DELAY_COMP",			/* Delay compensation for analog video out */
	 SVM,
	 SVM_DELAY_COMP,
	 0x0000000f,
	 0x0000000f,
	 0,
	 REG_32B,
	 0,
	},
    {"SVM_PIX_MAX",			/* Nbr of pixel per line */
	 SVM,
	 SVM_PIX_MAX,
	 0x000007ff,
	 0x000007ff,
	 0,
	 REG_32B,
	 0,
	},
    {"SVM_LIN_MAX",			/* Nbr of lines per field */
	 SVM,
	 SVM_LIN_MAX,
	 0x000007ff,
	 0x000007ff,
	 0,
	 REG_32B,
	 0,
	},
    {"SVM_SHAPE",			/* Modulation shape control */
	 SVM,
	 SVM_SHAPE,
	 0x00000003,
	 0x00000003,
	 0,
	 REG_32B,
	 0,
	},
    {"SVM_GAIN",			/* Gain control */
	 SVM,
	 SVM_GAIN,
	 0x0000001f,
	 0x0000001f,
	 0,
	 REG_32B,
	 0,
	},
    {"SVM_OSD_FACTOR",			/* OSD gain factor */
	 SVM,
	 SVM_OSD_FACTOR,
	 0x00000003,
	 0x00000003,
	 0,
	 REG_32B,
	 0,
	},
    {"SVM_FILTER",			/* SVM freq response selection for Video and OSD */
	 SVM,
	 SVM_FILTER,
	 0x00000003,
	 0x00000003,
	 0,
	 REG_32B,
	 0,
	},
    {"SVM_EDGE_CNT",			/* Edge count for current frame */
	 SVM,
	 SVM_EDGE_CNT,
	 0x000fffff,
	 0x000fffff,
	 0,
	 REG_32B | REG_EXCL_FROM_TEST,
	 0,
	}
};

/* Nbr of registers */
#define VOS_REGISTERS (sizeof(VosReg)/sizeof(Register_t))



#ifdef __cplusplus
}
#endif

#endif /* __DSPCFG_TREG_H */
/* ------------------------------- End of file ---------------------------- */


