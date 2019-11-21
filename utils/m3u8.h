#ifndef M3U8_H
#define M3U8_H

/**	@file m3u8.h
 *
 *	Simple parser for m3u8 files.
 */

#include <stdbool.h>
#include <stdio.h>

bool m3u8_get_next_ent(FILE* fp,
			double* duration_ret,
			size_t filename_arrsize,
			char* filename_ret);

bool dummy_m3u8_get_next_ent(FILE* fp,
			     size_t filename_arrsize,
			     char* filename_ret);

#endif /* M3U8_H */
