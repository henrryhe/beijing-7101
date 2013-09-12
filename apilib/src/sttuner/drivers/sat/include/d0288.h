/*---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_D0288_H
#define __STTUNER_SAT_D0288_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

/* Standard definitions */
#include "stddefs.h"
/* internal types */
#include "dbtypes.h"    /* types of satellite driver database */
#include "sttuner.h"
#include "ioarch.h"
#include "stcommon.h"
#include "util288.h"

typedef struct
	{
		BOOL 		     Locked;						/* Transponder locked					*/
		U32                  Frequency;						/* transponder frequency (in KHz)		*/
		U32 		     SymbolRate;						/* transponder symbol rate  (in Mbds)	*/
		FE_288_Modulation_t  Modulation;	/* modulation							*/
		FE_288_Rate_t        Rate;				/* puncture rate 						*/       
		S32                  Power_dBm;							/* Power of the RF signal (dBm)			*/			
		U32                  CN_dBx10;							/* Carrier to noise ratio				*/
		U32	             BER;	
		S16	             SpectralInv;		/* I,Q Inversion  */ 	
		S16                  derotFreq;							/* Bit error rate						*/
	}
FE_288_SignalInfo_t;

/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STX0288_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STX0288_UnInstall(STTUNER_demod_dbase_t *Demod);

#ifdef __cplusplus
extern "C"
{
#endif 

#endif

