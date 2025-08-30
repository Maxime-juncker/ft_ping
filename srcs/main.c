#include <arpa/inet.h>
#include <math.h>
#include <bits/types/struct_timeval.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

#include "options.h"
#include "text.h"
#include "ft_ping.h"

int stop = 0;

void sig_handler(int signal)
{
	if (signal == SIGINT)
	{
		stop = SIGINT;
	}
}


long	get_current_time_ms(void)
{
	long long		time;
	struct timeval	
	tv;

	gettimeofday(&tv, NULL);
	time = (tv.tv_sec * 1000 + tv.tv_usec);
	return (time);
}

void get_new_packet(t_connection_info* info)
{
	static int	seq = 0;
	size_t		packet_size = (long)get_option(info->options, SIZE)->data;

	struct icmp* icmp = (struct icmp*)info->packet;
	bzero(info->packet, packet_size);

	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = info->pid;
	icmp->icmp_seq = seq;
	icmp->icmp_cksum = calculate_checksum((unsigned char*)info->packet, packet_size);
	info->icmp = icmp;
	seq++;
}

int create_socket(t_connection_info *info)
{
	// create packet buffer
	long packet_size = (long)get_option(info->options, SIZE)->data;
	packet_size = (long)set_option(info->options, SIZE, (void*)(packet_size + sizeof(struct icmp*)))->data;
	info->packet = calloc(1, packet_size);
	if (!info->packet)
		return 1;

	// setup socket
	bzero(&info->addr, sizeof(struct sockaddr_in));
	info->addr.sin_family = AF_INET;

	info->socketfd = socket(info->addrinfo->ai_family, info->addrinfo->ai_socktype, info->addrinfo->ai_protocol);
	if (info->socketfd == -1)
	{
		perror("socket creation");
		return -1;
	}

	int ttl = (long)get_option(info->options, TTL)->data;
	if (setsockopt(info->socketfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(size_t)) != 0)
	{
		perror("setting ttl failed");
		return -1;
	}

	inet_pton(AF_INET, info->ip, &(info->addr.sin_addr));
	return 0;
}

struct addrinfo* getAddrIP(const char* name, t_connection_info* info)
{
	struct addrinfo		hint;
	struct addrinfo*	res;
	int					status;
	char				buffer[INET_ADDRSTRLEN];

	bzero(&hint, sizeof(struct addrinfo));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_RAW;
	hint.ai_protocol = IPPROTO_ICMP;
	
	status = getaddrinfo(name, 0, &hint, &res);
	if (status != 0)
	{
		dprintf(2, UNKNOWN_HOST_TXT);
		return NULL;
	}

	if (res->ai_family == AF_INET)
	{
		struct sockaddr_in* ip4 = (struct sockaddr_in*)res->ai_addr;
		inet_ntop(res->ai_family, &(ip4->sin_addr), buffer, sizeof(buffer));
		info->ip = strdup(buffer);
	}

	return res;
}

float update_stats(t_connection_info* infos, long before) 
{
	long after = get_current_time_ms();
	float timer = (float)(after - before) / 1000;
	if (timer > 0)
	{
		infos->stats.total_time_sqr += timer*timer;
		infos->stats.total_time += timer;
		infos->stats.number_of_ping++;
		if (timer < infos->stats.min || infos->stats.min == 0)
			infos->stats.min = timer;
		if (timer > infos->stats.max || infos->stats.max == 0)
			infos->stats.max = timer;
	}

	return timer;
}

int receive_packet(t_connection_info* infos)
{
	char				buffer[4086];
	int					prev_seq[10] = {-1};
	struct sockaddr_in	addr;
	socklen_t			addr_len = sizeof(socklen_t);
	struct iphdr*		hdr;

	infos->bytes = recvfrom(infos->socketfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addr_len);
	if (infos->bytes < 0)
	{
		perror("recvfrom");
		return 1;
	}

	struct icmp* icmp = (struct icmp*)(buffer + sizeof(struct iphdr));
	infos->is_dup = 0;
	for (int i = 0; i < 10; i++)
	{
		if (icmp->icmp_seq == prev_seq[i])
			infos->is_dup = 1;
	}
	prev_seq[icmp->icmp_seq%10] = icmp->icmp_seq;
	infos->response_icmp = icmp;

	infos->bytes -= sizeof(struct iphdr); // only want payload + icmp header

	hdr = (struct iphdr*)buffer;
	infos->packet_ttl = hdr->ttl;
	infos->stats.packet_received++;
	return 0;
}

void print_info(t_connection_info* infos, float timer)
{
	static char buffer[1024];

	if (get_option(infos->options, QUIET)->data)
		return ;

	if (get_option(infos->options, FLOOD)->data)
	{
		printf("\b");
		return ;
	}

	bzero(&buffer, 1024);
	sprintf(buffer, "%ld bytes from %s: icmp_seq=%d ttl=%u time=%.3f ms%s\n",
	   infos->bytes, infos->ip, infos->icmp->icmp_seq, infos->packet_ttl, timer, infos->is_dup ? " (!DUP)" : "");
	
	// to max out performance, print into buffer to avoid stdout
	size_t preload = (long)get_option(infos->options, PRELOAD)->data;
	if (preload)
	{
		if (infos->stats.packet_sent > preload)
		{
			// printf("%s", infos->buffer_output);
			set_option(infos->options, PRELOAD, 0x0);
		}
		else
		{
			infos->buffer_output = ft_strjoin(infos->buffer_output, buffer);
			if (infos->buffer_output == NULL) // TODO: faire foirer le malloc
				ping_shutdown(infos);
			return ;
		}
	}

	printf("%s", buffer);
}

void ping_loop(t_connection_info* infos)
{
	fd_set			readfds;
	struct timeval	tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	while (stop == 0)
	{
		get_new_packet(infos);

		FD_ZERO(&readfds);
		FD_SET(infos->socketfd, &readfds);

		long before = get_current_time_ms();
		ssize_t nbsent = sendto(infos->socketfd, infos->packet, (long)get_option(infos->options, SIZE)->data, 0,
						  (struct sockaddr *)&infos->addr, sizeof(infos->addr));
		if (nbsent < 0)
		{
			perror("ft_ping: sending packet");
			ping_shutdown(infos);
		}
		if (get_option(infos->options, FLOOD)->data)
		{
			printf(".");
		}
		infos->stats.packet_sent++;
		int result = select(infos->socketfd + 1, &readfds, NULL, NULL, &tv);
		if (result > 0 && FD_ISSET(infos->socketfd, &readfds))
		{
			if (receive_packet(infos) != 0)
				continue;
			float timer = update_stats(infos, before);
			print_info(infos, timer);
		}
		
		if (get_option(infos->options, FLOOD)->data)
			usleep(1);
		else if (get_option(infos->options, PRELOAD)->data)
			continue;
		else
			sleep((long)get_option(infos->options, INTERVAL)->data);
	}
}

int init(t_connection_info* infos, int argc, char* argv[])
{
	if (argc < 2)
	{
		dprintf(2, MISSING_ARG_TXT);
		
		return 1;
	}

	signal(SIGINT, &sig_handler);
	bzero(infos, sizeof(t_connection_info));

	infos->options = parse_options(argc, argv);
	if (infos->options == NULL)
		return 2;

	if (show_text(infos->options))
		return 1;

	infos->name = get_option(infos->options, NAME)->data;
	infos->pid = getpid();
	infos->addrinfo = getAddrIP(infos->name, infos);
	if (infos->addrinfo == NULL)
	{
		return 1;
	}
	
	if (create_socket(infos) != 0)
		return 1;

	printf("PING %s (%s): %ld data bytes", infos->name, infos->ip, (long)get_option(infos->options, SIZE)->data - sizeof(struct icmp*));
	if (get_option(infos->options, VERBOSE)->data)
		printf(", id %p = %d", (void*)(long)infos->pid, infos->pid);
	printf("\n");

	return 0;
}

void cleanup_infos(t_connection_info* infos)
{
	freeaddrinfo(infos->addrinfo);
	// free(infos->packet);
	free(infos->ip);
	free(infos->options);

	close(infos->socketfd);
	exit(0);
}

void ping_shutdown(t_connection_info* infos)
{
	// calculate statistics
	int packet_loss = 100 - ((float)infos->stats.packet_received / (float)infos->stats.packet_sent) * 100;
	float avg = infos->stats.total_time / infos->stats.number_of_ping;
	
	t_stats stats = infos->stats;

	float variance = stats.number_of_ping*stats.total_time_sqr - (stats.total_time*stats.total_time);
	if (stats.number_of_ping > 1)
	{
		variance /= (stats.number_of_ping * (stats.number_of_ping - 1));
		variance = sqrt(variance);
	}

	printf("--- %s ping statistics ---\n", infos->name);
	printf("%ld packets transmitted, %ld packets received, %d%% packet loss\n",
		infos->stats.packet_sent, infos->stats.packet_received, packet_loss);
	printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
		infos->stats.min, avg, infos->stats.max, variance);

	if (get_option(infos->options, DEBUG)->data)
	{
		printf("+++ DEBUG INFOS +++\n");
		printf("\tpacket_lost =%ld\n", infos->stats.packet_sent-infos->stats.packet_received);
		printf("\ttotal_time  =%.3fms\n", infos->stats.total_time);
	}

	cleanup_infos(infos);
}


int main(int argc, char *argv[])
{
	t_connection_info info;

	int error = init(&info, argc, argv);
	if (error != 0)
		return error;

	ping_loop(&info);
	ping_shutdown(&info);
}
