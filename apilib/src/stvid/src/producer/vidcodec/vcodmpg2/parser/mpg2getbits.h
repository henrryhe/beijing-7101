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

 Documents:

 Notes:

*****************************************************************************
 History:
 Rev Level    Date    Name    ECN#    Description
 -----------------------------------------------------------------------------
 1.00       10/20/04  JRB     ---     Initial Creation
*******************************************************************************/

#include "stddefs.h"
#include "stos.h"

#ifndef _GB_H_
#define _GB_H_

#ifdef __cplusplus
    extern "C"
    {
#endif

/* ====== defines ======*/

#define GB_NO_STARTCODE 0xFFFF          /*Invalid startcode value*/


/* ====== enums =======*/


/* ====== typedefs ======*/
typedef void * gb_Handle_t;

typedef struct
{
    U8      *buffer;   /*  Pointer to source buffer*/
    U32     bufferSize;    /*  size of source buffer*/
    U32     nByte;         /*  Current Byte Number*/
    U32     nBit;          /*  Current Bit number (msb = 7, lsb = 0)*/
    U32     active;        /*  TRUE = Handle in use*/
    U32     nByte_Old;   /*   Old Byte Number*/
    BOOL    Loop_BB;
    void*   TOP_BBAdress;
    void*   BASE_BBAdress;
} gb_BitStruct;

/* ====== globals ======*/


/* ====== prototypes ======*/

gb_Handle_t gb_InitHandle (void);
void gb_DestroyHandle(gb_Handle_t gbH);
void gb_InitBuffer(gb_Handle_t gbH, U8 *buffer, U32 bufferSize, void* TOP_ADR, void* BASE_ADR);
U32 gb_GetBits(gb_Handle_t gbH, U32 numBits, U32 *bitsAvail);
U32 gb_GetNextStartCode(gb_Handle_t gbH, BOOL lookAheadOnly, U32 *nByte);
U8 gb_Lookahead(gb_Handle_t gbH, U32 byteOffset);
void gb_GetIndexes(gb_Handle_t gbH, U32 *nByte, U32 *nBit);



#ifdef __cplusplus
    }
#endif
#endif



