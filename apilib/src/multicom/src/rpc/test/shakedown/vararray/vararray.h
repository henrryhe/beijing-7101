/*
 * shakedown/vararray/vararray.h
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Generic header for vararray shakedown
 */

#ifndef rpc_vararray_h
#define rpc_vararray_h

#include "harness.h"

typedef struct {
	char	c;
	int	i;
} struct_t;

/* prototypes */

/* primative types */
void st20_chararray_in   (int len, in.array(60, len)    unsigned char* data);
void st40_chararray_in   (int len, in.array(60, len)    unsigned char* data);
void st20_chararray_out  (int len, out.array(60, len)   unsigned char* data);
void st40_chararray_out  (int len, out.array(60, len)   unsigned char* data);
void st20_chararray_inout(int len, inout.array(60, len) unsigned char* data);
void st40_chararray_inout(int len, inout.array(60, len) unsigned char* data);

/* complex types */
void st20_structarray_in   (int len, in.array(60, len)    struct_t* data);
void st40_structarray_in   (int len, in.array(60, len)    struct_t* data);
void st20_structarray_out  (int len, out.array(60, len)   struct_t* data);
void st40_structarray_out  (int len, out.array(60, len)   struct_t* data);
void st20_structarray_inout(int len, inout.array(60, len) struct_t* data);
void st40_structarray_inout(int len, inout.array(60, len) struct_t* data);

/* utilities */
void st20_runtests(void);
void st40_runtests(void);

arenas {
	{ slve, OS_FOR_SLAVE, CPU_FOR_SLAVE },
	{ mstr, OS_FOR_MASTER, CPU_FOR_MASTER }
};

transport {{ slve, mstr, TRANS_EMBX }};

headers {
	{ slve, { "\"vararray.h\"" } },
	{ mstr, { "\"vararray.h\"" } }
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
