
#ifndef H_UTIL297E
	#define H_UTIL297E
	
	#define FE_297e_MAXLOOKUPSIZE 50

	typedef enum
	{
		COUNTER1 = 0,
		COUNTER2 = 1
	} FE_297e_ERRORCOUNTER_t;

	/* One point of the lookup table */
	typedef struct
	{
		S32 realval;	/*	real value */
		S32 regval;		/*	binary value */
	} FE_297e_LOOKPOINT_t;

	/*	Lookup table definition	*/
	typedef struct
	{
		S32 size;		/*	Size of the lookup table	*/
		FE_297e_LOOKPOINT_t table[FE_297e_MAXLOOKUPSIZE]; 	/*	Lookup table	*/
	} FE_297e_LOOKUP_t;
	
	/*	Monitoring structure */
	typedef struct
	{
		U32		FE_297e_TotalBlocks,
				FE_297e_TotalBlocksOld,
				FE_297e_TotalBlocksOffset,
				FE_297e_TotalCB,
				FE_297e_TotalCBOld,
				FE_297e_TotalCBOffset,
				FE_297e_TotalUCB,
				FE_297e_TotalUCBOld,
				FE_297e_TotalUCBOffset,
				FE_297e_BER_Reg,
				FE_297e_BER_U32,
				FE_297e_Saturation,
				FE_297e_WaitingTime;
	}FE_297e_Monitor;
	
	
	#define WAIT_N_MS_297e(X)    STOS_TaskDelayUs(X*1000)

	#define INRANGE_297e(X,Y,Z) (((X<=Y) && (Y<=Z))||((Z<=Y) && (Y<=X)) ? 1 : 0)
			
	U32 FE_297e_GetMclkFreq( IOARCH_Handle_t IOHandle, U32 ExtClk_Hz);
	U32 FE_297e_GetADCFreq( IOARCH_Handle_t IOHandle, U32 ExtClk_Hz);
	void FE_297e_GetErrorCount( IOARCH_Handle_t IOHandle, FE_297e_Modulation_t QAMSize, U32 SymbolRate_Bds, FE_297e_Monitor *Monitor_results);
	void FE_297e_ClearCounters( IOARCH_Handle_t IOHandle, FE_297e_Monitor *Monitor_results);
	U32 FE_297e_SetSymbolRate( IOARCH_Handle_t IOHandle,U32 AdcClk_Hz,U32 MasterClk_Hz,U32 SymbolRate, FE_297e_Modulation_t QAMSize);
	U32 FE_297e_GetSymbolRate( IOARCH_Handle_t IOHandle, U32 MasterClk_Hz);
	U32 FE_297e_GetCarrierToNoiseRatio_u32( IOARCH_Handle_t IOHandle, FE_297e_Modulation_t QAMSize);
	U32 FE_297e_GetDerotFreq( IOARCH_Handle_t IOHandle, U32 AdcClk_Hz);
	S32 FE_297e_GetRFLevel( IOARCH_Handle_t IOHandle);
	void FE_297e_OptimiseNByteAndGetBER( IOARCH_Handle_t IOHandle, FE_297e_Modulation_t QAMSize,U32 SymbolRate_Bds,FE_297e_Monitor *Monitor_results);
	void FE_297e_GetPacketsCount( IOARCH_Handle_t IOHandle, FE_297e_Monitor *Monitor_results);


#endif
	
