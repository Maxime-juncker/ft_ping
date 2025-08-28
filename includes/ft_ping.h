#ifndef PING_H 
#define PING_H

#include <stdint.h>
#include <sys/socket.h>

#include "options.h"


typedef struct s_connection_info
{
	char*				ip;
	char*				name;
	struct addrinfo*	addrinfo;
	char				packet[56];
	struct sockaddr_in	addr;
	int					socketfd;

	struct icmp*		icmp;

	size_t				packet_sent;
	size_t				packet_received;

	t_option*			options;

} t_connection_info;



uint16_t calculate_checksum(void *b, int len);


#endif
