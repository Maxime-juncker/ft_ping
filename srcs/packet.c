#include <asm-generic/socket.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>

#include "ft_ping.h"
#include "options.h"

void show_packet(unsigned char* packet, size_t size)
{
	size_t i;
	printf("+++ PACKET +++\n");
	for (i = 0; i < size; i++)
	{
		printf("%c%X", packet[i] < 16 ? '0' : '\0', packet[i]);
		if (i % 10 == 9)
			printf("\n");
		else
			printf(" ");
	}

	while (i % 10 > 0)
	{
		printf(".. ");
		i++;
	}
	
	printf("\n");
}

void get_new_packet(t_connection_info* info)
{
	static int	seq = 0;
	size_t		packet_size = (long)get_option(info->options, SIZE)->data;

	struct icmp* icmp = (struct icmp*)info->packet;
	// memset(info->packet, 10, packet_size);
	memset(info->packet, (long)get_option(info->options, PATTERN)->data, packet_size);
	bzero(icmp, sizeof(struct icmp));

	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = info->pid;
	icmp->icmp_seq = seq;
	icmp->icmp_cksum = calculate_checksum((unsigned char*)info->packet, packet_size);
	info->icmp = icmp;
	seq++;

	if (get_option(info->options, DEBUG)->data)
		show_packet(info->packet, packet_size);
}

int create_socket(t_connection_info *info)
{
	// create packet buffer
	long packet_size = (long)get_option(info->options, SIZE)->data;
	packet_size = (long)set_option(info->options, SIZE, (void*)(packet_size + sizeof(struct icmp)))->data;
	
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

	int tos = (long)get_option(info->options, TOS)->data;
	if (setsockopt(info->socketfd, IPPROTO_IP, IP_TOS, &tos, sizeof(size_t)) != 0)
	{
		perror("setting ttl failed");
		return -1;
	}

	if (get_option(info->options, IGNORE_ROUTING)->data)
	{
		int opt = 1;
		if (setsockopt(info->socketfd, SOL_SOCKET, SO_DONTROUTE, &opt, sizeof(int)))
		{
			perror("setting ignore routing failed");
			return -1;
		}
	}

	// char* timestamp = get_option(info->options, IP_TIMESTAMP)->data;
	int test = 1;
	if (setsockopt(info->socketfd, SOL_SOCKET, SO_TIMESTAMP, &test, sizeof(int)))
	{
		perror("setting timestamp failed");
		return 1;
	}

	inet_pton(AF_INET, info->ip, &(info->addr.sin_addr));
	return 0;
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

	if (get_option(infos->options, DEBUG)->data)
		show_packet((unsigned char*)buffer, infos->bytes);

	struct icmp* icmp = (struct icmp*)(buffer + sizeof(struct iphdr));
	infos->is_dup = 0;
	for (int i = 0; i < 10; i++)
	{
		if (icmp->icmp_seq == prev_seq[i])
			infos->is_dup = 1;
	}
	prev_seq[icmp->icmp_seq%10] = icmp->icmp_seq;
	infos->response_icmp = icmp;

	// infos->bytes -= sizeof(struct iphdr); // only want payload + icmp header

	hdr = (struct iphdr*)buffer;
	infos->packet_ttl = hdr->ttl;
	infos->stats.packet_received++;
	return 0;
}
