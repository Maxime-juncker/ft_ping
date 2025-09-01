#include "ft_ping.h"

int main(int argc, char *argv[])
{
	t_connection_info info;

	int error = init(&info, argc, argv);
	if (error != 0)
	{
		return error;
	}

	ping_loop(&info);
	ping_shutdown(&info);
}
