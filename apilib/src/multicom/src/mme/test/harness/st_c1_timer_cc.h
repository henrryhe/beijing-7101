/*
 * st_c1_timer_cc.h
 *
 * Copyright STMicroelectronics Ltd. 2004
 *
 * ST20C1 timer plugin for ST20DC1, ST20MC2, STm5700.
 */
 
#if ! defined(__ST_C1_TIMER_H)

#define __ST_C1_TIMER_H

#include "st_pwm.h"

#if defined MB385
#define _ST_C1_TIMER_BASE 0x10003000
#define _ST_C1_TIMER_INT_LEVEL_NO 0
#define _ST_C1_TIMER_INT_NO 64
#define _ST_C1_TIMER_PRESCALE 0x1f
#define _ST_CHIP_NAME "STm5700"
#define _ST_ILC_TYPE 3
#define _ST_PWM_TYPE 4
#endif

/*
 * Union of PWM register definitions.
 */
typedef union c1_timer_reg_u {
  pwm_3_bf_t pwm_3;
  pwm_4_bf_t pwm_4;
} c1_timer_reg_t;

#endif

