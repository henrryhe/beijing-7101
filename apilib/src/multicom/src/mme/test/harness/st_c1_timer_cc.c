/*
 * st_c1_timer_cc.c
 *
 * Copyright STMicroelectronics Ltd. 2004
 *
 * ST20C1 timer plugin for ST20DC1, ST20MC2, STm5700.
 */
      
#ifdef __OS20__ 

#include <interrup.h>
#include <c1timer.h> /* OS20 interface */

#include "st_c1_timer_cc.h"

#if __CORE__ == 1

/*
 * The following #error raising definitions should be defined in the build procedure.
 * See examples in ST20ROOT/board.
 */
#if ! defined(_ST_C1_TIMER_BASE)
  #error _ST_C1_TIMER_BASE must be defined.
#endif 
#if ! defined(_ST_C1_TIMER_INT_LEVEL_NO)
  #error _ST_C1_TIMER_INT_LEVEL_NO must be defined.
#endif 
#if ! defined(_ST_C1_TIMER_INT_NO)
  #error _ST_C1_TIMER_INT_NO must be defined.
#endif 
#if ! defined(_ST_C1_TIMER_PRESCALE)
  #error _ST_C1_TIMER_PRESCALE must be defined.
#endif
#if ! defined(_ST_CHIP_NAME)
  #error _ST_CHIP_NAME must be defined.
#endif
#if ! defined(_ST_ILC_TYPE)
  #error _ST_ILC_TYPE must be defined.
#endif

#if ! defined(_ST_C1_TIMER_TIMESLICE_TICK_PRESCALE)
  #define _ST_C1_TIMER_TIMESLICE_TICK_PRESCALE 0
#endif

#define _ST_C1_TIMER_STACKSIZE 1024

static int timer_installed = 0;

static int timer_interrupt_raised = 0;

static char timer_stack[_ST_C1_TIMER_STACKSIZE];

static c1_timer_reg_t *c1_timer = (c1_timer_reg_t *) _ST_C1_TIMER_BASE;

static void c1_timer_raise_int(void)
{
  timer_interrupt_raised = 1;
  interrupt_raise_number(_ST_C1_TIMER_INT_NO);
}

static void c1_timer_enable_int(void)
{
#if _ST_PWM_TYPE == 3
  /* ST20MC2 == PWM 3 */
  c1_timer->pwm_3.IntEnable.CompareInt0 = 1;
#else
  /* ST20DC1 || STm5700 == PWM 4 */
  c1_timer->pwm_4.IntEnable.CompareInt0 = 1;
#endif
}

static void c1_timer_disable_int(void)
{
#if _ST_PWM_TYPE == 3
  /* ST20MC2 == PWM 3 */
  c1_timer->pwm_3.IntEnable.CompareInt0 = 0;
#else
  /* ST20DC1 || STm5700 == PWM 4 */
  c1_timer->pwm_4.IntEnable.CompareInt0 = 0;
#endif
}

static void c1_timer_set(int value)
{
#if _ST_PWM_TYPE == 3
  /* ST20MC2 == PWM 3 */
  c1_timer->pwm_3.CompareVal0 = value;
#else
  /* ST20DC1 || STm5700 == PWM 4 */
  c1_timer->pwm_4.CompareVal0 = value;
#endif
}

static int c1_timer_read(void)
{
#if _ST_PWM_TYPE == 3
  /* ST20MC2 == PWM 3 */
  return (c1_timer->pwm_3.CaptureCount);
#else
  /* ST20DC1 || STm5700 == PWM 4 */
  return (c1_timer->pwm_4.CaptureCount);
#endif
}

static timer_api_t c1_timer_api = {
  c1_timer_read,
  c1_timer_set,
  c1_timer_enable_int,
  c1_timer_disable_int,
  c1_timer_raise_int
};

static void c1_timer_interrupt(void)
{
#if _ST_PWM_TYPE == 3
  /* ST20MC2 == PWM 3 */
  while (((1 == c1_timer->pwm_3.IntStatus.CompareInt0) &&
          (1 == c1_timer->pwm_3.IntEnable.CompareInt0)) || timer_interrupt_raised) {
    timer_interrupt_raised = 0;
    c1_timer->pwm_3.IntAck.CompareInt0 = 1; /*acknowledge capture counter int */
    timer_interrupt();
  }
#else
  /* ST20DC1 || STm5700 == PWM 4 */    
  while (((1 == c1_timer->pwm_4.IntStatus.CompareInt0) &&
          (1 == c1_timer->pwm_4.IntEnable.CompareInt0)) || timer_interrupt_raised) {
    timer_interrupt_raised = 0;
    c1_timer->pwm_4.IntAck.CompareInt0 = 1;	/*acknowledge capture counter int */
    timer_interrupt();
  }
#endif
}

int c1_timer_initialize(void)
{
  if (timer_installed)      /* this function should *ONLY* be called once */
    return (-1);
  timer_installed = 1;

#if _ST_PWM_TYPE == 3
  /* ST20MC2 == PWM 3 */
      
  /*
   * C1 timeslice tick hardwired. No config required.
   */
   
  /*
   * Set up Capture counter hardware
   */
  c1_timer->pwm_3.CaptureCount = 0;
  
  c1_timer->pwm_3.IntEnable.CompareInt0 = 0;
  c1_timer->pwm_3.IntAck.CompareInt0 = ~0;
  
  c1_timer->pwm_3.Control.CaptureClkValue = _ST_C1_TIMER_PRESCALE;
  c1_timer->pwm_3.Control.CaptureEnable = 1;
#else
  /* ST20DC1 || STm5700 == PWM 4 */
    
  /*
   * Enable the C1 timeslice tick
   */
  c1_timer->pwm_4.Control.PWMClkValue = _ST_C1_TIMER_TIMESLICE_TICK_PRESCALE;
  c1_timer->pwm_4.Control.PWMEnable = 1;
      
  /*
   * Set up Capture counter hardware
   */
  c1_timer->pwm_4.CaptureCount = 0;
      
  c1_timer->pwm_4.IntEnable.CompareInt0 = 0;
  c1_timer->pwm_4.IntAck.CompareInt0 = ~0;
      
  c1_timer->pwm_4.Control.CaptureClkValue = _ST_C1_TIMER_PRESCALE;
  c1_timer->pwm_4.Control.CaptureEnable = 1;
#endif
    
  /*
   * Attach the interrupt handler
   */
  if (interrupt_init(_ST_C1_TIMER_INT_LEVEL_NO, timer_stack, sizeof(timer_stack),
                     interrupt_trigger_mode_rising, 0) == -1)
    return (-1);
  
  if (interrupt_install(_ST_C1_TIMER_INT_NO, _ST_C1_TIMER_INT_LEVEL_NO,
                        (void (*)(void *)) c1_timer_interrupt,
                        (void *) 0) == -1)
    return (-1);
  
  interrupt_enable_global();
  
#if _ST_ILC_TYPE == 1
  /* ST20DC1 || ST20MC2 == ILC1 */
  interrupt_enable(_ST_C1_TIMER_INT_LEVEL_NO);
#else
  /* STm5700 == ILC3 */
  interrupt_enable_number(_ST_C1_TIMER_INT_NO);
#endif
  
  /* initialize generic timer in OS/20 */
  timer_initialize(&c1_timer_api);
  
  return (0);			/* time installed */
}

#endif

#else /* __OS20__ */

extern void warning_suppression(void);

#endif /* __OS20__ */
