/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

  File Name: cell.h
Description: memory manager data headers (for handle data structures)

******************************************************************************/

#ifndef __CELL_H
 #define __CELL_H


/* Includes ------------------------------------------------------------ */


#include "stddefs.h"
#include "handle.h"


/* Exported Types ------------------------------------------------------ */


typedef volatile struct CellHeader_s 
{
    volatile struct Cell_s *next_p;
    volatile struct Cell_s *prev_p;
    FullHandle_t            Handle;
    size_t                  Size;
} CellHeader_t;


/* --------------------------------------------------------------------- */


typedef volatile struct Cell_s  /* arbitrary sized struct */
{
    CellHeader_t Header;
    U8           data;  /* U8 data[] (there will be lots of data following this) 
    ...
    ...
    ...
    ... */
} Cell_t;

#endif /* __CELL_H */
