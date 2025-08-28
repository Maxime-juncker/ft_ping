#include <stdint.h>

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


