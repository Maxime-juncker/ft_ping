#ifndef PING_H 
#define PING_H

#include <stdint.h>
#include <arpa/inet.h>

#include "options.h"

typedef struct s_stats
{
	size_t	packet_sent;
	size_t	packet_received;

	float	total_time;
	int		number_of_ping;
	float	total_time_sqr;

	float	min;
	float	max;
	float	stddev;
	
} t_stats;

typedef struct s_connection_info
{
	char*				ip;
	char*				name;
	int					pid;
	int					socketfd;

	struct addrinfo*	addrinfo;
	struct sockaddr_in	addr;
	struct icmp*		icmp;

	struct icmp*		response_icmp;
	int					is_dup;
	
	char*				packet;
	unsigned int		packet_ttl;
	ssize_t				bytes;

	t_option*			options;
	t_stats				stats;

	char*				buffer_output;

	long				timer;

} t_connection_info;

extern int stop;

void sig_handler(int signal);
long	get_current_time_micro(void);

void ping_shutdown(t_connection_info* infos);
void cleanup_infos(t_connection_info* infos);
uint16_t calculate_checksum(void *b, int len);

char	*ft_strjoin(char *s1, char const *s2);

#endif
