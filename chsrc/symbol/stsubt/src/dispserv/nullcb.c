/******************************************************************************\
 * File Name : nullcb.c
 *
 * Description:
 *     This file just provides implementation of the
 *     predefined NULL display  params
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author      : Marino Congiu - Jan 1999
 *
\******************************************************************************/
 
#include <stdlib.h>
#include <subtdev.h>
#include <dispserv.h>

/* ------------------------------------------------- */
/* --- Predefined NULL Display Service Structure --- */
/* ------------------------------------------------- */
 
const STSUBT_DisplayService_t STSUBT_NULL_DisplayServiceStructure = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL 
} ;


