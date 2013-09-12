/*
 * shakedown/vararray/vararray.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Generic header for termarray shakedown
 */

#ifndef rpc_termarray_h
#define rpc_termarray_h

#define DEBUG
#include "harness.h"

typedef struct {
	char	c;
	int	i;
} struct_t;

/* prototypes */

/* primative types */
void st20_chararray_in   (in.termarray(60, __element__ == 0) unsigned char* data);
void st40_chararray_in   (in.termarray(60, __element__ == 0) unsigned char* data);
void st20_chararray_out  (int len, out.termarray(60, __element__ == 0) unsigned char* data);
void st40_chararray_out  (int len, out.termarray(60, __element__ == 0) unsigned char* data);
void st20_chararray_inout(inout.termarray(60, __element__ == 0) unsigned char* data);
void st40_chararray_inout(inout.termarray(60, __element__ == 0) unsigned char* data);

void st20_structarray_in   (in.termarray(60, __element__.c == 0) struct_t* data);
void st40_structarray_in   (in.termarray(60, __element__.c == 0) struct_t* data);
void st20_structarray_out  (int len, out.termarray(60, __element__.c == 0) struct_t* data);
void st40_structarray_out  (int len, out.termarray(60, __element__.c == 0) struct_t* data);
void st20_structarray_inout(inout.termarray(60, __element__.c == 0) struct_t* data);
void st40_structarray_inout(inout.termarray(60, __element__.c == 0) struct_t* data);

void st20_runtests(void);
void st40_runtests(void);

arenas {
	{ slve, OS_FOR_SLAVE, CPU_FOR_SLAVE },
	{ mstr, OS_FOR_MASTER, CPU_FOR_MASTER }
};

transport {{ slve, mstr, TRANS_EMBX }};

headers {
	{ slve, { "\"termarray.h\"" } },
	{ mstr, { "\"termarray.h\"" } }
};

import {
	{ st20_chararray_in,                     slve, mstr },
	{ st40_chararray_in,                     mstr, slve },
	{ st20_chararray_out,                    slve, mstr },
	{ st40_chararray_out,                    mstr, slve },
	{ st20_chararray_inout,                  slve, mstr },
	{ st40_chararray_inout,                  mstr, slve },
	{ st20_structarray_in,                   slve, mstr },
	{ st40_structarray_in,                   mstr, slve },
	{ st20_structarray_out,                  slve, mstr },
	{ st40_structarray_out,                  mstr, slve },
	{ st20_structarray_inout,                slve, mstr },
	{ st40_structarray_inout,                mstr, slve },
	{ st20_runtests,                         slve, mstr },
	{ st40_runtests,                         mstr, slve },
};

#endif
