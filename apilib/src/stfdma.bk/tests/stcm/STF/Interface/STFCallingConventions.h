#ifndef STFCALLINGCONVENTIONS_INC
#define STFCALLINGCONVENTIONS_INC

#include "OSSTFCallingConventions.h"


//! Library call means that this function can be called from outside libary
#ifndef LIBRARYCALL
#define LIBRARYCALL
#endif

//! OS specific calling conventions
#ifndef OSSPECIFICCALL
#define OSSPECIFICCALL
#endif

//! C Calling conventions
#ifndef CLANGUAGECALL
#define CLANGUAGECALL
#endif

//! Pascal calling conventions
#ifndef PASCALLANGUAGECALL
#define PASCALLANGUAGECALL
#endif

#endif // STFCALLINGCONVENTIONS_INC
