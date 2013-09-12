/*
 * union.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Example showing how to copy a union as a binary object.
 */

/* In this example we use the rpc_info {} construct to force strpcgen
 * to treat a union as a structure containing a character array. This
 * basically treats the union as an opaque binary object.
 *
 * This approach relies on the compilers on each CPU packing the union
 * into an identical binary format. THIS IS NOT GUARANTEED TO BE THE
 * CASE. Before applying this approach the user should first check
 * that their compilers pack unions in the same way. The check must be
 * repeated after every major compiler upgrade.
 */

/* this is the union we want to copy. normally it contains two members
 * id and jtag. Because neither of these members are arrays we have had
 * to add a special member to assist RPC, _binobj. Although any array
 * type can be used in this case we have picked a single character array
 * since this will not impact the size of the union.
 */
typedef union {
    int id;

    struct {
        unsigned int JTAG_bit:1;
        unsigned int manufacturer:11;
        unsigned int device_code:16;
        unsigned int revision:4;
    } jtag;

    char _binobj[1];
} jtagcode_t;

/* by re-defining jtagcode_t in the RPC info section we cause it to be
 * treated differently by strpcgen but new definition will be not be
 * presented to the C compiler because the rpc_info section is removed by
 * the stripper. the redefinition of the type will cause both the stripper
 * and the generator to produce a harmless warning about redefined types. 
 * in fact in this case we are using the __STRPC__ macro (defined 
 * automatically by rpccc) to suppress that warning when running the code
 * through the stripper.
 *
 * to copy the union we are using an array strategy using sizeof. sizeof
 * will be evaluated by the C compiler in the generated code and will
 * use the original definition of jtagcode_t for this purpose. in the
 * rpc_info definition of jtagcode_t it is *vital* that the array is of
 * character type however it is not required that the array type match
 * the original type.
 *
 * finally note that if jtagcode_t is never passed across RPC by reference
 * it would be possible to optimise the inout.array into in.array to
 * eliminate a redundant copyback.
 */
rpc_info {
#ifndef __STRPC__
	typedef struct { 
		inout.array(sizeof(jtagcode_t)) char _binobj[1];
	} jtagcode_t;
#endif
};

/* an example function using the union */
void decodeJTAG(jtagcode_t, 
                out unsigned int *, out unsigned int *, out unsigned int *);

import {{ decodeJTAG, cllr, clle }};

