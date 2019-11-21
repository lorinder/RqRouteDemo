#ifndef SEGMENT_LIST_H
#define SEGMENT_LIST_H

#include <stdbool.h>

struct segment {
	char* file_name;	// The file name of this segment
};

typedef struct {
	int n_segments;
	struct segment* segments;
} segment_list;

bool segment_list_load(segment_list* L, const char* directory);
void segment_list_free(segment_list* L);

#endif /* SEGMENT_LIST_H */
