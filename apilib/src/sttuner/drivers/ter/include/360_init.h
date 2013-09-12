#ifndef H_360INIT
	#define H_360INIT
	#include "chip.h"
	#ifdef HOST_PC
	#include "gen_types.h"
	#endif
	
/*	A AJOUTER dans 360_Init.h*/
/* structures -------------------------------------------------------------- */

    typedef struct
    {
        STCHIP_Info_t *Chip;     /* pointer to parameters to pass to the CHIP API */
        U32            NbDefVal; /* number of default values (must match number of 399 registers) */
        U8            *DefVal;   /* pointer to table of default values */
    } STV0360_InitParams_t;


/* functions --------------------------------------------------------------- */

/* create instance of 399 register mappings */
STCHIP_Handle_t STV0360_Init(STV0360_InitParams_t *InitParams);  
	
	


#endif













