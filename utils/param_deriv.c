#include <assert.h>

#include "param_deriv.h"

#define PD_MAX_K	50000

static inline int ceildiv(int a, int b)
{
	return (a + b - 1) / b;
}

int param_deriv(coding_params* ret, int file_size)
{
	const int T = PD_T;

	const int n_srcsym_tot = ceildiv(file_size, PD_T);
	const int n_cb = ceildiv(n_srcsym_tot, PD_MAX_K);

	assert(n_cb == 1);

	const int K_large = ceildiv(n_srcsym_tot, n_cb);
	const int n_small = K_large*n_cb - n_srcsym_tot;
	const int n_large = n_cb - n_small;

	assert(1 <= n_large && n_large <= n_cb);
	assert(K_large*n_large + (K_large - 1)*(n_cb - n_large) == n_srcsym_tot);

	/* Assign to return struct */
	ret->T = T;
	ret->n_srcsym_tot = n_srcsym_tot;
	ret->n_cb = n_cb;
	ret->K_large = K_large;
	ret->n_large = n_large;

	return 0;
}
