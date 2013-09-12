
#ifndef _UTIL_H
#define _UTIL_H

#include "blt_init.h"

void *gam_emul_malloc(stblit_Device_t* Device_p, int size);
void gam_emul_free(stblit_Device_t* Device_p, void *ptr);


#endif /* _UTIL_H */
