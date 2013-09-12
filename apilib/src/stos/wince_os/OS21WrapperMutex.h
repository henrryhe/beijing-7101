/*! Time-stamp: <@(#)OS21WrapperMutex.h   8/23/2005 - 5:23:38 PM   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperMutex.h
 *
 *  Project : 
 *
 *  Package : 
 *
 *  Company : TeamLog 
 *
 *  Author  : William Hennebois            Date: 8/23/2005
 *
 *  Purpose :  Declaration of class mutex
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  8/23/2005  WH : First Revision
 *
 *********************************************************************
 */

#ifndef __OS21WrapperMutex__
#define __OS21WrapperMutex__

#define OS21_DEF_MIN_STACK_SIZE		(16 * 1024)

#define OBJECT_TYPE_LIST			 OBJECT_TYPE_LIST_WINCE
#define ST40_PHYS_MASK		(0x1FFFFFFF)
#define ST40_PHYS_ADDR(a)       (((unsigned int)(a) & ST40_PHYS_MASK))


////////////////////////////////////////////////////////
///////////////                 ////////////////////////
//////////////// Types		    ////////////////////////
///////////////                 ////////////////////////
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
///////////////                 ////////////////////////
//////////////// Struct		    ////////////////////////
///////////////                 ////////////////////////
////////////////////////////////////////////////////////



////////////////////////////////////////////////////////
///////////////                      ///////////////////
//////////////// mutex functions ///////////////////
///////////////                      ///////////////////
////////////////////////////////////////////////////////



mutex_t * mutex_create_fifo       (void);
int       mutex_delete            (mutex_t* mutex);
void      mutex_lock              (mutex_t* mutex);
int       mutex_release           (mutex_t* mutex);



#endif // __OS21WrapperMutex__