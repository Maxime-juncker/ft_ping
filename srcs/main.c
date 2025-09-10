#include "ft_ping.h"

int main(int argc, char *argv[])
{
	t_connection_info info;

	int error = init(&info, argc, argv);
	if (error != 0)
	{
		cleanup_infos(&info);
		return error;
	}

	error = ping_loop(&info);
	if (error != 0)
	{
		cleanup_infos(&info);
		return error;
	}
	
	ping_shutdown(&info);
}
