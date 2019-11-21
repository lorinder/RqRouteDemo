#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "param_deriv.h"
#include "segstate.h"

#define ALIGN 4096

void segstate_init(segstate* S, int file_nbr, size_t file_len)
{
	/* Set file parameters and code parameters */
	S->file_nbr = file_nbr;
	S->file_len = file_len;
	if (param_deriv(&S->P, file_len) != 0) {
		fprintf(stderr, "%s:%d:  param_deriv() failed.\n",
		  __FILE__, __LINE__);
		exit(1);
	}

	/* Initialize cb states */
	S->cbstates = malloc(S->P.n_cb * sizeof(codeblockstate));
	int i;
	for (i = 0; i < S->P.n_large; ++i)
		codeblockstate_init(&S->cbstates[i], S->P.K_large, S->P.T);
	for (; i < S->P.n_cb; ++i)
		codeblockstate_init(&S->cbstates[i], S->P.K_large - 1, S->P.T);

	S->mode = RECEIVING;

	/* Output state is not yet prepared */
	S->out_fd = -1;
	S->out_cb = -1;
	S->out_cb_buf = NULL;
	S->out_cb_buf_sz = 0;
	S->out_cb_buf_offs = 0;
	S->codec_mem = NULL;
}

void segstate_free(segstate* S)
{
	if (S->out_cb_buf) {
		free(S->out_cb_buf);
		S->out_cb_buf = NULL;
	}
	if (S->cbstates) {
		for (int i = 0; i < S->P.n_cb; ++i)
			codeblockstate_free(&S->cbstates[i]);
		free(S->cbstates);
		S->cbstates = NULL;
	}
}

void segstate_add_packet(segstate* S,
			const route_header* header,
			size_t payload_size,
			const void* payload)
{
	assert(header->toi == S->file_nbr);
	assert(header->tos == S->file_len);
	assert(S->P.n_cb == 1);
	assert(0 <= header->payload_id);
	assert(S->mode == RECEIVING);

	int32_t symb_id;

	if (header->ens_id == ROUTE_ENS_ID_SOURCE) {
	  symb_id = header->payload_id/S->P.T;
	}
	else {
	  symb_id = S->P.K_large + header->payload_id/S->P.T;
	}

	/* Insert symbol */
	codeblockstate_add_sym(&S->cbstates[0], /* There is just 1 code block */
				symb_id,
				payload_size,
				payload);
}

static void load_cb_buf(segstate* S)
{
	codeblockstate* C = &S->cbstates[S->out_cb];
	codeblockstate_get_data(C,
			S->out_cb_buf_sz,
			S->out_cb_buf,
			S->codec_mem);
}

void segstate_ready_for_write(segstate* S)
{
    S->mode = READY_TO_WRITE;
}

segstate_mode segstate_get_mode(segstate* S)
{
    return S->mode;
}

void segstate_prepare_output(segstate* S,
			int out_fd,
			RqMemBlocks* codec_mem)
{
	assert(S->out_fd == -1);
	S->mode = WRITING;
	S->out_fd = out_fd;
	S->out_cb = 0;
	S->out_cb_buf_sz = S->P.T * S->P.K_large;
	S->out_cb_buf = aligned_alloc(ALIGN, S->out_cb_buf_sz);
	S->out_cb_buf_offs = 0;
	S->codec_mem = codec_mem;
	load_cb_buf(S);
}

bool segstate_write_more(segstate* S)
{
	assert(S->out_fd != -1);
	assert(S->mode == WRITING);

	/* Check if we're done */
	if (S->out_cb == S->P.n_cb)
		return false;

	/* Compute data size of current buffer */
	const size_t T = S->P.T;
	size_t n_pad_bytes = 0;
	if (S->out_cb == S->P.n_cb - 1) {
		/* The last CB has a particular size, since we need to
		 * remove the padding of the last symbol.
		 */
		n_pad_bytes = (S->P.n_srcsym_tot * T) - S->file_len;
		assert(n_pad_bytes < T);
	}
	size_t cb_size = T*(S->P.K_large - (S->out_cb >= S->P.n_large)) - n_pad_bytes;

	/* Write from the current CB */
	assert(S->out_cb_buf_offs < cb_size);
	ssize_t n_write = write(S->out_fd,
				S->out_cb_buf + S->out_cb_buf_offs,
				cb_size - S->out_cb_buf_offs);
	if (n_write < 0) {
		perror("write()");
		return false;
	}
	S->out_cb_buf_offs += n_write;

	/* Load new cb if we reached the end */
	if (S->out_cb_buf_offs == cb_size) {
		S->out_cb_buf_offs = 0;
		++S->out_cb;
		if (S->out_cb == S->P.n_cb) {
			/* Last CB done, finished. */
			return false;
		}

		load_cb_buf(S);
	}

	/* further writes are necessary */
	return true;
}
