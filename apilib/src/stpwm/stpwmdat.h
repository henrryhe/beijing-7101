/*****************************************************************************
File Name   : stpwmdat.h

Description : Internal Constants for the PWM driver.

Copyright (C) 2005 STMicroelectronics

Reference   :

$ClearCase (VOB: stpwm)

ST API Definition "PWM (Driver) API" DVD-API-10 Revision 1.5
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STPWMDAT_H
#define __STPWMDAT_H

/* Includes --------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* 32-bit Hardware Register offsets
   from STPWM_CONTROLLER_BASE */

#define STPWM_VAL_0                 0
#define STPWM_VAL_1                 1

#if !defined (ST_5100)
#define STPWM_VAL_2                 2
#define STPWM_VAL_3                 3
#endif

#define STPWM_CONTROL              20

#if defined (ST_5100)
#define STPWM_CAPTURE0_VAL          0
#define STPWM_CAPTURE1_VAL          0
#define STPWM_COMPARE0_VAL          0
#define STPWM_COMPARE1_VAL          0
#define STPWM_CAPTURE0_EDGE         0
#define STPWM_CAPTURE1_EDGE         0
#define STPWM_COMPARE0_OUT_VAL      0
#define STPWM_COMPARE1_OUT_VAL      0
#define STPWM_INT_ENAB              0
#define STPWM_INT_STAT              0
#define STPWM_INT_ACKN              0
#define STPWM_CMP_CPT_CNT           0

#else

#define STPWM_CAPTURE0_VAL          4
#define STPWM_CAPTURE1_VAL          5
#define STPWM_COMPARE0_VAL          8
#define STPWM_COMPARE1_VAL          9
#define STPWM_CAPTURE0_EDGE         12
#define STPWM_CAPTURE1_EDGE         13
#define STPWM_COMPARE0_OUT_VAL      16
#define STPWM_COMPARE1_OUT_VAL      17

#define STPWM_INT_ENAB              21
#define STPWM_INT_STAT              22
#define STPWM_INT_ACKN              23

#define STPWM_CMP_CPT_CNT           25

#endif

/* Hardware Access Field Values */

#define STPWM_DISABLE_VALUE         0
#define STPWM_INT_ACKN_CLEAR_ALL    0xFFFFFFFF
#define STPWM_PWM_INT               0x00000001    /* b0 only */
#define STPWM_CAPTURE0_INT          0x00000002    /* b1*/
#define STPWM_CAPTURE1_INT          0x00000004    /* b2*/
#define STPWM_COMPARE0_INT          0x00000020    /* b5*/
#define STPWM_COMPARE1_INT          0x00000040    /* b6*/
#define STPWM_CAPTURE_ENAB          0x00000400    /* b10 of PWM_CTRL*/

#if defined (ST_7710)

#define STPWM_PWM_ENAB          0x00000010    /* b4 only */
#else
#define STPWM_PWM_ENAB          0x00000200    /* b9 only */
#endif

#define STPWM_COUNT_RETAIN      0x000005F0    /* b10, 8..4 Counter bits */


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STPWMDAT_H */

/* End of stpwmdat.h */

