#ifndef OPTION_H
#define OPTION_H

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
	NAME,
}	option_type;

typedef enum e_ctypes
{
	INT = 0,
	HEX,
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

t_option*	parse_options(int argc, char* argv[]);
void		show_options(t_option* options);

t_option* get_option(t_option* options, int id);
t_option* set_option(t_option* options, int id, void* data);

#endif // !OPTION_H
