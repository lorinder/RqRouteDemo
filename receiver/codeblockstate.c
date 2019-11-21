#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codeblockstate.h"

#define ALIGN		4096

/* Multiplier for hashing */
#define H_MULT		29

void codeblockstate_init(codeblockstate* S,
			int K, int T)
{
	/* Code params */
	S->K = K;
	S->T = T;

	/* ESI array */
	S->n_esi_max = K + 30;
	S->n_esi = 0;
	S->esi = malloc(sizeof(*S->esi) * S->n_esi_max);
	S->n_src_esi = 0;

	/* Create hash table */
	S->hash_esi_sz =  S->n_esi_max * 5 / 2;
	if (S->hash_esi_sz % H_MULT == 0)
		++S->hash_esi_sz;
	S->hash_esi = malloc(S->hash_esi_sz * sizeof(*S->hash_esi));
	for (int i = 0; i < S->hash_esi_sz; ++i)
		S->hash_esi[i] = -1;

	/* Allocate symbol buffer */
	S->syms_buf = aligned_alloc(ALIGN, S->T * S->n_esi_max);
}

void codeblockstate_add_sym(codeblockstate* S,
			int32_t sym_id,
			size_t payload_size,
			const void* payload)
{
	assert(payload_size <= S->T);

	/* Silently skip this symbol if the buffer is full */
	if (S->n_esi >= S->n_esi_max)
		return;

	/* Insert in hash table, discard if duplicate */
	int hent = (sym_id * H_MULT) % S->hash_esi_sz;
	while (S->hash_esi[hent] != -1) {
		if (S->hash_esi[hent] == sym_id) {
			/* Symbol already received, discard */
			printf("Info:  Received duplicate "
			  "symbol, ESI=%d. Discarding.\n", sym_id);
			return;
		}

		/* Next slot */
		if (++hent == S->hash_esi_sz)
			hent = 0;
	}
	S->hash_esi[hent] = sym_id;

	/* Insert ESI in array */
	S->esi[S->n_esi] = sym_id;
	const size_t offs = S->T * S->n_esi;
	memcpy(offs + S->syms_buf, payload, payload_size);
	if (payload_size < S->T) {
		/* Zero pad if the symbol does not extend to the end */
		memset(payload_size + offs + S->syms_buf, 0, S->T - payload_size);
	}

	/* Update counters */
	++S->n_esi;
	if (sym_id < S->K)
		++S->n_src_esi;
}

static void copy_source_syms(codeblockstate* S, void* data_out)
{
	const size_t T = S->T;
	for (int i = 0; i < S->n_esi; ++i) {
		const int esi = S->esi[i];
		if (esi >= S->K)
			continue;
		memcpy(esi * T + (char*)data_out,
			i * T + S->syms_buf,
			T);
	}
}

// XXX: Error handling & stats collection.
bool codeblockstate_get_data(codeblockstate* S,
			size_t data_out_sz,
			void* data_out,
			RqMemBlocks* codec_mem)
{
	const size_t T = S->T;
	assert(data_out_sz >= S->K * T);

	memset(data_out, 0, S->K * T);
	if (S->n_src_esi >= S->K || S->n_esi < S->K) {
		/* If we have all the source symbols, or not enough
		 * source and repair to attempt decoding, we just copy
		 * over the symbols.
		 */
		if (S->n_esi < S->K) {
			fprintf(stderr, "Error:  Not enough "
			  "symbols received for decoding.\n");
		}
		copy_source_syms(S, data_out);
	} else {
		/* Attempt to decode */
		int rc = RqMemBlocksIncludeCase(codec_mem,
					S->K,
					S->n_esi - S->K,
					S->K,
					T);
		if (rc < 0) {
			fprintf(stderr, "Error: "
			  "RqMemBlocksIncludeCase() failed.\n");
			return false;
		}

#define RUN_NOFAIL(x)	RUN_NOFAIL_internal(x, RUN_NOFAIL_internal_rc)
#define RUN_NOFAIL_internal(x, rc) \
		do { \
			int rc; \
			if ((rc = (x)) < 0) { \
				fprintf(stderr, "Error:%s:%d: %s " \
				  "failed, error %d.\n", \
				  __FILE__, __LINE__, #x, rc); \
				return false; \
			} \
		} while (0)

		/* Generate the intermediate block */
		RUN_NOFAIL(RqInterInit(S->K,
					S->n_esi - S->K,
					codec_mem->pInterWorkMem,
					codec_mem->nInterWorkMemSize));
		for (int j = 0; j < S->n_esi; ++j) {
			RUN_NOFAIL(RqInterAddIds(codec_mem->pInterWorkMem,
					S->esi[j],
					1));
		}
		rc = RqInterCompile(codec_mem->pInterWorkMem,
					codec_mem->pInterProgMem,
					codec_mem->nInterProgMemSize);
		if (rc == RQ_ERR_INSUFF_IDS) {
			fprintf(stderr, "Error:  Decoder failed.\n");
			copy_source_syms(S, data_out);
			return false;
		} else if (rc != 0) {
			fprintf(stderr, "Error: RqInterCompile error: %d\n",
			  rc);
			return false;
		}
		RUN_NOFAIL(RqInterExecute(codec_mem->pInterProgMem,
					T,
					S->syms_buf,
					T * S->n_esi_max,
					codec_mem->pInterSymMem,
					codec_mem->nInterSymMemSize));

		/* Reconstruct source block */
		RUN_NOFAIL(RqOutInit(S->K,
				codec_mem->pOutWorkMem,
				codec_mem->nOutWorkMemSize));
		RUN_NOFAIL(RqOutAddIds(codec_mem->pOutWorkMem, 0, S->K));
		RUN_NOFAIL(RqOutCompile(codec_mem->pOutWorkMem,
				codec_mem->pOutProgMem,
				codec_mem->nOutProgMemSize));
		RUN_NOFAIL(RqOutExecute(codec_mem->pOutProgMem,
				T,
				codec_mem->pInterSymMem,
				data_out,
				data_out_sz));
		printf("Successful decoding.\n");
		return true;
#undef RUN_NOFAIL
	}
	return (S->n_src_esi >= S->K);
}

void codeblockstate_free(codeblockstate* S)
{
	free(S->syms_buf);
	free(S->hash_esi);
	free(S->esi);
}
