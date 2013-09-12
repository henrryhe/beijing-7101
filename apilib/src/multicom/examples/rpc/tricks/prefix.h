/*
 * prefix.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Example showing how to copy a length prefixed array.
 */

/* we can pass length prefixed arrays easily once we realize that,
 * when using the termarray strategy, (&(__element__) - __count__) is 
 * a pointer to the first element of the array (i.e. the prefix).
 *
 * note that if the system were being designed from scratch it would
 * better to implement a length prefixed array in a form that can be
 * directly passed by RPC:
 *
 *   struct prefixed_array {
 *       char len;
 *       in.array(255, __struct__.len) char c[255];
 *   };
 */

void printLengthPrefixed(
	in.termarray(256, *(&(__element__) - __count__) <= __count__) char *c);
import {{ printLengthPrefixed, cllr, clle }};

