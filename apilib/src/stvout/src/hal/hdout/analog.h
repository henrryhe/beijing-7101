/************************************************************************

COPYRIGHT (C) STMicroelectronics 2000

Source file name : analog.h

VOUT driver module.
************************************************************************/

#ifndef __ANALOG_H
#define __ANALOG_H

/*
******************************************************************************
Includes
******************************************************************************
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
******************************************************************************
Exported Types
******************************************************************************
*/
/*
******************************************************************************
Exported Constants
******************************************************************************
*/

#if defined (ST_7200)
#define HD_TVOUT_HDF_OFFSET 0x1800
#else
#define HD_TVOUT_HDF_OFFSET 0 /*used only for STi7200*/
#endif
/*
******************************************************************************
Exported Variables (Globals, not static)
******************************************************************************
*/
/*
******************************************************************************
Exported Macros
******************************************************************************
*/
/*
******************************************************************************
Exported Functions (not static)
******************************************************************************
*/
ST_ErrorCode_t stvout_HalSetAnalogRgbYcbcr( void* BaseAddress, U8 RgbYcbcr);
ST_ErrorCode_t stvout_HalSetAnalogSDRgbYuv( void* BaseAddress, U8 RgbYuv);
ST_ErrorCode_t stvout_HalSetAnalogEmbeddedSync( void* BaseAddress, BOOL EmbeddedSync);
ST_ErrorCode_t stvout_HalDisableRGBDac( void* BaseAddress_p, U8 Dac);
ST_ErrorCode_t stvout_HalEnableRGBDac( void* BaseAddress_p, U8 Dac);
ST_ErrorCode_t stvout_HalSetSyncroInChroma( void* BaseAddress, BOOL SyncroInChroma);
ST_ErrorCode_t stvout_HalSetColorSpace( void* BaseAddress, U8 ColorSpace);
#ifdef STVOUT_HDDACS
ST_ErrorCode_t stvout_HalSetUpsampler(stvout_Device_t *Device_p, U8 ColorSpace);
ST_ErrorCode_t stvout_HalGetUpsampler( void* BaseAddress, U8 ColorSpace);
ST_ErrorCode_t stvout_HalSetUpsamplerOff(stvout_Device_t *Device_p);
#endif
/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __ANALOG_H */
