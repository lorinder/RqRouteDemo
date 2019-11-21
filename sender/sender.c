#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include <rq_api.h>

#include "m3u8.h"
#include "packet_header.h"
#include "param_deriv.h"
#include "timeutil.h"

#include "cli.h"
#include "udp.h"

#define ALIGN 4096

int get_file_size(const char* fnam)
{
	FILE* fp = fopen(fnam, "rb");
	if (fp == NULL) {
		perror("fopen() in get_file_size()");
		return -1;
	}
	if (fseek(fp, 0, SEEK_END) < 0) {
		perror("fseek()");
		return -1;
	}
	long offs = ftell(fp);
	fclose(fp);
	return (int)offs;
}

bool xmit_file(const char* filename,
		int file_nbr,
		double overhead,
		udp* U)
{
	union {
		route_header header;
		char packetbuf[2048];
	} packet;

	route_header* header = &packet.header;
	void* packet_payload = &packet.packetbuf[sizeof(route_header)];
	size_t payload_maxsz = sizeof(packet.packetbuf) - sizeof(route_header);

	/* Get the parameters */
	int fsiz = get_file_size(filename);
	if (fsiz < 0)
		return false;
	if (fsiz == 0) {
		// XXX: Probably want to fix this.
		fprintf(stderr, "Error:  Can't handle files of size 0.\n");
		return false;
	}
	coding_params P;
	param_deriv(&P, fsiz);

	/* Allocate memory for the RaptorQ codec */
	const int nExtra = (int)ceil(P.K_large * overhead);
	size_t interWorkSize = 0, interProgSize = 0, interSymNum = 0;
	RqInterWorkMem* interWork = NULL;
	RqInterProgram* interProg = NULL;
	size_t interBlockSize = 0;
	void* interBlock = NULL;
	size_t outWorkSize = 0, outProgSize = 0;
	RqOutWorkMem* outWork = NULL;
	RqOutProgram* outProg = NULL;
	size_t srcBlockSize = 0;
	void* srcBlock = NULL;
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
	if (nExtra > 0) {
		RUN_NOFAIL(RqInterGetMemSizes(P.K_large,
						nExtra,
						&interWorkSize,
						&interProgSize,
						&interSymNum));
		interWork = malloc(interWorkSize);
		interProg = malloc(interProgSize);
		interBlockSize = P.T * interSymNum;
		interBlock = aligned_alloc(ALIGN, interBlockSize);

		RUN_NOFAIL(RqOutGetMemSizes(nExtra, &outWorkSize, &outProgSize));
		outWork = malloc(outWorkSize);
		outProg = malloc(outProgSize);

	}

	/* Alloc for the source block and repair symbols.
	 *
	 * We use the source block memory both to store the
	 * source block as well as repair symbols; for this
	 * reason it needs to be made large enough so it can
	 * keep all the repair symbols necessary.
	 */
	srcBlockSize = P.T
	  * (P.K_large > nExtra ? P.K_large : nExtra);
	srcBlock = aligned_alloc(ALIGN, srcBlockSize);

	/* Read code block by code block and send */
	FILE* fp = fopen(filename, "rb");
	for (int cb = 0; cb < P.n_cb; ++cb) {
		const int K = P.K_large - (cb >= P.n_large);

		/* Read the source block */
		size_t sz_read = fread(srcBlock, 1, P.T * K, fp);
		if (sz_read < P.T * K) {
			/* Zero pad the last packet */
			assert(cb == P.n_cb - 1);
			assert(P.T * (K - 1) < sz_read);
			memset(srcBlock + sz_read, 0, P.T * K - sz_read);
		}

		/* Transmit source symbols */
		for (int j = 0; j < K; ++j) {
			header->ens_id = ROUTE_ENS_ID_SOURCE;
			header->toi = file_nbr & 65535; /* modulo 2^16 */
			header->payload_id = j * P.T;
			header->tos = fsiz;

			/* Compute payload size */
			size_t payload_size = P.T;
			if (sz_read - P.T * j < payload_size)
				payload_size = sz_read - P.T * j;

			/* Copy payload into packet buffer */
			if (payload_size > payload_maxsz) {
				fprintf(stderr, "Error:  Packet payload "
				  "larger than supported.\n");
				return false;
			}
			memcpy(packet_payload,
				srcBlock + P.T * j,
				payload_size);

			/* xmit packet */
			size_t xmit_sz = \
			  sizeof(route_header) + payload_size;
			udp_sendmsg(U, xmit_sz, &packet);
		}

		/* Compute and transmit repair */
		if (nExtra > 0) {
			/* Generate programs if needed */
			if (cb == 0 || cb == P.n_large) {
				RUN_NOFAIL(RqInterInit(K, 0,
				  interWork, interWorkSize));
				RUN_NOFAIL(RqInterAddIds(
				  interWork, 0, K));
				RUN_NOFAIL(RqInterCompile(
				  interWork, interProg, interProgSize));
				RUN_NOFAIL(RqOutInit(K,
				  outWork, outWorkSize));
				RUN_NOFAIL(RqOutAddIds(outWork,
				  K, nExtra));
				RUN_NOFAIL(RqOutCompile(outWork,
				  outProg, outProgSize));
			}

			/* Compute repair symbols */
			RUN_NOFAIL(RqInterExecute(interProg,
				P.T, srcBlock, srcBlockSize,
				interBlock, interBlockSize));
			RUN_NOFAIL(RqOutExecute(outProg,
				P.T,
				interBlock,
				srcBlock,
				srcBlockSize));

			/* Send the repair symbols over the wire */
			for (int j = 0; j < nExtra; ++j) {
			  header->ens_id = ROUTE_ENS_ID_REPAIR;
			  header->toi = file_nbr & 65535; /* modulo 65535 */
			  header->payload_id = j * P.T;
			  header->tos = fsiz;

				const size_t payload_size = P.T;
				if (payload_size > payload_maxsz) {
					fprintf(stderr, "Error:  Packet payload "
					  "larger than supported.\n");
					return false;
				}
				memcpy(packet_payload,
					srcBlock + P.T * j,
					payload_size);

				const size_t xmit_sz
				  = sizeof(route_header) + payload_size;
				udp_sendmsg(U, xmit_sz, &packet);
			}
		}
	}

	/* Free resources */
#undef RUN_NOFAIL_internal
#undef RUN_NOFAIL
	fclose(fp);
	if (srcBlock) free(srcBlock);
	if (outProg) free(outProg);
	if (outWork) free(outWork);
	if (interBlock) free(interBlock);
	if (interProg) free(interProg);
	if (interWork) free(interWork);

	return true;
}

bool send_segments(const char* dirname,
		udp* U,
		double overhead)
{
	const size_t len_dirname = strlen(dirname);
	const size_t len_pbuf = 40;
	char* fnam_buf = malloc(len_dirname + len_pbuf);
	strcpy(fnam_buf, dirname);
	char* fnam_p = fnam_buf + len_dirname;
	*fnam_p++ = '/';

	/* Open the m3u8 */
	strcpy(fnam_p, "list.m3u8");
	printf("Opening m3u8 file: %s\n", fnam_buf);
	FILE* fpp = fopen(fnam_buf, "r");
	if (fpp == NULL) {
		perror("fopen(\"list.m3u8\")");
		return false;
	}

	/* Prepare CLI */
	if (cli_init() == -1)
		return false;
	cli_prompt();

	/* Process segment by segment */
	struct timespec t0;
	clock_gettime(CLOCK_REALTIME, &t0);
	double dur_accum = 0, d;
	int file_nbr = 0;
	while (m3u8_get_next_ent(fpp, &d, len_pbuf, fnam_p)) {
		bool xmit_ok = xmit_file(fnam_buf,
					file_nbr,
					overhead,
					U);
		if (!xmit_ok) {
			return false;
		}

		/* Update accumulated time,
		   wait if necessary. */
		dur_accum += d;
		do {
			struct timespec t1;
			clock_gettime(CLOCK_REALTIME, &t1);
			double delta = get_delta_seconds(&t1, &t0);
			if (delta >= dur_accum)
				break;

			/* Need to wait.
			 * Check for command prompt input while waiting.
			 */
			struct timespec wait_t;
			get_timespec_from_seconds(&wait_t,
						dur_accum - delta);

			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(0, &rfds);
			pselect(1, &rfds, NULL, NULL, &wait_t, NULL);
			if (FD_ISSET(0, &rfds)) {
				cli_commands(&overhead, U);
				cli_prompt();
			}
		} while (1);

		/* Advance file number */
		++file_nbr;
	}

	fclose(fpp);
	return true;
}

static void usage()
{
	puts(	"Sender --- Segment encoder and UDP sender.\n"
		"\n"
		"usage:\n"
		"  -h          display help and exit.\n"
		"  -d <dir>    set the segment directory.\n"
		"  -s #        set the rng seed.\n"
		"  -t #        maximum output rate [Mbps].\n"
		"  -p #        packet drop probability.\n"
		"  -o #        overhead as a fraction of K.\n"
		"  -r <ip>     receiver ip address \n"
	);
}

int main(int argc, char** argv)
{
	char* segment_dir = strdup("segments");
	udp_params P = {
		.throttle_speed = 15,
		.drop_prob = 0,
		.recv_addr = "127.0.0.1",
	};
	double overhead = 0;
	unsigned int rng_seed = time(0);

	/* Read command line args */
	int c;
	while ((c = getopt(argc, argv, "hd:s:t:p:o:r:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'd':
			free(segment_dir);
			segment_dir = strdup(optarg);
			break;
		case 's':
			rng_seed = (unsigned int)atol(optarg);
			break;
		case 't':
			P.throttle_speed = atof(optarg);
			break;
		case 'p':
			P.drop_prob = atof(optarg);
			break;
		case 'r':
		  strcpy (P.recv_addr, optarg);
			break;
		case 'o':
			overhead = atof(optarg);
			break;
		case '?':
			return 1;
		};
	}

	/* Seed RNG */
	srandom(rng_seed);

	/* Prepare UDP send */
	udp* U = udp_create(&P);
	if (U == NULL) {
		fprintf(stderr, "Error:  Could not open UDP socket.\n");
		return 1;
	}

	send_segments(segment_dir, U, overhead);

	if (U) udp_free(U);
	free(segment_dir);
	return 0;
}
