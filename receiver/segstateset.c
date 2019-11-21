#include <assert.h>
#include "segstateset.h"

void segstateset_init(segstateset* S, int min_file_nbr)
{
	segstateset R = {
		.min_file_nbr = min_file_nbr,
		.first_index_in_array = 0,
		.states_initialized = { 0 }
	};
	*S = R;
}

int segstateset_get_min_file_nbr(segstateset* S)
{
	return S->min_file_nbr;
}

void segstateset_inc_min_file_nbr(segstateset* S)
{
	S->states_initialized[S->first_index_in_array] = 0;
	++S->first_index_in_array;
	if (S->first_index_in_array == SEGSTATE_SET_N)
		S->first_index_in_array = 0;
	++S->min_file_nbr;
}

static segstate_status
  get_internal_index(segstateset* S, int file_nbr, int* idx)
{
	if (file_nbr < S->min_file_nbr) {
		return SS_BELOW_MIN;
	} else if (file_nbr >= S->min_file_nbr + SEGSTATE_SET_N) {
		return SS_ABOVE_MAX;
	}
	*idx = (file_nbr - S->min_file_nbr + S->first_index_in_array)
			% SEGSTATE_SET_N;
	return (S->states_initialized[*idx] ? SS_OK : SS_UNINITIALIZED);
}

segstate_status segstateset_get(segstateset* S,
				int file_nbr,
				segstate** p_ret)
{
	int idx;
	segstate_status status = get_internal_index(S, file_nbr, &idx);
	*p_ret = (status <= 1 ? &S->states[idx] : NULL);
	return status;
}

segstate_status segstateset_mark_initialized(
				segstateset* S,
				int file_nbr)
{
	int idx;
	segstate_status status = get_internal_index(S, file_nbr, &idx);
	/* Shouldn't mark a segment initialized if it already was. */
	assert(status != SS_OK);
	if (status >= 2)
		return status;
	S->states_initialized[idx] = 1;
	return SS_OK;
}

void segstateset_mark_readytowrite(segstateset* S,
				   int file_nbr)
{
    for (int i = 0; i < SEGSTATE_SET_N; i++)
	{
	    /* Mark all segments below file_nbr ready to write */
	    if (S->states_initialized[i] &&
		segstate_get_mode(&S->states[i]) == RECEIVING
		&& S->states[i].file_nbr < file_nbr)
		{
		    segstate_ready_for_write(&S->states[i]);
		}
	}
    return;
}
