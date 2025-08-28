#ifndef PING_H 
#define PING_H

#include <stdint.h>
#include <sys/socket.h>

#include "options.h"

typedef struct s_stats
{
	size_t	packet_sent;
	size_t	packet_received;

	float	total_time;
	int		number_of_ping;

	float	min;
	float	max;
	float	stddev;
	
} t_stats;

typedef struct s_connection_info
{
	char*				ip;
	char*				name;
	int					socketfd;

	struct addrinfo*	addrinfo;
	struct sockaddr_in	addr;
	struct icmp*		icmp;
	
	char				packet[56];
	unsigned int		packet_ttl;

	t_option*			options;
	t_stats				stats;

} t_connection_info;



uint16_t calculate_checksum(void *b, int len);


#endif
