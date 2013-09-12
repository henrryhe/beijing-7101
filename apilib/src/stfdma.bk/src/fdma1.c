/******************************************************************************

    File Name   : 

    Description : 

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#if defined (ST_OSLINUX)
#include <linux/stddef.h>
#include <linux/interrupt.h>
#include <asm/semaphore.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#endif

#ifdef STFDMA_EMU                       /* For test purposes only */
#define STTBX_Print(x)  printf x
#else
#if !defined (ST_OSLINUX)
#include "sttbx.h"
#endif
#endif

#include "local.h"
#include "fdmacfg.h"    /* FDMA block config table */

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Indicator for end of config data table */
#define FDMA_CONFIG_TABLE_END      0xffffffff   

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */







int stfdma_FDMA1Conf(void *ProgAddr, void *DataAddr)
{
    int i;
    
    /* Clear DMEM region to 0.
     * DMEM region exists between the start of the DMEM offset and the start of the IMEM offset.
     * Both offsets are offsets from the FDMA device base address.
     * NOTE: volatile and #pragma device_t not used since area being written is memory that is
     * not expected to be changed by other thread or device during write.
     */

    STTBX_Print(("Clearing DMEM\n"));     
    i = 0;
    while (i  < (IMEM_OFFSET - DMEM_OFFSET))
    {
        *((U32*)((char*)DataAddr + i)) = 0;
        
        /*  Clearing U32s and so need skip 4bytes each time */
        i+=sizeof(U32);
    }
    
    /* Load FDMA config data from external table.
     * Data stored in "address, data" pairs in table.
     * Calculate address from baseaddress and address, then write data to location.
     * NOTE: volatile and #pragma device_t not used since area being written is memory that is
     * not expected to be changed by other thread or device during write.         
     */

    STTBX_Print(("Loading FDMA config data\n"));
    i = 0;
    while (FDMAConfig[i] != FDMA_CONFIG_TABLE_END)
    {        
        *((U32*)((char*)ProgAddr + FDMAConfig[i])) = FDMAConfig[i+1];
        
        /* Move to next "address,data" pair start */
        i+=2;
    }
    
    return 0;
}
