#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
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

typedef struct s_connection_info
{
	char*				ip;
	struct addrinfo*	addrinfo;
	char				packet[56];
	struct sockaddr_in	addr;
	int					socketfd;

	struct icmp*		icmp;

	size_t				packet_sent;
	size_t				packet_received;

	t_option*			options;

} t_connection_info;

uint16_t calculate_checksum(void *b, int len)
{
    uint16_t *buf = b;
    unsigned int sum = 0;
    uint16_t result;

    for (sum = 0; len > 1; len -= 2)
	{
        sum += *buf++;
    }

    if (len == 1)
	{
        sum += *(unsigned char*)buf << 8;
    }

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    result = ~sum;
    return result;
}

void get_new_packet(t_connection_info* info)
{
	static int seq = 0;

	struct icmp* icmp = (struct icmp*)info->packet;
	bzero(info->packet, sizeof(info->packet));

	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = getpid();
	icmp->icmp_seq = seq;
	icmp->icmp_cksum = calculate_checksum((unsigned char*)info->packet, 56);

	info->icmp = icmp;

	seq++;
}

int create_socket(t_connection_info *info)
{
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

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		dprintf(2, MISSING_ARG_TXT);
		return 1;
	}
	signal(SIGINT, &sig_handler);

	t_connection_info info;
	bzero(&info, sizeof(t_connection_info));

	info.options = parse(argc, argv);
	if (info.options == NULL)
		return 1;

	if (show_text(info.options))
		return 0;

	info.addrinfo = getAddrIP(get_option(info.options, NAME)->data, &info);
	if (info.addrinfo == NULL)
	{
		return 1;
	}

	if (create_socket(&info) != 0)
		exit(1);

	fd_set readfds;
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	printf("PING %s (%s): %d data bytes\n", argv[1], info.ip, 64);

	while (stop == 0)
	{
		get_new_packet(&info);

		FD_ZERO(&readfds);
		FD_SET(info.socketfd, &readfds);

		long before = get_current_time_ms();
		ssize_t nbsent = sendto(info.socketfd, info.packet, sizeof(info.packet), 0, (struct sockaddr *)&info.addr, sizeof(info.addr));
		if (nbsent < 0)
			perror("sendto failed");

		info.packet_sent++;
		ssize_t bytes = 0;
		int result = select(info.socketfd + 1, &readfds, NULL, NULL, &tv);
		if (result > 0 && FD_ISSET(info.socketfd, &readfds))
		{
			char buffer[1024];
			bytes = recv(info.socketfd, buffer, sizeof(buffer), 0);
			info.packet_received++;
		}
		
		long after = get_current_time_ms();

		float timer = (float)(after - before) / 1000;

		printf("%ld bytes from %s: icmp_seq=%d ttl=TODO time=%.3f ms\n", bytes, info.ip, info.icmp->icmp_seq, timer);

		sleep((long)get_option(info.options, INTERVAL)->data);
	}

	printf("--- %s ping statistics ---\n", argv[1]);
	int packet_loss = 100 - ((float)info.packet_received / (float)info.packet_sent) * 100;
	printf("%ld packets transmitted, %ld packets received, %d%% packet loss\n", info.packet_sent, info.packet_received, packet_loss);
}
