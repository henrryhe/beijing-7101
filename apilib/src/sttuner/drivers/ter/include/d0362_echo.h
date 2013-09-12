#ifndef H_362ECHO
	#define H_362ECHO		
	
	/************************
		Echo structure 
	*************************/
	typedef struct
	{
	U8 best_EPQ_val;
	U8 bad_EPQ_val;
	S8 best_EPQ_val_idx ;
	U8 EPQ_ref;
	U8 I2CSpeed;
	U8 past_EPQ_val[8];
	U8 EPQ_val[16];
	S8  L1s2va3vp4;
	} FE_362_OFDMEchoParams_t;


#endif

