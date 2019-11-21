#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m3u8.h"

bool m3u8_get_next_ent(FILE* fp,
			double* duration_ret,
			size_t filename_arrsize,
			char* filename_ret)
{
	/* Read lines until a non-comment line is found */
	do {
		char* r = fgets(filename_ret, filename_arrsize, fp);
		if (r == NULL)
			return false;
		if (*r == '\0')
			continue;

		/* Check for and parse EXTINF */
		const char* extinf = "#EXTINF:";
		const size_t l_extinf = strlen(extinf);
		if (strncmp(r, extinf, l_extinf) == 0) {
			const int
			  c = sscanf(r + l_extinf, "%lg,", duration_ret);
			if (c != 1) {
				fprintf(stderr, "Error:  Parsing EXTINF"
				 " failed.\n");
				return false;
			}
		}
	} while (*filename_ret == '#');

	/* Chop the ending newline off the filename */
	size_t l = strlen(filename_ret);
	assert(l > 0);
	if (filename_ret[l - 1] == '\n')
		filename_ret[l - 1] = '\0';
	return true;
}

bool dummy_m3u8_get_next_ent(FILE* fp,
			     size_t filename_arrsize,
			     char* filename_ret)
{
	/* Read lines until a non-comment line is found */
	do {
		char* r = fgets(filename_ret, filename_arrsize, fp);
		if (r == NULL)
			return false;
		if (*r == '\0')
			continue;
		
	} while (*filename_ret == '#');

	/* Chop the ending newline off the filename */
	size_t l = strlen(filename_ret);
	assert(l > 0);
	if (filename_ret[l - 1] == '\n')
		filename_ret[l - 1] = '\0';
	return true;
}
