/************************************************************************

File Name   : denc_drv.h

Description : internal driver description

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
06.06.00           Creation.                                         JG
16.10.00           Modification for multi-init ...,                  JG
 *                 Hal for STV0119 and STI55xx,70xx
06 Feb 2001        Reorganize HAL for providing mutual exclusion on  HSdLM
 *                 registers access
22 Jun 2001        Handle new optional setting.                      HSdLM
 *                 To prevent issues with endianness, 16, 24 and
 *                 32 bits access routines have been removed.
30 Aou 2001        Remove dependency upon STI2C if not needed, to    HSdLM
 *                 support ST40GX1
14 Feb 2002        Add WA_GNBvd11019                                 HSdLM
26 Jul 2002        Modify WA_GNBvd11019                              BS
13 Sep 2004        Add support for ST40/OS21                         MH
***********************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __DENCDRV_H
#define __DENCDRV_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stdenc.h"
#include "denc_ra.h"
#include "stos.h"
#ifdef STDENC_I2C
#include "sti2c.h"
#endif /* #ifdef STDENC_I2C */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* work-arounds STi5514 */
#if defined (ST_5514)
#define WA_GNBvd11019 /* Create a Ram image of all DENC registers */
#endif /* ST_5514 */



#define STDENC_MAX_DEVICE       4
#define STDENC_MAX_OPEN         10

#define STDENC_MIN_CHROMA_DELAY (-4)
#define STDENC_MAX_CHROMA_DELAY_V10_MORE 5
#define STDENC_MAX_CHROMA_DELAY_V10_LESS 4
#define STDENC_STEP_CHROMA_DELAY_V10_MORE 1
#define STDENC_STEP_CHROMA_DELAY_V10_LESS 2

#define STDENC_VALID_UNIT  0x01011010    /* 0xXY0ZY0ZX with XYZ = 011 (STDENC_DRIVER_ID = 17 = 0x011) */


/* Constants for workarounds */
#ifdef WA_GNBvd11019
#define MAX_DENC_REGISTERS 92
#endif /* WA_GNBvd11019 */

#if defined (ST_5528)
#define WA_GNBvd36366
#endif
#if defined WA_GNBvd36366
extern U8 TempReg;
#endif


/* Exported Types --------------------------------------------------------- */
typedef enum STDENC_DencId_e
{
    STDENC_DENC_ID_1,
    STDENC_DENC_ID_2
} STDENC_DencId_t;

typedef U8 HALDENC_Version_t;

typedef struct
{
    /* Device identification */
    ST_DeviceName_t         DeviceName;

    /* Device properties set by init parameters, set at init and not modifiable afterwards */
    STDENC_DeviceType_t     DeviceType;
    STDENC_AccessType_t     AccessType;
    U32                     MaxOpen;

    /* Device properties induced by init parameters, set at init and not modifiable afterwards */
    HALDENC_Version_t       DencVersion;
    void *                  BaseAddress_p;

#ifdef ST_OSLINUX
	U32 					DencMappedWidth;
	void *					DencMappedBaseAddress_p; /* Address where will be mapped the driver
	registers */
#endif

#ifdef STDENC_I2C
    STI2C_Handle_t          I2CHandle;
#endif /* #ifdef STDENC_I2C */
    U8                      RegShift; /* register access shift */

    /* Device properties modifiable from API */
    STDENC_EncodingMode_t   EncodingMode;
    BOOL                    SECAMSquarePixel;
    BOOL                    BlackLevelPedestal;
    BOOL                    BlackLevelPedestalAux; /* only for macrocell version >= 12 */
    S8                      ChromaDelay;
    S8                      ChromaDelayAux;
    BOOL                    LumaTrapFilter;
    BOOL                    LumaTrapFilterAux;
    BOOL                    YCbCr444Input;

    /* Device properties for inside use, not seen from API */
    U8                      RegCfg0;
    semaphore_t             *RegAccessMutex_p;  /* protect register access */
    semaphore_t             *ModeCtrlAccess_p;  /* protect accessing encoding mode data from API */
    const stdenc_RegFunctionTable_t * FunctionsTable_p;

#ifdef WA_GNBvd11019
    U8                      RegRamImage[MAX_DENC_REGISTERS];
    BOOL                    RamToDefault;
#endif /* WA_GNBvd11019 */
    BOOL                    IsAuxEncoderOn;
    BOOL                    IsExternal;
    STDENC_DencId_t         DencId;

} DENCDevice_t;

typedef struct
{
    DENCDevice_t *          Device_p;
    U32                     UnitValidity;
} DENC_t;

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

#define HALDENC_RegAccessLock(Dev)       semaphore_wait((Dev)->RegAccessMutex_p)
#define HALDENC_RegAccessUnlock(Dev)     semaphore_signal((Dev)->RegAccessMutex_p)
#define HALDENC_ReadReg8(Dev,Addr,Val)   ((Dev)->FunctionsTable_p->ReadReg8(  (Dev),(Addr),(Val)))

#if defined WA_GNBvd36366       /* pb with DENC register accesses on 5528 cut2.0 */
    #define HALDENC_WriteReg8(Dev,Addr,Val)  (Dev)->FunctionsTable_p->WriteReg8( (Dev),(Addr),(Val));    \
                                                                                  HALDENC_ReadReg8(Dev,Addr,&TempReg)
#else
    #define HALDENC_WriteReg8(Dev,Addr,Val)  ((Dev)->FunctionsTable_p->WriteReg8( (Dev),(Addr),(Val)))
#endif

/* Exported Functions ----------------------------------------------------- */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DENCDRV_H */

/* End of denc_drv.h */


