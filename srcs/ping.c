#include <math.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/select.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <sys/signal.h>

#include "options.h"
#include "text.h"
#include "ft_ping.h"

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

		long before = get_current_time_micro();
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
	
		long timeout = (long)get_option(infos->options, TIMEOUT)->data;
		if (timeout > 0 && infos->curr_time > timeout)
			ping_shutdown(infos);	

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

	printf("PING %s (%s): %ld data bytes", infos->name, infos->ip, (long)get_option(infos->options, SIZE)->data - sizeof(struct icmp));
	if (get_option(infos->options, VERBOSE)->data)
		printf(", id %p = %d", (void*)(long)infos->pid, infos->pid);
	printf("\n");

	infos->begin_time = get_current_time_micro();

	return 0;
}

void ping_shutdown(t_connection_info* infos)
{
	infos->curr_time = (float)(get_current_time_micro() - infos->begin_time) / 1000000;

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
		printf("\trun for     =%.3fs\n", infos->curr_time);
	}

	cleanup_infos(infos);
}
