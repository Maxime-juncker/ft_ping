#include <asm-generic/socket.h>
#include <bits/types/struct_iovec.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <linux/net_tstamp.h>

#include "ft_ping.h"
#include "options.h"

void show_packet(unsigned char* packet, size_t size)
{
	size_t i;
	printf("+++ DUMP DATA +++\n");
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

struct ip_timestamp_opt
{
    uint8_t code;        // IPOPT_TS (68)
    uint8_t length;      // Total length of option
    uint8_t pointer;     // Pointer to next timestamp slot
    uint8_t flags;       // Timestamp flags (IPOPT_TS_TSONLY, etc.)
    uint32_t timestamps[9]; // Up to 9 timestamps (36 bytes max)
};

void set_timestamp_option(int sockfd)
{
    struct ip_timestamp_opt ts_opt;
    
    ts_opt.code = IPOPT_TS;
    ts_opt.length = 40; // 4 bytes header + 36 bytes for timestamps
    ts_opt.pointer = 5;  // Points to first timestamp (after header)
    ts_opt.flags = IPOPT_TS_TSANDADDR; // Only timestamps, no addresses
    
    // Clear timestamp array
    memset(ts_opt.timestamps, 0, sizeof(ts_opt.timestamps));
    
    if (setsockopt(sockfd, IPPROTO_IP, IP_OPTIONS, &ts_opt, ts_opt.length) < 0)
	{
        perror("failed to set timestamp");
    }
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

	set_timestamp_option(info->socketfd);
	// int optval = IPOPT_TS_TSONLY; // Or IPOPT_TS_TSANDADDR for tsaddr mode
	// if (setsockopt(info->socketfd, IPPROTO_IP, IP_OPTIONS, &optval, sizeof(optval)))
	// {
	// 	perror("setting timestamp failed");
	// 	return 1;
	// }

	// int timestamp = SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_OPT_ID | SOF_TIMESTAMPING_TX_SOFTWARE;
	// if (setsockopt(info->socketfd, SOL_SOCKET, SO_TIMESTAMPING,&timestamp, sizeof(int)))
	// {
	// 	perror("setting timestamp failed");
	// 	return 1;
	// }

	inet_pton(AF_INET, info->ip, &(info->addr.sin_addr));
	return 0;
}

#include <time.h>

void parse_ip_options(uint8_t *options, int options_len)
{
    int i = 0;
    while (i < options_len)
	{
        uint8_t option_type = options[i];
        
        if (option_type == IPOPT_EOL)
		{
            printf("End of options list\n");
            break;
        }
        
        if (option_type == IPOPT_NOP)
		{
            printf("NOP option\n");
            i++;
            continue;
        }
        
        if (option_type == IPOPT_TS)
		{ 
            uint8_t option_len = options[i + 1];
            uint8_t pointer = options[i + 2];
            uint8_t flags = options[i + 3];
            
            printf("IP Timestamp Option found:\n");
            printf("  Length: %d, Pointer: %d, Flags: %d\n", 
                   option_len, pointer, flags);
            
            int num_timestamps = (pointer - 5) / 4;
			num_timestamps = num_timestamps > 4 ? 4 : num_timestamps;
            
            printf("  Number of timestamps recorded: %d (%d)\n", num_timestamps, pointer);
            
			int j = 0;
			uint32_t ip = 0;
			uint32_t ts = 0;
			struct in_addr addr;
            for (j = 0; j < num_timestamps && (j * 4 + 4) < option_len; j += 2)
			{
                // Timestamps start at options[i + 4]
                memcpy(&ip, &options[i + 4 + j * 4], 4);
                memcpy(&ts, &options[i + 4 + (j + 1) * 4], 4);
				ts = ntohl(ts);
				addr.s_addr = ip;
				printf("%s:\t%d\n", inet_ntoa(addr), ts);
            }
			while (j < 8)
			{
				printf("%s:\t%d\n", inet_ntoa(addr), ts);
				j+=2;
			}
            
            i += option_len;
        } 
    }
}

int receive_packet(t_connection_info* infos)
{
	char				buffer[4086];
	char				control[4086];
	int					prev_seq[10] = {-1};
	struct msghdr		msg;
	struct iovec		iov[1];
	struct iphdr*		hdr;


	bzero(&msg, sizeof(struct msghdr));

	iov[0].iov_base = buffer;
	iov[0].iov_len = sizeof(buffer);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	msg.msg_control = control;
    msg.msg_controllen = sizeof(control);


	infos->bytes = recvmsg(infos->socketfd, &msg, 0);
	if (infos->bytes < 0)
	{
		perror("recvmsg");
		return 1;
	}

	
	if (get_option(infos->options, DEBUG)->data)
		show_packet((unsigned char*)buffer, infos->bytes);

	hdr = (struct iphdr*)buffer;
	int ip_header_len = hdr->ihl * 4;
	infos->packet_ttl = hdr->ttl;

	struct icmp* icmp = (struct icmp*)(buffer + ip_header_len);
	infos->is_dup = 0;
	for (int i = 0; i < 10; i++)
	{
		if (icmp->icmp_seq == prev_seq[i])
			infos->is_dup = 1;
	}
	prev_seq[icmp->icmp_seq%10] = icmp->icmp_seq;
	infos->response_icmp = icmp;
	infos->stats.packet_received++;

	if (icmp->icmp_type == ICMP_TIME_EXCEEDED)
	{
		printf("%ld bytes from _gateway (%s): Time to Live exceeded\n", infos->bytes, inet_ntoa(*(struct in_addr*)&hdr->saddr));
		wait_interval(infos);
		return 1;
	}

    printf("IP header length: %d bytes\n", ip_header_len);
    if (ip_header_len > 20)
	{
        int options_len = ip_header_len - 20;
        printf("IP options length: %d bytes\n", options_len);
        
        unsigned char *options = (unsigned char *)(buffer + sizeof(struct iphdr));
		show_packet(options, options_len);
		parse_ip_options(options, options_len);
    }

	return 0;
}

