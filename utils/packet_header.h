#ifndef PACKET_HEADER_H
#define PACKET_HEADER_H

#include <inttypes.h>

#define ROUTE_ENS_ID_SOURCE 0
#define ROUTE_ENS_ID_REPAIR 1

struct route_header_T {
	uint16_t	ens_id;	        /* Ensemble id: 0 for source, 1 for repair */
	uint16_t	toi;	        /* Transmission object identifier */
	uint32_t	payload_id;	/* Byte offset into source or repair block */
	uint32_t	tos;	        /* Transport object size - source block size */
};

typedef struct route_header_T route_header;

#endif /* PACKET_HEADER_H */
