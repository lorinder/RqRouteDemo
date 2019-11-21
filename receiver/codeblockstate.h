#ifndef CODEBLOCKSTATE_H
#define CODEBLOCKSTATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <rq_memblocks.h>

typedef struct {
	/* Code parameters */
	int K;
	int T;

	/* Array of ESIs received */
	int n_esi_max;
	int n_esi;
	int* esi;

	/* Number of source symbols received */
	int n_src_esi;

	/* Hashing table of ESIs received */
	int hash_esi_sz;
	int* hash_esi;

	/* Symbol data
	   (same order as ESI array) */
	char* syms_buf;
} codeblockstate;

void codeblockstate_init(codeblockstate* S, int K, int T);
void codeblockstate_add_sym(codeblockstate* S,
				int32_t sym_id,
				size_t payload_size,
				const void* payload);
bool codeblockstate_get_data(codeblockstate* S,
			size_t data_out_sz,
			void* data_out,
			RqMemBlocks* codec_mem);
void codeblockstate_free(codeblockstate* S);

#endif /* CODEBLOCKSTATE_H */
