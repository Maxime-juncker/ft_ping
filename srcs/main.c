#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAX_EVENT 10

typedef struct s_connection_info
{
	char*				ip;
	struct addrinfo*	addrinfo;
	char				packet[64];
	struct sockaddr_in	addr;
	int					socketfd;

	int					epollfd;

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

	inet_pton(AF_INET, info->ip, &(info->addr.sin_addr));

	struct icmp *icmp = (struct icmp *)info->packet;
	bzero(info->packet, sizeof(info->packet));

	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = getpid();
	icmp->icmp_seq = 0;
	icmp->icmp_cksum = calculate_checksum((unsigned char *)info->packet, 64);

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
	
	printf("+++ addr info for %s +++\n", name);
	status = getaddrinfo(name, 0, &hint, &res);
	if (status != 0)
	{
		perror(gai_strerror(status));
		return NULL;
	}

	char *ip = NULL;
	while (res != NULL)
	{
		if (res->ai_family == AF_INET)
		{
			struct sockaddr_in* ip4 = (struct sockaddr_in*)res->ai_addr;
			inet_ntop(res->ai_family, &(ip4->sin_addr), buffer, sizeof(buffer));
			printf("IPv4: %s\n", buffer);
			info->ip = strdup(buffer);
			return res;
		}
		// else
		// {
		// 	struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
		// 	inet_ntop(res->ai_family, &(ipv6->sin6_addr), buffer, sizeof buffer);
		// 	printf("IPv6: %s\n", buffer);
		// }
		res = res->ai_next;
	}

	freeaddrinfo(res);
	printf("resolve %s to %s\n", name, ip);
	return res;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("missing arg");
		return 1;
	}

	
	t_connection_info info;
	info.addrinfo = getAddrIP(argv[1], &info);

	if (create_socket(&info) != 0)
		exit(1);
	// if (create_server(&info) != 0)
	// 	exit(1); //TODO: close socket
	
	// struct epoll_event events[MAX_EVENT];

	fd_set readfds;

	struct timeval tv;

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	while (1)
	{

		FD_ZERO(&readfds);
		FD_SET(info.socketfd, &readfds);

		printf("sending: %ldb to %s\n",
		 sendto(info.socketfd, info.packet, sizeof(info.packet), 0, (struct sockaddr *)&info.addr, sizeof(info.addr)),
		 info.ip);

		int result = select(info.socketfd + 1, &readfds, NULL, NULL, &tv);
		if (result > 0 && FD_ISSET(info.socketfd, &readfds)) {
			char buffer[1024];
			ssize_t bytes = recv(info.socketfd, buffer, sizeof(buffer), 0);
			printf("%ld\n", bytes);	
		}

		sleep(1);

	}
}
