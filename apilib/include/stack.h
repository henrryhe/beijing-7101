/************************************************************************
File Name   : stack.h

Description : Task stack sizes.

Copyright (C) 1999 STMicroelectronics

Reference   :

************************************************************************/

#ifndef __STACK_H
#define __STACK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * interrupt handler stacks
 */
#define LEVEL_0_STACKSIZE                   512
#define LEVEL_1_STACKSIZE                   512
#define LEVEL_2_STACKSIZE                   512
#define LEVEL_3_STACKSIZE                   1024
#define LEVEL_4_STACKSIZE                   1024
#define LEVEL_5_STACKSIZE                   512
#define LEVEL_6_STACKSIZE                   512
#define LEVEL_7_STACKSIZE                   512

/*
 * stack debug
 */
#define INITIALISE_STACK(_base_, _size_)    { \
    unsigned int _i_; for (_i_=0;_i_<_size_ / sizeof(int);_i_++) \
        ((int*)_base_)[_i_] = 0xaaaaaaaa;\
}

#ifdef __cplusplus
}
#endif

#endif /* __STACK_H */

/* EOF */





