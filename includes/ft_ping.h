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
	
	unsigned char*		packet;
	unsigned int		packet_ttl;
	ssize_t				bytes;

	t_option*			options;
	t_stats				stats;

	char*				buffer_output;

	long				begin_time;
	float				curr_time;

} t_connection_info;

struct ip_timestamp_option
{
	uint8_t code;
	uint8_t length;
	uint8_t pointer;
	uint8_t flags;
	uint32_t data[9];
};

extern int stop;

void sig_handler(int signal);
long	get_current_time_micro(void);

void cleanup_infos(t_connection_info* infos);
uint16_t calculate_checksum(void *b, int len);

char	*ft_strjoin(char *s1, char const *s2);

// packet.c
void	get_new_packet(t_connection_info* info);
int		create_socket(t_connection_info *info);
int		receive_packet(t_connection_info* infos);

// ping_utils.c
float				update_stats(t_connection_info* infos, long before); 
void				cleanup_infos(t_connection_info* infos);
struct addrinfo*	getAddrIP(const char* name, t_connection_info* info);

// ping.c
void	ping_loop(t_connection_info* infos);
int		init(t_connection_info* infos, int argc, char* argv[]);
void	ping_shutdown(t_connection_info* infos);
void wait_interval(t_connection_info* infos);

#endif
