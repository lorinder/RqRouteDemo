#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "timeutil.h"
#include "udp.h"

struct udp_T {
	int sock_fd;
	struct sockaddr_in addr;

	udp_params P;

	/* Speed throttling */
	size_t throttle_nbytes;		/* number of packets already
					   received in this burst */
	struct timespec throttle_start;	/* Start time of this burst
					   (only valid if
					   throttle_nbytes > 0) */
};

udp* udp_create(const udp_params* params)
{
	udp* U = malloc(sizeof(udp));

	U->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (U->sock_fd < 0) {
		perror("socket()");
		free(U);
		return NULL;
	}

	memset(&U->addr, 0, sizeof(U->addr));

	/* Set parameters */
	U->P = *params;

	printf ("Receiver address: %s\n", U->P.recv_addr);
	U->addr.sin_family = AF_INET;
	U->addr.sin_addr.s_addr = inet_addr(U->P.recv_addr);
	U->addr.sin_port = htons(6910);
	
	/* Throttle */
	U->throttle_nbytes = 0;

	return U;
}

void udp_free(udp* U)
{
	close(U->sock_fd);
	free(U);
}

void udp_sendmsg(udp* U, size_t msg_size, void* msg)
{
	/* Send packet unless dropped */
	if (U->P.drop_prob == 0 || random()/(RAND_MAX + 1.0) >= U->P.drop_prob)
	{
		sendto(U->sock_fd, msg, msg_size, 0,
			(const struct sockaddr*)&U->addr, sizeof(U->addr));
	}

	/* Throttle */
	if (U->P.throttle_speed > 0) {
		/* Update structures */
		if (U->throttle_nbytes == 0) {
			clock_gettime(CLOCK_REALTIME, &U->throttle_start);
		}
		U->throttle_nbytes += msg_size;

		/* Throttle if burst is complete */
		if (U->throttle_nbytes >= 32768) {
			struct timespec now;
			clock_gettime(CLOCK_REALTIME, &now);
			const double delta = get_delta_seconds(&now,
						&U->throttle_start);
			const double Mbits_sent
			  = U->throttle_nbytes * 8. / 1000000;
			const double time_needed
			  = Mbits_sent / U->P.throttle_speed;
			if (time_needed > delta) {
				sleep_frac(time_needed - delta);
			}

			/* Reset counts for next burst */
			U->throttle_nbytes = 0;
		}
	}
}

void udp_get_params(udp* sender, udp_params* params)
{
	*params = sender->P;
}

void udp_set_params(udp* sender, const udp_params* params)
{
	sender->P = *params;
}
