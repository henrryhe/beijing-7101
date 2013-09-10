/*******************************************************************************

File name : stsys.h

Description : Public Header of system module API

COPYRIGHT (C) STMicroelectronics 2004

Date                Modification                                        Name
----                ------------                                        ----
24 Nov 1998         Created                                             PhL
10 Mar 1999         Formated and renamed                                HG
                    Added 24 bit functions                              HG
 2 Sep 1999         Implemented as macros                               HG
12 Jul 2000         Added STSYS_GetRevision()                           HG
18 Apr 2001         Add support for OS40                                HSdLM
18 Sep 2001         Add compilation flags for access optimization       HSdLM
25 May 2004         Add support for ST20-C1 through compilation flag    HG
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STSYS_H
#define __STSYS_H

#if !defined ST_OSLINUX || !defined MODULE
/* Under Linux and kernel space, the STSYS functions are defined into STOS vob */

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#if defined(ST200_OS21)
#include <os21/st200/mmap.h>
#endif /* ST200_OS21 */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

#if defined(ARCHITECTURE_ST20) && ! defined(STSYS_NO_PRAGMA)
#pragma ST_device(STSYS_DU32)
#pragma ST_device(STSYS_DU16)
#pragma ST_device(STSYS_DU8)
#endif /*ifdef ARCHITECTURE_ST20*/

typedef volatile U32 STSYS_DU32;
typedef volatile U16 STSYS_DU16;
typedef volatile U8  STSYS_DU8;


typedef volatile U32 STSYS_MU32;
typedef volatile U16 STSYS_MU16;
typedef volatile U8  STSYS_MU8;


/* Exported Constants ------------------------------------------------------- */
#define ST40_UNCACHED_ACCESS_MASK   0xA0000000
#define ST40_PHYSICAL_ACCESS_MASK   0x1FFFFFFF

/* Exported Variables ------------------------------------------------------- */
#if defined(ST200_OS21)
/* Variable needed to remember the offset to add to a physical address
 * to have a CPU uncached address to the memory. This is more efficient
 * than calling mmap_translate_uncached every time we make an uncached
 * access */
extern S32 STSYS_ST200UncachedOffset;
#endif /* defined(ST200_OS21) */

/* Exported Macros ---------------------------------------------------------- */

/*
--------------------------------------------------------------------------------
Get revision of STSYS API
--------------------------------------------------------------------------------
*/
#define STSYS_GetRevision() ((ST_Revision_t) "STSYS-REL_1.4.4")

/* ########################################################################### */
/*                     DEVICE ACCESS                                           */
/* ########################################################################### */

/* ------------------------------------------------------- */
/* U8 STSYS_ReadRegDev8(void *Address_p); */
/* ------------------------------------------------------- */
#define STSYS_ReadRegDev8(Address_p) (*((STSYS_DU8 *) (Address_p)))

/* ------------------------------------------------------- */
/* void STSYS_WriteRegDev8(void *Address_p, U8 Value); */
/* ------------------------------------------------------- */
#define STSYS_WriteRegDev8(Address_p, Value)                            \
    {                                                                   \
        *((STSYS_DU8 *) (Address_p)) = (U8) (Value);                    \
    }

/* ------------------------------------------------------- */
/* U16 STSYS_ReadRegDev16BE(void *Address_p); */
/* ------------------------------------------------------- */
#if (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
     (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( little endian CPU or ( big endian CPU and access not optimized ) )*/
#define STSYS_ReadRegDev16BE(Address_p) ((U16) (((*(((STSYS_DU8 *) (Address_p))    )) << 8) | \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 1))     )))
#else
/* ( big endian CPU and access optimized )*/
#define STSYS_ReadRegDev16BE(Address_p) (*((STSYS_DU16 *) (Address_p)))
#endif

/* ------------------------------------------------------- */
/* U16 STSYS_ReadRegDev16LE(void *Address_p); */
/* ------------------------------------------------------- */
#if (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
   (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( big endian CPU or ( little endian CPU and access not optimized ) )*/
#define STSYS_ReadRegDev16LE(Address_p) ((U16) (((*(((STSYS_DU8 *) (Address_p))    )) ) |     \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 1)) << 8)))
#else
/* ( little endian CPU and access optimized )*/
#define STSYS_ReadRegDev16LE(Address_p) (*((STSYS_DU16 *) (Address_p)))
#endif

/* ------------------------------------------------------- */
/* void STSYS_WriteRegDev16BE(void *Address_p, U16 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegDev16BE(Address_p, Value)                               \
    {                                                                         \
        *((STSYS_DU16 *) (Address_p)) = (U16) ((((Value) & 0xFF00) >> 8) |    \
                                               (((Value) & 0x00FF) << 8));    \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegDev16BE(Address_p, Value)                         \
    {                                                                   \
        *((STSYS_DU16 *) (Address_p)) = (U16) (Value);                  \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegDev16BE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_DU8 *) (Address_p))    ) = (U8) ((Value) >> 8);       \
        *(((STSYS_DU8 *) (Address_p)) + 1) = (U8) ((Value)     );       \
    }
#endif /* _NO_OPTIMIZATION */

/* ------------------------------------------------------- */
/* void STSYS_WriteRegDev16LE(void *Address_p, U16 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegDev16LE(Address_p, Value)                         \
    {                                                                   \
        *((STSYS_DU16 *) (Address_p)) = (U16) (Value);                  \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegDev16LE(Address_p, Value)                              \
    {                                                                        \
        *((STSYS_DU16 *) (Address_p)) = (U16) ((((Value) & 0xFF00) >> 8) |   \
                                               (((Value) & 0x00FF) << 8));   \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegDev16LE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_DU8 *) (Address_p))    ) = (U8) ((Value)     );       \
        *(((STSYS_DU8 *) (Address_p)) + 1) = (U8) ((Value) >> 8);       \
    }
#endif /* _NO_OPTIMIZATION */


/* ------------------------------------------------------- */
/* U32 STSYS_ReadRegDev24BE(void *Address_p); */
/* ------------------------------------------------------- */
#if (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
     (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( little endian CPU or ( big endian CPU and access not optimized ) )*/
#define STSYS_ReadRegDev24BE(Address_p) ((U32) (((*(((STSYS_DU8 *) (Address_p))    )) << 16) | \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 1)) <<  8) | \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 2))      )))
#else
/* ( big endian CPU and access optimized )*/
#define STSYS_ReadRegDev24BE(Address_p) ((U32) (((*(((STSYS_DU8  *) (Address_p))    )) << 16) | \
                                                ((*(((STSYS_DU16 *) (Address_p)) + 1))      )))
#endif

/* ------------------------------------------------------- */
/* U32 STSYS_ReadRegDev24LE(void *Address_p); */
/* ------------------------------------------------------- */
#if (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
   (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( big endian CPU or ( little endian CPU and access not optimized ) )*/
#define STSYS_ReadRegDev24LE(Address_p) ((U32) (((*(((STSYS_DU8 *) (Address_p))    ))      ) | \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 1)) << 8 ) | \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 2)) << 16)))
#else
/* ( little endian CPU and access optimized )*/
#define STSYS_ReadRegDev24LE(Address_p) ((U32) (((*(((STSYS_DU8  *) (Address_p)) + 2)) << 16) | \
                                                ((*(((STSYS_DU16 *) (Address_p))    ))      )))
#endif


/* ------------------------------------------------------- */
/* void STSYS_WriteRegDev24BE(void *Address_p, U32 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegDev24BE(Address_p, Value)                                      \
    {                                                                                \
        *(((STSYS_DU16 *) (Address_p))    ) = (U16) ((((Value) & 0xFF0000) >> 16) |  \
                                                     (((Value) & 0x00FF00)      ));  \
        *(((STSYS_DU8  *) (Address_p)) + 2) =  (U8)   ((Value)                   );  \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegDev24BE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_DU16 *) (Address_p))    ) = (U16) ((Value) >> 8);     \
        *(((STSYS_DU8  *) (Address_p)) + 2) =  (U8) ((Value)     );     \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegDev24BE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_DU8 *) (Address_p))    ) = (U8) ((Value) >> 16);      \
        *(((STSYS_DU8 *) (Address_p)) + 1) = (U8) ((Value) >> 8 );      \
        *(((STSYS_DU8 *) (Address_p)) + 2) = (U8) ((Value)      );      \
    }
#endif /* _NO_OPTIMIZATION */

/* ------------------------------------------------------- */
/* void STSYS_WriteRegDev24LE(void *Address_p, U32 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegDev24LE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_DU16 *) (Address_p))    ) = (U16) (Value);            \
        *(((STSYS_DU8 *)  (Address_p)) + 2) = (U8) ((Value) >> 16);     \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegDev24LE(Address_p, Value)                                    \
    {                                                                              \
        *(((STSYS_DU16 *) (Address_p))   ) = (U16) ((((Value) & 0xFF00) >> 8) |    \
                                                    (((Value) & 0x00FF) << 8));    \
        *(((STSYS_DU8 *) (Address_p)) + 2) =  (U8)  (((Value)         ) >> 16);    \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegDev24LE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_DU8 *) (Address_p))    ) = (U8) ((Value)      );      \
        *(((STSYS_DU8 *) (Address_p)) + 1) = (U8) ((Value) >> 8 );      \
        *(((STSYS_DU8 *) (Address_p)) + 2) = (U8) ((Value) >> 16);      \
    }
#endif /* _NO_OPTIMIZATION */

/* ------------------------------------------------------- */
/* U32 STSYS_ReadRegDev32BE(void *Address_p); */
/* ------------------------------------------------------- */
#if (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
     (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( little endian CPU or ( big endian CPU and access not optimized ) )*/
#define STSYS_ReadRegDev32BE(Address_p) ((U32) (((*(((STSYS_DU8 *) (Address_p))    )) << 24) | \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 1)) << 16) | \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 2)) <<  8) | \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 3))     )))
#else
/* ( big endian CPU and access optimized )*/
#define STSYS_ReadRegDev32BE(Address_p) (*((STSYS_DU32 *) (Address_p)))
#endif

/* ------------------------------------------------------- */
/* U32 STSYS_ReadRegDev32LE(void *Address_p); */
/* ------------------------------------------------------- */
#if (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
   (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( big endian CPU or ( little endian CPU and access not optimized ) )*/
#define STSYS_ReadRegDev32LE(Address_p) ((U32) (((*(((STSYS_DU8 *) (Address_p))    ))      ) |  \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 1)) << 8 ) |  \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 2)) << 16) |  \
                                                ((*(((STSYS_DU8 *) (Address_p)) + 3)) << 24)))
#else
/* ( little endian CPU and access optimized )*/
#define STSYS_ReadRegDev32LE(Address_p) (*((STSYS_DU32 *) (Address_p)))
#endif

/* ------------------------------------------------------- */
/* void STSYS_WriteRegDev32BE(void *Address_p, U32 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegDev32BE(Address_p, Value)                         \
    {                                                                   \
        *((STSYS_DU32 *) (Address_p)) = (U32) ((((Value) & 0xFF000000) >> 24) |   \
                                               (((Value) & 0x00FF0000) >> 8 ) |   \
                                               (((Value) & 0x0000FF00) << 8 ) |   \
                                               (((Value) & 0x000000FF) << 24));   \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegDev32BE(Address_p, Value)                         \
    {                                                                   \
        *((STSYS_DU32 *) (Address_p)) = (U32) (Value);                  \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegDev32BE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_DU8 *) (Address_p))    ) = (U8) ((Value) >> 24);      \
        *(((STSYS_DU8 *) (Address_p)) + 1) = (U8) ((Value) >> 16);      \
        *(((STSYS_DU8 *) (Address_p)) + 2) = (U8) ((Value) >> 8 );      \
        *(((STSYS_DU8 *) (Address_p)) + 3) = (U8) ((Value)      );      \
    }
#endif /* _NO_OPTIMIZATION */

/* ------------------------------------------------------- */
/* void STSYS_WriteRegDev32LE(void *Address_p, U32 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegDev32LE(Address_p, Value)                         \
    {                                                                   \
        *((STSYS_DU32 *) (Address_p)) = (U32) (Value);                  \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegDev32LE(Address_p, Value)                                   \
    {                                                                             \
        *((STSYS_DU32 *) (Address_p)) = (U32) ((((Value) & 0xFF000000) >> 24) |   \
                                               (((Value) & 0x00FF0000) >> 8 ) |   \
                                               (((Value) & 0x0000FF00) << 8 ) |   \
                                               (((Value) & 0x000000FF) << 24));   \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegDev32LE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_DU8 *) (Address_p))    ) = (U8) ((Value)      );      \
        *(((STSYS_DU8 *) (Address_p)) + 1) = (U8) ((Value) >> 8 );      \
        *(((STSYS_DU8 *) (Address_p)) + 2) = (U8) ((Value) >> 16);      \
        *(((STSYS_DU8 *) (Address_p)) + 3) = (U8) ((Value) >> 24);      \
    }
#endif /* _NO_OPTIMIZATION */


/* ########################################################################### */
/*                     'MEMORY' ACCESS                                         */
/* ########################################################################### */


/* ------------------------------------------------------- */
/* U8 STSYS_ReadRegMem8(void *Address_p); */
/* ------------------------------------------------------- */
#define STSYS_ReadRegMem8(Address_p) (*((STSYS_MU8 *) (Address_p)))

/* ------------------------------------------------------- */
/* void STSYS_WriteRegMem8(void *Address_p, U8 Value); */
/* ------------------------------------------------------- */
#define STSYS_WriteRegMem8(Address_p, Value)                            \
    {                                                                   \
        *((STSYS_MU8 *) (Address_p)) = (U8) (Value);                    \
    }

/* ------------------------------------------------------- */
/* U16 STSYS_ReadRegMem16BE(void *Address_p); */
/* ------------------------------------------------------- */
#if (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
     (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( little endian CPU or ( big endian CPU and access not optimized ) )*/
#define STSYS_ReadRegMem16BE(Address_p) ((U16) (((*(((STSYS_MU8 *) (Address_p))    )) << 8) | \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 1))     )))
#else
/* ( big endian CPU and access optimized )*/
#define STSYS_ReadRegMem16BE(Address_p) (*((STSYS_MU16 *) (Address_p)))
#endif

/* ------------------------------------------------------- */
/* U16 STSYS_ReadRegMem16LE(void *Address_p); */
/* ------------------------------------------------------- */
#if (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
   (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( big endian CPU or ( little endian CPU and access not optimized ) )*/
#define STSYS_ReadRegMem16LE(Address_p) ((U16) (((*(((STSYS_MU8 *) (Address_p))    )) ) |     \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 1)) << 8)))
#else
/* ( little endian CPU and access optimized )*/
#define STSYS_ReadRegMem16LE(Address_p) (*((STSYS_MU16 *) (Address_p)))
#endif

/* ------------------------------------------------------- */
/* void STSYS_WriteRegMem16BE(void *Address_p, U16 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegMem16BE(Address_p, Value)                               \
    {                                                                         \
        *((STSYS_MU16 *) (Address_p)) = (U16) ((((Value) & 0xFF00) >> 8) |    \
                                               (((Value) & 0x00FF) << 8));    \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegMem16BE(Address_p, Value)                         \
    {                                                                   \
        *((STSYS_MU16 *) (Address_p)) = (U16) (Value);                  \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegMem16BE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_MU8 *) (Address_p))    ) = (U8) ((Value) >> 8);       \
        *(((STSYS_MU8 *) (Address_p)) + 1) = (U8) ((Value)     );       \
    }
#endif /* _NO_OPTIMIZATION */

/* ------------------------------------------------------- */
/* void STSYS_WriteRegMem16LE(void *Address_p, U16 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegMem16LE(Address_p, Value)                         \
    {                                                                   \
        *((STSYS_MU16 *) (Address_p)) = (U16) (Value);                  \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegMem16LE(Address_p, Value)                              \
    {                                                                        \
        *((STSYS_MU16 *) (Address_p)) = (U16) ((((Value) & 0xFF00) >> 8) |   \
                                               (((Value) & 0x00FF) << 8));   \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegMem16LE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_MU8 *) (Address_p))    ) = (U8) ((Value)     );       \
        *(((STSYS_MU8 *) (Address_p)) + 1) = (U8) ((Value) >> 8);       \
    }
#endif /* _NO_OPTIMIZATION */


/* ------------------------------------------------------- */
/* U32 STSYS_ReadRegMem24BE(void *Address_p); */
/* ------------------------------------------------------- */
#if (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
     (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( little endian CPU or ( big endian CPU and access not optimized ) )*/
#define STSYS_ReadRegMem24BE(Address_p) ((U32) (((*(((STSYS_MU8 *) (Address_p))    )) << 16) | \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 1)) <<  8) | \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 2))      )))
#else
/* ( big endian CPU and access optimized )*/
#define STSYS_ReadRegMem24BE(Address_p) ((U32) (((*(((STSYS_MU8  *) (Address_p))    )) << 16) | \
                                                ((*(((STSYS_MU16 *) (Address_p)) + 1))      )))
#endif

/* ------------------------------------------------------- */
/* U32 STSYS_ReadRegMem24LE(void *Address_p); */
/* ------------------------------------------------------- */
#if (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
   (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( big endian CPU or ( little endian CPU and access not optimized ) )*/
#define STSYS_ReadRegMem24LE(Address_p) ((U32) (((*(((STSYS_MU8 *) (Address_p))    ))      ) | \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 1)) << 8 ) | \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 2)) << 16)))
#else
/* ( little endian CPU and access optimized )*/
#define STSYS_ReadRegMem24LE(Address_p) ((U32) (((*(((STSYS_MU8  *) (Address_p)) + 2)) << 16) | \
                                                ((*(((STSYS_MU16 *) (Address_p))    ))      )))
#endif


/* ------------------------------------------------------- */
/* void STSYS_WriteRegMem24BE(void *Address_p, U32 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegMem24BE(Address_p, Value)                                      \
    {                                                                                \
        *(((STSYS_MU16 *) (Address_p))    ) = (U16) ((((Value) & 0xFF0000) >> 16) |  \
                                                     (((Value) & 0x00FF00)      ));  \
        *(((STSYS_MU8  *) (Address_p)) + 2) =  (U8)   ((Value)                   );  \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegMem24BE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_MU16 *) (Address_p))    ) = (U16) ((Value) >> 8);     \
        *(((STSYS_MU8  *) (Address_p)) + 2) =  (U8) ((Value)     );     \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegMem24BE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_MU8 *) (Address_p))    ) = (U8) ((Value) >> 16);      \
        *(((STSYS_MU8 *) (Address_p)) + 1) = (U8) ((Value) >> 8 );      \
        *(((STSYS_MU8 *) (Address_p)) + 2) = (U8) ((Value)      );      \
    }
#endif /* _NO_OPTIMIZATION */

/* ------------------------------------------------------- */
/* void STSYS_WriteRegMem24LE(void *Address_p, U32 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegMem24LE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_MU16 *) (Address_p))    ) = (U16) (Value);            \
        *(((STSYS_MU8 *)  (Address_p)) + 2) = (U8) ((Value) >> 16);     \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegMem24LE(Address_p, Value)                                    \
    {                                                                              \
        *(((STSYS_MU16 *) (Address_p))   ) = (U16) ((((Value) & 0xFF00) >> 8) |    \
                                                    (((Value) & 0x00FF) << 8));    \
        *(((STSYS_MU8 *) (Address_p)) + 2) =  (U8)  (((Value)         ) >> 16);    \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegMem24LE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_MU8 *) (Address_p))    ) = (U8) ((Value)      );      \
        *(((STSYS_MU8 *) (Address_p)) + 1) = (U8) ((Value) >> 8 );      \
        *(((STSYS_MU8 *) (Address_p)) + 2) = (U8) ((Value) >> 16);      \
    }
#endif /* _NO_OPTIMIZATION */

/* ------------------------------------------------------- */
/* U32 STSYS_ReadRegMem32BE(void *Address_p); */
/* ------------------------------------------------------- */
#if (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
     (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( little endian CPU or ( big endian CPU and access not optimized ) )*/
#define STSYS_ReadRegMem32BE(Address_p) ((U32) (((*(((STSYS_MU8 *) (Address_p))    )) << 24) | \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 1)) << 16) | \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 2)) <<  8) | \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 3))     )))
#else
/* ( big endian CPU and access optimized )*/
#define STSYS_ReadRegMem32BE(Address_p) (*((STSYS_MU32 *) (Address_p)))
#endif

/* ------------------------------------------------------- */
/* U32 STSYS_ReadRegMem32LE(void *Address_p); */
/* ------------------------------------------------------- */
#if (defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) || \
   (!defined (STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE) && defined (STSYS_MEMORY_ACCESS_NO_OPTIMIZATION)))
/* ( big endian CPU or ( little endian CPU and access not optimized ) )*/
#define STSYS_ReadRegMem32LE(Address_p) ((U32) (((*(((STSYS_MU8 *) (Address_p))    ))      ) |  \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 1)) << 8 ) |  \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 2)) << 16) |  \
                                                ((*(((STSYS_MU8 *) (Address_p)) + 3)) << 24)))
#else
/* ( little endian CPU and access optimized )*/
#define STSYS_ReadRegMem32LE(Address_p) (*((STSYS_MU32 *) (Address_p)))
#endif

/* ------------------------------------------------------- */
/* void STSYS_WriteRegMem32BE(void *Address_p, U32 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegMem32BE(Address_p, Value)                         \
    {                                                                   \
        *((STSYS_MU32 *) (Address_p)) = (U32) ((((Value) & 0xFF000000) >> 24) |   \
                                               (((Value) & 0x00FF0000) >> 8 ) |   \
                                               (((Value) & 0x0000FF00) << 8 ) |   \
                                               (((Value) & 0x000000FF) << 24));   \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegMem32BE(Address_p, Value)                         \
    {                                                                   \
        *((STSYS_MU32 *) (Address_p)) = (U32) (Value);                  \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegMem32BE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_MU8 *) (Address_p))    ) = (U8) ((Value) >> 24);      \
        *(((STSYS_MU8 *) (Address_p)) + 1) = (U8) ((Value) >> 16);      \
        *(((STSYS_MU8 *) (Address_p)) + 2) = (U8) ((Value) >> 8 );      \
        *(((STSYS_MU8 *) (Address_p)) + 3) = (U8) ((Value)      );      \
    }
#endif /* _NO_OPTIMIZATION */

/* ------------------------------------------------------- */
/* void STSYS_WriteRegMem32LE(void *Address_p, U32 Value); */
/* ------------------------------------------------------- */
#ifndef STSYS_MEMORY_ACCESS_NO_OPTIMIZATION     /* optimized */
#ifndef STSYS_MEMORY_ACCESS_BIG_NOT_LITTLE      /* little endian CPU */
#define STSYS_WriteRegMem32LE(Address_p, Value)                         \
    {                                                                   \
        *((STSYS_MU32 *) (Address_p)) = (U32) (Value);                  \
    }
#else                                           /* big endian CPU */
#define STSYS_WriteRegMem32LE(Address_p, Value)                                   \
    {                                                                             \
        *((STSYS_MU32 *) (Address_p)) = (U32) ((((Value) & 0xFF000000) >> 24) |   \
                                               (((Value) & 0x00FF0000) >> 8 ) |   \
                                               (((Value) & 0x0000FF00) << 8 ) |   \
                                               (((Value) & 0x000000FF) << 24));   \
    }
#endif /* _BIG_NOT_LITTLE */
#else                                           /* not optimized */
#define STSYS_WriteRegMem32LE(Address_p, Value)                         \
    {                                                                   \
        *(((STSYS_MU8 *) (Address_p))    ) = (U8) ((Value)      );      \
        *(((STSYS_MU8 *) (Address_p)) + 1) = (U8) ((Value) >> 8 );      \
        *(((STSYS_MU8 *) (Address_p)) + 2) = (U8) ((Value) >> 16);      \
        *(((STSYS_MU8 *) (Address_p)) + 3) = (U8) ((Value) >> 24);      \
    }
#endif /* _NO_OPTIMIZATION */



/* Exported Functions ------------------------------------------------------- */

/* ########################################################################### */
/*                     UNCACHED 'MEMORY' ACCESS                                */
/* ########################################################################### */
#if (defined(ST_OS20) || defined(NATIVE_CORE))
#define STSYS_ReadRegMemUncached8(Address_p)             STSYS_ReadRegDev8((Address_p))
#define STSYS_WriteRegMemUncached8(Address_p, Value)     STSYS_WriteRegDev8((Address_p), (Value))
#define STSYS_ReadRegMemUncached16LE(Address_p)          STSYS_ReadRegDev16LE((Address_p))
#define STSYS_WriteRegMemUncached16LE(Address_p), Value) STSYS_WriteRegDev16LE((Address_p), (Value))
#define STSYS_ReadRegMemUncached16BE(Address_p)          STSYS_ReadRegDev16BE((Address_p))
#define STSYS_WriteRegMemUncached16BE(Address_p, Value)  STSYS_WriteRegDev16BE((Address_p), (Value))
#define STSYS_ReadRegMemUncached32LE(Address_p)          STSYS_ReadRegDev32LE((Address_p))
#define STSYS_WriteRegMemUncached32LE(Address_p, Value)  STSYS_WriteRegDev32LE((Address_p), (Value))
#define STSYS_ReadRegMemUncached32BE(Address_p)          STSYS_ReadRegDev32BE((Address_p))
#define STSYS_WriteRegMemUncached32BE(Address_p, Value)  STSYS_WriteRegDev32BE((Address_p), (Value))

#elif defined(ST200_OS21)
#define STSYS_ReadRegMemUncached8(Address_p)             STSYS_ReadRegDev8(((S32)(Address_p) + STSYS_ST200UncachedOffset))
#define STSYS_WriteRegMemUncached8(Address_p, Value)     STSYS_WriteRegDev8(((S32)(Address_p) + STSYS_ST200UncachedOffset), (Value))
#define STSYS_ReadRegMemUncached16LE(Address_p)          STSYS_ReadRegDev16LE(((S32)(Address_p) + STSYS_ST200UncachedOffset))
#define STSYS_WriteRegMemUncached16LE(Address_p, Value)  STSYS_WriteRegDev16LE(((S32)(Address_p) + STSYS_ST200UncachedOffset), (Value))
#define STSYS_ReadRegMemUncached16BE(Address_p)          STSYS_ReadRegDev16BE(((S32)(Address_p) + STSYS_ST200UncachedOffset))
#define STSYS_WriteRegMemUncached16BE(Address_p, Value)  STSYS_WriteRegDev16BE(((S32)(Address_p) + STSYS_ST200UncachedOffset), (Value))
#define STSYS_ReadRegMemUncached32LE(Address_p)          STSYS_ReadRegDev32LE(((S32)(Address_p) + STSYS_ST200UncachedOffset))
#define STSYS_WriteRegMemUncached32LE(Address_p, Value)  STSYS_WriteRegDev32LE(((S32)(Address_p) + STSYS_ST200UncachedOffset), (Value))
#define STSYS_ReadRegMemUncached32BE(Address_p)          STSYS_ReadRegDev32BE(((S32)(Address_p) + STSYS_ST200UncachedOffset))
#define STSYS_WriteRegMemUncached32BE(Address_p, Value)  STSYS_WriteRegDev32BE(((S32)(Address_p) + STSYS_ST200UncachedOffset), (Value))

#elif defined(ST_OS21)
#define STSYS_ReadRegMemUncached8(Address_p)             STSYS_ReadRegDev8((((U32)(Address_p) & ST40_PHYSICAL_ACCESS_MASK) | ST40_UNCACHED_ACCESS_MASK))
#define STSYS_WriteRegMemUncached8(Address_p, Value)     STSYS_WriteRegDev8((((U32)(Address_p) & ST40_PHYSICAL_ACCESS_MASK) | ST40_UNCACHED_ACCESS_MASK), (Value))
#define STSYS_ReadRegMemUncached16LE(Address_p)          STSYS_ReadRegDev16LE((((U32)(Address_p) & ST40_PHYSICAL_ACCESS_MASK) | ST40_UNCACHED_ACCESS_MASK))
#define STSYS_WriteRegMemUncached16LE(Address_p, Value)  STSYS_WriteRegDev16LE((((U32)(Address_p) & ST40_PHYSICAL_ACCESS_MASK) | ST40_UNCACHED_ACCESS_MASK), (Value))
#define STSYS_ReadRegMemUncached16BE(Address_p)          STSYS_ReadRegDev16BE((((U32)(Address_p) & ST40_PHYSICAL_ACCESS_MASK) | ST40_UNCACHED_ACCESS_MASK))
#define STSYS_WriteRegMemUncached16BE(Address_p, Value)  STSYS_WriteRegDev16BE((((U32)(Address_p) & ST40_PHYSICAL_ACCESS_MASK) | ST40_UNCACHED_ACCESS_MASK), (Value))
#define STSYS_ReadRegMemUncached32LE(Address_p)          STSYS_ReadRegDev32LE((((U32)(Address_p) & ST40_PHYSICAL_ACCESS_MASK) | ST40_UNCACHED_ACCESS_MASK))
#define STSYS_WriteRegMemUncached32LE(Address_p, Value)  STSYS_WriteRegDev32LE((((U32)(Address_p) & ST40_PHYSICAL_ACCESS_MASK) | ST40_UNCACHED_ACCESS_MASK), (Value))
#define STSYS_ReadRegMemUncached32BE(Address_p)          STSYS_ReadRegDev32BE((((U32)(Address_p) & ST40_PHYSICAL_ACCESS_MASK) | ST40_UNCACHED_ACCESS_MASK))
#define STSYS_WriteRegMemUncached32BE(Address_p, Value)  STSYS_WriteRegDev32BE((((U32)(Address_p) & ST40_PHYSICAL_ACCESS_MASK) | ST40_UNCACHED_ACCESS_MASK), (Value))

#elif defined(ST_OSLINUX)
/* These macros are defined into compat.h */

#else /* Unknown CPU architecture */

#error "ERROR : Uncached access undefined for this CPU architecture"
#endif


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif  /* #ifndef S_OSLINUX */
#endif  /* #ifndef __STSYS_H */

/* End of stsys.h */
