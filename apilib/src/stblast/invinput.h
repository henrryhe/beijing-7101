/*****************************************************************************
File Name   : invinput.h

Description : Prototype for InvertedInputCompensate routine.
              Provides a software inversion of received symbol stream
Copyright (C) 2002 STMicroelectronics

History     : Created 2002 (MV)

Reference   :

*****************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __INVINPUT_H
#define __INVINPUT_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stblast.h"

BOOL InvertedInputCompensate(STBLAST_Symbol_t *Buf,
                                const U32 ExpectedStartSymbolMark,
                                const U32 SymbolsAvailable);

#endif
