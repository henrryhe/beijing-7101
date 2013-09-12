/*****************************************************************************
                           Explorer System Software

                   Copyright 2004 Scientific-Atlanta, Inc.

                           Subscriber Engineering
                           5030 Sugarloaf Parkway
                               P.O.Box 465447
                          Lawrenceville, GA 30042

                        Proprietary and Confidential
              Unauthorized distribution or copying is prohibited
                            All rights reserved

 No part of this computer software may be reprinted, reproduced or utilized
 in any form or by any electronic, mechanical, or other means, now known or
 hereafter invented, including photocopying and recording, or using any
 information storage and retrieval system, without permission in writing
 from Scientific Atlanta, Inc.

*****************************************************************************

 File Name:    mpg2getbits.cpp

 See Also:

 Project:      Zeus ASIC

 Compiler:     ST40

 Author:       John Bean

 Description:
                   Provides an interface to get data from a buffer bitwise.

 Documents:

 Notes:

*****************************************************************************
 History:
 Rev Level    Date    Name    ECN#    Description
 -----------------------------------------------------------------------------
 1.00       11/02/04  JRB     ---     Initial Creation from SCGen
*****************************************************************************/

/* ====== includes ======*/

#if !defined(ST_OSLINUX)
#include <stdio.h>
#endif

#include "mpg2getbits.h"

/* ====== file info ======*/


/* ====== defines ======*/

/* TODO: Is there a max number of decodes at one time?*/
#define  MAX_GBHANDLES  32

/*====== enums ======*/


/*====== globals ======*/


/*====== statics ======*/

static gb_BitStruct gbHandles[MAX_GBHANDLES];
static BOOL bHandlesInit = FALSE;

/*====== prototypes ======*/


/*******************************************************************
 Function:   gb_InitHandle()

 Parameters: N/A

 Returns:    Handle for use with buffer, NULL if none available

 Scope:      Public

 Purpose:    Initializes GetBits handle

 Behavior:   Allocates and initializes Handle for GetBits use.

 Exceptions: None

*******************************************************************/

gb_Handle_t gb_InitHandle (void)
{
   U32         index;

/*   Initialize the active flags, if they haven't been yet*/
   if (bHandlesInit == FALSE)
   {
      bHandlesInit = TRUE;
      for (index = 0; index < MAX_GBHANDLES; index++)
      {
         gbHandles[index].active = FALSE;
      }
   }

/*   Find the first available handle*/
   for (index = 0; index < MAX_GBHANDLES; index++)
   {
      if (gbHandles[index].active == FALSE)
      {
         gbHandles[index].active     = TRUE;
         gbHandles[index].buffer     = NULL;
         gbHandles[index].bufferSize = 0;
         return ((gb_Handle_t)&(gbHandles[index]));
      }
   }

   return (NULL);
}

/*******************************************************************
Function:   gb_DestroryHandle()

Parameters: gbH     - GetBits handle

Returns:    None

Scope:      Public

Purpose:    De-allocates memory associated w/a GetBits handle

Behavior:   Frees memory, if a non-NULL handle provided

Exceptions: None

*******************************************************************/

void gb_DestroyHandle(gb_Handle_t gbH)
{
   gb_BitStruct	*gb = (gb_BitStruct *)gbH;

   if (gb != NULL)
   {
      gb->active = FALSE;
   }
}

/*******************************************************************
Function:   gb_InitBuffer()

Parameters: gbH        - GetBits handle
            buffer     - Pointer to buffer
            bufferSize - Size of buffer in Bytes

Returns:    None

Scope:      Public

Purpose:    Initializes GetBits buffer structure with buffer info

Behavior:   If a non-NULL handle provided, initializes buffer info
            in preparation for getting data bitwise.

Exceptions: None

*******************************************************************/

void gb_InitBuffer(gb_Handle_t gbH, U8 *buffer, U32 bufferSize, void* TOP_ADR, void* BASE_ADR)
{
   gb_BitStruct	*gb = (gb_BitStruct *)gbH;

   if (gb == NULL)
   {
      return;
   }

/*   Initialize structure to first bit of stream*/
   gb->buffer = buffer;
   gb->bufferSize = bufferSize;
   gb->nByte = 0;
   gb->nBit = 7;
   gb->nByte_Old = 0;
   gb->Loop_BB = false;
   gb->TOP_BBAdress = TOP_ADR;
   gb->BASE_BBAdress = BASE_ADR;
}

/*******************************************************************
Function:   gb_GetBits()

Parameters: gbH        - GetBits handle
            numBits    - Number of bits to return
            bitsAvail  - Number of bits available after read (if non-NULL pointer)

Returns:    bits requested, or 0 if all requested bits not available

Scope:      Public

Purpose:    Returns up to 32 bits of data from buffer bitwise

Behavior:   Loops through each bit in the destination. If the corresponding
            bit in the source is set, then the destination bit is set.

            Note that the data is read bitwise msb->lsb, and in byte order.
            The bitfield returned is in a 32-bit value in the same endianness
            as the processor.


Exceptions: None

*******************************************************************/

U32 gb_GetBits(gb_Handle_t gbH, U32 numBits, U32 *bitsAvail)
{
    gb_BitStruct *gb = (gb_BitStruct *)gbH;
    U32           bitsLeft = (gb->nBit + 1) + ((gb->bufferSize - gb->nByte) * 8);
    U32           bits = 0;
    U32           bitMask;
    U32           byteMask;

    if (gb == NULL)
    {
        return(0);
    }

/*   Make sure there are enough bits remaining in the buffer*/
    if ((numBits > bitsLeft) || (numBits == 0))
    {
        if (bitsAvail != NULL)
        {
            *bitsAvail = bitsLeft;
        }
        return (0);
    }

/*   Limit requested bits to 32*/
    numBits = (numBits > 32) ? 32 : numBits;


/*   Loop for each bit in the bitmask, filling in the destination bits*/

    bitMask = 1 << (numBits - 1);    /* Destination bitmask*/
    byteMask = 1 << (gb->nBit);      /* Source bitmask*/

    while (bitMask > 0)
    {
        if ((byteMask == 0x80) && (numBits >= 8))
        {
/*         Move full bytes to destination, when source alignned*/
            while (numBits >= 8)
            {
                if (((U32)(gb->buffer) + gb->nByte) > (U32)gb->TOP_BBAdress)
                {
                    gb->nByte_Old = gb->nByte;
                    gb->buffer=gb->BASE_BBAdress;
                    gb->nByte=0;
                    gb->Loop_BB = true;

                }

                bits |= gb->buffer[gb->nByte] << (numBits - 8);
                gb->nByte++;
                bitMask >>= 8;
                numBits -=8;
            }
        }
        else
        {
            if (((U32)(gb->buffer) + gb->nByte) > (U32)(gb->TOP_BBAdress))
            {
                gb->buffer=gb->BASE_BBAdress;
                gb->nByte=0;
            }

/*         Move Data bitwise from source to destination*/
            if (gb->buffer[gb->nByte] & byteMask)
            {

                bits |= bitMask;
            }

            byteMask >>= 1;
            if (byteMask == 0)
            {
/*            Move source to the next byte in the buffer*/
                gb->nByte++;
                gb->nBit = 7;
                byteMask = 0x80;
            }
            else
            {

/*            Move source to the next bit in the same byte*/
                gb->nBit --;
            }

            bitMask >>= 1;
            numBits --;
        }
    }


    if (bitsAvail != NULL)
    {
        if (gb->Loop_BB)
        {
            *bitsAvail = (gb->nBit + 1) + ((gb->bufferSize - (gb->nByte+gb->nByte_Old)) * 8);
        }
        else
        {
            *bitsAvail = (gb->nBit + 1) + ((gb->bufferSize - gb->nByte) * 8);
        }
    }

    return (bits);
}

/*******************************************************************
Function:   gb_GetNextStartCode()

Parameters: gbH           - GetBits handle
            LookAheadOnly - TRUE if the current position not to be updated
            nByte         - pointer for returning buffer offset for startcode

Returns:    next startcode, or GB_NO_STARTCODE if no more in buffer

Scope:      Public

Purpose:    Returns next startcode

Behavior:   Starts with the current byte, and looks for a startcode.
            If the startcode is found, the value is returned.
            If the request isn't a lookahead request, the buffer byte and bit
            indexes are updated to point to the bit immediately following the
            startcode.

Exceptions: None

*******************************************************************/

U32 gb_GetNextStartCode(gb_Handle_t gbH, BOOL lookAheadOnly, U32 *nByte)
{
   gb_BitStruct	*gb = (gb_BitStruct *)gbH;
   U32           esIdx;
   U32           nsc = GB_NO_STARTCODE;

   /*Loop through the buffer until a startcode is foud*/
   /*or we run out of buffer*/
   for (esIdx = gb->nByte; esIdx + 3 < gb->bufferSize; esIdx++)
   {
      if ( (gb->buffer[esIdx]     == 0x00) &&
           (gb->buffer[esIdx + 1] == 0x00) &&
           (gb->buffer[esIdx + 2] == 0x01))
      {
         if (lookAheadOnly == FALSE)
         {
            gb->nByte = esIdx + 4;
            gb->nBit = 7;
         }

         if (nByte != NULL)
         {
            *nByte = esIdx;
         }

         nsc = (gb->buffer[esIdx] << 24)    + (gb->buffer[esIdx + 1] << 16) +
               (gb->buffer[esIdx + 2] << 8) + (gb->buffer[esIdx + 3]);
         break;
      }
   }

   if ((nsc == GB_NO_STARTCODE) && (nByte != NULL))
   {
      *nByte = gb->bufferSize;
   }

   return (nsc);

}

/*******************************************************************
Function:   gb_Lookahead()

Parameters: gbH        - GetBits handle
            byteOffset - requested byte's offset from current byte

Returns:    value of byte requested, or 0x00 if unavailable

Scope:      Public

Purpose:    Returns byte requested from buffer

Behavior:

Exceptions: None

*******************************************************************/

U8 gb_Lookahead(gb_Handle_t gbH, U32 byteOffset)
{
   gb_BitStruct	*gb = (gb_BitStruct *)gbH;

   if ((byteOffset + gb->nByte) >= gb->bufferSize)
   {
      return (0);
   }

   return (gb->buffer[gb->nByte + byteOffset]);
}

/*******************************************************************
Function:   gb_GetIndexes()

Parameters: gbH   - GetBits handle
            nByte - current byte offset into buffer
            nBit  - current bit offset into byte

Returns:    N/A

Scope:      Public

Purpose:    Returns current offsets into ES buffer

Behavior:   Returns offsets, if pointers are non-NULL

Exceptions: None

*******************************************************************/

void gb_GetIndexes(gb_Handle_t gbH, U32 *nByte, U32 *nBit)
{
   gb_BitStruct	*gb = (gb_BitStruct *)gbH;

   if (nByte != NULL)
   {
      *nByte = gb->nByte;
   }

   if (nBit != NULL)
   {
      *nBit = gb->nBit;
   }

   return;
}
