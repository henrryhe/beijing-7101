/*******************************************************************************

File name   : denc_out.h

Description : VOUT driver header for denc

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
22 Fev 2001        Created                                          JG
13 Sep 2001        Update STDENC macro names                        HSdLM
19 Mar 2003        Add support for DENC cell version 12             HSdLM
*******************************************************************************/

#ifndef __DENC_OUT_H
#define __DENC_OUT_H

/* Includes ----------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

/* Private variables (static) --------------------------------------------- */

/* Global Variables --------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */

#define OKHandle(h)       (((STDENC_CheckHandle(h)) == ST_NO_ERROR) ? ST_NO_ERROR : STVOUT_ERROR_DENC_ACCESS)

/* Exported Constants ------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t stvout_HalSetOutputsConfiguration(
        STDENC_Handle_t             Handle,
        U8                          Version,
        STVOUT_DeviceType_t         Type,
        STVOUT_DAC_t                Dac,
        VOUT_OutputsConfiguration_t OutputsConfiguration);

ST_ErrorCode_t stvout_HalSetOption        ( stvout_Device_t *             Device_p,
                                            const STVOUT_OptionParams_t * const OptionParams_p );
ST_ErrorCode_t stvout_HalGetOption        ( stvout_Device_t *             Device_p,
                                            STVOUT_OptionParams_t *       const OptionParams_p );
ST_ErrorCode_t stvout_HalDisableDac       ( stvout_Device_t * Device_p, U8 Dac   );
ST_ErrorCode_t stvout_HalEnableDac        ( stvout_Unit_t * Unit_p, U8 Dac       );
ST_ErrorCode_t stvout_HalSetInvertedOutput( stvout_Unit_t * Unit_p, BOOL Inverted);
ST_ErrorCode_t stvout_HalWriteDACC        ( stvout_Unit_t * Unit_p, U8 DACValue  );
ST_ErrorCode_t stvout_HalWriteDAC1        ( stvout_Unit_t * Unit_p, U8 DACValue  );
ST_ErrorCode_t stvout_HalWriteDAC2        ( stvout_Unit_t * Unit_p, U8 DACValue  );
ST_ErrorCode_t stvout_HalWriteDAC3        ( stvout_Unit_t * Unit_p, U8 DACValue  );
ST_ErrorCode_t stvout_HalWriteDAC4        ( stvout_Unit_t * Unit_p, U8 DACValue  );
ST_ErrorCode_t stvout_HalWriteDAC5        ( stvout_Unit_t * Unit_p, U8 DACValue  );
ST_ErrorCode_t stvout_HalWriteDAC6        ( stvout_Unit_t * Unit_p, U8 DACValue  );
ST_ErrorCode_t stvout_HalEnableBCS        ( stvout_Unit_t * Unit_p, BOOL Enable  );
ST_ErrorCode_t stvout_HalMaxDynamic       ( stvout_Unit_t * Unit_p, BOOL MaxDyn  );
ST_ErrorCode_t stvout_HalSetBrightness    ( stvout_Unit_t * Unit_p, U8 Brightness);
ST_ErrorCode_t stvout_HalSetContrast      ( stvout_Unit_t * Unit_p, S8 Contrast  );
ST_ErrorCode_t stvout_HalSetSaturation    ( stvout_Unit_t * Unit_p, U8 Saturation);
ST_ErrorCode_t stvout_HalSetChromaFilter  ( stvout_Unit_t * Unit_p, S16 *Coeffs_p );
ST_ErrorCode_t stvout_HalSetLumaFilter    ( stvout_Unit_t * Unit_p, S16 *Coeffs_p );
ST_ErrorCode_t stvout_HalSetInputSource   ( stvout_Device_t *              Device_p,
                                            STVOUT_Source_t                Source
                                          );

ST_ErrorCode_t stvout_HalAdjustChromaLumaDelay ( stvout_Device_t * Device_p,
                                                 STVOUT_ChromaLumaDelay_t ChrLumDelay);

void stvout_FindCurrentConfigurationFromReg(U8 MaskedRegValue,
                                             STVOUT_DACOutput_t TriDACConfig[3],
                                             const STVOUT_DAC_CONF_t AuthorizedDacsConfiguration[],
                                             int AuthorizedDacsConfigurationSize);
/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DENC_OUT_H */
