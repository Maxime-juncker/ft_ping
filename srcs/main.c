#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>

#define MAX_EVENT 10

typedef struct s_connection_info
{
	char*				ip;

	char				packet[64];
	struct sockaddr_in	addr;
	int					socketfd;

	int					epollfd;

} t_connection_info;

uint16_t get_checksum(unsigned char *packet, int bytes)
{
	uint32_t		checksum = 0;
	unsigned char	*end = packet + bytes;

	// make bit parity
	if (bytes % 2 == 1)
	{
		end = packet + bytes - 1;	
		checksum += (*end) << 8;
	}

	while (packet < end)
	{
		checksum += packet[0] << 8;
		checksum += packet[1];
		packet += 2;
	}

	// make the carray wrap around
	uint32_t carray = checksum >> 16;
	while (carray)
	{
		checksum = (checksum & 0xffff) + carray;
		carray = checksum >> 16;
	}

	checksum = ~checksum;
	return checksum & 0xffff;
}

int create_server(t_connection_info* info)
{
	info->epollfd = epoll_create1(0);
	if (info->epollfd == -1)
	{
		perror("epoll create failed");
		return 1;
	}

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = info->socketfd;

	if (epoll_ctl(info->epollfd, EPOLL_CTL_ADD, info->socketfd, &event) != 0)
	{
		perror("epoll_ctl failed");
		return 1;
	}

	return 0;
}

int create_socket(t_connection_info *info)
{
	bzero(&info->addr, sizeof(struct sockaddr_in));
	info->addr.sin_family = AF_INET;

	info->socketfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
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
	icmp->icmp_seq = 1;
	icmp->icmp_cksum = get_checksum((unsigned char *)info->packet, 64);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("missing arg");
		return 1;
	}

	t_connection_info info;
	info.ip = argv[1];

	if (create_socket(&info) != 0)
		exit(1);
	if (create_server(&info) != 0)
		exit(1); //TODO: close socket
	
	struct epoll_event events[MAX_EVENT];

	while (1)
	{

		printf("sending: %ldb to %s\n",
		 sendto(info.socketfd, info.packet, sizeof(info.packet), 0, (struct sockaddr *)&info.addr, sizeof(info.addr)),
		 info.ip);

		int res = epoll_wait(info.epollfd, events, MAX_EVENT, -1);
		if (res == -1)
		{
			perror("epoll wait failed");
			return 1;
		}

		printf("receive something YEAAAH\n");

		sleep(1);

	}
}
