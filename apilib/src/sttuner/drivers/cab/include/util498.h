#ifndef H_UTIL498
	#define H_UTIL498
	
	#define FE_498_MAXLOOKUPSIZE 50
	/* One point of the lookup table */
	typedef struct
	{
		S32 realval;	/*	real value */
		S32 regval;		/*	binary value */
	} FE_498_LOOKPOINT_t;
	typedef struct
	{
		S32 size;		/*	Size of the lookup table	*/
		FE_498_LOOKPOINT_t table[FE_498_MAXLOOKUPSIZE]; 	/*	Lookup table	*/
	} FE_498_LOOKUP_t;
	
	/*	Monitoring structure */
	typedef struct
	{
		U32		FE_498_TotalBlocks,
				FE_498_TotalBlocksOld,
				FE_498_TotalBlocksOffset,
				FE_498_TotalCB,
				FE_498_TotalCBOld,
				FE_498_TotalCBOffset,
				FE_498_TotalUCB,
				FE_498_TotalUCBOld,
				FE_498_TotalUCBOffset,
				FE_498_BER_Reg,
				FE_498_BER_U32,
				FE_498_Saturation,
				FE_498_WaitingTime;
	}FE_498_Monitor;
	
        U32 FE_498_GetMclkFreq(U32 ExtClk_Hz);
	U32 FE_498_GetADCFreq(U32 ExtClk_Hz);
	void FE_498_GetErrorCount(FE_498_Modulation_t QAMSize, U32 SymbolRate_Bds, FE_498_Monitor *Monitor_results);
	void FE_498_ClearCounters(FE_498_Monitor *Monitor_results);
	U32 FE_498_SetSymbolRate(U32 AdcClk_Hz,U32 MasterClk_Hz,U32 SymbolRate, FE_498_Modulation_t QAMSize);
	U32 FE_498_GetSymbolRate(U32 MasterClk_Hz);
	S32 FE_498_GetCarrierToNoiseRatio_u32(FE_498_Modulation_t QAMSize);
	U32 FE_498_GetDerotFreq(U32 AdcClk_Hz);
	S32 FE_498_GetRFLevel(void);
	void FE_498_OptimiseNByteAndGetBER(FE_498_Modulation_t QAMSize,U32 SymbolRate_Bds,FE_498_Monitor *Monitor_results);
	void FE_498_GetPacketsCount(FE_498_Monitor *Monitor_results);

#endif

