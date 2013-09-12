/*
 * shakedown/roundtrip/master.c
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * ST20 side of the simple primative types shakedown test.
 */

#include "str.h"

int main()
{
	harness_boot();

	st20_run_tests();
	st40_run_tests();

	harness_passed();
	return 0;
}

void st20_run_tests()
{
	int i, j;
	char c;
	char string1[MAX_LENGTH*4];
	char string2[MAX_LENGTH*4];
	char *bang;

	VERBOSE(printf("st20_run_tests()\n"));

	for (i=0; i<MAX_LENGTH; i+=5) {
		/* populate the arrays */
		for (j=0, c='a'; j<(i-1); j++, c++) {
			string1[j] = string2[j] = c;
		}
		string1[j] = string2[j] = '\0';

		/* do the reverces */
		st20_strrev(string1);
		st40_strrev(string2);

		/* ensure the two agree */
		assert(0 == strcmp(string1, string2));
	}

	/* send a large string in the hope it will overflow something and crash the
	 * other system if the overflow checks are broken
	 */
	bang = malloc(4096);
	assert(NULL != bang);
	memset(bang, '#', 4096);
	bang[4095] = '\0';
	st40_strbang(bang);
	free(bang);
}
