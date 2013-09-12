/*****************************************************************************
File Name   : rmapdoublebit.c

Description : STBLAST RMAP specifications with double bit coding
			  It has been implemented for phillips premire project

Copyright (C) 2004 STMicroelectronics

History     : Created April 2004

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#endif

#include "stddefs.h"
#include "stblast.h"
#include "blastint.h"
#include "rmapdoublebit.h"

#if defined(IR_INVERT)
#include "invinput.h"
#endif

#define T 		(108)
#define MIN_T    90
#define MAX_T	 140

#define BIT_TIME 4


#define SYMBOL_TIMEOUT 			0xFFFF
#define DRIFT_THRESHOLD         4
#define RMAP_BOUNDARY_LIMIT	    50
#define RMAP_MAX_FRAME_SIZE     24
#define CUSTOMID_BIT_START      2
#define CUSTOMID_BIT_END        7
#define DEVICEID_BIT_START      9
#define DEVICEID_BIT_END        11
#define ADDRESS_BIT_START       12
#define ADDRESS_BIT_END         14
#define REPEAT_CODE_BIT         15
#define PRMAP_COMMAND_BIT_START 16
#define PRMAP_COMMAND_BIT_END   24

/*****************************************************************************
BLAST_RMapDoubleBitDecode()
Description:
    Decode rcmm symbols into user buffer

Parameters:
    UserBuf_p           user buffer to place data into
    SymbolBuf_p         symbol buffer to consume
    SymbolsAvailable    how many symbols present in buffer
    NumDecoded_p        how many data words we decoded
    SymbolsUsed_p       how many symbols we used
    RcProtocol_p        protocol block

Return Value:
    ST_NO_ERROR

See Also:
    BLAST_SpaceDecode
    BLAST_RC5Encode
*****************************************************************************/

ST_ErrorCode_t BLAST_RMapDoubleBitDecode(U32                  *UserBuf_p,
	                                     STBLAST_Symbol_t     *SymbolBuf_p,
	                                     U32            	  SymbolsAvailable,
	                                     U32                  *NumDecoded_p,
	                                     U32                  *SymbolsUsed_p,
	                                     const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U8 j,i;
    U32 SymbolsLeft;
    U32 SymbolPeriod = 0;
    U32 MarkPeriod = 0;
    U32 SpacePeriod = 0;
    U32 LastSpacePeriod = 0;
    U32 LastMarkPeriod = 0;
    U32 TempMarkPeriod = 0;
    U32 TempSpacePeriod = 0;
    U32 TempLastSpacePeriod = 0;
    U32 TempLastMarkPeriod = 0;
    U32 ExtentedLastSpacePeriod = 0;
    
    U8 DeviceId= 0;
    U8 CustomId = 0;
    U8 Address = 0;
    
    BOOL MarkFirst = TRUE;
    BOOL NextMarkLevel = FALSE;
	BOOL StartFound = FALSE; 
    U8 LevelArray[400]={0};  /* check these values */
    U8 LevelArraySize;
    U32 DataIndex = 0;
    U32 PreviousSymbolsEnough = 0;
    U32 NumberOfSymbols = 0;
    
    /* Set number of symbols allowed */
    SymbolsLeft = SymbolsAvailable;
    *SymbolsUsed_p = 0;
    *NumDecoded_p = 0;
       
    if(ProtocolParams_p->Ruwido.FrameLength == 24)
    {
        if(SymbolsAvailable < 9)
        {
            *NumDecoded_p = 0;
            return ST_NO_ERROR; 
        }
    }
    else
    {
        *NumDecoded_p = 0;
        return ST_NO_ERROR; 
    }
    
    SymbolPeriod = (SymbolBuf_p->SymbolPeriod + (RMAP_BOUNDARY_LIMIT)) /T;
    MarkPeriod 	 = (SymbolBuf_p->MarkPeriod   + (RMAP_BOUNDARY_LIMIT)) /T;
    SpacePeriod  =  SymbolPeriod - MarkPeriod;   
    
    if( SpacePeriod > 2)
    {
    	LastSpacePeriod =  BIT_TIME - ( SpacePeriod  - MarkPeriod);   
    	SpacePeriod     =  SpacePeriod - LastSpacePeriod;
    	if( LastSpacePeriod > 0)

    		NextMarkLevel = FALSE;
    }
    if(MarkPeriod > 2)
    {
       LastMarkPeriod =  BIT_TIME - ( SpacePeriod  - MarkPeriod);   
       MarkPeriod     =  MarkPeriod - LastMarkPeriod;
       if(LastMarkPeriod > 0)
    		NextMarkLevel = TRUE;
    }
       	
    j = 0;
    if ((SpacePeriod == 2  && MarkPeriod == 2 ) && MarkFirst == TRUE)
    {
        StartFound 		= 1;
        LevelArray[j] 	= 1;
        LevelArray[j+1] = 1;
        SymbolBuf_p++;
    }    
       
    if( StartFound != 1)
    {
        *NumDecoded_p = 0;
        /*Though ST_NO_ERROR is returned,*NumDecoded_p = 0 tells the calling
         *function that the incoming signal is in error*/
        return ST_NO_ERROR;
    }     
    for(j = CUSTOMID_BIT_START; j < PRMAP_COMMAND_BIT_END && SymbolsLeft > 0; j=j+2 )
    {

          if( PreviousSymbolsEnough != 1)
          {
				SymbolPeriod 		= (SymbolBuf_p->SymbolPeriod + (RMAP_BOUNDARY_LIMIT)) /T;
			    MarkPeriod 	 		= (SymbolBuf_p->MarkPeriod   + (RMAP_BOUNDARY_LIMIT)) /T;
			    SpacePeriod 		=  SymbolPeriod - MarkPeriod; 
			    TempMarkPeriod  	=  MarkPeriod;
			    TempSpacePeriod	    =  SpacePeriod;
			    TempLastSpacePeriod =  LastSpacePeriod;
			    TempLastMarkPeriod  =  LastMarkPeriod;
			    
			    if(SymbolBuf_p->SymbolPeriod == SYMBOL_TIMEOUT)
		  		{
		           if(j == (PRMAP_COMMAND_BIT_END - 2))
		           {
		        		PreviousSymbolsEnough = 1;  
		           }
		           else
		           { 
		           		PreviousSymbolsEnough = 0;
		           		NumberOfSymbols = 10;
		           		
		           }
		  		} 
		  		if((NumberOfSymbols == 10) && (j == 22))
		  		{
		  			PreviousSymbolsEnough = 1;
		  			
		  			if(LastMarkPeriod == 1)
		  			{
		  				NextMarkLevel = TRUE;
		  			 	SpacePeriod = 3;
		  			}
		  			else if(LastMarkPeriod == 2)
		  			{
		  				NextMarkLevel = TRUE;
		  			 	SpacePeriod = 2;
		  			}
		  			if(LastSpacePeriod == 2)
		  			{
		  				NextMarkLevel = FALSE;
		  			    MarkPeriod = 2;
		  			}
		  			else if(LastSpacePeriod == 3)
		  			{
		  				NextMarkLevel = FALSE;
		  			    MarkPeriod = 1;		  			    
		  			}
		  		}
		    	SymbolBuf_p++;	  
		        SymbolsLeft--;
		  }  
		  
		
		  /* No previous symbols(space or mark) remaining*/
		  if((LastSpacePeriod == 0) && (LastMarkPeriod == 0))
		  {
		    	/* Mark Period can never be greater then Space*/  
		    	if(SpacePeriod > MarkPeriod)  
		    	{
			    	if(SpacePeriod > BIT_TIME - MarkPeriod)
    		    	{
					    LastSpacePeriod = SpacePeriod - (BIT_TIME - MarkPeriod);
					    SpacePeriod     = SpacePeriod - LastSpacePeriod;
				 	 }
			    }
			    switch(NextMarkLevel)
			  	{
     				case TRUE:
								if((MarkPeriod == 2) && (SpacePeriod == 2))
								{
									 LevelArray[j] 	 = 1;
	       							 LevelArray[j+1] = 1;
								}
								
								else if((MarkPeriod == 1) && (SpacePeriod == 3)) 
							    {
									 LevelArray[j] 	 = 1;
	       			 				 LevelArray[j+1] = 0;
	       					    }
	       					    else
	       			 			{
	       			 			    *NumDecoded_p = 0;
	            	 				return ST_NO_ERROR;   
	            	 			}
	       					    break;
 					case FALSE:
 								if((SpacePeriod == 2) && (MarkPeriod == 2))
 								{
 								     LevelArray[j] 	 = 0;
					 				 LevelArray[j+1] = 0;
					 			}
					 			else if((SpacePeriod == 3) && (MarkPeriod == 1)) 
					 			{
					 				LevelArray[j] = 0;
	       			 				LevelArray[j+1] =1;			
	       			 			}
	       			 			else
	       			 			{
	       			 			    *NumDecoded_p = 0;
	            	 				return ST_NO_ERROR;   
	            	 			}
	       			 			break;
	       			default:
	       				
	       					    break;
 			    }
	            if( LastSpacePeriod > 0)
	    			 NextMarkLevel = FALSE;   /* Next Symbols would start from Space*/
   	 
			}	
			/* When previous levels (Mark or Space)are remaining*/
		    else
		    {
               if(PreviousSymbolsEnough != 1)
               {	    
				    if((LastSpacePeriod + MarkPeriod) > BIT_TIME)
				    {
				       if(LastSpacePeriod > 2)   
				       {
						  MarkPeriod     = BIT_TIME - LastSpacePeriod;
		       	  		  LastMarkPeriod = TempMarkPeriod - MarkPeriod;
		       	  	   }
		       	  	   if(MarkPeriod > 2)
		       	  	   {
		       	  	   	   LastMarkPeriod = (LastSpacePeriod + TempMarkPeriod) - BIT_TIME;
		       	  	   	   MarkPeriod = TempMarkPeriod - LastMarkPeriod;
		       	  	   }
				    }
				    /* see this case*/
				    else if((LastMarkPeriod + SpacePeriod) > BIT_TIME)
				    {
				       if(SpacePeriod > 2)
				       {
				       	  SpacePeriod = BIT_TIME - LastMarkPeriod;
				       	  LastSpacePeriod = TempSpacePeriod - SpacePeriod;
				       }
		            } 	
		           
		       }
				switch(NextMarkLevel)
			  	{
     				case TRUE:
							  if((LastMarkPeriod == 2) && (SpacePeriod == 2))
							  {
									 LevelArray[j] 	 = 1;
	       							 LevelArray[j+1] = 1;
							  }
							  else if((LastMarkPeriod == 1) && (SpacePeriod == 3)) 
							  {
									 LevelArray[j] 	 = 1;
	       			 				 LevelArray[j+1] = 0;
	       					  }
	       					  else
	       			 		  {
	       			 			    *NumDecoded_p = 0;
	            	 				return ST_NO_ERROR;   
	            	 		  }
	       					     break;
 					case FALSE:
 							  if((LastSpacePeriod == 2) && (MarkPeriod == 2))
 							  {
 								     LevelArray[j] 	 = 0;
					 				 LevelArray[j+1] = 0;
					 		  }
					 		  else if((LastSpacePeriod == 3) && (MarkPeriod == 1)) 
					 		  {
					 				LevelArray[j] = 0;
	       			 				LevelArray[j+1] =1;			
	       			 		  }
	       			 		  else
	       			 		  {
	       			 			    *NumDecoded_p = 0;
	            	 				return ST_NO_ERROR;   
	            	 		  }
	       			 		  break;
	       			default:
	       				      break;
 			    }
 			    
 			    if(PreviousSymbolsEnough == 1)
 			    {
 			    		   LastMarkPeriod = 0;
						   LastSpacePeriod = 0;
						   TempMarkPeriod = 0;
						   TempSpacePeriod = 0;
						   PreviousSymbolsEnough = 0;
						   if(ExtentedLastSpacePeriod != 0)
						   {
						   	  NextMarkLevel = FALSE;
						   	  LastSpacePeriod = ExtentedLastSpacePeriod;
						   }
						   
			    }
			    else
			    {
		           	LastSpacePeriod = ( LastSpacePeriod + TempSpacePeriod + MarkPeriod ) - BIT_TIME; 

		    	    if(LastMarkPeriod != 0)
		    	    {  
						NextMarkLevel = TRUE;
		    		}
	
		    		/* save the original set level to decide the level b/w LM or LS*/
		    		else if( LastSpacePeriod > 0)
		    		{
		    			NextMarkLevel = FALSE;
				 	}    
		      	     /* See this case*/ 
		      	    if( TempMarkPeriod > 2)
		      	   	{
				   		LastMarkPeriod =  (TempLastSpacePeriod + TempMarkPeriod) - BIT_TIME ;
				 		if(LastMarkPeriod > 0)
				    		NextMarkLevel = TRUE;
				   		
				   	} 
				   	if((LastMarkPeriod == 2) && (LastSpacePeriod == 2))
				   	{
				     	PreviousSymbolsEnough = 1;
				     	if(NextMarkLevel == TRUE)
				     	{
				     		SpacePeriod = LastSpacePeriod;
				        }
				        else
				        {
				        	MarkPeriod = LastMarkPeriod;
				        }
				    }	
				    else if(((LastMarkPeriod == 2) || (LastMarkPeriod == 1))&& ((LastSpacePeriod > 2)
				    && (LastSpacePeriod < 20)))
				    {
				        PreviousSymbolsEnough = 1;
				        if(NextMarkLevel == TRUE)
				     	{
				     		ExtentedLastSpacePeriod = LastSpacePeriod + LastMarkPeriod - BIT_TIME;
				     		SpacePeriod = BIT_TIME - LastMarkPeriod;
				        }
				        else
				        {
				        	MarkPeriod = LastMarkPeriod;
				        }
    			    }
				}/* if(PreviousSymbolsEnough != 1)*/
		    }/*  if((LastSpacePeriod == 0) && (LastMarkPeriod == 0))*/
		}/*for*/

        DataIndex = j;
     
        /* build up Custom Id */
        for ( j = CUSTOMID_BIT_START; j <= CUSTOMID_BIT_END; j++)
        {
            CustomId <<= 1;
            CustomId |= LevelArray[j];
        }
            
        if ( CustomId != ProtocolParams_p->Ruwido.CustomID )            
        {
            *NumDecoded_p = 0;  
            return ST_NO_ERROR;
        }  
            
        /* build up device Id */
        for ( j = DEVICEID_BIT_START; j <= DEVICEID_BIT_END; j++)
        {
            DeviceId <<= 1;
            DeviceId |= LevelArray[j];
        }
            
        if ( DeviceId != ProtocolParams_p->Ruwido.DeviceID )            
        {
            *NumDecoded_p = 0;  
            return ST_NO_ERROR;
        }  
            
        /* build up Address field */
        for ( j = ADDRESS_BIT_START; j <= ADDRESS_BIT_END; j++)
        {
            Address <<= 1;
            Address |= LevelArray[j];
        }
            
        if ( Address != ProtocolParams_p->Ruwido.Address )            
        {
            *NumDecoded_p = 0;  
            return ST_NO_ERROR;
        }  
       
	    /* the size of Array */
	    LevelArraySize = DataIndex;
	    {
	        U32 Cmd = 0;
	        /* Form the Cmd */
	        for(i= 0; i < LevelArraySize ; i++)
	        {
	            Cmd <<= 1;
	            Cmd |= LevelArray[i];
	        }
	        
	    	*UserBuf_p = Cmd;
	    	*NumDecoded_p = 1;
	    	return ST_NO_ERROR;
	    }
}
