//
// FILE:      OSSTFCallingConventions.h
// AUTHOR:    Stephan Bergmann
// COPYRIGHT: (c) 2002 STMicroelectronics.  All Rights Reserved.
// CREATED:   27.01.2003
//
// PURPOSE:   
//
// HISTORY:
//

// Includes

// OS : OS20/OS21

#ifndef OSSTFCALLINGCONVENTIONS_INC
#define OSSTFCALLINGCONVENTIONS_INC

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
 
#endif // OSSTFCALLINGCONVENTIONS_INC
