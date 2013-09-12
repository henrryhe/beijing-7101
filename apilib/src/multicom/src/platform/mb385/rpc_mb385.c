/*
 * rpc_mb385.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Replacement OS21 board support for MB385.
 */

/* The board support files provided with ST200 R3.0 (and R3.0 patch 1) 
 * do not support any external interrupts. This alternative board 
 * support is intended as a temporary stop-gap solution to these 
 * omissions. It will be removed from the multicom distribution once 
 * suitable board support files are available in the toolset.
 */

#include <os21/st200.h>

#include "rpc_mb385.h"

/*
 * Declare the frequency of the external crystal
 * on this board
 */
unsigned int bsp_xtal_frequency_hz = 27000000;

/*
 * For setting up the timers correctly, OS21 needs to know
 * the timer input clock frequency. This can be done in a
 * number of ways:
 *
 * 1. Tell OS21 directly by simply assigning the correct
 *    value to bsp_timer_input_clock_frequency_hz. This
 *    ensures accuracy providing you get it right.
 *
 * 2. Tell OS21 directly by simply assigning the result
 *    of calling bsp_timer_input_clock_frequency () to
 *    bsp_timer_input_clock_frequency_hz - providing
 *    this function exists.
 *
 * 3. Leave the value of bsp_timer_input_clock_frequency_hz
 *    unchanged (at zero). OS21 will detect this and try to
 *    figure it out for itself using the real time clock (if
 *    fitted). If not fitted OS21 will panic!
 */
unsigned int bsp_timer_input_clock_frequency_hz = 0;

/*
 * This is used to tell OS21 what the timeslice frequency
 * is. You can change this to any sensible value. OS21 will
 * panic if the value you choose is not sensible!
 */
unsigned int bsp_timeslice_frequency_hz = 50;

/*
 * This is a function that obtains the timer input clock frequency.
 */
unsigned int bsp_timer_input_clock_frequency (void);

/*
 * This funtion is called from kernel_initialize.
 */
void bsp_initialize (void)
{
  /*
   * We set the timer input clock frequency using method 2 above.
   */
  bsp_timer_input_clock_frequency_hz = bsp_timer_input_clock_frequency ();
}

/*
 * This function is called from kernel_start
 */
void bsp_start (void)
{
}

/*
 * This function is called when an unexpected exception has occurred.
 *
 * If this BSP function exits, the kernel will proceed to announce
 * the exception, emit a register dump and attempt to shutdown.
 *
 * This function is passed an unsigned int which describes the type
 * of exception that occurred.
 */
void bsp_exp_handler (unsigned int exp_code)
{
}

/*
 * This function is called when an OS21 kernel panic occurs.
 *
 * Once this BSP function exits, the kernel will proceed to announce
 * the panic, before masking interrupts and looping forever.
 *
 * This function is passed a char * which points to the panic message.
 *
 */
void bsp_panic (const char *message)
{
}

/*
 * This function is called when OS21 shuts down.
 *
 * Once this BSP function exits, the kernel will proceed to shutdown,
 * stopping timers and masking interrupts etc.
 *
 */
void bsp_shutdown (void)
{
}

/*
 * This function is called when OS21 aborts whilst running
 * disconnected from a host debugger. It is called with interrupts
 * disabled.
 *
 * The default behaviour is to simply return, causing OS21 to enter
 * a spin loop.
 */
void bsp_terminate (void)
{
}

/*
 * This function returns the board type as a string.
 */
const char * bsp_board_type (void)
{
  return ("MB385 (extended multicom support)");
}

/*
 * This function returns the cpu type as a string.
 */
const char * bsp_cpu_type (void)
{
  return ("ST220");
}

/*
 * Definitions of the interrupts we'll be using. Each interrupt
 * MUST be assigned a UNIQUE number. The actual value of the
 * number is NOT significant - any numbering scheme is arbitrary.
 */

/*
 * These timer interrupts MUST be present, otherwise OS21 will be unable
 * to program the timer units and you'll get a link time error.
 */
interrupt_name_t OS21_INTERRUPT_TIMER_0  = 1;
interrupt_name_t OS21_INTERRUPT_TIMER_1  = 2;
interrupt_name_t OS21_INTERRUPT_TIMER_2  = 3;
interrupt_name_t OS21_INTERRUPT_MBOX1_0  = 38;
interrupt_name_t OS21_INTERRUPT_MBOX1_1  = 39;
interrupt_name_t OS21_INTERRUPT_REMOTE_0 = 56;
interrupt_name_t OS21_INTERRUPT_REMOTE_1 = 57;
interrupt_name_t OS21_INTERRUPT_REMOTE_2 = 58;
interrupt_name_t OS21_INTERRUPT_REMOTE_3 = 59;
interrupt_name_t OS21_INTERRUPT_REMOTE_4 = 60;
interrupt_name_t OS21_INTERRUPT_REMOTE_5 = 61;
interrupt_name_t OS21_INTERRUPT_REMOTE_6 = 62;
interrupt_name_t OS21_INTERRUPT_REMOTE_7 = 63;

/*
 * These ILC interrupt names are also compulsory - you'll get a
 * link error if you omit them. You must also ensure that the
 * value of OS21_INTERRUPT_ILC_n is equal to (OS21_INTERRUPT_ILC_0 + n).
 */
interrupt_name_t OS21_INTERRUPT_ILC_0  = 100;
interrupt_name_t OS21_INTERRUPT_ILC_1  = 101;
interrupt_name_t OS21_INTERRUPT_ILC_2  = 102;
interrupt_name_t OS21_INTERRUPT_ILC_3  = 103;
interrupt_name_t OS21_INTERRUPT_ILC_4  = 104;
interrupt_name_t OS21_INTERRUPT_ILC_5  = 105;
interrupt_name_t OS21_INTERRUPT_ILC_6  = 106;
interrupt_name_t OS21_INTERRUPT_ILC_7  = 107;
interrupt_name_t OS21_INTERRUPT_ILC_8  = 108;
interrupt_name_t OS21_INTERRUPT_ILC_9  = 109;
interrupt_name_t OS21_INTERRUPT_ILC_10 = 110;
interrupt_name_t OS21_INTERRUPT_ILC_11 = 111;
interrupt_name_t OS21_INTERRUPT_ILC_12 = 112;
interrupt_name_t OS21_INTERRUPT_ILC_13 = 113;
interrupt_name_t OS21_INTERRUPT_ILC_14 = 114;
interrupt_name_t OS21_INTERRUPT_ILC_15 = 115;

/*
 * This is a "dummy" interrupt which can be used for
 * testing purposes etc.
 */
interrupt_name_t OS21_INTERRUPT_TEST = 999;

/*
 * This is the interrupt table.
 *
 * There are FIVE fields in the table as follows:
 *
 * FIELD 1: The interrupt name.
 * FIELD 2: The interrupt controller the interrupt belongs to.
 * FIELD 3: Which controller register set the interrupt belongs to.
 * FIELD 4: Which set of bits the interrupt belongs to.
 * FIELD 5: Whether the interrupt is maskable.
 *
 */
interrupt_table_entry_t bsp_interrupt_table [] =
{
  { &OS21_INTERRUPT_TIMER_0,   OS21_CTRL_INTC, 0, 0       },
  { &OS21_INTERRUPT_TIMER_1,   OS21_CTRL_INTC, 0, 1       },
  { &OS21_INTERRUPT_TIMER_2,   OS21_CTRL_INTC, 0, 2       },
  { &OS21_INTERRUPT_MBOX1_0,   OS21_CTRL_INTC, 2, 38 - 32 },
  { &OS21_INTERRUPT_MBOX1_1,   OS21_CTRL_INTC, 2, 39 - 32 },
  { &OS21_INTERRUPT_REMOTE_0,  OS21_CTRL_INTC, 2, 56 - 32 },
  { &OS21_INTERRUPT_REMOTE_1,  OS21_CTRL_INTC, 2, 57 - 32 },
  { &OS21_INTERRUPT_REMOTE_2,  OS21_CTRL_INTC, 2, 58 - 32 },
  { &OS21_INTERRUPT_REMOTE_3,  OS21_CTRL_INTC, 2, 59 - 32 },
  { &OS21_INTERRUPT_REMOTE_4,  OS21_CTRL_INTC, 2, 60 - 32 },
  { &OS21_INTERRUPT_REMOTE_5,  OS21_CTRL_INTC, 2, 61 - 32 },
  { &OS21_INTERRUPT_REMOTE_6,  OS21_CTRL_INTC, 2, 62 - 32 },
  { &OS21_INTERRUPT_REMOTE_7,  OS21_CTRL_INTC, 2, 63 - 32 }, 
  { &OS21_INTERRUPT_TEST,      OS21_CTRL_INTC, 2, 63 - 32 }
};

/*
 * If an ILC is provided then we may use this table to map
 * interrupts to the INTC. This table must be present but
 * empty if the CPU does not have an ILC.
 *
 * There are FOUR fields in the table as follows:
 *
 * FIELD 1: The name of the interrupt that arrives on the above input.
 * FIELD 2: The number of the input into the ILC.
 * FIELD 3: The number of the output from the ILC.
 *          (will be in the interrupt table).
 * FIELD 4: The ILC trigger mode setting.
 *
 */
ilc_table_entry_t bsp_ilc_table[] = 
{
  { 0, 0, 0, 0 }                /* Dummy entry to shut compiler up! */
};

/*
 * These tell OS21 the size of the various tables.
 */
unsigned int bsp_interrupt_table_entries = sizeof (bsp_interrupt_table) / sizeof (interrupt_table_entry_t);
unsigned int bsp_ilc_table_entries       = 0;

/*
 * This tells OS21 the base addresses of the ILC.
 * This should be NULL if an ILC is not present.
 */
void * bsp_ilc_base_address = NULL;

/*
 * This tells OS21 of any specific interrupt subsystem initialization
 * requirements.
 */
interrupt_init_flags_t bsp_interrupt_init_flags = 0;
