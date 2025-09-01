#include <stdio.h>
#include <string.h>
#include <netinet/ip_icmp.h>

#include "text.h"

int show_text(t_option* options)
{
	if ((long)get_option(options, HELP)->data == 1)
	{
		printf(HELP_TXT);
		return 1;
	}
	else if ((long)get_option(options, USAGE)->data == 1)
	{
		printf(USAGE_TXT);
		return 1;
	}
	else if ((long)get_option(options, VERSION)->data == 1)
	{
		printf(VERSION_TXT);
		return 1;
	}
	if ((long)get_option(options, DEBUG)->data == 1)
		show_options(options);

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
			set_option(infos->options, PRELOAD, 0x0);
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
