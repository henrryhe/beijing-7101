#ifndef H_D0372_UTIL
	#define H_D0372_UTIL
		#include "stddefs.h"
		#include "ioreg.h"
		#define FE_372_MAXLOOKUPSIZE 115
				/*	Signal type enum	*/
		typedef enum
		{
			NOAGC1_372=0,
			AGC1OK_372,
			NOTIMING_372,
			ANALOGCARRIER_372,
			TIMINGOK_372,
			NOAGC2_372,
			AGC2OK_372,
			NOCARRIER_372,
			CARRIEROK_372,
			NODATA_372,
			FALSELOCK_372,
			DATAOK_372,
			OUTOFRANGE_372,
			RANGEOK_372,
			FSM1OK_372,
			NOFSM1_372,
			FSM2OK_372,
			NOFSM2_372,
			POWEROK_372,
			NOPOWER_372
		} FE_372_SIGNALTYPE_t;
		
		/* One point of the lookup table */
	typedef struct
	{
		S32 realval;	/*	real value */
		S32 regval;		/*	binary value */
	} FE_372_LOOKPOINT_t;
	
		/*	Lookup table definition	*/
	typedef struct
	{
		S32 size;		/*	Size of the lookup table	*/
		FE_372_LOOKPOINT_t table[FE_372_MAXLOOKUPSIZE]; 	/*	Lookup table	*/
	} FE_372_LOOKUP_t;
	
		/**************** Basic Registers functions ****************/
	
		unsigned long 			UTIL_372_PowOf2(int);
		void UTIL_372_Set_NCOcnst_Regs(IOARCH_Handle_t IOHandle,int value);
		void UTIL_372_Set_vcxoOffset_Regs(IOARCH_Handle_t IOHandle,int value);
		U32 UTIL_372_Calc_NCOcnst(U32 IFfrequency,U32 clk);
		U32 UTIL_372_Calc_vcxoOffset(U32  SymbolRate,U32 clk);
		int UTIL_372_Get_NCOerr_Value(IOARCH_Handle_t IOHandle) ;
		int UTIL_372_Get_FrequencyOffset(IOARCH_Handle_t IOHandle,int clk);
#endif
