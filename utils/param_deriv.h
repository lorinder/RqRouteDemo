#ifndef PARAM_DERIV_H
#define PARAM_DERIV_H

/* Constant symbol size used in all parametrizations */
#define PD_T			1024

struct coding_params_T {
	int T;			/* Symbol size in bytes */
	int n_srcsym_tot;	/* Total number of source symbols in
	                           aggregate over all code blocks */
	int n_cb;		/* Number of codeblocks */
	int K_large;		/* The larger of the K values
				   (The smaller value will be
				   K_large - 1) */
	int n_large;		/* The number of larger code blocks */
};

typedef struct coding_params_T coding_params;

/**	Compute coding parameters from file size.
 */
int param_deriv(coding_params* ret, int file_size);

#endif /* PARAM_DERIV_H */
