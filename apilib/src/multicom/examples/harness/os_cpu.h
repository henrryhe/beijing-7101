/*
 * os_cpu.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Examine the build system and setup CPU_FOR_RPC and OS_FOR_RPC
 */

#ifndef OS_CPU_H
#define OS_CPU_H

#if defined(__OS20__)
#define OS_FOR_RPC OS_OS20
#elif defined(__OS21__)
#define OS_FOR_RPC OS_OS21
#elif defined(__LINUX__)
#define OS_FOR_RPC OS_LINUX_USER
#else
#error Unknown operating system
#endif

#if defined(__ST20__)
#define CPU_FOR_RPC CPU_ST20
#elif defined(__SH4__)
#define CPU_FOR_RPC CPU_ST40
#elif defined(__ST200__)
#define CPU_FOR_RPC CPU_ST200
#else
#error Unknown CPU
#endif

#endif /* OS_CPU_H */
