/*
 *********************************************************************
 *  @file   : OS21WrapperInterrupt.h
 *
 *  Purpose : Wrapper for the OS21 Interrupt library, this file should
 *  include all structs defines and functions OS21 that should be
 *  re-implemented on WinCE
 *
 *********************************************************************
 */
#ifndef __OS21_WRAPPER_INTERRUPT__
#define  __OS21_WRAPPER_INTERRUPT__

#include <SH4_intc.h>
#include <SH4202T_intc2_irq.h>

////////////////////////////////////////////////////////
///////////////                 ////////////////////////
//////////////// Types		    ////////////////////////
///////////////                 ////////////////////////
////////////////////////////////////////////////////////

#define OS21_INTERRUPT_VID_GLH 4
typedef unsigned int interrupt_trigger_mode_t;
// typedef unsigned int interrupt_name_t; JLX: now defined in stddefs.h
typedef int		 interrupt_t;

// interrupt_handle converts an OS21 interrupt number to an OS21 interrupt handle
// WinCE: we don't want this !!
#define interrupt_handle(INT_NUMBER) (INT_NUMBER)

// Type of ISR (Interrupt Service Routine) functions for OS21.
// In the WinCE port, we call these functions from an IST.
typedef int (*interrupt_handler_t)(void * param);





//#define _USE_NAMED_INTERRUPTS for Tracing IST only
#ifdef _USE_NAMED_INTERRUPTS

// WHE:
// In order to trace and modify dynamicaly the IST, we keep the name ( in tName) of the file that, init the interrupt
// Can be removed by changing #if 0
// mainly used in the debug toolbox of DSHOW project

#define interrupt_install_WinCE(a,b,c,d)			 _Named_interrupt_install_WinCE(a,b,c,__FILE__)
#define interrupt_install_shared_WinCE(a,b,c,d)  _Named_interrupt_install_shared_WinCE(a,b,c,d,__FILE__)

int _Named_interrupt_install_WinCE(unsigned int  interruptId,
					  interrupt_handler_t userHandler,
					  void *              userParam,
					  char *			  pComment
					 );

int _Named_interrupt_install_shared_WinCE(unsigned int                 interruptId,
					  interrupt_handler_t userHandler,
					  void *              userParam,
					  char *			  pComment
					 );


#else
// interrupt_install
// OS2x: Connects an ISR (Interrupt Service Routine) to a logical interrupt line.
//       The mapping logical line <-> IRQ line is defined in the OS2x BSP.
// WinCE: An wrapper IST is connected to the interrupt and calls the user-provided "ISR".
// Note: interrupt_install does not enable the interrupt.
int interrupt_install_WinCE(unsigned int                 interruptId,
					  interrupt_handler_t userHandler,
					  void *              userParam,
            char *             pName
					 );

int interrupt_install_shared_WinCE(unsigned int                 interruptId,
					  interrupt_handler_t userHandler,
					  void *              userParam,
					  char *              pName
					 );
					 
int interrupt_install_WinCE_2(
							unsigned int        interruptId, // this is an IRQ number
                            DWORD *             sysIntr, 
							HANDLE              hSyncEvent,
							int                 stackSize,
                            interrupt_handler_t userHandler,
                            void *              userParam,
							char *              taskName
                           );
#endif 


#define interrupt_install(a,b,c,d)		  interrupt_install_WinCE(interrupt_handle(a),(interrupt_handler_t )c,d,__FILE__)
#define interrupt_install_shared(a,b,c,d) interrupt_install_shared_WinCE(interrupt_handle(a),(interrupt_handler_t )c,d,__FILE__)
#define interrupt_uninstall_shared(a,b,c) interrupt_uninstall_shared_Wince(interrupt_handle(a),(interrupt_handler_t )b, c)

#define interrupt_uninstall(a,b) interrupt_uninstall_WinCE(interrupt_handle(a))
#define interrupt_enable_number(number) interrupt_enable_WinCE(interrupt_handle(number))
#define interrupt_disable_number(number) interrupt_disable_WinCE(interrupt_handle(number))
#define interrupt_disable(number) interrupt_disable_WinCE(interrupt_handle(number))
#define interrupt_clear_number(number) interrupt_clear_WinCE(interrupt_handle(number))
#define interrupt_raise_number(number) interrupt_raise_WinCE(interrupt_handle(number))
#define interrupt_poll_number(number,value) interrupt_poll_WinCE((unsigned int)interrupt_handle(number),value)
#define task_flags_high_priority_process  0 /* does not exist for ST40 */
#define interrupt_mask_all()		interrupt_mask_all_WinCE() // no parameter
#define interrupt_unmask_all()		interrupt_unmask_all_WinCE() // WHE() // ??????
#define interrupt_unmask(a)  interrupt_unmask_all_WinCE()

#define interrupt_lock() interrupt_lock_WinCE() /*interrupt_mask_all_WinCE()*/
#define interrupt_unlock() interrupt_unlock_WinCE() /*interrupt_unmask_all_WinCE()*/

#define interrupt_priority_number(number,b) interrupt_priority_WinCE((unsigned int)interrupt_handle(number),b)
// OLGN - Link 3.9.0 + Multicom
#define interrupt_enable(number) interrupt_enable_WinCE(interrupt_handle(number))

////////////////////////////////////////////////////////
///////////////                 ////////////////////////
////////////// Interrupt functions /////////////////////
///////////////                 ////////////////////////
////////////////////////////////////////////////////////





int interrupt_uninstall_shared_Wince (unsigned int  Id,interrupt_handler_t handler,void * param);


// interrupt_uninstall_WinCE
// OS2x: Disconnects the ISR from the interrupt.
// WinCE: Uninstall the wrapper IST.
int interrupt_uninstall_WinCE(unsigned int interruptId);

// interrupt_enable_WinCE
// OS2x: Enables the interrupt at the interrupt controller.
// WinCE: Unmasks the interrupt
int interrupt_enable_WinCE(unsigned int interruptId);


// interrupt_disable_WinCE
// OS2x: Disables the interrupt at the controller.
// WinCE: masks the interrupt
int interrupt_disable_WinCE(unsigned int interruptId);

// interrupt_mask_all
// OS2x: masks all interrupts
// WinCE: disables all interrupts
int interrupt_mask_all_WinCE(void);
int interrupt_unmask_all_WinCE(void);

//interrupt_lock(), interrupt_unlock()
//OS2x : masks all CPU interrupts (could be used like a crtical section)
// WinCE: Put current thread priority to 0.
void interrupt_lock_WinCE();
void interrupt_unlock_WinCE();

// interrupt_unmask
// OS2x: unmasks all interrupts, restores mask from interrupt_mask_all().
// WinCE: enables all interrupts
///void interrupt_unmask(int priority);????? WHE

int interrupt_raise_WinCE(unsigned int interruptId);


//??????????????????????? USED in STCM lib audio
int   interrupt_poll_WinCE (unsigned int interruptId, int * value); 
int   interrupt_clear_WinCE(unsigned int interruptId);
int   interrupt_priority_WinCE(unsigned int interruptId,int * priority);

unsigned long  GetNewSysintr(unsigned int interruptId);
void		   ReleaseSysintr(DWORD sysintr);
// WHE: Dshow 
char * _WCE_EnumStapiISTName(int Index);
int _WCE_SetCeISTPriority(int Index, int WincePriority);
int _WCE_GetCeISTPriority(int Index);

#endif // __OS21_WRAPPER_INTERRUPT__
