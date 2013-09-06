#define  CHMEMORY_DRIVER_DEBUG
/******************************************************************************
*
* File : ChMemory.C
*
* Description : 
*
*
* NOTES :
*
*
* Author : 
*
* Status :
*
* History : 0.0    2004-6-26   Start coding
*           
* opyright: Changhong 2004 (c)
*
*****************************************************************************/
#include   "ChMemory.h"


/*static*/ CHCA_UCHAR          MgKernBlock[MGKERN_MEMORY_SIZE];	  
#pragma ST_section         (MgKernBlock,"mgkern_section")	


/*CHCA_Partition_t                   TheMgKernPartition;   */        
static CHCA_Partition_t           *MgKernPartition=NULL;




/*****************************************************************************
 * Interface description
 *****************************************************************************/


/*******************************************************************************
 *Function name: CHCA_PartitionInit
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype:
 *         void CHCA_PartitionInit(void)
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
void CHCA_PartitionInit(void)
{
      /* for mg kernel partition added 040619*/
      /*interrupt_lock();*/
      /*task_lock ();*/
     MgKernPartition= partition_create_heap( (U8*) MgKernBlock, sizeof(MgKernBlock)); 
      /*task_unlock ();*/
      /*interrupt_unlock();	*/
	  
}


/*******************************************************************************
 *Function name: CHCA_PartitionDel
 *           
 *
 *Description: 
 *             
 *             
 *
 *Prototype:
 *        void CHCA_PartitionDel(void)
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
void CHCA_PartitionDel(void)
{
	/* for del mgkernel partition */
	/*interrupt_lock();
       task_lock ();*/
	partition_delete( MgKernPartition);
       /*task_unlock ();
	interrupt_unlock();*/
}


/*******************************************************************************
 *Function name: CHCA_MemAllocate
 *           
 *
 *Description: This function allocates a memory block with a size indicated by 
 *             Size parameter.The handle returned corresponds to the address of
 *             the memory block allocated.
 *
 *Prototype:
 *     TCHCAMemHandle   CHCA_MemAllocate(CHCA_ULONG  MemSize)     
 *           
 *
 *
 *input:
 *     MemSize:   Size of the memory block to be reserved,in bytes.       
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
 TCHCAMemHandle   CHCA_MemAllocate(CHCA_ULONG  MemSize)
 {
       TCHCAMemHandle        *pRamHandle; 
	   
	if((MemSize > MGKERN_MEMORY_SIZE) || (MemSize==0))
	{
             do_report(severity_info," [CHCA_MemAlloc==>] the Size is too large to be allocated!");
	      return 0;
	}   
	
	pRamHandle = (TCHCAMemHandle *)memory_allocate(MgKernPartition,(CHCA_UINT)MemSize);
	
#ifdef    CHMEMORY_DRIVER_DEBUG	
	do_report(0, "[CHCA_MemAlloc==>] mgca malloc nbytes = 0x%x, ptr = 0x%x\n", MemSize, pRamHandle);
#endif

	return pRamHandle;  
 }


/*******************************************************************************
 *Function name : CHCA_MemDeallocate
 *           

 *Description:This function releases a memory block related to the hMem handle
 *           
 *
 *Prototype:
 *      CHCA_DDIStatus CHCA_MemDeallocate( TCHCAMemHandle  hMemHandle)
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
 *     CHCADDIOk:         The memory block has been freed      
 *     CHCADDIBadParam:   Memory block handle unknown           
 *     CHCADDIError:      Interface execution error      
 *           
 *Comments:
 *
 *******************************************************************************/
CHCA_DDIStatus CHCA_MemDeallocate( TCHCAMemHandle  hMemHandle)
{
       CHCA_DDIStatus   StatusMgDdi = CHCADDIOK;

       if(hMemHandle==NULL)
       {
#ifndef    CHMEMORY_DRIVER_DEBUG	       
             do_report(severity_info,"\n[CHCA_MemDeallocate==>]the mem handel is null\n"); 
#endif
	      StatusMgDdi = CHCADDIBadParam;		 
	      return StatusMgDdi;		 
	}

       memory_deallocate(MgKernPartition, hMemHandle);
	   
#ifdef    CHMEMORY_DRIVER_DEBUG		   
	do_report(severity_info, "[CHCA_MemDeallocate==>] mgca free ptr = 0x%x\n", hMemHandle);
#endif

	return StatusMgDdi;
 }







