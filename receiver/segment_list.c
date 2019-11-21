#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m3u8.h"
#include "segment_list.h"

bool segment_list_load(segment_list* L, const char* directory)
{
	size_t maxlen_fn = 40;
	size_t l_directory = strlen(directory);

	char fullpath[l_directory + maxlen_fn + 2];
	strcpy(fullpath, directory);
	fullpath[l_directory] = '/';
	char* relative_name = &fullpath[l_directory + 1];

	/* Open m3u8 list */
	strcpy(relative_name, "dummy_playlist.m3u8");
	FILE* fp = fopen(fullpath, "r");
	if (fp == NULL) {
		perror("fopen()");
		return false;
	}

	/* read entries */
	int arrsz = 16;
	L->segments = malloc(arrsz * sizeof(*L->segments));
	L->n_segments = 0;
	while (dummy_m3u8_get_next_ent(fp, maxlen_fn, relative_name)) {
		L->segments[L->n_segments].file_name = strdup(fullpath);

		++L->n_segments;
		if (L->n_segments == arrsz) {
			arrsz *= 2;
			L->segments = realloc(L->segments,
					arrsz * sizeof(*L->segments));
		}
	}

	fclose(fp);
	return true;
}

void segment_list_free(segment_list* L)
{
	if (L->segments) {
		for (int i = 0; i < L->n_segments; ++i)
			free(L->segments[i].file_name);
		free(L->segments);
	}
}
