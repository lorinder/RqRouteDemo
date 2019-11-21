#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "fd_ops.h"
#include "packet_header.h"
#include "timeutil.h"

#include "segment_list.h"
#include "segstate.h"
#include "segstateset.h"
#include "rq_memblocks.h"

int task_start_write (const segment_list* L, segstateset *SS, RqMemBlocks * codec_mem)
{
    int out_fd = -1;
    int file_nbr = segstateset_get_min_file_nbr(SS);
    segstate* S;
    segstate_status status = segstateset_get(SS, file_nbr, &S);

    if (status == SS_UNINITIALIZED || !(segstate_get_mode(S) == READY_TO_WRITE)) {
	printf("File %d: Not ready to write\n",
	       file_nbr);
	return -1;
    }

    out_fd = open(L->segments[file_nbr].file_name,
		  O_WRONLY|O_CREAT, 0666);
    if (out_fd < 0) {
	perror("open()");
	return -1;
    }
    make_nonblock(out_fd);
    if (status == SS_OK)
	segstate_prepare_output(S,
				out_fd,
				codec_mem);
    else if (status == SS_UNINITIALIZED) {
	/* In this case, we received no
	   packets whatsoever for the segment.
	*/
	printf("File %d:  Completely empty.\n",
	       file_nbr);
	close(out_fd);
	out_fd = -1;
	segstateset_inc_min_file_nbr(SS);
    }
    return out_fd;
}

int task_write (segstateset *SS, int out_fd, const segment_list* L, RqMemBlocks * codec_mem)
{

    int file_nbr = segstateset_get_min_file_nbr(SS);
    segstate* S;
    segstate_status status = segstateset_get(SS, file_nbr, &S);
    assert(status == SS_OK);
    (void)status;
    if (!segstate_write_more(S)) {
	printf("File nbr %d:  Finished writing.\n",
	       file_nbr);
	segstate_free(S);
	close(out_fd);
	out_fd = -1;
	segstateset_inc_min_file_nbr(SS);

	/* Check if the next segment is ready to write */
	out_fd = task_start_write (L, SS, codec_mem);
    } else {
	printf("Wrote some for file nbr %d\n", file_nbr);
    }
    return out_fd;
}


void receive_write(const segment_list* L,
			double deadline_delay,
			double extra_delay,
			int ready_fd)
{
	/* Create socket */
	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0) {
		perror("socket()");
		return;
	}

	/* Bind socket */
	struct sockaddr_in A;
	memset(&A, 0, sizeof(A));
	A.sin_family = AF_INET;
	A.sin_addr.s_addr = INADDR_ANY;
	A.sin_port = htons(6910);
	if (bind(sock_fd, (const struct sockaddr*)&A, sizeof(A)) < 0) {
		perror("bind()");
		return;
	}

	/* Prepare segment state set */
	segstateset SS;
	segstateset_init(&SS, 0);

	/* Output variables */
	int out_fd = -1;
	RqMemBlocks codec_mem = { 0 };

	/* Timeline & starting state */
	bool started = false;
	struct timespec start_time;

	/* First segment write started with extra delay */
	bool write_ok = false;

	/* File nbr of last packet received */
	int recv_file_nbr = -1;

	struct timespec dline;

	/* Processing loop */
	while (1) {

		/* Get seg nbr, check if done */
		int seg_nbr = segstateset_get_min_file_nbr(&SS);
		if (seg_nbr == L->n_segments) {
			printf("Done.\n");
			break;
		}

		/* Call select to figure out what to do next */
		enum {
			TASK_UNKNOWN = -1,
			TASK_RECV,
			TASK_WRITE,
			TASK_START_WRITE,
			TASK_SIGNAL_READY,
		} nexttask = -1;

		if (out_fd == -1) {
			fd_set rfds, wfds;
			FD_ZERO(&rfds);
			FD_SET(sock_fd, &rfds);
			FD_ZERO(&wfds);

			int rv;

			if (started && !write_ok) {
			    /* Compute deadline time for first segment write */
			    dline = start_time;
			    add_delta_to(&dline, deadline_delay);
			    if (seg_nbr == 0) {
				add_delta_to(&dline, extra_delay);
			    }

			    /* Find current time */
			    struct timespec now;
			    rv = clock_gettime(CLOCK_REALTIME, &now);
			    if (rv < 0) {
				perror("clock_gettime()");
				return;
			    }

			    /* Compute the difference -> timeout */
			    const double
				delta = get_delta_seconds(&dline, &now);
			    struct timespec timeout = { 0 };
			    if (delta > 0) {
				get_timespec_from_seconds(&timeout, delta);
			    }

			    rv = pselect(sock_fd + 1,
					 &rfds,
					 NULL,
					 NULL,
					 &timeout,
					 NULL);
			} else if (!started) {
			    /* Not received anything so far. Waiting for first recv */
			    int max_fd = sock_fd;
			    if (ready_fd > 0) {
				FD_SET(ready_fd, &wfds);
				if (ready_fd > sock_fd)
				    max_fd = ready_fd;
			    }
			    rv = pselect(max_fd + 1,
					 &rfds,
					 &wfds,
					 NULL,
					 NULL,
					 NULL);
			}
			else {
			    /* Nothing to write, just receiving data */
			    rv = pselect(sock_fd + 1,
					&rfds,
					NULL,
					NULL,
					NULL,
					NULL);
			}

			/* Check pselect() result */
			if (rv < 0) {
				perror("pselect()");
				return;
			}
			if (rv > 0) {
			    if (ready_fd != -1 && FD_ISSET(ready_fd, &wfds)) {
				nexttask = TASK_SIGNAL_READY;
			    } else {
				assert(FD_ISSET(sock_fd, &rfds));
				nexttask = TASK_RECV;
			    }
			} else {
			    /* First segment timeout */
			    nexttask = TASK_START_WRITE;
			    write_ok = true;
			}
		} else {
		    /* Reading and writing data */
		    fd_set rfds;
		    FD_ZERO(&rfds);
		    FD_SET(sock_fd, &rfds);
		    fd_set wfds;
		    FD_ZERO(&wfds);
		    FD_SET(out_fd, &wfds);
		    const int
			nfds = (out_fd > sock_fd ? out_fd : sock_fd) + 1;
		    int rv = pselect(nfds,
				     &rfds,
				     &wfds,
				     NULL,
				     NULL,
				     NULL);

		    /* Check result */
		    if (rv < 0) {
			perror("pselect()");
			return;
		    }
		    assert(rv == 1 || rv == 2);
		    if (FD_ISSET(sock_fd, &rfds)){
			nexttask = TASK_RECV;
		    }
		    else{
			nexttask = TASK_WRITE;
		    }
		}

		/* Now do what we decided */
		switch (nexttask) {
		case TASK_RECV: {
			union {
				char packetbuf[2048];
				route_header header;
			} packet;

			ssize_t rv = recv(sock_fd, &packet, sizeof(packet), 0);
			if (rv < 0) {
				perror("recv()");
				return;
			}
			assert(rv >= sizeof(route_header));

			segstate* S;
			const int file_nbr = packet.header.toi;
			segstate_status status =		\
			  segstateset_get(&SS, file_nbr, &S);
			switch (status) {
			case SS_UNINITIALIZED:
				if (!started) {
				    /* First recv - record start time */
				    started = true;
				    clock_gettime(CLOCK_REALTIME,
						  &start_time);
				}
				segstate_init(S,
					file_nbr,
					packet.header.tos);

				segstateset_mark_initialized(&SS, file_nbr);
				if (file_nbr > recv_file_nbr) {
				    recv_file_nbr = file_nbr;
				    /* Mark segstates below recv_file_nbr as
				       ready to write */
				    segstateset_mark_readytowrite(&SS, recv_file_nbr);
				    if (write_ok && out_fd == -1) {
					out_fd = task_start_write(L, &SS, &codec_mem);
				    }
				}

				/* fallthrough */
			case SS_OK:
				segstate_add_packet(S, &packet.header,
					rv - sizeof(route_header),
					packet.packetbuf +
					sizeof(route_header));
				break;
			case SS_BELOW_MIN:
				fprintf(stderr, "Error:  Received packet "
				  "for file %d past deadline (current "
				  "min: %d).\n",
				  file_nbr,
				  segstateset_get_min_file_nbr(&SS));
				break;
			case SS_ABOVE_MAX:
				fprintf(stderr, "Error:  Received packet "
				  "for file %d too far in future (current "
				  "min: %d).\n",
				  file_nbr,
				  segstateset_get_min_file_nbr(&SS));
				break;
			};

			break;
		}
		case TASK_WRITE: {
		    out_fd = task_write (&SS, out_fd, L, &codec_mem);
			break;
		}
		case TASK_START_WRITE: {
		    if (write_ok)
			out_fd = task_start_write (L, &SS, &codec_mem);
		    break;
		}
		case TASK_SIGNAL_READY: {
			/* Here we're asked to write something on the
			 * ready_fd descriptor for synchronization.
			 */
			assert(ready_fd >= 0);
			write(ready_fd, "x", 1);
			close(ready_fd);
			ready_fd = -1;
			break;
		}
		default:
			assert(0);
		}
	};

	/* Finish */
	RqMemBlocksFreeMem(&codec_mem);
}


static void usage()
{
	puts(	"Receiver --- UDP receiver and segment decoder.\n"
		"\n"
		"   -o <dir>     segment output directory.\n"
		"   -d #         deadline delay in seconds.\n"
		"   -x #         playout delay on top of deadline.\n"
		"   -r #         file descriptor to signal ready time on.\n"
	);
}

int main(int argc, char** argv)
{
	char* outdir = strdup("seg_recv");
	double deadline_delay = 1;
	double extra_delay = 3;
	int ready_fd = -1;

	/* Read command line args */
	int c;
	while ((c = getopt(argc, argv, "ho:d:x:r:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'o':
			free(outdir);
			outdir = strdup(optarg);
			break;
		case 'd':
			deadline_delay = atof(optarg);
			break;
		case 'x':
			extra_delay = atof(optarg);
			break;
		case 'r':
			ready_fd = atoi(optarg);
			break;
		case '?':
			return 1;
		};
	}

	/* Load the segment list */
	segment_list L;
	if (!segment_list_load(&L, outdir))
		return 1;
	printf("There are %d segments.\n", L.n_segments);
	puts("  Ready to receive...");

	/* Receive and write */
	receive_write(&L, deadline_delay, extra_delay, ready_fd);

	segment_list_free(&L);
	free(outdir);
	return 0;
}
