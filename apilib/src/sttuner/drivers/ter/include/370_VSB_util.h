#ifndef H_370_VSB_UTIL
	#define H_370_VSB_UTIL
		#include "stddefs.h"
		#include "ioreg.h"
		#define FE_370_MAXLOOKUPSIZE 115
				/*	Signal type enum	*/
		typedef enum
		{
			NOAGC1_370VSB=0,
			AGC1OK_370VSB,
			NOTIMING_370VSB,
			ANALOGCARRIER_370VSB,
			TIMINGOK_370VSB,
			NOAGC2_370VSB,
			AGC2OK_370VSB,
			NOCARRIER_370VSB,
			CARRIEROK_370VSB,
			NODATA_370VSB,
			FALSELOCK_370VSB,
			DATAOK_370VSB,
			OUTOFRANGE_370VSB,
			RANGEOK_370VSB,
			FSM1OK_370VSB,
			NOFSM1_370VSB,
			FSM2OK_370VSB,
			NOFSM2_370VSB,
			POWEROK,
			NOPOWER
		} FE_370_VSB_SIGNALTYPE_t;
		
		/* One point of the lookup table */
	typedef struct
	{
		S32 realval;	/*	real value */
		S32 regval;		/*	binary value */
	} FE_370_LOOKPOINT_t;
	
		/*	Lookup table definition	*/
	typedef struct
	{
		S32 size;		/*	Size of the lookup table	*/
		FE_370_LOOKPOINT_t table[FE_370_MAXLOOKUPSIZE]; 	/*	Lookup table	*/
	} FE_370_LOOKUP_t;
	
		/**************** Basic Registers functions ****************/
	
		long           RegGetExtClk(void)                        	;
		void           RegSetExtClk(long)                        	;
		unsigned long 			PowOf2(int) 						;
		unsigned long int GetBERPeriod(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle);
		int Get_vcxoOffset_Value(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle);
		int Get_NCOcnst_Value(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle) ;
		void Set_NCOcnst_Regs(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle,int value);
		void Set_vcxoOffset_Regs(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle,int value);
		U32 Calc_NCOcnst(U32 IFfrequency,U32 clk);
		U32 Calc_vcxoOffset(U32 SymbolRate,U32 clk);
		
		
		int Get_NCOerr_Value(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle) ;
		S32 Get_FrequencyOffset(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle,U32 clk);
#endif
