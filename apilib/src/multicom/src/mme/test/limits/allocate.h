#ifndef ALLOCATE_H
#define ALLOCATE_H

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#if defined __LINUX__ && !defined __KERNEL__

int   KernelAllocateInit();
int   KernelAllocateDeinit();
void* KernelAllocate(unsigned size, unsigned cached, unsigned* mapSize);
int   KernelFree(void* address, unsigned length);

#else

#define KernelAllocateInit()       1
#define KernelAllocateDeinit()     1

#define KernelAllocate(_X, _Y, _Z) (malloc(_X))
#define KernelFree(_X, _Y)         (free(_X))

#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif
