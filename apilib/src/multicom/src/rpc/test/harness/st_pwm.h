/*
 * st_pwm.h
 *
 * Copyright STMicroelectronics Ltd. 2004
 *
 */

 
#if ! defined(__ST_PWM_H)

#define __ST_PWM_H

/*
 * PWM register definitions.
 *
 * Two PWM module types are supported. They are named pwm_3 and pwm_4.
 * The postfixes 3 and 4 denote the number of capture/ compare units
 * contained in the pwm.
 *
 * Registers defined as 8 or 16 bits wide all occupy a 4 byte word
 * aligned field, so 32 bit ints can be used to map onto thes fields.
 * 
 */

/*
 * Useful bit twiddling macros
 */
#define _ST_SHIFTLEFT <<
#define _ST_SHIFTRIGHT >> 
#define _ST_BITMASK(X) ( 1 _ST_SHIFTLEFT X)
#define _ST_BITSMASK(L,U) (((( 1 _ST_SHIFTLEFT (U - L)) - 1) _ST_SHIFTLEFT L) | ( 1 _ST_SHIFTLEFT U))

/*
 * Bit twiddling fields for Control register
 */
#define _ST_PWM_3_4_PWM_CLK_VALUE (_ST_BITSMASK(0,3))
#define _ST_PWM_3_4_CAPTURE_CLK_VALUE (_ST_BITSMASK(4,8))
#define _ST_PWM_3_4_PWM_ENABLE (_ST_BITMASK(9))
#define _ST_PWM_3_4_CAPTURE_ENABLE (_ST_BITMASK(10))

/*
 * Bit twiddling fields for Capture X Edge Control registers
 */
#define _ST_PWM_3_4_CAPTURE_X_EDGE (_ST_BITSMASK(0,1))

/*
 * Bit twiddling fields for Compare X Output Value registers
 */
#define _ST_PWM_3_4_COMPARE_X_OUT_VAL (_ST_BITMASK(0))

/*
 * PWM 3 & 4 control register bit field definition
 */
typedef struct pwm_3_4_control_s {
    volatile int PWMClkValue:4;
    volatile int CaptureClkValue:5;
    volatile int PWMEnable:1;
    volatile int CaptureEnable:1;
} pwm_3_4_control_t;

/*
 * PWM 3 Interrupts bit field definition.
 * Register layout is the same for interrupt enable, status and acknowledge registers.
 */
typedef struct pwm_3_interrupt_s {
    volatile int PWMInt:1;	/* 0  */
    volatile int Reserved:2;	/* 1,2  */
    volatile int CaptureInt0:1;	/* 3  */
    volatile int CaptureInt1:1;	/* 4  */
    volatile int CaptureInt2:1;	/* 5  */
    volatile int CompareInt0:1;	/* 6  */
    volatile int CompareInt1:1;	/* 7  */
    volatile int CompareInt2:1;	/* 8  */
} pwm_3_interrupt_t;

/*
 * PWM 3 register set definition.
 * Registers with bit fields use C bit field definitions.
 */
typedef struct pwm_3_bf_s 
{
  volatile int PWMVal0;         /* 00 */
  volatile int PWMVal1;         /* 04 */
  volatile int PWMVal2;         /* 08 */

  volatile int CaptureVal0;     /* 0c */
  volatile int CaptureVal1;     /* 10 */
  volatile int CaptureVal2;     /* 14 */

  volatile int CompareVal0;     /* 18 */
  volatile int CompareVal1;     /* 1c */
  volatile int CompareVal2;     /* 20 */

  pwm_3_4_control_t Control;    /* 24 */

  volatile int CaptureEdge0;    /* 28 */
  volatile int CaptureEdge1;    /* 2c */
  volatile int CaptureEdge2;    /* 30 */

  volatile int CompareOutVal0;  /* 34 */
  volatile int CompareOutVal1;  /* 38 */
  volatile int CompareOutVal2;  /* 3c */

  pwm_3_interrupt_t IntEnable;  /* 40 */
  pwm_3_interrupt_t IntStatus;  /* 44 */
  pwm_3_interrupt_t IntAck;     /* 48 */

  volatile int PWMCount;        /* 4c */
  volatile int CaptureCount;    /* 50 */
} pwm_3_bf_t;

/*
 * PWM 3 register set definition.
 * Registers with bit fields defined as ints. Bit twiddling required.
 */

/*
 * Bit twiddling fields.
 * The following interrupt register bit field definitions apply to registers...
 * Interrupt Enable register,
 * Interrupt Status register and
 * Interrupt Acknowledge register.
 */
#define _ST_PWM_3_PWM_INT (_ST_BITMASK(0))
#define _ST_PWM_3_CAPTURE_0_INT (_ST_BITMASK(3))
#define _ST_PWM_3_CAPTURE_1_INT (_ST_BITMASK(4))
#define _ST_PWM_3_CAPTURE_2_INT (_ST_BITMASK(5))
#define _ST_PWM_3_COMPARE_0_INT (_ST_BITMASK(6))
#define _ST_PWM_3_COMPARE_1_INT (_ST_BITMASK(7))
#define _ST_PWM_3_COMPARE_2_INT (_ST_BITMASK(8))
 
typedef struct pwm_3_ints_s 
{
  volatile int PWMVal0;         /* 00 */
  volatile int PWMVal1;         /* 04 */
  volatile int PWMVal2;         /* 08 */

  volatile int CaptureVal0;     /* 0c */
  volatile int CaptureVal1;     /* 10 */
  volatile int CaptureVal2;     /* 14 */

  volatile int CompareVal0;     /* 18 */
  volatile int CompareVal1;     /* 1c */
  volatile int CompareVal2;     /* 20 */

  volatile int Control;         /* 24 */

  volatile int CaptureEdge0;    /* 28 */
  volatile int CaptureEdge1;    /* 2c */
  volatile int CaptureEdge2;    /* 30 */

  volatile int CompareOutVal0;  /* 34 */
  volatile int CompareOutVal1;  /* 38 */
  volatile int CompareOutVal2;  /* 3c */

  volatile int IntEnable;       /* 40 */
  volatile int IntStatus;       /* 44 */
  volatile int IntAck;          /* 48 */

  volatile int PWMCount;        /* 4c */
  volatile int CaptureCount;    /* 50 */
} pwm_3_ints_t;

/*
 * PWM 4 Interrupts bit field definition.
 * Register layout is the same for interrupt enable, status and acknowledge registers.
 */ 
typedef struct pwm_4_interrupt_s {
    volatile int PWMInt:1;	/* 0  */
    volatile int CaptureInt0:1;	/* 1  */
    volatile int CaptureInt1:1;	/* 2  */
    volatile int CaptureInt2:1;	/* 3  */
    volatile int CaptureInt3:1;	/* 4  */
    volatile int CompareInt0:1;	/* 5  */
    volatile int CompareInt1:1;	/* 6  */
    volatile int CompareInt2:1;	/* 7  */
    volatile int CompareInt3:1;	/* 8  */
} pwm_4_interrupt_t;

/*
 * PWM 4 register set definition.
 * Registers with bit fields use C bit field definitions.
 */
typedef struct pwm_4_bf_s {
    volatile int PWMVal0;
    volatile int PWMVal1;	/* 04 */
    volatile int PWMVal2;
    volatile int PWMVal3;	/* 0c */

    volatile int CaptureVal0;	/* 10 */
    volatile int CaptureVal1;	/* 14 */
    volatile int CaptureVal2;	/* 18 */
    volatile int CaptureVal3;	/* 1C */

    volatile int CompareVal0;	/* 20 */
    volatile int CompareVal1;	/* 24 */
    volatile int CompareVal2;	/* 28 */
    volatile int CompareVal3;	/* 2C */

    volatile int CaptureEdge0;	/* 30 */
    volatile int CaptureEdge1;	/* 34 */
    volatile int CaptureEdge2;	/* 38 */
    volatile int CaptureEdge3;	/* 3C */

    volatile int CompareOutVal0;/* 40 */
    volatile int CompareOutVal1;/* 44 */
    volatile int CompareOutVal2;/* 48 */
    volatile int CompareOutVal3;/* 4C */

    pwm_3_4_control_t Control;	/* 50 */

    pwm_4_interrupt_t IntEnable;/* 54 */
    pwm_4_interrupt_t IntStatus;/* 58 */
    pwm_4_interrupt_t IntAck;   /* 5C */

    volatile int PWMCount;	/* 60 */
    volatile int CaptureCount;	/* 64 */
} pwm_4_bf_t;

/*
 * PWM 4 register set definition.
 * Registers with bit fields defined as ints. Bit twiddling required.
 */

/*
 * Bit twiddling fields.
 * The following interrupt register bit field definitions apply to registers...
 * Interrupt Enable register,
 * Interrupt Status register and
 * Interrupt Acknowledge register.
 */
#define _ST_PWM_4_PWM_INT (_ST_BITMASK(0))
#define _ST_PWM_4_CAPTURE_0_INT (_ST_BITMASK(1))
#define _ST_PWM_4_CAPTURE_1_INT (_ST_BITMASK(2))
#define _ST_PWM_4_CAPTURE_2_INT (_ST_BITMASK(3))
#define _ST_PWM_4_CAPTURE_3_INT (_ST_BITMASK(4))
#define _ST_PWM_4_COMPARE_0_INT (_ST_BITMASK(5))
#define _ST_PWM_4_COMPARE_1_INT (_ST_BITMASK(6))
#define _ST_PWM_4_COMPARE_2_INT (_ST_BITMASK(7))
#define _ST_PWM_4_COMPARE_3_INT (_ST_BITMASK(8))

typedef struct pwm_4_ints_s {
    volatile int PWMVal0;
    volatile int PWMVal1;	/* 04 */
    volatile int PWMVal2;
    volatile int PWMVal3;	/* 0c */

    volatile int CaptureVal0;	/* 10 */
    volatile int CaptureVal1;	/* 14 */
    volatile int CaptureVal2;	/* 18 */
    volatile int CaptureVal3;	/* 1C */

    volatile int CompareVal0;	/* 20 */
    volatile int CompareVal1;	/* 24 */
    volatile int CompareVal2;	/* 28 */
    volatile int CompareVal3;	/* 2C */

    volatile int CaptureEdge0;	/* 30 */
    volatile int CaptureEdge1;	/* 34 */
    volatile int CaptureEdge2;	/* 38 */
    volatile int CaptureEdge3;	/* 3C */

    volatile int CompareOutVal0;/* 40 */
    volatile int CompareOutVal1;/* 44 */
    volatile int CompareOutVal2;/* 48 */
    volatile int CompareOutVal3;/* 4C */

    volatile int Control;	/* 50 */

    volatile int IntEnable;	/* 54 */
    volatile int IntStatus;	/* 58 */
    volatile int IntAck;	/* 5C */

    volatile int PWMCount;	/* 60 */
    volatile int CaptureCount;	/* 64 */
} pwm_4_ints_t;

#endif /* ! defined(__ST_PWM_H) */
