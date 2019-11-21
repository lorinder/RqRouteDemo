#ifndef SEGSTATE_H
#define SEGSTATE_H

/**	@file segstate.h
 *
 *	Segment state.
 */

#include <stdbool.h>

#include "codeblockstate.h"
#include "param_deriv.h"
#include "packet_header.h"
#include "rq_memblocks.h"

typedef enum {RECEIVING, READY_TO_WRITE, WRITING} segstate_mode;

/* segment state */
struct segstate_T {
	int file_nbr;
	size_t file_len;

        segstate_mode mode;
	coding_params P;
	codeblockstate *cbstates;

	/* Output related state */
	int out_fd;		// output file descriptor
	int out_cb;		// current codeblock ID being output
	char* out_cb_buf;	// output buffer for the codeblock.
	size_t out_cb_buf_sz;	// its size.
	size_t out_cb_buf_offs;	// output write position
	RqMemBlocks* codec_mem;	// memory blocks for the decoder (borrowed)
};

typedef struct segstate_T segstate;

void segstate_init(segstate* S, int file_nbr, size_t file_len);
void segstate_free(segstate* S);

void segstate_add_packet(segstate* S,
			const route_header* header,
			size_t payload_size,
			const void* payload);

void segstate_prepare_output(segstate* S,
			int out_fd,
			RqMemBlocks* codec_mem);
bool segstate_write_more(segstate* S);

void segstate_ready_for_write(segstate* S);
segstate_mode segstate_get_mode(segstate* S);
#endif /* SEGSTATE_H */
