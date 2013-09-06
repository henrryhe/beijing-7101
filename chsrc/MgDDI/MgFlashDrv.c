#ifdef    TRACE_CAINFO_ENABLE
#define  MGFLASH_DRIVER_DEBUG
#endif
/******************************************************************************
*
* File : MgFlashDrv.C
*
* Description : SoftCell driver
*
*
* NOTES :
*
*
* Author :
*
* Status : 
*
* History : 0.0   2004-06-08  Start coding
*           
*
* Copyright: Changhong 2004 (c)
*
*****************************************************************************/
/*****************************************************************************
 *BASIC DEFINITIONS
 *****************************************************************************/
#include "MgFlashDrv.h"



/*****************************************************************************
 *Variable description
 *****************************************************************************/
 #if 0
#define     CHFLASH_BANK0_BASE                            0x7fc00000  /*4M*/
#define     CHFALAH_DBASE_SIZE_BLOCKPER           0x20000  /*the size of flash per block 128K for M54032D  ,yxl 2004-06-15 add*/
#define     CHFALAH_DBASE_OFFSET_ADDRESS        CHFALAH_DBASE_SIZE_BLOCKPER*29  /*the offset address of dbase within flash ,for M54032D*/
#define     CHFALAH_DBASE_START_ADDRESS         (CHFLASH_BANK0_BASE+CHFALAH_DBASE_OFFSET_ADDRESS) 


#if 0
#define     MGKERNEL_FLASH_BASEDRESS    0x7FC1D800 
#define     MGKERNEL_FLASH_DATADRESS    0x7FC1D808 
#define     MGKERNEL_FLASH_ENDDRESS      0x7FC20000
#else
#define     MGKERNEL_DBASE_SIZE              0x1d800/*118*1024*/

#if  0
#define     MGKERNEL_FLASH_BASEDRESS      (CHFALAH_DBASE_START_ADDRESS+MGKERNEL_DBASE_SIZE)
#define     MGKERNEL_FLASH_DATADRESS    (MGKERNEL_FLASH_BASEDRESS+0x8)
#define     MGKERNEL_FLASH_ENDDRESS      (CHFALAH_DBASE_START_ADDRESS+CHFALAH_DBASE_SIZE_BLOCKPER)
#else
#define     MGKERNEL_FLASH_BASEDRESS    0x7ffbd800/*0x7ffdd800  30*/
#define     MGKERNEL_FLASH_DATADRESS    0x7ffbd808/*0x7ffdd808*/
#define     MGKERNEL_FLASH_ENDDRESS      0x7ffc0000/*0x7ffe0000*/
#endif
#endif
#endif



#define     MGKERNEL_OFFSET                       (MGKERNEL_FLASH_ENDDRESS - MGKERNEL_FLASH_DATADRESS)
#define     MGKERNEL_FLASH_MAX_SIZE       MGKERNEL_OFFSET


CHCA_BOOL     MgKernelFlashOpen = false;

/*****************************************************************************
 *Interface description
 *****************************************************************************/






/*******************************************************************************
 *Function name: MGDDIFlashOpen
 *           
 *
 *Description:This function opens a FLASH area indentified by the 8 bytes array
 *            provided at the address given in FlashID parameter.This function           
 *            returns a DDI FLASH memory handle that will be used for read and
 *            write operation
 *
 *Prototype:
 *      TMGDDIFlashHandle  MGDDIFlashOpen(ubyte*  FlashID)     
 *           
 *
 *
 *input:
 *     FlashID:   Identifier of a FLASH memory area to open        
 *           
 *
 *output:
 *              
 *           
 *           
 *Return code:
 *     FLASH memory handle(address),if not null.
 *     NULL,if a problem has occurred during allocation:
 *        - FlashID parameter is null
 *        - unknown FLASH area identifier,
 *        - specified FLASH area is already opend,
 *        - no resource available
 *
 *Comments:
 *
 *
 *******************************************************************************/
 TMGDDIFlashHandle  MGDDIFlashOpen(ubyte*  FlashID)
 {
	
       if(FlashID==NULL)
      	{
             do_report(severity_info,"\n FlashID parameter is null \n");   
	      return NULL;		 
	}

	if(memcmp(FlashID,MGDDIFlashID,8))  
	{
	     do_report(severity_info,"\n unknown FLASH area identifier \n");
            return NULL;
	}

       if(MgKernelFlashOpen)
       {
             do_report(severity_info,"\n specified FLASH area is already opend \n");
	      return NULL;		 
	}
 
       MgKernelFlashOpen = true;

	return (TMGDDIFlashHandle)MGKERNEL_FLASH_DATADRESS;
	
 }



/*******************************************************************************
 *Function name: MGDDIFlashClose
 *           
 *
 *Description:This function closes a FLASH memory area identified by the hFlash 
 *            handle.Closing a FLASH memory area induces a hardware FLASH memory
 *            update
 *
 *Prototype:
 *      TMGDDIStatus  MGDDIFlashClose(TMGDDIFlashHandle   hFlash)     
 *           
 *input:
 *      hFlash:      FLASH  memory handle           
 *           
 *
 *output:
 *
 *
 *Return code:
 *       MGDDIOk:         FLASH area closed.   
 *       MGDDIBadParam:   FLASH memory handle unknown.
 *       MGDDIError:      Interface execution error.
 *
 *Comments:
 *
 *
 *******************************************************************************/
 TMGDDIStatus  MGDDIFlashClose(TMGDDIFlashHandle   hFlash) 
 {
       TMGDDIStatus                 StatusMgDdi =MGDDIOK;  
 
       if((hFlash==NULL)||(hFlash != (TMGDDIFlashHandle)MGKERNEL_FLASH_DATADRESS))
       {
              StatusMgDdi = MGDDIBadParam;
#ifdef    MGFLASH_DRIVER_DEBUG			  
	       do_report(severity_info,"\n FLASH memory handle unknown \n");
#endif			
              return(StatusMgDdi);
       }

       MgKernelFlashOpen = false; 

       return(StatusMgDdi);	  
 }	 



/*******************************************************************************
 *Function name: MGDDIFlashWrite
 *           
 *
 *Description: This function writes the data provided at the pBuffer address with 
 *             the size Size in the FLASH memory area identified by the hFlash 
 *             handle.The data buffer is written with an offset given by offset  
 *             parameter.Caution must be exercised,since the beginning of the area,
 *             together with its size,is indicated in bytes
 *Prototype:
 *     TMGDDIStatus  MGDDIFlashWrite      
 *           (TMGDDIFlashHandle  hFlash,
 *            udword  offset,ubyte*  pBuffer,udword  Size)  
 *
 *input:
 *     hFlash:              FLASH memory handle      
 *     offset:              Memory offset in bytes related to beginning of area
 *     pBuffer:             Address of data to be written.
 *     Size:                Size of data to be written in bytes
 *
 *output:
 *           
 *
 *
 *Return code:
 *    MGDDIOK:          Data buffer has been written.
 *    MGDDIBadParam:    FLASH memory handle unknown or pareameter null or invalid
 *    MGDDIError:       Interface execution error
 *
 *Comments:
 *     2005-03-19   same date will not be written into the flash again      
 *******************************************************************************/
 TMGDDIStatus  MGDDIFlashWrite
	 (TMGDDIFlashHandle  hFlash,
	  udword  offset,ubyte*  pBuffer,udword  Size) 
 {
       TMGDDIStatus                 StatusMgDdi=MGDDIOK; 
	CHCA_UINT                     i;  
	CHCA_ULONG                  writeoffset; 
       CHCA_ULONG                  Flashaddr;
       CHCA_CHAR                    *temp;




 	if((hFlash==NULL)||(hFlash != (TMGDDIFlashHandle)MGKERNEL_FLASH_DATADRESS)\
	    ||(pBuffer==NULL)||(Size==0)||(Size>=MGKERNEL_FLASH_MAX_SIZE)) 
       {
              StatusMgDdi = MGDDIBadParam;
#ifdef    MGFLASH_DRIVER_DEBUG			  
	       do_report(severity_info,"\n FLASH memory handle unknown or pareameter null or invalid \n");
#endif
		return(StatusMgDdi);   
	}

       if(!MgKernelFlashOpen)
       {
             StatusMgDdi = MGDDIBadParam;
#ifdef    MGFLASH_DRIVER_DEBUG			  
	       do_report(severity_info,"\n the flash is closed \n");
#endif
		return(StatusMgDdi);   
	}

#ifdef    MGFLASH_DRIVER_DEBUG
       do_report(severity_info,"\n [MGDDIFlashWrite] kerneloffset[%d] len is %d \n",MGKERNEL_OFFSET,Size);
       for(i=0;i<Size;i++)
	     do_report(severity_info,"%4x",*(pBuffer+i));
	do_report(severity_info,"\n");   
#endif	

       
       if((MGKERNEL_FLASH_DATADRESS+offset+Size)>= MGKERNEL_FLASH_ENDDRESS)
       {
              StatusMgDdi = MGDDIBadParam;
#ifdef    MGFLASH_DRIVER_DEBUG			  
	       do_report(severity_info,"\n FLASH memory handle unknown or pareameter null or invalid \n");
#endif
		return(StatusMgDdi);   
	}
  #if 0     /*20061214 del*/
	writeoffset = 128*1024-MGKERNEL_OFFSET + offset;
#else
   
     writeoffset=offset;
  #endif
#ifdef    MGFLASH_DRIVER_DEBUG			  
	do_report(severity_info,"\n FLASH memory write offset[%d]  CA Area offset[%d] \n",writeoffset,offset);
#endif


#if 0 /*test2 on 050319,test is not passed,the index is must been saved*/
      if(!CheckPairFlashStatus())
      {
             return(StatusMgDdi);			
      }
#endif
#if/*def NEW_FLASH_UPDATE*/0
        CH_PUB_UpdateCAData(writeoffset,pBuffer,Size);
#else
        if(CHCA_FlashWrite(writeoffset,pBuffer,Size))
	 {
	       StatusMgDdi = MGDDIError;	
	#ifdef MGFLASH_DRIVER_DEBUG
		do_report(severity_info," [MGDDIFlashWrite] Write flash Error !");
	#endif
        }	   
#endif
	
	 return(StatusMgDdi);



 }

/*******************************************************************************
 *Function name: MGDDIFlashRead
 *           
 *
 *Description: The function reads a FLASH memory area identified by hFlash handle.
 *             data is read with an offset Offset,starting from the beginning of 
 *             the area.Caution must be exercised,since this offset is expressed 
 *             bytes.the data read is placed in memory block provided at the address 
 *             given in the pBuffer parameter.Its maximum size is specified when called,
 *             by the pSize parameter,which is then filled with the size of the data
 *             actually read.
 *
 *Prototype:
 *      TMGDDIStatus MGDDIFlashRead      
 *           (TMGDDIFlashHandle  hFlash,
 *            udword  Offset,ubyte* pBuffer,udword* pSize) 
 *
 *input:
 *      hFlash:                 FLASH  memory  handle
 *      Offset:                 Memory offset in bytes related to beginning of area.
 *      pBuffer:                Storage address of data read.
 *      pSize:                  Maximum size of the data read and yet to be read.
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     MGDDIOK:              The data has been read. 
 *     MGDDIBadParam:        FLASH memory handle unknown or parameter null or invalid
 *     MGDDIError:           Interface execution error.
 *
 *
 *Comments:
 *           
 *******************************************************************************/
 TMGDDIStatus MGDDIFlashRead
	  (TMGDDIFlashHandle  hFlash,
	   udword  Offset,ubyte* pBuffer,udword* pSize)
 {
       TMGDDIStatus                 StatusMgDdi=MGDDIOK;
       CHCA_UINT                     i;
	CHCA_CHAR                    *temp;   
	CHCA_ULONG                  readlen,Flashaddr;


 	if((hFlash==NULL)||(hFlash != (TMGDDIFlashHandle)MGKERNEL_FLASH_DATADRESS)\
	    ||(pBuffer==NULL)||(pSize==NULL)) 
       {
              StatusMgDdi = MGDDIBadParam;
#ifdef    MGFLASH_DRIVER_DEBUG			  
	       do_report(severity_info,"\n FLASH memory handle unknown or parameter null or invalid \n");
#endif
		return(StatusMgDdi);   
	}

       if(!MgKernelFlashOpen)
       {
             StatusMgDdi = MGDDIBadParam;
#ifdef    MGFLASH_DRIVER_DEBUG			  
	       do_report(severity_info,"\n the flash is closed \n");
#endif
		return(StatusMgDdi);   
	}

 	readlen = *pSize;  

#ifdef    MGFLASH_DRIVER_DEBUG			  
	do_report(severity_info,"\n[MGDDIFlashRead::] the readlen[%d] Offset[%d] \n",readlen,Offset);
#endif
      CH_FlashLock(); /*add this on 050305*/

       Flashaddr = MGKERNEL_FLASH_DATADRESS + Offset;

       if((Flashaddr+readlen)>=MGKERNEL_FLASH_ENDDRESS)
       {
            StatusMgDdi = MGDDIError;
			
#ifdef  MGFLASH_DRIVER_DEBUG			  
	     do_report(severity_info,"\n The read len is too large \n");
#endif	

            CH_FlashUnLock();/*add this on 050305*/

	     return(StatusMgDdi);   		
       }

       /*read the data of the flash*/
       for(i=0;i<readlen;i++)
       {
              temp=(CHCA_CHAR*)(Flashaddr+i);
		pBuffer[i]=*temp;	 
#ifdef  MGFLASH_DRIVER_DEBUG			  
     	       do_report(severity_info,"%4x",pBuffer[i]);
#endif
	}
       CH_FlashUnLock();/*add this on 050305*/
   

	/*do_report(severity_info,"\n");*/

	return(StatusMgDdi);   

 
 }




