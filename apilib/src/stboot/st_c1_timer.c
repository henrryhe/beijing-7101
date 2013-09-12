/*
 * st_c1_timer.c
 *
 * Copyright STMicroelectronics Ltd. 2004
 *
 * ST20C1 timer plugin
 */
      
#if defined(PROCESSOR_C1)

#include <interrup.h>
#include <c1timer.h> /* OS20 interface */
#include "st_c1_timer.h"

#if defined(ST_5105)
#define _ST_C1_TIMER_BASE          0x30003000
#define _ST_C1_TIMER_INT_LEVEL_NO  1
#define _ST_C1_TIMER_INT_NO        43
#define _ST_C1_TIMER_PRESCALE      0x1F /*counter prescaler*/
#define _ST_ILC_TYPE               3
#define _ST_PWM_TYPE               4
#endif

#if defined(ST_7710)
#define _ST_C1_TIMER_BASE          0x30003000
#ifdef WA_GNBvd44290
#define _ST_C1_TIMER_INT_LEVEL_NO  14
#else
#define _ST_C1_TIMER_INT_LEVEL_NO  1
#endif
#define _ST_C1_TIMER_INT_NO        16
#define _ST_C1_TIMER_PRESCALE      0x1F /*counter prescaler*/
#define _ST_ILC_TYPE               3
#define _ST_PWM_TYPE               4
#endif

#if defined(ST_5700)
#define _ST_C1_TIMER_BASE          0x10003000
#define _ST_C1_TIMER_INT_LEVEL_NO  1
#define _ST_C1_TIMER_INT_NO        64
#define _ST_C1_TIMER_PRESCALE      0x1F /*counter prescaler*/
#define _ST_ILC_TYPE               3
#define _ST_PWM_TYPE               4
#endif

#if defined(ST_5188)
#define _ST_C1_TIMER_BASE          0x30003000
#define _ST_C1_TIMER_INT_LEVEL_NO  1
#define _ST_C1_TIMER_INT_NO        43
#define _ST_C1_TIMER_PRESCALE      0x1F /*counter prescaler*/
#define _ST_ILC_TYPE               3
#define _ST_PWM_TYPE               4
#endif

#if defined(ST_5107)
#define _ST_C1_TIMER_BASE          0x30003000
#define _ST_C1_TIMER_INT_LEVEL_NO  1
#define _ST_C1_TIMER_INT_NO        43
#define _ST_C1_TIMER_PRESCALE      0x1F /*counter prescaler*/
#define _ST_ILC_TYPE               3
#define _ST_PWM_TYPE               4
#endif

/*Timeslice Tick formula: sysclk/2/256/(prescale+1) ticks per second*/
#if ! defined(_ST_C1_TIMER_TIMESLICE_TICK_PRESCALE)
  #define _ST_C1_TIMER_TIMESLICE_TICK_PRESCALE 11 /*timeslice tick prescaler*/
#endif

static int timer_installed = 0;

static int timer_interrupt_raised = 0;

static c1_timer_reg_t *c1_timer = (c1_timer_reg_t *) _ST_C1_TIMER_BASE;


#ifdef WA_GNBvd44290
static timer_api_t c1_timer_api;

static void c1_timer_raise_int(void);
static void c1_timer_enable_int(void);
static void c1_timer_disable_int(void);
static void c1_timer_set(int value);
static int c1_timer_read(void);
static void c1_timer_interrupt(void);
int c1_timer_initialize(void); /* no placement called only once */

typedef struct stboot_WA_GNBvd44290_s
{
    void (*CallBack)(void *);
    void * HDMI_Data_p;
}stboot_WA_GNBvd44290_t;
static stboot_WA_GNBvd44290_t stboot_WA_GNBvd44290 = {0};
void stboot_WA_GNBvd44290_InstallCallback(void (*CallBack)(void *), void * Data_p);
void stboot_WA_GNBvd44290_UnInstallCallback(void);

void stboot_WA_GNBvd44290_InstallCallback(void (*CallBack)(void *), void * Data_p)
{
    stboot_WA_GNBvd44290.CallBack = CallBack;
    stboot_WA_GNBvd44290.HDMI_Data_p = Data_p;
}
void stboot_WA_GNBvd44290_UnInstallCallback(void)
{
    stboot_WA_GNBvd44290.CallBack = 0;
}

/*int TimerOccurence[6] = {0};
#pragma ST_section( TimerOccurence, "c1_timer_data")
  #define COUNT_TIMER(a) TimerOccurence[a]++*/
#define COUNT_TIMER(a)

#endif /* WA_GNBvd44290 */



static void c1_timer_raise_int(void)
{
  #ifdef WA_GNBvd44290
  COUNT_TIMER(0);
  #endif
  timer_interrupt_raised = 1;
  interrupt_raise_number(_ST_C1_TIMER_INT_NO);
}

static void c1_timer_enable_int(void)
{
  #ifdef WA_GNBvd44290
  COUNT_TIMER(1);
  #endif
  if (0 == c1_timer->pwm_4.IntEnable.CompareInt0)
  {
  c1_timer->pwm_4.IntAck.CompareInt0    = ~0;
  }
  c1_timer->pwm_4.IntEnable.CompareInt0 = 1;
}

static void c1_timer_disable_int(void)
{
  #ifdef WA_GNBvd44290
  COUNT_TIMER(2);
  #endif
  c1_timer->pwm_4.IntEnable.CompareInt0 = 0;
}

static void c1_timer_set(int value)
{
  #ifdef WA_GNBvd44290
  COUNT_TIMER(3);
  #endif
  c1_timer->pwm_4.CompareVal0 = value;
}

static int c1_timer_read(void)
{
  #ifdef WA_GNBvd44290
  COUNT_TIMER(4);
  #endif
  return (c1_timer->pwm_4.CaptureCount);
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
    #ifdef WA_GNBvd44290
    /*COUNT_TIMER(5);*/
    #define PWM_INT_BIT_CMP0 0x20
    #define PWM_INT_BIT_CMP1 0x40
    int IntStatus, IntEnable, IntAck;

    IntStatus = *((volatile int*) 0x30003058);
    IntEnable = *((volatile int*) 0x30003054);
    IntAck = *((volatile int*) 0x3000305C);

    if ((IntStatus & PWM_INT_BIT_CMP0) || timer_interrupt_raised)
    {
        IntStatus &= ~(int)PWM_INT_BIT_CMP0;
        timer_interrupt_raised = 0;
        *((volatile int*) 0x3000305C) = PWM_INT_BIT_CMP0;
        timer_interrupt();
    }

    if (IntStatus & PWM_INT_BIT_CMP1)
    {
        IntStatus &= ~(int)PWM_INT_BIT_CMP1;
        *((volatile int*) 0x3000305C) = PWM_INT_BIT_CMP1;
        if(stboot_WA_GNBvd44290.CallBack != 0)
            stboot_WA_GNBvd44290.CallBack(stboot_WA_GNBvd44290.HDMI_Data_p);
    }


    #else /* WA_GNBvd44290 */
    while (((1 == c1_timer->pwm_4.IntStatus.CompareInt0) &&
          (1 == c1_timer->pwm_4.IntEnable.CompareInt0)) || timer_interrupt_raised)
    {
    	timer_interrupt_raised = 0;
    	c1_timer->pwm_4.IntAck.CompareInt0 = 1;	/*acknowledge capture counter int */
    	timer_interrupt();
    }
    #endif /* WA_GNBvd44290 */
}

int c1_timer_initialize(void);

int c1_timer_initialize(void)
{
  if (timer_installed)      /* this function should *ONLY* be called once */
    return (-1);
  timer_installed = 1;


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
    
  /* Attach the interrupt handler */


  if (interrupt_install(_ST_C1_TIMER_INT_NO, _ST_C1_TIMER_INT_LEVEL_NO,
                        (void (*)(void *)) c1_timer_interrupt,
                        (void *) 0) == -1)
    return (-1);
  
  interrupt_enable(_ST_C1_TIMER_INT_LEVEL_NO);
  
  interrupt_enable_number(_ST_C1_TIMER_INT_NO);
  
  /* initialize generic timer in OS/20 */
  timer_initialize(&c1_timer_api);
  
  return (0);			/* time installed */
}

#endif

/*#endif*/

