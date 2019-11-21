#ifndef UDP_H
#define UDP_H

/**	@file udp.h
 *
 *	Simple UDP sender.
 */

#include <arpa/inet.h>

struct udp_T;
typedef struct udp_T udp;

typedef struct {
	double throttle_speed;
	double drop_prob;
        char recv_addr[INET_ADDRSTRLEN];
} udp_params;

udp* udp_create(const udp_params* params);
void udp_free(udp* sender);

void udp_sendmsg(udp* sender, size_t msg_size, void* msg);

void udp_get_params(udp* sender, udp_params* params);
void udp_set_params(udp* sender, const udp_params* params);

#endif /* UDP_H */
