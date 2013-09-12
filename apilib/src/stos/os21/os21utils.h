#ifndef __H_OS21UTILS
#define __H_OS21UTILS

#include "stos.h"

/* ---------- Parameters casting  -------- */
#define CAST_CONST_PARAM_TO_DATA_TYPE(DataType, Data)        ((DataType)(*(DataType*)((void*)&(Data))))

#endif  /* __H_OS21UTILS */

/* End of file */

