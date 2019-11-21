#ifndef SEGSTATESET_H
#define SEGSTATESET_H

#include <stdbool.h>

#include "segstate.h"

/* Maximum number of concurrent sets processed */
#define SEGSTATE_SET_N		4

typedef struct {
	int min_file_nbr;

	int first_index_in_array;
	bool states_initialized[SEGSTATE_SET_N];
	segstate states[SEGSTATE_SET_N];
} segstateset;

void segstateset_init(segstateset* S, int min_file_nbr);

int segstateset_get_min_file_nbr(segstateset* S);
void segstateset_inc_min_file_nbr(segstateset* S);

typedef enum {
	SS_OK			= 0,
	SS_UNINITIALIZED	= 1,
	SS_BELOW_MIN		= 2,
	SS_ABOVE_MAX		= 3,
} segstate_status;

segstate_status segstateset_get(segstateset* S,
			int file_nbr,
			segstate** p_ret);

segstate_status segstateset_mark_initialized(segstateset* S,
			int file_nbr);

void segstateset_mark_readytowrite(segstateset* S,
				   int file_nbr);

#endif /* SEGSTATESET_H */
