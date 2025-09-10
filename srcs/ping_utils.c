#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

#include "text.h"
#include "ft_ping.h"

float update_stats(t_connection_info* infos, long before) 
{
	long after = get_current_time_micro();
	float time_ms = (float)(after - before) / 1000;
	float time_sec = time_ms / 1000;

	infos->curr_time = (float)(get_current_time_micro() - infos->begin_time) / 1000000;
	if (time_ms > 0)
	{
		long linger = get_option(infos->options, LINGER)->data.dec;
		if (linger > 0 && time_sec > linger)
		{
			ping_shutdown(infos);

		}
		infos->stats.total_time_sqr += time_ms*time_ms;
		infos->stats.total_time += time_ms;
		infos->stats.number_of_ping++;
		if (time_ms < infos->stats.min || infos->stats.min == 0)
			infos->stats.min = time_ms;
		if (time_ms > infos->stats.max || infos->stats.max == 0)
			infos->stats.max = time_ms;
	}

	return time_ms;
}

void cleanup_infos(t_connection_info* infos)
{
	freeaddrinfo(infos->addrinfo);
	free(infos->packet);
	free(infos->ip);
	free(infos->options);

	close(infos->socketfd);
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


