/************************************************************************
File Name   : vbi_drv.h

Description : VBI driver header file (unit relative definitions)

COPYRIGHT (C) STMicroelectronics 2000

Date               Modification                                      Name
----               ------------                                      ----
06 Jul 2000       Created                                            JG
...
11 Dec 2000       Improve error tracking                             HSdLM
22 Feb 2001       Use STDENC mutual exclusion register accesses      HSdLM
04 Jul 2001       Remove unused macro                                HSdLM
14 Nov 2001       Add TTX source selection capability needed by      HSdLM
 *                ST40GX1
***********************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VBI_DRV_H
#define __VBI_DRV_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stdenc.h"
#include "stgxobj.h" /* needed by Viewport CGMS */
#include "stlayer.h" /* needed by Viewport CGMS */
#include "stavmem.h" /* needed by Viewport CGMS */
#include "stos.h"
#include "stvbi.h"
/*#if !defined(ST_OSLINUX)*/
#include "sttbx.h"
/*#endif*/

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */
#define STVBI_MAX_DEVICE     3
#define STVBI_MAX_OPEN       STVBI_VBI_TYPE_MAX_SUPPORTED

#define STVBI_VALID_UNIT     0x09019010    /* 0xXY0ZY0ZX with XYZ = 091 (STVBI_DRIVER_ID = 145 = 0x091) */

#ifdef COPYPROTECTION
#define VBI_MV_MAX_USER_DATA 17 /* for MV 7.01 */
#endif /* #ifdef COPYPROTECTION */

#ifndef STVBI_DEBUG
#undef STTBX_Report
#define STTBX_Report(x)
#endif /** STVBI_DEBUG  **/

/* Exported Types --------------------------------------------------------- */
typedef U8 HALVBI_DencCellId_t;

#ifdef COPYPROTECTION
/* Copy Protection additional parameters */
typedef struct
{
    U8                              UserData[VBI_MV_MAX_USER_DATA];
    BOOL                            IsEnabled;
    BOOL                            AreDataWaiting;
    BOOL                            AreDataSet;
    BOOL                            AreDynParamsSet;
} VBI_MVMoreParams_t;
#endif /* #ifdef COPYPROTECTION */


typedef struct
{
    STVBI_Configuration_t Conf;
    STVBI_DynamicParams_t Dyn;
#ifdef COPYPROTECTION
    VBI_MVMoreParams_t    MVMore;
#endif /* #ifdef COPYPROTECTION */
} VBI_Parameters_t;

typedef struct
{
    /* Device identification */
    ST_DeviceName_t             DeviceName;

    /* Device properties set by init parameters, set at init and not modifiable afterwards */
    STVBI_DeviceType_t          DeviceType;
    U32                         MaxOpen;

    /* Device properties induced by init parameters, set at init and not modifiable afterwards */
    STDENC_Handle_t             DencHandle;
    HALVBI_DencCellId_t         DencCellId;
    void *                      BaseAddress_p;

    /* Device properties modifiable from API */
    STVBI_TeletextSource_t      TeletextSource;

    /* Device properties needed for ViewportVBI */
    STAVMEM_PartitionHandle_t   AVMemPartitionHandle;
    STAVMEM_BlockHandle_t       AVMemBlockHandle ;
    STLAYER_Handle_t            LayerHandle;
    STLAYER_ViewPortHandle_t    VPHandle;
    STGXOBJ_Bitmap_t            CgmsBitmap;

    /* Device properties for inside use, not seen from API */
    U16                         VbiRegistered;
} VBIDevice_t;

typedef struct
{
    VBIDevice_t *         Device_p;      /* Pointer to the device to be able to retrieve the init() concerned. */
    U32                   UnitValidity;   /* Only the value STVBI_VALID_UNIT means the unit is valid */
    BOOL                  IsVbiEnabled;
    BOOL                  AreDynamicParamsWaiting;
    VBI_Parameters_t      Params;
} VBI_t;


/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

#define REGISTER_VBI(p, v)      (p->Device_p->VbiRegistered |= v)
#define UNREGISTER_VBI(p)       (p->Device_p->VbiRegistered &= ~(p->Params.Conf.VbiType))

#define SET_VBI_TYPE(p,t)       (p->Params.Conf.VbiType = t)

#define SET_VBI_STATUS(p, s)    (p->IsVbiEnabled = s)
#define GET_VBI_STATUS(p)       (p->IsVbiEnabled)

/* Exported Functions ----------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VBI_DRV_H */

/* End of vbidrv.h */
