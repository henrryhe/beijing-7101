/*******************************************************************************

File name : acc_bptr.c

Description : AVMEM memory access file for byte pointers method

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
 8 Dec 1999         Created                                          HG
01 Jun 2001         Add stavmem_ before non API exported symbols     HSdLM
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#ifndef ST_OSLINUX
    #include <string.h>
#endif

#include "stddefs.h"
/*#ifndef ST_OSLINUX */
#include "sttbx.h"
#include "stsys.h"
/*#endif */
#include "stavmem.h"
#include "avm_acc.h"
#include "acc_bptr.h"
#include "stos.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#define AVMEM_LITTLE_ENDIAN 1
#define AVMEM_BIG_ENDIAN    2
/* Set endianness of the memory below */
#define MEMORY_ENDIANNESS_IS AVMEM_LITTLE_ENDIAN
/*#define MEMORY_ENDIANNESS_IS AVMEM_BIG_ENDIAN*/


/* Private Function prototypes ---------------------------------------------- */

static void InitBytePointers(void);

static void Copy1DHandleOverlapBytePointers(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy2DNoOverlapBytePointers(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                        void * const DestAddr_p, const U32 DestPitch);
static void Copy2DHandleOverlapBytePointers(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                            void * const DestAddr_p, const U32 DestPitch);
static void Fill1DBytePointers(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize);
static void Fill2DBytePointers(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                               void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch);
static void Fill1Dw32b(const U32 PatternMSBFirst, void * const DestAddr_p, const U32 DestSize);


/* Private Variables (static)------------------------------------------------ */


/* Private Macros ----------------------------------------------------------- */


/* Exported variables ------------------------------------------------------- */

stavmem_MethodOperations_t stavmem_BytePointers = {
    InitBytePointers,                       /* Private function */
    /* All functions must be supported by byte pointers */
    {
        Copy1DHandleOverlapBytePointers,    /* Private function: use function handling overlaps as there is almost no diference (just one additional if) */
        Copy1DHandleOverlapBytePointers,    /* Private function */
        Copy2DNoOverlapBytePointers,        /* Private function */
        Copy2DHandleOverlapBytePointers,    /* Private function */
        Fill1DBytePointers,                 /* Private function */
        Fill2DBytePointers                  /* Private function */
    },
    /* No need to check: byte pointers is always successful */
    {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    }
};


/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : InitBytePointers
Description : Initialise byte pointers copy/fill method
 Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitBytePointers(void)
{
    ; /* Nothing to do */
}


/*******************************************************************************
Name        : Copy1DHandleOverlapBytePointers
Description : Perform 1D copy with care of overlaps, with byte pointers method
Parameters  : source, destination, and size
Assumptions :
Limitations :
Returns     : Nothing: always successful
*******************************************************************************/
static void Copy1DHandleOverlapBytePointers(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    U32 Index;

    if (((U8 *) SrcAddr_p) < ((U8 *) DestAddr_p))
    {
        /* Risk of overlap of the regions to the right: copy backward direction */
        for (Index = Size; Index > 0; Index--)
        {
            *(((U8 *) DestAddr_p) + (Index - 1)) = *(((U8 *) SrcAddr_p) + (Index - 1));
        }
    }
    else
    {
        /* Risk of overlap of the regions to the left: copy forward direction */
        for (Index = 0; Index < Size; Index++)
        {
            *(((U8 *) DestAddr_p) + Index) = *(((U8 *) SrcAddr_p) + Index);
        }
    }
}


/*******************************************************************************
Name        : Copy2DNoOverlapBytePointers
Description : Perform 2D copy with no care of overlaps, with byte pointers method
Parameters  : source, dimensions, source pitch, destination, destination pitch
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy2DNoOverlapBytePointers(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                        void * const DestAddr_p, const U32 DestPitch)
{
    U32 y, x;

    /* From ChangeDirectionPoint to SrcHeight, copy forward direction */
    for (y = 0; y < SrcHeight; y++)
    {
        for (x = 0; x < SrcWidth; x++)
        {
            *(((U8 *) DestAddr_p) + (DestPitch * y) + x) = *(((U8 *) SrcAddr_p) + (SrcPitch * y) + x);
        }
    }
}


/*******************************************************************************
Name        : Copy2DHandleOverlapBytePointers
Description : Perform 2D copy with care of overlaps, with byte pointers method
Parameters  : source, dimensions, source pitch, destination, destination pitch
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy2DHandleOverlapBytePointers(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                            void * const DestAddr_p, const U32 DestPitch)
{
    U32 y;
    U32 ChangeDirectionPoint;
    U32 Width = SrcWidth;

    /* To handle overlap of regions: must care of directions of copy in order not to loose data
       Depending on the location of the overlap, there will be 2 directions of copy with
       a particular point where the direction change. */

    /* Start searching from first addresses */
    ChangeDirectionPoint = 1;
    if (((U8 *) DestAddr_p) < ((U8 *) SrcAddr_p))
    {
        /* Case where destination region comes before: start copy in forward direction */

        /* Look for the point where the direction of copy must change */
        /* Test addresses of begining of rows for source and destination */
        do
        {
            ChangeDirectionPoint++;
            if (ChangeDirectionPoint == (SrcHeight + 1))
            {
                break;
            }
        } while ((((U8 *) DestAddr_p) + ((ChangeDirectionPoint - 1) * DestPitch)) < (((U8 *) SrcAddr_p) + ((ChangeDirectionPoint - 1) * SrcPitch)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from height down to ChangeDirectionPoint and from 1 up to ChangeDirectionPoint */
        /* From 1 to ChangeDirectionPoint-1, copy forward direction */
        for (y = 1; y < ChangeDirectionPoint; y++)
        {
            /* Overlap or not doesn't cost much for byte pointers */
            Copy1DHandleOverlapBytePointers((void *) (((U8 *) SrcAddr_p) + ((y - 1) * SrcPitch)), (void *) (((U8 *) DestAddr_p) + ((y - 1) * DestPitch)), Width);
        }
        /* From ChangeDirectionPoint to SrcHeight, copy backward direction */
        for (y = SrcHeight; y >= ChangeDirectionPoint; y--)
        {
            /* Overlap or not doesn't cost much for byte pointers */
            Copy1DHandleOverlapBytePointers((void *) (((U8 *) SrcAddr_p) + ((y - 1) * SrcPitch)), (void *) (((U8 *) DestAddr_p) + ((y - 1) * DestPitch)), Width);
        }
    }
    else {
        /* Case where source region comes before: start copy in backward direction */

        /* Look for the point where the direction of copy must change */
        /* Test addresses of begining of rows for source and destination */
        do
        {
            ChangeDirectionPoint++;
            if (ChangeDirectionPoint == (SrcHeight + 1))
            {
                break;
            }
        } while ((((U8 *) DestAddr_p) + ((ChangeDirectionPoint - 1) * DestPitch)) > (((U8 *) SrcAddr_p) + ((ChangeDirectionPoint - 1) * SrcPitch)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from ChangeDirectionPoint up to height and down to 1 */
        /* From 1 to ChangeDirectionPoint-1, copy backward direction */
        for (y = ChangeDirectionPoint - 1; y > 0; y--)
        {
            /* Overlap or not doesn't cost much for byte pointers */
            Copy1DHandleOverlapBytePointers((void *) (((U8 *) SrcAddr_p) + ((y - 1) * SrcPitch)), (void *) (((U8 *) DestAddr_p) + ((y - 1) * DestPitch)), Width);
        }
        /* From ChangeDirectionPoint to SrcHeight, copy forward direction */
        for (y = ChangeDirectionPoint; y <= SrcHeight; y++)
        {
            /* Overlap or not doesn't cost much for byte pointers */
            Copy1DHandleOverlapBytePointers((void *) (((U8 *) SrcAddr_p) + ((y - 1) * SrcPitch)), (void *) (((U8 *) DestAddr_p) + ((y - 1) * DestPitch)), Width);
        }
    }
}


/*******************************************************************************
Name        : Fill1DBytePointers
Description : Perform 1D fill with byte pointers method
Parameters  : pattern, pattern size, destination, destination size
Assumptions :
Limitations :
Returns     : Nothing: always successful
*******************************************************************************/
static void Fill1DBytePointers(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize)
{
    U32 IndexDest, IndexPat;
    U32 PatternMSBFirst;

    if ((PatternSize <= 2) || (PatternSize == 4))
    {
        /* For 1, 2 and 4 bytes patterns, use version with 32bits pattern (faster) */
        IndexPat = 0;
        PatternMSBFirst = 0;
        for (IndexPat = 0; IndexPat < 4; IndexPat++)
        {
            /* Copy 4 bytes of pattern: 4 times the same byte if 1 byte pattern,
                                     or 2 times the same 2 bytes if 2 bytes pattern,
                                     or 1 time the 4 bytes if 4 bytes pattern */
/*            PatternMSBFirst = (PatternMSBFirst << 8) | *(((U8 *) Pattern_p) + (IndexPat % PatternSize));*/
            PatternMSBFirst = (PatternMSBFirst << 8) | *(((U8 *) Pattern_p) + (IndexPat & (PatternSize - 1)));
        }
        Fill1Dw32b(PatternMSBFirst, DestAddr_p, DestSize);
    }
    else
    {
        /* No particular pattern size: this is the general case */
        IndexDest = 0;
        while ((DestSize - IndexDest) > PatternSize)
        {
            Copy1DHandleOverlapBytePointers(Pattern_p, (void *) (((U8 *) DestAddr_p) + IndexDest), PatternSize);
            IndexDest += PatternSize;
        }
        Copy1DHandleOverlapBytePointers(Pattern_p, (void *) (((U8 *) DestAddr_p) + IndexDest), DestSize - IndexDest);
    }
}


/*******************************************************************************
Name        : Fill1Dw32b
Description : Fill memory with 32 bits pattern
Parameters  : Pattern (with MSB first to be copied), destination address and size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Fill1Dw32b(const U32 PatternMSBFirst, void * const DestAddr_p, const U32 DestSize)
{
    U32 BytesCounter = 0;
    U32 AlignedPattern;


    /* Copy not aligned bytes at the begining */
    /* 0, 1, 2 or 3 bytes before the first 32 aligned byte */
    while ((((U32) (((U8 *) DestAddr_p) + BytesCounter)) & 3) != 0)
    {
        if (BytesCounter < DestSize)
        {
            *(((U8 *) DestAddr_p) + BytesCounter) = (U8) (PatternMSBFirst >> (8 * (3 - BytesCounter)));
            BytesCounter++;
        }
        else
        {
            return;
        }
    }

    /* Copy aligned bytes */
    /* BytesCounter has here the number of bytes already copied: 0 to 3 */
#if (MEMORY_ENDIANNESS_IS == AVMEM_LITTLE_ENDIAN)
    AlignedPattern = (U32)                   (U8) (PatternMSBFirst >> (8 * ((4 - BytesCounter    ) & 0x3)));
    AlignedPattern = (AlignedPattern << 8) | (U8) (PatternMSBFirst >> (8 * ((4 - BytesCounter + 1) & 0x3)));
    AlignedPattern = (AlignedPattern << 8) | (U8) (PatternMSBFirst >> (8 * ((4 - BytesCounter + 2) & 0x3)));
    AlignedPattern = (AlignedPattern << 8) | (U8) (PatternMSBFirst >> (8 * ((4 - BytesCounter + 3) & 0x3)));
#else
    AlignedPattern = (U32)                   (U8) (PatternMSBFirst >> (8 * ((4 - BytesCounter + 3) & 0x3)));
    AlignedPattern = (AlignedPattern << 8) | (U8) (PatternMSBFirst >> (8 * ((4 - BytesCounter + 2) & 0x3)));
    AlignedPattern = (AlignedPattern << 8) | (U8) (PatternMSBFirst >> (8 * ((4 - BytesCounter + 1) & 0x3)));
    AlignedPattern = (AlignedPattern << 8) | (U8) (PatternMSBFirst >> (8 * ((4 - BytesCounter    ) & 0x3)));
#endif

    while ((DestSize - BytesCounter) >= 4)
    {
        /* Choose U32* access whatever the method */
        /**((U32 *) (((U8 *)DestAddr_p ) + BytesCounter)) = AlignedPattern;*/

        *((((U32 *) DestAddr_p) + (BytesCounter >> 2))) = AlignedPattern;
/*        BestMethod.Execute.Copy1DNoOverlap(&AlignedPattern, (void *) (((U8 *) DestAddr_p) + BytesCounter), 4);*/
        BytesCounter += 4;
    }

    /* Copy not aligned bytes at the end */
    while (BytesCounter < DestSize)
    {
        /* 1 <= (DestSize - BytesCounter) <= 3 */
        *(((U8 *) DestAddr_p) + BytesCounter) = (U8) (PatternMSBFirst >> (8 * ((3 - BytesCounter) & 0x3)));
        BytesCounter++;
    }
}


/*******************************************************************************
Name        : Fill2DBytePointers
Description : Perform 2D fill with byte pointers method
Parameters  : pattern, pattern characteristics, destination, destination  characteristics
Assumptions :
Limitations :
Returns     : Nothing: always successful
*******************************************************************************/
static void Fill2DBytePointers(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                               void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch)
{
    U32 Line, PatLine, IndexDest;
    U32 IndexPat, PatternMSBFirst;

    if (PatternHeight == 1)
    {
        /* Patterneight is 1: fill 2D with 1D pattern ! */
        if ((PatternWidth <= 2) || (PatternWidth == 4))
        {
            IndexPat = 0;
            PatternMSBFirst = 0;
            /* For 1, 2 and 4 bytes patterns, use version with 32bits pattern (faster) */
            for (IndexPat = 0; IndexPat < 4; IndexPat++)
            {
                /* Copy 4 bytes of pattern: 4 times the same byte if 1 byte pattern,
                                        or 2 times the same 2 bytes if 2 bytes pattern,
                                        or 1 time the 4 bytes if 4 bytes pattern */
/*                PatternMSBFirst = (PatternMSBFirst << 8) | *(((U8 *) Pattern_p) + (IndexPat % PatternWidth));*/
                PatternMSBFirst = (PatternMSBFirst << 8) | *(((U8 *) Pattern_p) + (IndexPat & (PatternWidth - 1)));
            }
            for (Line = 0; Line < DestHeight; Line++)
            {
                Fill1Dw32b(PatternMSBFirst,  (void *) (((U8 *) DestAddr_p) + (Line * DestPitch)), DestWidth);
            }
        }
        else
        {
            /* No particular pattern size: this is the general case */
            /* Process each line the same way */
            for (Line = 0; Line < DestHeight; Line++)
            {
                IndexDest = 0;
                while ((DestWidth - IndexDest) > PatternWidth)
                {
                    Copy1DHandleOverlapBytePointers(Pattern_p, (void *) (((U8 *) DestAddr_p) + IndexDest + (Line * DestPitch)), PatternWidth);
                    IndexDest += PatternWidth;
                }
                Copy1DHandleOverlapBytePointers(Pattern_p, (void *) (((U8 *) DestAddr_p) + IndexDest + (Line * DestPitch)), DestWidth - IndexDest);
            }
        }
    }
    else
    {
        /* General case of fill 2D with 2D pattern */
        PatLine = 0;
        for (Line = 0; Line < DestHeight; Line++)
        {
            IndexDest = 0;
            while ((DestWidth - IndexDest) > PatternWidth)
            {
                Copy1DHandleOverlapBytePointers((void *) (((U8 *) Pattern_p) + (PatLine * PatternPitch)), (void *) (((U8 *) DestAddr_p) + IndexDest + (Line * DestPitch)), PatternWidth);
                IndexDest += PatternWidth;
            }
            Copy1DHandleOverlapBytePointers((void *) (((U8 *) Pattern_p) + (PatLine * PatternPitch)), (void *) (((U8 *) DestAddr_p) + IndexDest + (Line * DestPitch)), DestWidth - IndexDest);

            PatLine++;
            if (PatLine == PatternHeight)
            {
                PatLine = 0;
            }
        }
    }
}



/* End of acc_bptr.c */
