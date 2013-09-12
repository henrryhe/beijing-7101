/* ----------------------------------------------------------------------------
File Name: drivers.c

Description: 

     This module handles low-level driver management functions.


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 18-June-2001
version: 3.1.0
 author: GJP
comment: write for multi-instance.
    
Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */
#ifndef ST_OSLINUX
   

#include <string.h>
#endif
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "drivers.h"   /* functions to accesss system data */

#ifdef STTUNER_USE_SAT
 #include "sdrv.h"     /* satellite driver APIs */
#endif

	#ifdef STTUNER_USE_CAB
	 #include "cdrv.h"     /* cable driver APIs */
	#endif

#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
 #include "tdrv.h"     /* terrestrial driver APIs */
#endif

#ifdef STTUNER_USE_SHARED
 #include "sharedrv.h"     /* shared/common drivers */
#endif


static ST_Partition_t  *MemoryPartition;

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_GetLLARevision()

Description:
    Obtains the revision number of the tuner LLA device driver.
    
Parameters:
    demodname eg:STTUNER_DEMOD_STV0360,STTUNER_DEMOD_STV0361,STTUNER_DEMOD_STV0362,STTUNER_DEMOD_STV0299,STTUNER_DEMOD_STV0399,
    		 STTUNER_DEMOD_STV0297J,STTUNER_DEMOD_STV0297,STTUNER_DEMOD_STV0297E,STTUNER_DEMOD_STV0498
    		 STTUNER_ALL_IN_CURRENT_BUILD(for getting versions of all in current build).
---------------------------------------------------------------------------- */
ST_Revision_t STTUNER_DRV_GetLLARevision(int demodtype)
{
	#ifdef STTUNER_DRV_SAT_STV0399
	extern ST_Revision_t Drv0399_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_SAT_STB0899
	extern ST_Revision_t DrvSTB0899_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_SAT_STB0900
	extern ST_Revision_t DrvSTB0900_GetLLARevision(void);
	#endif
	
	
	#ifdef STTUNER_DRV_SAT_STX0288
	extern ST_Revision_t FE_288_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_SAT_STV0299
	extern ST_Revision_t Drv0299_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_TER_STV0360
	extern ST_Revision_t Drv0360_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_TER_STV0361
	extern ST_Revision_t Drv0361_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_TER_STV0362
	extern ST_Revision_t Drv0362_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_TER_STB0370VSB
	extern ST_Revision_t Drv0370VSB_GetLLARevision(void);
	#endif
	
	
	#ifdef STTUNER_DRV_CAB_STV0297
	extern ST_Revision_t Drv0297_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_CAB_STV0297J
	extern ST_Revision_t Drv0297J_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_CAB_STV0297E
	extern ST_Revision_t Drv0297E_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_CAB_STV0498
	extern ST_Revision_t Drv0498_GetLLARevision(void);
	#endif
	
	#ifdef STTUNER_DRV_CAB_STB0370QAM
	extern ST_Revision_t Drv0370QAM_GetLLARevision(void);
	#endif

#ifdef STTUNER_ECHO	
	#ifdef STTUNER_DRV_SAT_STBSTC1
	extern ST_Revision_t DrvSTBSTC1_GetLLARevision(void);
	#endif
	#ifdef STTUNER_DRV_SAT_STB0900
	extern ST_Revision_t DrvSTB0900_GetLLARevision(void);
	#endif
	
#endif


	#ifdef STTUNER_DRV_TER_STV0372
	extern ST_Revision_t Drv0372_GetLLARevision(void);
	#endif
	

	char temp1[50],temp2[250];/*max length of string describing LLA version=49+1(for '\0');total 5 demods,therefore 5*50=250*/
	char * LLARevision1=temp1;
	char * LLARevision2=temp2;
	memset(LLARevision1,'\0',sizeof(temp1));
	memset(LLARevision2,'\0',sizeof(temp2));
	
	if(demodtype == STTUNER_ALL_IN_CURRENT_BUILD)
	{
			#ifdef STTUNER_DRV_SAT_STV0399
    		strcpy(LLARevision1,Drv0399_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif

    		#ifdef STTUNER_DRV_SAT_STB0899
    		strcpy(LLARevision1,DrvSTB0899_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif
    		
    		#ifdef STTUNER_DRV_SAT_STB0900
    		strcpy(LLARevision1,DrvSTB0900_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif
    		
    		
    		#ifdef STTUNER_DRV_SAT_STX0288
    		strcpy(LLARevision1,FE_288_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif

		#ifdef STTUNER_DRV_SAT_STV0299
		strcpy(LLARevision1,Drv0299_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif
    		
    		#ifdef STTUNER_DRV_TER_STV0360   
			strcpy(LLARevision1,Drv0360_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif
    		
    		#ifdef STTUNER_DRV_TER_STV0361   
			strcpy(LLARevision1,Drv0361_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif
    		
    		#ifdef STTUNER_DRV_TER_STV0362   
			strcpy(LLARevision1,Drv0362_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif
    		
    		#ifdef STTUNER_DRV_TER_STB0370VSB   
			strcpy(LLARevision1,Drv0370VSB_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif
    		
			#ifdef STTUNER_DRV_CAB_STV0297   
			strcpy(LLARevision1,Drv0297_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
			#endif
		
			#ifdef STTUNER_DRV_CAB_STV0297J
    		strcpy(LLARevision1,Drv0297J_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
			#endif
		
			#ifdef STTUNER_DRV_CAB_STV0297E
    		strcpy(LLARevision1,Drv0297E_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
			#endif
			
			#ifdef STTUNER_DRV_CAB_STV0498
    		strcpy(LLARevision1,Drv0498_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
			#endif
		
			#ifdef STTUNER_DRV_CAB_STB0370QAM  
			strcpy(LLARevision1,Drv0370QAM_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif


			#ifdef STTUNER_DRV_TER_STV0372  
			strcpy(LLARevision1,Drv0372_GetLLARevision());
    		strcat(LLARevision2,LLARevision1);
    		#endif

		if(strcmp(LLARevision2,"") == 0) /*checking if LLARevision2 is empty*/
		{
			LLARevision2 = " Error: No LLA found in current build ";
			STTBX_Print(("\nError: No LLA found in current build"));
		}
		return(LLARevision2);
	}
	else
	{
    		if(demodtype == STTUNER_DEMOD_STV0399)
    		{
			#ifdef STTUNER_DRV_SAT_STV0399
			strcpy(LLARevision1,Drv0399_GetLLARevision());
        			#endif
			}
    		else
			if(demodtype == STTUNER_DEMOD_STB0899)
    		{
			#ifdef STTUNER_DRV_SAT_STB0899
			strcpy(LLARevision1,DrvSTB0899_GetLLARevision());
        			#endif
			}
		else
			if(demodtype == STTUNER_DEMOD_STB0900)
    		{
			#ifdef STTUNER_DRV_SAT_STB0900
			strcpy(LLARevision1,DrvSTB0900_GetLLARevision());
        			#endif
			}
			else
			if(demodtype == STTUNER_DEMOD_STX0288)
    		{
			#ifdef STTUNER_DRV_SAT_STX0288
			strcpy(LLARevision1,FE_288_GetLLARevision());
        			#endif
			}
			
    		else
	    	if(demodtype == STTUNER_DEMOD_STV0299)
    		{
    			#ifdef STTUNER_DRV_SAT_STV0299
			strcpy(LLARevision1,Drv0299_GetLLARevision());
    			#endif
    		}
    		
			else    		   
	    	if(demodtype == STTUNER_DEMOD_STV0360)
    		{
    			#ifdef STTUNER_DRV_TER_STV0360
			strcpy(LLARevision1,Drv0360_GetLLARevision());
    			#endif
    		}
    		else    
	    	if(demodtype == STTUNER_DEMOD_STV0361)
    		{
    			#ifdef STTUNER_DRV_TER_STV0361
			strcpy(LLARevision1,Drv0361_GetLLARevision());
    			#endif
    		}
    		else
    		if(demodtype == STTUNER_DEMOD_STV0362)
    		{
    			#ifdef STTUNER_DRV_TER_STV0362
			strcpy(LLARevision1,Drv0362_GetLLARevision());
    			#endif
    		}
    		else    
	    	if(demodtype == STTUNER_DEMOD_STB0370VSB)
    		{
    			#ifdef STTUNER_DRV_TER_STB0370VSB
			strcpy(LLARevision1,Drv0370VSB_GetLLARevision());
    			#endif
    		}
    		
    		else
	    	if(demodtype == STTUNER_DEMOD_STV0297)
    		{
    			#ifdef STTUNER_DRV_CAB_STV0297
			strcpy(LLARevision1,Drv0297_GetLLARevision());
    			#endif
    		}
    		else
	    	if(demodtype == STTUNER_DEMOD_STV0297J)
    		{
    			#ifdef STTUNER_DRV_CAB_STV0297J
			strcpy(LLARevision1,Drv0297J_GetLLARevision());
    			#endif
    		}
    		else
	    	if(demodtype == STTUNER_DEMOD_STV0297E)
    		{
    			#ifdef STTUNER_DRV_CAB_STV0297E
			strcpy(LLARevision1,Drv0297E_GetLLARevision());
    			#endif
    		}
    		else
    		if(demodtype == STTUNER_DEMOD_STV0498)
    		{
    			#ifdef STTUNER_DRV_CAB_STV0498
			strcpy(LLARevision1,Drv0498_GetLLARevision());
    			#endif
    		}
    		else
    		if(demodtype == STTUNER_DEMOD_STB0370QAM)
    		{
    			#ifdef STTUNER_DRV_CAB_STB0370QAM
			strcpy(LLARevision1,Drv0370QAM_GetLLARevision());
    			#endif
    		}
    		
    	
		
		
			if(demodtype == STTUNER_DEMOD_STV0372)
    		{
    			#ifdef STTUNER_DRV_TER_STV0372
				strcpy(LLARevision1,Drv0372_GetLLARevision());
    			#endif
    		}
        	if(strcmp(LLARevision1,"") == 0) /*checking if LLARevision1 is empty*/
	    	{
	    		LLARevision1 = " Error: Device not present in current build ";
	    		STTBX_Print(("\nError: Device not supported in current build"));
	    	}
	    	
	    	return(LLARevision1);
    	}
}

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_Install()

Description:
    Install drivers
    
Parameters:

Return Value:
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_Install(DRIVERS_InstallParams_t *InstallParams)
{
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
   const char *identity = "STTUNER drivers.c STTUNER_DRIVER_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_USE_SAT
    SDRV_InstallParams_t SAT_InstallParams;
#endif

	#ifdef STTUNER_USE_CAB
	    CDRV_InstallParams_t CAB_InstallParams;
	#endif

#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
    TDRV_InstallParams_t TER_InstallParams;
#endif

#ifdef STTUNER_USE_SHARED
    SHARE_InstallParams_t SHARE_InstallParams;
#endif

    MemoryPartition = InstallParams->MemoryPartition;

#ifdef STTUNER_USE_SAT
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("%s installing satellite drivers \n", identity));
#endif
    SAT_InstallParams.MemoryPartition = MemoryPartition;
    SAT_InstallParams.TunerType = InstallParams->TunerType;

    Error |= STTUNER_SAT_DRV_Install(&SAT_InstallParams);
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("%s Error = %d\n", identity, Error));
#endif
#endif

	#ifdef STTUNER_USE_CAB
		#ifdef STTUNER_DEBUG_MODULE_DRIVERS
		    STTBX_Print(("%s installing cable drivers \n", identity));
		#endif
		    CAB_InstallParams.MemoryPartition = MemoryPartition;
		    Error |= STTUNER_CABLE_DRV_Install(&CAB_InstallParams);
		#ifdef STTUNER_DEBUG_MODULE_DRIVERS
		    STTBX_Print(("%s Error = %d\n", identity, Error));
		#endif
	#endif


#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("%s installing terrestrial drivers \n", identity));
#endif
    TER_InstallParams.MemoryPartition = MemoryPartition;
    TER_InstallParams.TunerType = InstallParams->TunerType;
    Error |= STTUNER_TERR_DRV_Install(&TER_InstallParams);
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("%s Error = %d\n", identity, Error));
#endif
#endif


#ifdef STTUNER_USE_SHARED
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("%s installing shared drivers ", identity));
#endif
    SHARE_InstallParams.MemoryPartition = MemoryPartition;
    Error |= STTUNER_SHARE_DRV_Install(&SHARE_InstallParams);
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("%s Error = %d\n", identity, Error));
#endif
#endif

    return(Error); /* >1 or no device ID selected */
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_UnInstall()

Description:
    Install drivers
    
Parameters:

Return Value:
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_UnInstall(void)
{
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
   const char *identity = "STTUNER drivers.c STTUNER_DRV_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_USE_SAT
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("%s uninstalling satellite drivers ", identity));
#endif
    Error |= STTUNER_SAT_DRV_UnInstall();
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("Error = %d", Error));
#endif
#endif

	#ifdef STTUNER_USE_CAB
		#ifdef STTUNER_DEBUG_MODULE_DRIVERS
		    STTBX_Print(("%s uninstalling cable drivers ", identity));
		#endif
		    Error |= STTUNER_CABLE_DRV_UnInstall();
		#ifdef STTUNER_DEBUG_MODULE_DRIVERS
		    STTBX_Print(("Error = %d", Error));
		#endif
	#endif

#if defined(STTUNER_USE_TER) || defined(STTUNER_USE_ATSC_VSB)
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("%s uninstalling terrestrial drivers ", identity));
#endif
    Error |= STTUNER_TERR_DRV_UnInstall();
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("Error = %d", Error));
#endif
#endif
#ifdef STTUNER_USE_SHARED
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("%s uninstalling shared drivers ", identity));
#endif
    Error |= STTUNER_SHARE_DRV_UnInstall();
#ifdef STTUNER_DEBUG_MODULE_DRIVERS
    STTBX_Print(("Error = %d\n", Error));
#endif
#endif
    return(Error); /* >1 or no device ID selected */
}



/* End of drivers.c */
