/*****************************************************************************

File name   : cfg_treg.h

Description : Configuration register description for test

COPYRIGHT (C) ST-Microelectronics 1999, 2000.

Date            Modification                                                Name
----            ------------                                                ----
15-Sep-1999     Created                                                     FC
13-Jan-2000     Added LMI registers                                         FC
21-Mar-2000     Changed all LMI regsisters                                  FC
21-Jun-2000     Changed CFG_INT_LINE to EMPI_INT_LINE                       PC
20-Jul-2000     Added Clock Gen registers                                   FC
26-Nov-2001     Move video configuration registers into video_treg.h        JFC
                (It isn't conform to 7015 data sheet naming but it is more
                logical that way and prepares for video IP that manage
                their own configuration registers).

TODO:
=====
TODO: CFG_MCF
TODO: CFG_MST
TODO: All clock-gen registers

*****************************************************************************/

#ifndef __CFG_TREG_H
#define __CFG_TREG_H

/* Includes --------------------------------------------------------------- */

#include "dumpreg.h"
#include "cfg_reg.h"

#ifdef __cpluCFGlus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Registers description */
static const Register_t CFGReg[] =
{
	{"CFG_CCF",	       			/* Chip Configuration, enable interfaces */
	 CFG,
	 CFG_CCF,
	 0x7,
	 0x7,
	 0x0,
	 REG_32B,
	 0
	},
	{"CFG_CDCF",	       			/* CD input configuration */
	 CFG,
	 CFG_CDCF,
	 0x3,
	 0x3,
	 0x0,
	 REG_32B,
	 0
	},
	{"CFG_CDG",	       			/* CD Mngt config, bitstream channel gathering */
	 CFG,
	 CFG_CDG,
	 0x3f,
	 0x3f,
	 0x0,
	 REG_32B,
	 0
	},
	{"CFG_IDCOD",	       		/* JTAG Id */
	 CFG,
	 CFG_IDCOD,
	 0xffffffff,
	 0x0,
	 0x0,
	 REG_32B,
	 0
	},
	{"CFG_USRID",	       		/* User Id */
	 CFG,
	 CFG_USRID,
	 0xffffffff,
	 0x0,
	 0x0,
	 REG_32B,
	 0
	},

	{"EMPI_INT_LINE",	       	/* Interrupt Line Register */
	 CFG,
	 EMPI_INT_LINE,
	 0x7ffff,
	 0x0,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_WPRI",	       		/* Priority of write micro accesses */
	 CFG,
	 EMPI_WPRI,
	 0xfff,
	 0xfff,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_RPRI",	       		/* Priority of read micro accesses */
	 CFG,
	 EMPI_RPRI,
	 0xfff,
	 0xfff,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_DPRI",	       		/* Priority of DMA micro accesses */
	 CFG,
	 EMPI_DPRI,
	 0xff,
	 0xff,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_WTIM",	       		/* DMA flush timer */
	 CFG,
	 EMPI_WTIM,
	 0xfff,
	 0xfff,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_PTIM",	       		/* Timer for micro priority latency */
	 CFG,
	 EMPI_PTIM,
	 0x3ff,
	 0x3ff,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_RCFG",	       		/* Configuration of read ahead */
	 CFG,
	 EMPI_RCFG,
	 0x3,
	 0x3,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_DADD0",	   	     	/* DMA0 channel start address */
	 CFG,
	 EMPI_DADD0,
	 0x3ffffff,
	 0x3ffffff,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_DADD1",	       		/* DMA1 channel start address */
	 CFG,
	 EMPI_DADD1,
	 0x3ffffff,
	 0x3ffffff,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_END",	  			/* Configuration of endianness (all 0 or all 1) */
	 CFG,
	 EMPI_END,
	 0xffffffff,
	 0xffffffff,
	 0x0,
	 REG_32B | REG_SPECIAL | REG_SIDE_WRITE,
	 0
	},
	{"EMPI_DCFG0",	   	     	/* DMA0 channel config */
	 CFG,
	 EMPI_DCFG0,
	 0x1f,
	 0x1f,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_DCFG1",	   	     	/* DMA1 channel config */
	 CFG,
	 EMPI_DCFG1,
	 0x1f,
	 0x1f,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_ITM",	   	     	/* Interrupt mask */
	 CFG,
	 EMPI_ITM,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
	{"EMPI_ITS",	   	     	/* Interrupt status */
	 CFG,
	 EMPI_ITS,
	 0x1,
	 0x0,
	 0x0,
	 REG_32B | REG_SPECIAL | REG_SIDE_READ,
	 0
	},
	{"EMPI_STA",	   	     	/* Status register */
	 CFG,
	 EMPI_STA,
	 0x1,
	 0x0,
	 0x0,
	 REG_32B,
	 0
	},

	{"LMI_TRCR",
	 LMI,
	 LMI_TRCR,
	 0xf,
	 0xf,
	 0x9,
	 REG_32B,
	 0
	},
	{"LMI_TRC",
	 LMI,
	 LMI_TRC,
	 0xf,
	 0xf,
	 0x8,
	 REG_32B,
	 0
	},
	{"LMI_TRP",
	 LMI,
	 LMI_TRP,
	 0x3,
	 0x3,
	 0x3,
	 REG_32B,
	 0
	},
	{"LMI_TWRD",
	 LMI,
	 LMI_TWRD,
	 0x7,
	 0x7,
	 0x3,
	 REG_32B,
	 0
	},
	{"LMI_TRAS",
	 LMI,
	 LMI_TRAS,
	 0xf,
	 0xf,
	 0x6,
	 REG_32B,
	 0
	},
	{"LMI_TRRD",
	 LMI,
	 LMI_TRRD,
	 0x3,
	 0x3,
	 0x2,
	 REG_32B,
	 0
	},
	{"LMI_TRCD",
	 LMI,
	 LMI_TRCD,
	 0x3,
	 0x3,
	 0x3,
	 REG_32B,
	 0
	},
	{"LMI_TDPLR",
	 LMI,
	 LMI_TDPLR,
	 0x3,
	 0x3,
	 0x1,
	 REG_32B,
	 0
	},
	{"LMI_TDPLW",
	 LMI,
	 LMI_TDPLW,
	 0x7,
	 0x7,
	 0x3,
	 REG_32B,
	 0
	},
	{"LMI_TBSTW",
	 LMI,
	 LMI_TBSTW,
	 0x3,
	 0x3,
	 0x3,
	 REG_32B,
	 0
	},
	{"LMI_LAF_DEPTH",
	 LMI,
	 LMI_LAF_DEPTH,
	 0xf,
	 0xf,
	 0x7,
	 REG_32B,
	 0
	},
	{"LMI_EMRS",
	 LMI,
	 LMI_EMRS,
	 0x1ff,
	 0x1ff,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_MRS",
	 LMI,
	 LMI_MRS,
	 0x1ff,
	 0x1ff,
	 0x23,
	 REG_32B,
	 0
	},
	{"LMI_REQ_EMRS",
	 LMI,
	 LMI_REQ_EMRS,
	 0x0,
	 0xffffffff,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_REQ_MRS",
	 LMI,
	 LMI_REQ_MRS,
	 0x0,
	 0xffffffff,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_REQ_CBR",
	 LMI,
	 LMI_REQ_CBR,
	 0x0,
	 0xffffffff,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_REQ_PALL",
	 LMI,
	 LMI_REQ_PALL,
	 0x0,
	 0xffffffff,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_CKE",
	 LMI,
	 LMI_CKE,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_ENA",
	 LMI,
	 LMI_ENA,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_CFG",
	 LMI,
	 LMI_CFG,
	 0x10fff,
	 0x10fff,
	 0x2,
	 REG_32B,
	 0
	},
	{"LMI_REF_CFG",
	 LMI,
	 LMI_REF_CFG,
	 0x107ff,
	 0x107ff,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_IO_CFG",
	 LMI,
	 LMI_IO_CFG,
	 0x3fffff,
	 0x3fffff,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_DLL_CTRL",
	 LMI,
	 LMI_DLL_CTRL,
	 0x7f,
	 0x7f,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_DLL_STATUS",
	 LMI,
	 LMI_DLL_STATUS,
	 0x3ff,
	 0x0,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_CTRL",	   	     	/* LMI control */
	 LMI,
	 LMI_CTRL,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_DLL_RST",	   	     	/* DLL Reset control */
	 LMI,
	 LMI_DLL_RST,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
	{"LMI_DLL_LCKCTL",	   	     	/* DLL Lock control */
	 LMI,
	 LMI_DLL_LCKCTL,
	 0xf,
	 0xf,
	 0x0,
	 REG_32B,
	 0
	},

	{"CKG_SYN0_PAR",	   	    /* Frequency synthesizer parameters           */
	 CKG,
	 CKG_SYN0_PAR,
	 0xfffffff,
	 0xfffffff,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SYN1_PAR",	   	    /* Frequency synthesizer parameters  		  */
	 CKG,
	 CKG_SYN1_PAR,
	 0xfffffff,
	 0xfffffff,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SYN2_PAR",	   	    /* Frequency synthesizer parameters  		  */
	 CKG,
	 CKG_SYN2_PAR,
	 0xfffffff,
	 0xfffffff,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SYN3_PAR",	   	    /* Frequency synthesizer parameters  		  */
	 CKG,
	 CKG_SYN3_PAR,
	 0xfffffff,
	 0xfffffff,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SYN4_PAR",	   	    /* Frequency synthesizer parameters  		  */
	 CKG,
	 CKG_SYN4_PAR,
	 0xfffffff,
	 0xfffffff,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SYN5_PAR",	   	    /* Frequency synthesizer parameters  		  */
	 CKG,
	 CKG_SYN5_PAR,
	 0xfffffff,
	 0xfffffff,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SYN6_PAR",	   	    /* Frequency synthesizer parameters  		  */
	 CKG,
	 CKG_SYN6_PAR,
	 0xfffffff,
	 0xfffffff,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SYN7_PAR",	   	    /* Frequency synthesizer parameters  		  */
	 CKG,
	 CKG_SYN7_PAR,
	 0xfffffff,
	 0xfffffff,
	 0x0,
	 REG_32B,
	 0
	},

    {"CKG_SYN3_REF",	   	    /* Synthesizer 3 ref clock selection 		  */
	 CKG,
	 CKG_SYN3_REF,
	 0x3,
	 0x3,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SYN7_REF",	   	    /* Synthesizer 7 denc pixel clock selection   */
	 CKG,
	 CKG_SYN7_REF,
	 0x3,
	 0x3,
	 0x0,
	 REG_32B,
	 0
	},

    {"CKG_PCMO_CFG",	   	    /* PCM output clock generation config		  */
	 CKG,
	 CKG_PCMO_CFG,
	 0x7f,
	 0x7f,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_PCMO_MD",	   	     	/* PCM output freq synth MD parameter		  */
	 CKG,
	 CKG_PCMO_MD,
	 0x1f,
	 0x1f,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_PCMO_PE",	   	     	/* PCM output freq synth PE parameter		  */
	 CKG,
	 CKG_PCMO_PE,
	 0xffff,
	 0xffff,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_PCMO_PSEL",	   	    /* PCM output freq synth parameter selection  */
	 CKG,
	 CKG_PCMO_PSEL,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_PCMO_SDIV",	   	    /* PCM output freq synth SDIV parameter  	  */
	 CKG,
	 CKG_PCMO_SDIV,
	 0x7,
	 0x7,
	 0x0,
	 REG_32B,
	 0
	},

    {"CKG_PRE_DIV",	   	     	/* PreDivider setup  						  */
	 CKG,
	 CKG_PRE_DIV,
	 0xf,
	 0xf,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_AHS",	   	     	/* Audio bit handshake clock selection		  */
	 CKG,
	 CKG_SEL_AHS,
	 0x3,
	 0x3,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_AUD",	   	     	/* Audio DSP clock selection 				  */
	 CKG,
	 CKG_SEL_AUD,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_DENC",	   	    /* DENC and auxiliary pixel clock selection   */
	 CKG,
	 CKG_SEL_DENC,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_DDR",	   	     	/* DDR interface clock selection 			  */
	 CKG,
	 CKG_SEL_DDR,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_DEC",	   	     	/* Video decoder decompression clock selection*/
	 CKG,
	 CKG_SEL_DEC,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_DISP",	   	    /* Display processors clock selection		  */
	 CKG,
	 CKG_SEL_DISP,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_GFX",	   	     	/* Graphics engine clock selection			  */
	 CKG,
	 CKG_SEL_GFX,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_MEM",	   	     	/* Memory subsystem clock selection  		  */
	 CKG,
	 CKG_SEL_MEM,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_PIX",	   	     	/* Main pixel clock selection				  */
	 CKG,
	 CKG_SEL_PIX,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_RCLK2",	   	    /* RCLK2 selection							  */
	 CKG,
	 CKG_SEL_RCLK2,
	 0x3,
	 0x3,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_USR1",	   	    /* User clock 1 selection					  */
	 CKG,
	 CKG_SEL_USR1,
	 0x3,
	 0x3,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_USR2",	   	    /* User clock 2 selection					  */
	 CKG,
	 CKG_SEL_USR2,
	 0x3,
	 0x3,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_USR3",	   	    /* User clock 3 selection					  */
	 CKG,
	 CKG_SEL_USR3,
	 0x3,
	 0x3,
	 0x0,
	 REG_32B,
	 0
	},
    {"CKG_SEL_VID",	   	     	/* Video MPEG decoder clock selection		  */
	 CKG,
	 CKG_SEL_VID,
	 0x1,
	 0x1,
	 0x0,
	 REG_32B,
	 0
	}

};

/* Nbr of registers */
#define CFG_REGISTERS (sizeof(CFGReg)/sizeof(Register_t))



#ifdef __cpluCFGlus
}
#endif

#endif /* __CFG_TREG_H */
/* ------------------------------- End of file ---------------------------- */



