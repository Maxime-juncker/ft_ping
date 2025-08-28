#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
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

void ping_loop(t_connection_info* infos)
{
	fd_set			readfds;
	struct iphdr*	hdr;
	struct timeval	tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	while (stop == 0)
	{
		get_new_packet(infos);

		FD_ZERO(&readfds);
		FD_SET(infos->socketfd, &readfds);

		long before = get_current_time_ms();
		ssize_t nbsent = sendto(infos->socketfd, infos->packet, sizeof(infos->packet), 0,
						  (struct sockaddr *)&infos->addr, sizeof(infos->addr));
		if (nbsent < 0)
			perror("sendto failed");

		infos->packet_sent++;
		ssize_t bytes = 0;
		int result = select(infos->socketfd + 1, &readfds, NULL, NULL, &tv);
		if (result > 0 && FD_ISSET(infos->socketfd, &readfds))
		{
			char buffer[1024];
			struct sockaddr_in addr;
			socklen_t addr_len = sizeof(addr);
			bytes = recvfrom(infos->socketfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addr_len);
			if (bytes < 0)
			{
				perror("recvfrom");
			}

			hdr = (struct iphdr*)buffer;
			infos->packet_received++;
		}
		
		long after = get_current_time_ms();

		float timer = (float)(after - before) / 1000;

		if (!get_option(infos->options, QUIET)->data)
			printf("%ld bytes from %s: icmp_seq=%d ttl=%u time=%.3f ms\n", bytes, infos->ip, infos->icmp->icmp_seq, hdr->ttl, timer);

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

	infos->options = parse(argc, argv);
	if (infos->options == NULL)
		return 2;

	if (show_text(infos->options))
		return 1;

	infos->name = get_option(infos->options, NAME)->data;
	infos->addrinfo = getAddrIP(infos->name, infos);
	if (infos->addrinfo == NULL)
	{
		return 1;
	}

	if (create_socket(infos) != 0)
		return 1;

	printf("PING %s (%s): %d data bytes\n", infos->name, infos->ip, 64);

	return 0;
}

void ping_shutdown(t_connection_info* infos)
{
	printf("--- %s ping statistics ---\n", infos->name);
	int packet_loss = 100 - ((float)infos->packet_received / (float)infos->packet_sent) * 100;
	printf("%ld packets transmitted, %ld packets received, %d%% packet loss\n", infos->packet_sent, infos->packet_received, packet_loss);
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
