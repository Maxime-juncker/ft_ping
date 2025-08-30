#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>


int stop = 0;

void sig_handler(int signal)
{
	if (signal == SIGINT)
	{
		stop = SIGINT;
	}
}

long	get_current_time_micro(void)
{
	long long		time;
	struct timeval	
	tv;

	gettimeofday(&tv, NULL);
	time = (tv.tv_sec * 1000000 + tv.tv_usec);
	return (time);
}



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

char	*ft_strjoin(char *s1, char const *s2)
{
	char	*result;
	size_t	len1;
	size_t	len2;

	if (s1 == NULL && s2 == NULL)
		return (NULL);
	if (s1 == NULL)
		return (strdup(s2));
	if (s2 == NULL)
		return (strdup(s1));
	len1 = strlen(s1);
	len2 = strlen(s2);
	result = malloc(len1 + len2 + 1);
	if (result == NULL)
		return (NULL);
	strlcpy(result, s1, len1 + 1);
	strlcpy(result + len1, s2, len2 + 1);
	return (result);
}
