#ifdef    TRACE_CAINFO_ENABLE
#define  MGRAM_DRIVER_DEBUG
#endif
/******************************************************************************
*
* File : MgRamDrv.C
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
 #include "MgRamDrv.h"


#if 0
 #define MGKERN_MEMORY_SIZE   0x40000  
 static unsigned char MgKernBlock[MGKERN_MEMORY_SIZE];	  
 #pragma ST_section     (MgKernBlock,"mgkern_section")		  

 ST_Partition_t  TheMgKernPartition;           
 static ST_Partition_t *MgKernPartition=&TheMgKernPartition;
 #endif


/*****************************************************************************
 * Interface description
 *****************************************************************************/

#if 0
/*******************************************************************************
 *Function name: MgKerPartitionInit
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype:
 *         void MgKerPartitionInit(void)
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *Return code:
 *     
 *     
 *     
 *     
 *     
 *
 *comments:
 *
 *
 *
 *******************************************************************************/
void MgKerPartitionInit(void)
{
      /* for mg kernel partition added 040619*/
      interrupt_lock();
      task_lock ();
      partition_init_heap(&TheMgKernPartition, (U8*) MgKernBlock, sizeof(MgKernBlock)); 
      task_unlock ();
      interrupt_unlock();	
	  
}


/*******************************************************************************
 *Function name: MgKernelPartitionDEl
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype:
 *         void MgKernelPartitionDEl(void)
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *Return code:
 *     
 *     
 *     
 *     
 *     
 *
 *comments:
 *
 *
 *
 *******************************************************************************/

void MgKernelPartitionDEl(void)
{
	/* for del mgkernel partition */
	interrupt_lock();
       task_lock ();
	partition_delete( &TheMgKernPartition );
       do_report(severity_info,"\n MgKernelPartition delete successful !!\n");
	
       task_unlock ();
	interrupt_unlock();
	
}
#endif


/*******************************************************************************
 *Function name: MGDDIMemAlloc
 *           
 *
 *Description: This function allocates a memory block with a size indicated by 
 *             Size parameter.The handle returned corresponds to the address of
 *             the memory block allocated.
 *
 *Prototype:
 *     TMGDDIMemHandle   MGDDIMemAlloc(udword  Size)       
 *           
 *
 *
 *input:
 *     Size:   Size of the memory block to be reserved,in bytes.       
 *           
 *
 *output:
 *           
 *
 *Return code:
 *     Handle of the memory block(address),if not null.
 *           NULL,if a problem has occurred during allocation:
 *                - size null 
 *                - there is no memory space available, 
 *                - no resource available.
 *
 *comments:
 *
 *
 *
 *******************************************************************************/
 TMGDDIMemHandle   MGDDIMemAlloc(udword  Size)
 {
       TMGDDIMemHandle     MemHandle;

       MemHandle = (TMGDDIMemHandle)CHCA_MemAllocate(Size);
#ifdef    MGRAM_DRIVER_DEBUG	   
	do_report(severity_info,"\n[MGDDIMemAlloc==>]MemHandle[%d]\n",MemHandle); 
#endif
	   
	return MemHandle;  
 }


/*******************************************************************************
 *Function name : MGDDIMemFree
 *           

 *Description:This function releases a memory block related to the hMem handle
 *           
 *
 *Prototype:
 *      TMGDDIStatus MGDDIMemFree( TMGDDIMemHandle  hMem)     
 *           
 *
 *
 *input:
 *     hMem:     Handle of the memory block to be freed.      
 *           
 *
 *output:
 *           
 *           
 *Return code           
 *     MGDDIOk:         The memory block has been freed      
 *     MGDDIBadParam:   Memory block handle unknown           
 *     MGDDIError:      Interface execution error      
 *           
 *Comments:
 *
 *******************************************************************************/
 TMGDDIStatus MGDDIMemFree( TMGDDIMemHandle  hMem)
 {
      TMGDDIStatus    StatusMgDdi;

	StatusMgDdi = (TMGDDIStatus)CHCA_MemDeallocate((TCHCAMemHandle)hMem); 
	
#ifdef    MGRAM_DRIVER_DEBUG	   
	do_report(severity_info,"\n[MGDDIMemAlloc==>]MemHandle[%d]\n",hMem);   
#endif

	return StatusMgDdi;
 }





