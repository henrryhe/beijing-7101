/*******************************************************************************

File name   : vin_drv.h

Description : vin driver header file (unit relative definitions)

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
27 Sep 2000        Created                                           JA
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIN_DRV_H
#define __VIN_DRV_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stos.h"
#include "stvin.h"
#include "vin_init.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */


  /* EXAMPLE */
typedef enum
{
    VIN_UNIT_STATUS_STOPPED,
    VIN_UNIT_STATUS_RUNNING,
    VIN_UNIT_STATUS_PAUSING
} VinUnitStatus_t;

typedef struct
{
    stvin_Device_t *Device_p;   /* Pointer to the device to be able to retrieve the init() concerned. */
    U32 UnitValidity;           /* Only the value VIN_VALID_UNIT means the unit is valid */
} stvin_Unit_t;


/* Exported Constants ------------------------------------------------------- */

#define STVIN_INVALID_HANDLE  ((STVIN_Handle_t) NULL)

#if !(defined(ST_7710) || defined(ST_5528) || defined(ST_7100) || defined(ST_7109))
#define STVIN_MAX_DEVICE  2     /* Max number of Init() allowed */
#else
#define STVIN_MAX_DEVICE  1     /* Max number of Init() allowed */
#endif
#define STVIN_MAX_OPEN    2     /* Max number of Open() allowed per Init() */
#define STVIN_MAX_UNIT    (STVIN_MAX_OPEN * STVIN_MAX_DEVICE)


/* Below is a 'magic number', unique for each driver.
Proposed syntax: 0xXY0ZY0ZX, where XYZ is the driver ID in hexa
  ex: id=21  decimal (0x015) ->  0x01051050
  ex: id=142 decimal (0x08e) ->  0x080e80e0
  ex: id=333 decimal (0x14d) ->  0x140d40d1

  => id=161 decimal (0x0A1) ->  0x0A01A010

This should be an uncommon number, without 00's.
This number enables to detect validity of the handle, supposing
there will never be this number by chance in memory.
Note: Drivers could use the same syntax for handles of internal modules, replacing the 0's
by a module ID like this: 0xXYnZYnZX, where n is the module ID. */
#define STVIN_VALID_UNIT      0x0A01A010

/* Exported Variables ------------------------------------------------------- */

extern stvin_Unit_t stvin_UnitArray[];


/* Exported Macros ---------------------------------------------------------- */

/* Passing a (STVIN_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((stvin_Unit_t *) (Handle)) >= stvin_UnitArray) &&                    \
                               (((stvin_Unit_t *) (Handle)) < (stvin_UnitArray + STVIN_MAX_UNIT)) &&  \
                               (((stvin_Unit_t *) (Handle))->UnitValidity == STVIN_VALID_UNIT))




/* Exported Functions ------------------------------------------------------- */



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIN_DRV_H */

/* End of vin_drv.h */
