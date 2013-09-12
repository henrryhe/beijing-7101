/************************************************************************

File Name   : vout_drv.h

Description :

COPYRIGHT (C) STMicroelectronics 2000

***********************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VOUT_DRV_H
#define __VOUT_DRV_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stos.h"

#include "sttbx.h"

#include "stdenc.h"
#include "stvout.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */
/* Below is a 'magic number', unique for each driver.
Proposed syntax: 0xXY0ZY0ZX, where XYZ is the driver ID in hexa
  ex: id=21  decimal (0x015) ->  0x01051050
  ex: id=142 decimal (0x08e) ->  0x080e80e0
  ex: id=333 decimal (0x14d) ->  0x140d40d1
This should be an uncommon number, without 00's.
This number enables to detect validity of the handle, supposing
there will never be this number by chance in memory.
Note: Drivers could use the same syntax for handles of internal modules, replacing the 0's
by a module ID like this: 0xXYnZYnZX, where n is the module ID. */
#define STVOUT_VALID_UNIT      0x09029020
#define HDMI_VALID_HANDLE      0x0175175f


typedef void* HDMI_Handle_t;


typedef enum
{
    MAIN,
    AUXILIARY
} VOUT_DigitalConfiguration_t;

typedef enum
{
    VOUT_OUTPUTS_RGB,
    VOUT_OUTPUTS_YUV,
    VOUT_OUTPUTS_YC,
    VOUT_OUTPUTS_CVBS,
    VOUT_OUTPUTS_HD_RGB,
    VOUT_OUTPUTS_HD_YCBCR
} VOUT_OutputsConfiguration_t;

/* Exported Types ----------------------------------------------------------- */
typedef struct
{
    STVOUT_DeviceType_t     DeviceType;
    ST_DeviceName_t         DeviceName;

    ST_Partition_t*         CPUPartition_p;
    U32                     MaxOpen;
    ST_DeviceName_t         DencName;
    ST_DeviceName_t         VTGName;
    STDENC_Handle_t         DencHandle;
    U8                      DencVersion;
    void*                   DeviceBaseAddress_p; /**/
    void*                   BaseAddress_p;       /**/

#ifdef ST_OSLINUX
    U32                     VoutMappedWidth;
    U32                     SYSCFGMappedWidth;
    void *                  VoutMappedBaseAddress_p; /* Address where will be mapped the VOUT driver registers */
    void *                  SYSCFGMappedBaseAddress_p; /* Address where will be mapped the PLL & FS registers */
#if defined(STVOUT_HDMI)
    U32                     HdmiMappedWidth;
    void *                  HdmiMappedBaseAddress_p; /* Address where will be mapped the HDMI  registers */
    U32                     HdcpMappedWidth;
    void *                  HdcpMappedBaseAddress_p; /* Address where will be mapped the HDCP  registers */
#endif
#endif  /* ST_OSLINUX */

#if defined (ST_7100)||defined (ST_7109)
    void *                  SYSCFGBaseAddress_p;
#endif


    STVOUT_OutputType_t     OutputType;
    BOOL                    AuxNotMain;
    BOOL                    VoutStatus;
    STVOUT_OutputParams_t   Out;
    STVOUT_DAC_t            DacSelect;
#ifdef STVOUT_HDDACS
    BOOL                    HD_Dacs;
    STVOUT_Format_t         Format;
    void*                   HDFBaseAddress_p;
#endif
#ifdef STVOUT_HDMI
    HDMI_Handle_t           HdmiHandle;
    void*                   SecondBaseAddress_p; /* only for the HDCP cell on STi7710*/
#endif /* STVOUT_HDMI */

} stvout_Device_t;

typedef struct
{
    stvout_Device_t*        Device_p;
    U32                     UnitValidity; /* Only the value XXX_VALID_UNIT means the unit is valid */
} stvout_Unit_t;

/* Exported Constants ------------------------------------------------------- */

#define STVOUT_MAX_DEVICE  10     /* Max number of Init() allowed */
#define STVOUT_MAX_OPEN    3      /* Max number of Open() allowed per Init() */
#define STVOUT_MAX_UNIT    (STVOUT_MAX_OPEN * STVOUT_MAX_DEVICE)


/* Exported Variables ----------------------------------------------------- */
extern stvout_Unit_t *stvout_UnitArray;

/* Exported Macros -------------------------------------------------------- */
#define ERROR(err)      (((err) != ST_NO_ERROR)? STVOUT_ERROR_DENC_ACCESS : (err))
#define OK(err)          ((err) == ST_NO_ERROR)
/* Passing a (STVOUT_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((stvout_Unit_t *) (Handle)) >= stvout_UnitArray) &&                    \
                               (((stvout_Unit_t *) (Handle)) < (stvout_UnitArray + STVOUT_MAX_UNIT)) &&  \
                               (((stvout_Unit_t *) (Handle))->UnitValidity == STVOUT_VALID_UNIT))


/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t stvout_HalDisable( stvout_Device_t *Vout);
ST_ErrorCode_t stvout_PowerOffDac( stvout_Device_t * Device_p);
ST_ErrorCode_t halvout_PowerOnDac(  stvout_Unit_t *Unit_p);
#if defined(ST_7200)|| defined(ST_7111)|| defined (ST_7105)
ST_ErrorCode_t stvout_HALVosSetAWGSettings(  stvout_Unit_t *Unit_p);
#endif

void stvout_Report( char *stringfunction, ST_ErrorCode_t Error);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VOUT_DRV_H */

/* End of vout_drv.h */
