/*******************************************************************************

File name   : evin.h

Description : EXTVIN module header file (device relative definitions)

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
28 Nov 2000        Created                                           BD
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __EXTVIN_H
#define __EXTVIN_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sti2c.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STEXTVIN_MAX_DEVICE  3     /* Max number of Init() allowed */
#define STEXTVIN_MAX_OPEN    1     /* Max number of Open() allowed per Init() */
#define STEXTVIN_MAX_UNIT    (STEXTVIN_MAX_OPEN * STEXTVIN_MAX_DEVICE)

/*#define STEXTVIN_INVALID_HANDLE  ((STEXTVIN_Handle_t) NULL)*/


/* Below is a 'magic number', unique for each driver.
Proposed syntax: 0xXY0ZY0ZX, where XYZ is the driver ID in hexa
   id=172 decimal (0x0ac) ->  0x0a0ca0c0

This should be an uncommon number, without 00's.
This number enables to detect validity of the handle, supposing
there will never be this number by chance in memory.
Note: Drivers could use the same syntax for handles of internal modules, replacing the 0's
by a module ID like this: 0xXYnZYnZX, where n is the module ID. */
#define STEXTVIN_VALID_UNIT      0x0a0ca0c0



/* Exported Types ----------------------------------------------------------- */

typedef struct
{
    ST_DeviceName_t DeviceName;
    U32             MaxOpen;
    /* common informations for all open on a particular device */
    ST_Partition_t *CPUPartition_p;  /* Where the module allocated memory for its own usage,
                                   also where it will have to be freed at termination */
    void *MemoryAllocated_p;    /* Pointer to the memory to free at termination */

    STEXTVIN_DeviceType_t  DeviceType;
    ST_DeviceName_t        I2CDeviceName;
    U8                     I2CAddress;

} stextvin_Device_t;

typedef struct
{
    stextvin_Device_t *Device_p;   /* Pointer to the device to be able to retrieve the init() concerned. */
    U32 UnitValidity;           /* Only the value EXTVIN_VALID_UNIT means the unit is valid */


} stextvin_Unit_t;/* Open structure *****/


/* Exported Variables ------------------------------------------------------- */

extern stextvin_Unit_t STEXTVIN_UnitArray[STEXTVIN_MAX_UNIT];
extern STI2C_Handle_t  I2cHandle_SAA7114;
extern STI2C_Handle_t  I2cHandle_TDA8752;
extern STI2C_Handle_t  I2cHandle_SAA7111; /* DB331 board */
extern STI2C_Handle_t  I2cHandle_STV2310;


/* Exported Macros ---------------------------------------------------------- */
/* Passing a (STEXTVIN_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((stextvin_Unit_t *) (Handle)) >= STEXTVIN_UnitArray) &&                    \
                               (((stextvin_Unit_t *) (Handle)) < (STEXTVIN_UnitArray + STEXTVIN_MAX_UNIT)) &&  \
                               (((stextvin_Unit_t *) (Handle))->UnitValidity == STEXTVIN_VALID_UNIT))


/* Exported Functions ------------------------------------------------------- */




/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __EXTVIN_H */

/* End of evin.h */
