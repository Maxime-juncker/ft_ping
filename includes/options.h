#ifndef OPTION_H
#define OPTION_H

#define E_UNKNOW -1
#define E_INVALID -2

// colors
# define RESET	"\033[0m"
# define BLACK	"\033[0;30m"
# define RED	"\033[0;31m"
# define GREEN	"\033[0;32m"
# define YELLOW	"\033[0;33m"
# define BLUE	"\033[0;34m"
# define PURPLE	"\033[0;35m"
# define CYAN	"\033[0;36m"
# define WHITE	"\033[0;37m"
# define GRAY	"\033[0;90m"

// Background
# define B_BLACK	"\033[40m"
# define B_RED		"\033[41m"
# define B_GREEN	"\033[42m"
# define B_YELLOW	"\033[43m"
# define B_BLUE		"\033[44m"
# define B_PURPLE	"\033[45m"
# define B_CYAN		"\033[46m"
# define B_WHITE	"\033[47m"

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

typedef union u_types
{
	char	c;
	char*	str;
	long	dec;
	double	fract;
	void*	ptr;
}	t_opttype;

typedef enum e_ctypes
{
	INT = 0,
	DOUBLE,
	HEX,
	STRING,
	VOID,
}	ctypes;

typedef struct s_option
{
	int				id;

	char			c;
	char*			name;
	int				need_arg;
	union u_types	data;
	ctypes			type;
	int				user_set;

}	t_option;

t_option*	parse_options(int argc, char* argv[]);
void		show_options(t_option* options);

t_option* get_option(t_option* options, int id);
t_option* set_option(t_option* options, int id, union u_types data);

#endif // !OPTION_H
