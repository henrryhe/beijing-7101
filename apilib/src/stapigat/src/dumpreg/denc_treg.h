/*****************************************************************************

File name   : denc_treg.h

Description : DENC register description for test

COPYRIGHT (C) ST-Microelectronics 1999.

Date               Modification                 Name
----               ------------                 ----
13-Sep-99          Created                      FC
27-Avr-01          define as static array       XD

TODO:
-----
TODO: Update from V8.0 to V10.0: Solve conflict with output stage....
*****************************************************************************/

#ifndef __DENC_TREG_H
#define __DENC_TREG_H

/* Includes --------------------------------------------------------------- */

#include "dumpreg.h"
#include "denc_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Registers description */
static const Register_t DencReg[] =
{
	{"DENC_CFG0",                      	/* Configuration 0 */
	 DENC,
	 DENC_CFG0,
	 0xff,
	 0xff,
	 0x92,
	 REG_32B,
	 0
	},
	{"DENC_CFG1",                      	/* Configuration 1 */
	 DENC,
	 DENC_CFG1,
	 0xff,
	 0xff,
	 0x44,
	 REG_32B,
	 0
	},
	{"DENC_CFG2",                      	/* Configuration 2 */
	 DENC,
	 DENC_CFG2,
	 0xff,
	 0xff,
	 0x20,
	 REG_32B,
	 0
	},
	{"DENC_CFG3",                      	/* Configuration 3 */
	 DENC,
	 DENC_CFG3,
	 0xf9,
	 0xf9,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_CFG4",                      	/* Configuration 4 */
	 DENC,
	 DENC_CFG4,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_CFG5",                      	/* Configuration 5 */
	 DENC,
	 DENC_CFG5,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_CFG6",                      	/* Configuration 6 */
	 DENC,
	 DENC_CFG6,
	 0xff,
	 0xff,
	 0x10,
	 REG_32B | REG_SPECIAL | REG_SIDE_WRITE,
	 0
	},
	{"DENC_CFG7",                      	/* Configuration 7 */
	 DENC,
	 DENC_CFG7,
	 0xef,
	 0xef,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_CFG8",                      	/* Configuration 8 */
	 DENC,
	 DENC_CFG8,
	 0xf8,
	 0xf8,
	 0x20,
	 REG_32B,
	 0
	},
	{"DENC_CFG9",                      	/* Configuration 9 */
	 DENC,
	 DENC_CFG9,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_STATUS",                     /* Status */
	 DENC,
	 DENC_STATUS,
	 0xff,
	 0x0,
	 0xb0,
	 REG_32B,
	 0
	},

	{"DENC_DFS_INC2",                   /* Increment for DFS, byte 2 */
	 DENC,
	 DENC_DFS_INC2,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_DFS_INC1",                   /* Increment for DFS, byte 1 */
	 DENC,
	 DENC_DFS_INC1,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_DFS_INC0",                   /* Increment for DFS, byte 0 */
	 DENC,
	 DENC_DFS_INC0,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_DFS_PHASE1",                 /* Phase DFS, byte 1 */
	 DENC,
	 DENC_DFS_PHASE1,
	 0x03,
	 0x03,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_DFS_PHASE0",                 /* Phase DFS, byte 0 */
	 DENC,
	 DENC_DFS_PHASE0,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},

	{"DENC_WSS_DATA1",                  /* WSS Data, byte 1 */
	 DENC,
	 DENC_WSS_DATA1,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_WSS_DATA0",                  /* WSS Data, byte 0 */
	 DENC,
	 DENC_WSS_DATA0,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},

	{"DENC_DAC13_MULT",                 /* DAC1 & DAC3 Multiplication factor */
	 DENC,
	 DENC_DAC13_MULT,
	 0xff,
	 0xff,
	 0x88,
	 REG_32B,
	 0
	},
	{"DENC_DAC45_MULT",                 /* DAC4 & DAC5 Multiplication factor */
	 DENC,
	 DENC_DAC45_MULT,
	 0xff,
	 0xff,
	 0x88,
	 REG_32B,
	 0
	},
	{"DENC_DAC6C_MULT",                 /* DAC6 & C Multiplication factor */
	 DENC,
	 DENC_DAC6C_MULT,
	 0xff,
	 0xff,
	 0x80,
	 REG_32B,
	 0
	},

	{"DENC_LINE2",                      /* Target and Reference lines, byte 2 */
	 DENC,
	 DENC_LINE2,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_LINE1",                      /* Target and Reference lines, byte 1 */
	 DENC,
	 DENC_LINE1,
	 0xff,
	 0xff,
	 0x80,
	 REG_32B,
	 0
	},
	{"DENC_LINE0",                      /* Target and Reference lines, byte 0 */
	 DENC,
	 DENC_LINE0,
	 0xc0,
	 0xc0,
	 0,
	 REG_32B,
	 0
	},

	{"DENC_CHIPID",                     /* DENC Identification Number */
	 DENC,
	 DENC_CHIPID,
	 0xff,
	 0x0,
	 0x80,								/* For V8.0 */
	 REG_32B,
	 0
	},

	{"DENC_VPS5",                      	/* VPS Data Register, byte 5 */
	 DENC,
	 DENC_VPS5,
	 0xcf,
	 0xcf,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_VPS4",                      	/* VPS Data Register, byte 4 */
	 DENC,
	 DENC_VPS4,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_VPS3",                      	/* VPS Data Register, byte 3 */
	 DENC,
	 DENC_VPS3,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_VPS2",                      	/* VPS Data Register, byte 2 */
	 DENC,
	 DENC_VPS2,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_VPS1",                      	/* VPS Data Register, byte 1 */
	 DENC,
	 DENC_VPS1,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_VPS0",                      	/* VPS Data Register, byte 0 */
	 DENC,
	 DENC_VPS0,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},

	{"DENC_CGMS0",                      /* CGMS Register, byte 0 */
	 DENC,
	 DENC_CGMS0,
	 0x0f,
	 0x0f,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_CGMS1",                      /* CGMS Register, byte 1 */
	 DENC,
	 DENC_CGMS1,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_CGMS2",                      /* CGMS Register, byte 2 */
	 DENC,
	 DENC_CGMS2,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},

	{"DENC_TTX1",                      	/* TTX Conf/Lines 1 */
	 DENC,
	 DENC_TTX1,
	 0xff,
	 0xff,
	 0x40,
	 REG_32B,
	 0
	},
	{"DENC_TTX2",                      	/* TTX Lines 2 */
	 DENC,
	 DENC_TTX2,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_TTX3",                      	/* TTX Lines 3 */
	 DENC,
	 DENC_TTX3,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_TTX4",                      	/* TTX Lines 4 */
	 DENC,
	 DENC_TTX4,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_TTX5",                      	/* TTX Lines 5 */
	 DENC,
	 DENC_TTX5,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_TTX_CONF",                   /* TTX configuration */
	 DENC,
	 DENC_TTX_CONF,
	 0xe0,
	 0xe0,
	 0x50,
	 REG_32B,
	 0
	},
	{"DENC_D2M_TTX",                    /* DAC2Mult & TTX */
	 DENC,
	 DENC_D2M_TTX,
	 0xfb,
	 0xfb,
	 0x82,
	 REG_32B,
	 0
	},

	{"DENC_CCCF10",                     /* Closed Caption Characters/Extended Data for Field 1, byte 0 */
	 DENC,
	 DENC_CCCF10,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_CCCF11",                     /* Closed Caption Characters/Extended Data for Field 1, byte 1 */
	 DENC,
	 DENC_CCCF11,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_CCCF20",                     /* Closed Caption Characters/Extended Data for Field 2, byte 0 */
	 DENC,
	 DENC_CCCF20,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_CCCF21",                     /* Closed Caption Characters/Extended Data for Field 2, byte 1 */
	 DENC,
	 DENC_CCCF21,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_CCLIF1",                     /* Closed Caption/Extended Data line insertion for Field 1 */
	 DENC,
	 DENC_CCLIF1,
	 0x1f,
	 0x1f,
	 0x0f,
	 REG_32B,
	 0
	},
	{"DENC_CCLIF2",                     /* Closed Caption/Extended Data line insertion for Field 2 */
	 DENC,
	 DENC_CCLIF2,
	 0x1f,
	 0x1f,
	 0x0f,
	 REG_32B,
	 0
	},


	{"DENC_BRIGHT",                     /* Brightness */
	 DENC,
	 DENC_BRIGHT,
	 0xff,
	 0xff,
	 0x80,
	 REG_32B,
	 0
	},
	{"DENC_CONTRAST",                   /* Contrast */
	 DENC,
	 DENC_CONTRAST,
	 0xff,
	 0xff,
	 0,
	 REG_32B,
	 0
	},
	{"DENC_SATURATION",                 /* Saturation */
	 DENC,
	 DENC_SATURATION,
	 0xff,
	 0xff,
	 0x80,
	 REG_32B,
	 0
	},

	{"DENC_CCOEF0",                 /* Chroma Coef 0 */
	 DENC,
	 DENC_CCOEF0,
	 0xff,
	 0xff,
	 0x31,
	 REG_32B,
	 0
	},
	{"DENC_CCOEF1",                 /* Chroma Coef 1 */
	 DENC,
	 DENC_CCOEF1,
	 0xff,
	 0xff,
	 0x27,
	 REG_32B,
	 0
	},
	{"DENC_CCOEF2",                 /* Chroma Coef 2 */
	 DENC,
	 DENC_CCOEF2,
	 0xff,
	 0xff,
	 0x54,
	 REG_32B,
	 0
	},
	{"DENC_CCOEF3",                 /* Chroma Coef 3 */
	 DENC,
	 DENC_CCOEF3,
	 0xff,
	 0xff,
	 0x47,
	 REG_32B,
	 0
	},
	{"DENC_CCOEF4",                 /* Chroma Coef 4 */
	 DENC,
	 DENC_CCOEF4,
	 0xff,
	 0xff,
	 0x5F,
	 REG_32B,
	 0
	},
	{"DENC_CCOEF5",                 /* Chroma Coef 5 */
	 DENC,
	 DENC_CCOEF5,
	 0xff,
	 0xff,
	 0x77,
	 REG_32B,
	 0
	},
	{"DENC_CCOEF6",                 /* Chroma Coef 6 */
	 DENC,
	 DENC_CCOEF6,
	 0xff,
	 0xff,
	 0x6C,
	 REG_32B,
	 0
	},
	{"DENC_CCOEF7",                 /* Chroma Coef 7 */
	 DENC,
	 DENC_CCOEF7,
	 0xff,
	 0xff,
	 0x7B,
	 REG_32B,
	 0
	},
	{"DENC_CCOEF8",                 /* Chroma Coef 8 */
	 DENC,
	 DENC_CCOEF8,
	 0xff,
	 0xff,
	 0x80,
	 REG_32B,
	 0
	},

	{"DENC_LCOEF0",                 /* Luma Coef 0 */
	 DENC,
	 DENC_LCOEF0,
	 0xff,
	 0xff,
	 0x21,
	 REG_32B,
	 0
	},
	{"DENC_LCOEF1",                 /* Luma Coef 1 */
	 DENC,
	 DENC_LCOEF1,
	 0xff,
	 0xff,
	 0x7F,
	 REG_32B,
	 0
	},
	{"DENC_LCOEF2",                 /* Luma Coef 2 */
	 DENC,
	 DENC_LCOEF2,
	 0xff,
	 0xff,
	 0xF7,
	 REG_32B,
	 0
	},
	{"DENC_LCOEF3",                 /* Luma Coef 3 */
	 DENC,
	 DENC_LCOEF3,
	 0xff,
	 0xff,
	 0x03,
	 REG_32B,
	 0
	},
	{"DENC_LCOEF4",                 /* Luma Coef 4 */
	 DENC,
	 DENC_LCOEF4,
	 0xff,
	 0xff,
	 0x1F,
	 REG_32B,
	 0
	},
	{"DENC_LCOEF5",                 /* Luma Coef 5 */
	 DENC,
	 DENC_LCOEF5,
	 0xff,
	 0xff,
	 0xFB,
	 REG_32B,
	 0
	},
	{"DENC_LCOEF6",                 /* Luma Coef 6 */
	 DENC,
	 DENC_LCOEF6,
	 0xff,
	 0xff,
	 0xAC,
	 REG_32B,
	 0
	},
	{"DENC_LCOEF7",                 /* Luma Coef 7 */
	 DENC,
	 DENC_LCOEF7,
	 0xff,
	 0xff,
	 0x07,
	 REG_32B,
	 0
	},
	{"DENC_LCOEF8",                 /* Luma Coef 8 */
	 DENC,
	 DENC_LCOEF8,
	 0xff,
	 0xff,
	 0x3D,
	 REG_32B,
	 0
	},
	{"DENC_LCOEF9",                 /* Luma Coef 9 */
	 DENC,
	 DENC_LCOEF9,
	 0xff,
	 0xff,
	 0xF8,
	 REG_32B,
	 0
	},

	{"DENC_CFG",                      	/* Denc Output Stage Configuration */
	 DENC,
	 DENC_CFG,
	 0x3,
	 0x3,
	 0,
	 REG_32B | REG_SPECIAL | REG_SIDE_WRITE,
	 0
	},
	{"DENC_TTX",                      	/* Denc TTX */
	 DENC,
	 DENC_TTX,
	 0x3ffff00,
	 0x3ffff00,
	 0,
	 REG_32B,
	 0
	},

};  
    
/* Nbr of registers */
#define DENC_REGISTERS (sizeof(DencReg)/sizeof(Register_t))
    
    
    
#ifdef __cplusplus
}   
#endif
    
#endif /* __DENC_TREG_H */
/* ------------------------------- End of file ---------------------------- */


