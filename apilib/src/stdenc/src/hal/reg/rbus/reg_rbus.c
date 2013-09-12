/************************************************************************

File Name   : reg_rbus.c

Description : DENC register access functions by RBUS

COPYRIGHT (C) STMicroelectronics 2002

Date               Modification                                     Name
----               ------------                                     ----
06 Feb 2001        Created                                          HSdLM
07 Mar 2001        Add DENCRA_ before function table names and      HSdLM
 *                 routines to return function table addresses
22 Jun 2001        Exported symbols => stdenc_...                   HSdLM
 *                 To prevent issues with endianness, 16, 24 and
 *                 32 bits access routines have been removed.
30 Aou 2001        Use target dependent register shifts to support
 *                 ST40GX1.                                         HSdLM
14 Feb 2002        Add WA_GNBvd11019                                HSdLM
26 Jul 2002        Modify WA_GNBvd11019                             BS
*******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <stdio.h>
#include "stlite.h"
#endif

#include "stsys.h"
#include "denc_drv.h"
#include "denc_ra.h"
#include "reg_rbus.h"

#ifdef WA_GNBvd11019
#include "denc_reg.h"

/* Private Types ------------------------------------------------------------ */
typedef enum
{
    DEFAULT = 0,
    RESERVED = 1,
    READ_ONLY = 2,
    TO_RESET = 4
} RBUS_RegValDef_e;

typedef struct RBUS_ResetReg_s
{
    U8 Value;
    RBUS_RegValDef_e Def;
} RBUS_ResetReg_t;

/* Private Variables (static)------------------------------------------------ */

static const RBUS_ResetReg_t RegDefTable[MAX_DENC_REGISTERS] = {
    /* Address 0 -> 4 */
    { 0x92, DEFAULT  }, { 0x44, DEFAULT  }, { 0x20, DEFAULT  }, { 0x00, DEFAULT  }, { 0x00, DEFAULT  },
    /* Address 5 -> 9 */
    { 0x00, DEFAULT  }, { 0x10, DEFAULT  }, { 0x00, DEFAULT  }, { 0x20, DEFAULT  }, { 0x3E, READ_ONLY | TO_RESET },
    /* Address 10 -> 14 */
    { 0x2A, DEFAULT  }, { 0x09, DEFAULT  }, { 0x8B, DEFAULT  }, { 0xDB, DEFAULT  }, { 0xFD, DEFAULT  },
    /* Address 15 -> 19 */
    { 0xFC, TO_RESET }, { 0xE4, TO_RESET }, { 0x88, TO_RESET }, { 0x88, TO_RESET }, { 0x80, TO_RESET },
    /* Address 20 -> 24 */
    { 0x00, RESERVED }, { 0x00, TO_RESET }, { 0x80, TO_RESET }, { 0x00, TO_RESET }, { 0xB1, READ_ONLY | DEFAULT  },
    /* Address 25 -> 29 */
    { 0xFF, DEFAULT  }, { 0xAB, DEFAULT  }, { 0xFF, DEFAULT  }, { 0xCD, DEFAULT  }, { 0xA7, DEFAULT  },
    /* Address 30 -> 34 */
    { 0xC2, DEFAULT  }, { 0x0E, DEFAULT  }, { 0xEE, DEFAULT  }, { 0x9A, DEFAULT  }, { 0x40, TO_RESET },
    /* Address 35 -> 39 */
    { 0x00, TO_RESET }, { 0x00, TO_RESET }, { 0x00, TO_RESET }, { 0x00, TO_RESET }, { 0xAB, DEFAULT  },
    /* Address 40 -> 44 */
    { 0xDE, DEFAULT  }, { 0x7E, DEFAULT  }, { 0xFF, DEFAULT  }, { 0x0F, TO_RESET }, { 0x0F, TO_RESET },
    /* Address 45 -> 49 */
    { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED },
    /* Address 50 -> 54 */
    { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED },
    /* Address 55 -> 59 */
    { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED },
    /* Address 60 -> 64 */
    { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x50, TO_RESET },
    /* Address 65 -> 69 */
    { 0x82, TO_RESET }, { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x00, RESERVED }, { 0x80, TO_RESET },
    /* Address 70 -> 74 */
    { 0x00, TO_RESET }, { 0x80, TO_RESET }, { 0x31, TO_RESET }, { 0x27, TO_RESET }, { 0x54, TO_RESET },
    /* Address 75 -> 79 */
    { 0x47, TO_RESET }, { 0x5F, TO_RESET }, { 0x77, TO_RESET }, { 0x6C, TO_RESET }, { 0x7B, TO_RESET },
    /* Address 80 -> 84 */
    { 0x80, TO_RESET }, { 0x22, TO_RESET }, { 0x21, TO_RESET }, { 0x7F, TO_RESET }, { 0xF7, TO_RESET },
    /* Address 85 -> 89 */
    { 0x03, TO_RESET }, { 0x1F, TO_RESET }, { 0xFB, TO_RESET }, { 0xAC, TO_RESET }, { 0x07, TO_RESET },
    /* Address 90 -> 91 */
    { 0x3D, TO_RESET }, { 0xC0, TO_RESET }
};
#endif /* WA_GNBvd11019 */

/* Private Constants ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */
#define GetBaseAddress(Device_p) ((U8 *)(((const DENCDevice_t * const)(Device_p))->BaseAddress_p))

/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t RBUS_ReadReg8(const void * const Device_p, const U32 Addr_p, U8 * const Value_p);
static ST_ErrorCode_t RBUS_WriteReg8(const void * const Device_p, const U32 Addr, const U8 Value);
static ST_ErrorCode_t RBUS_ShiftedReadReg8(const void * const Device_p, const U32 Addr, U8 * const Value_p);
static ST_ErrorCode_t RBUS_ShiftedWriteReg8(const void * const Device_p, const U32 Addr, const U8 Value);

/* Global Variables --------------------------------------------------------- */

/* Function pointer tables.*/

/* READ / WRITE FUNCTIONS FOR 8bits addressing targets (ex: STi5512)
 * 8 bits values are actually read/written.
 * */

const stdenc_RegFunctionTable_t stdenc_RBUSFunctionTable = {
    RBUS_ReadReg8,
    RBUS_WriteReg8
};

/* SHIFTED VERSION OF READ / WRITE FUNCTIONS FOR 32bits addressing targets (ex: STi7015)
 * 32bits values are actually read/written.
 * Little Endianness use. For 32bits : *Addr = LSB ... *(Addr+4) = MSB
 * */

const stdenc_RegFunctionTable_t stdenc_RBUSShiftedFunctionTable = {
    RBUS_ShiftedReadReg8,
    RBUS_ShiftedWriteReg8
};


/* Functions ---------------------------------------------------------------- */
#ifdef WA_GNBvd11019
static void RBUS_WriteDefaultValue(DENCDevice_t * const Device_p, const RBUS_RegValDef_e Reason)
{
    U8 i;

    switch(Reason)
    {
        case TO_RESET:
            /* Reset all register of ram image */
            for(i=0; i< MAX_DENC_REGISTERS; i++)
            {
                if(RegDefTable[i].Def & TO_RESET)
                {
                    Device_p->RegRamImage[i] = RegDefTable[i].Value;
                }
            }
            break;

        case DEFAULT:
            /* Set default value only once at init */
            for(i=0; i< MAX_DENC_REGISTERS; i++)
            {
                if((RegDefTable[i].Def & RESERVED) == 0)
                {
                    Device_p->RegRamImage[i] = RegDefTable[i].Value;
                }
            }
            Device_p->RamToDefault = TRUE;
            break;

        default:
            break;
    }
}

static ST_ErrorCode_t RBUS_ReadInRamImage(DENCDevice_t * const Device_p, const U32 Addr, U8 * const Value_p)
{
    if (Addr >= MAX_DENC_REGISTERS)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!Device_p->RamToDefault)
    {
        RBUS_WriteDefaultValue(Device_p, DEFAULT);
    }

    *Value_p = Device_p->RegRamImage[Addr];
    return(ST_NO_ERROR);
}

static ST_ErrorCode_t RBUS_WriteInRamImage(DENCDevice_t * const Device_p, const U32 Addr, const U8 Value)
{
    if (Addr >= MAX_DENC_REGISTERS)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (!Device_p->RamToDefault)
    {
        RBUS_WriteDefaultValue(Device_p, DEFAULT);
    }

    /* No check if write in a reserved register */
    /* Because it occurs for macrovision */
    if((RegDefTable[Addr].Def & READ_ONLY) == 0)
    {
        Device_p->RegRamImage[Addr] = Value;
    }

    if((Value & DENC_CFG6_RST_SOFT) && (Addr == DENC_CFG6))
    {
        RBUS_WriteDefaultValue(Device_p, TO_RESET);
        Device_p->RegRamImage[Addr] = Value & ~(DENC_CFG6_RST_SOFT);
    }
    return(ST_NO_ERROR);
}
#endif /* WA_GNBvd11019 */

static ST_ErrorCode_t RBUS_ReadReg8(const void * const Device_p, const U32 Addr, U8 * const Value_p)
{
#ifdef WA_GNBvd11019
    return( RBUS_ReadInRamImage((DENCDevice_t * const) Device_p, Addr, Value_p) );
#else /* WA_GNBvd11019 */
    *Value_p = STSYS_ReadRegDev8 (GetBaseAddress(Device_p) + (Addr));
    return(ST_NO_ERROR);
#endif /* WA_GNBvd11019 */
}

static ST_ErrorCode_t RBUS_WriteReg8(const void * const Device_p, const U32 Addr, const U8 Value)
{
#ifdef WA_GNBvd11019
    STSYS_WriteRegDev8( GetBaseAddress(Device_p) + (Addr), Value);
    return(RBUS_WriteInRamImage((DENCDevice_t * const) Device_p, Addr, Value));
#else
    STSYS_WriteRegDev8( GetBaseAddress(Device_p) + (Addr), Value);
    return(ST_NO_ERROR);
#endif /* WA_GNBvd11019 */
}

/* ----------------------------------------------------------------------- */
/* SHIFTED VERSION OF READ / WRITE FUNCTIONS FOR 32bits addressing targets */
/* ----------------------------------------------------------------------- */

static ST_ErrorCode_t RBUS_ShiftedReadReg8(const void * const Device_p, const U32 Addr, U8 * const Value_p)
{
    U32 Address;

    Address = (Addr) << (U8)(((const DENCDevice_t * const)Device_p)->RegShift);
    *Value_p = (U8)STSYS_ReadRegDev32LE( (U32)GetBaseAddress(Device_p) + Address );
    return(ST_NO_ERROR);
}

static ST_ErrorCode_t RBUS_ShiftedWriteReg8(const void * const Device_p, const U32 Addr, const U8 Value)
{
    U32 Address;

    Address = (Addr) << (U8)(((const DENCDevice_t * const)Device_p)->RegShift);
    STSYS_WriteRegDev32LE( (U32)GetBaseAddress(Device_p) + Address, (U32)Value);
    return(ST_NO_ERROR);
}

/* Public functions                                                         */

/*******************************************************************************
Name        : stdenc_RBUSGetFunctionTableAddress
Description : return address of stdenc_RBUSFunctionTable
Parameters  : none
Assumptions :
Limitations :
Returns     : address of stdenc_RBUSFunctionTable
*******************************************************************************/
const stdenc_RegFunctionTable_t * stdenc_GetRBUSFunctionTableAddress(void)
{
    return(&stdenc_RBUSFunctionTable);
}

/*******************************************************************************
Name        : stdenc_RBUSGetShiftedFunctionTableAddress
Description : return address of stdenc_RBUSShiftedFunctionTable
Parameters  : none
Assumptions :
Limitations :
Returns     : address of stdenc_RBUSShiftedFunctionTable
*******************************************************************************/
const stdenc_RegFunctionTable_t * stdenc_GetRBUSShiftedFunctionTableAddress(void)
{
    return(&stdenc_RBUSShiftedFunctionTable);
}


/* ----------------------------- End of file ------------------------------ */

