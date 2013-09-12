/*
 * shakedown/string/util.c
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 *
 */

#include "str.h"

#ifdef CONF_MASTER
void st20_strrev(char *str)
#else
void st40_strrev(char *str)
#endif
{
        int i, len;
        char tmp;

#ifdef CONF_MASTER
	VERBOSE(printf("st20_strrev(%s)\n", str));
#else
	VERBOSE(printf("st40_strrev(%s)\n", str));
#endif

        len = strlen(str) - 1;
        
        for (i=0; i<(len/2); i++) {
                tmp        = str[i];
                str[i]     = str[len
		-i];
                str[len-i] = tmp;
        }

	VERBOSE(printf("           (%s)\n", str));
}

#ifdef CONF_MASTER
void st20_strbang(char *str)
#else
void st40_strbang(char *str)
#endif
{
#ifdef CONF_MASTER
	VERBOSE(printf("st20_strbang(...)\n"));
#else
	VERBOSE(printf("st40_strbang(...)\n"));
#endif
}

