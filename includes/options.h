#ifndef OPTION_H

#define E_UNKNOW -1
#define E_INVALID -2

typedef enum e_option_type
{
	UNKNOW = -1,
	NONE = 0,
	HELP,
	VERBOSE,
	FLOOD,
	IP_TIMESTAMP,
	PRELOAD,
	NUMERIC,
	TIMEOUT,
	LINGER,
	PATTERN,
	IGNORE_ROUTING,
	SIZE,
	TOS,
	QUIET,
	VERSION,
	USAGE,
	DEBUG,
	INTERVAL,
	TTL,
}	option_type;

typedef enum e_ctypes
{
	INT = 0,
	STRING,
	VOID,
}	ctypes;

typedef struct s_option
{
	int		id;

	char	c;
	char*	name;
	int		need_arg;
	void*	data;
	ctypes	type;

}	t_option;

int parse(int argc, char* argv[]);

#endif // !OPTION_H
