#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "options.h"
#include "text.h"

t_option*	load_short_option(t_option* options, char c)
{
	int i = 0;
	while (options[i].id != NONE)
	{
		if (options[i].c == c)
			return &options[i];
		i++;
	}
	return NULL;
}

t_option* load_large_option(t_option* options, char* name)
{
	int i = 0;

	while (options[i].id != NONE)
	{
		if (strcmp(options[i].name, name) == 0)
			return &options[i];
		i++;
	}
	return NULL;
}

int set_value(t_option* option, char* argv[], int* idx, int argc)
{
	char*	end;
	if (option->need_arg == 0)
		option->data.dec = 1;
	else
	{
		if (*idx + 1 >= argc)
			return E_INVALID;
		*idx += 1;
		if (option->type == STRING)
			option->data.str = argv[*idx];
		else if (option->type == DOUBLE)
		{
			option->data.fract = strtod(argv[*idx], &end);
			if (end[0] != '\0')
				return E_INVALID;
		}
		else
		{
			option->data.dec = strtol(argv[*idx], &end, option->type == HEX ? 16 : 10);
			if (end[0] != '\0')
				return E_INVALID;
		}
	}

	option->user_set = 1;
	return 0;
}

int	load_option(t_option* options, char* argv[], int* idx, int argc)
{
	int			i = 0;
	t_option*	option = NULL;
	char*		arg = argv[*idx];

	while (arg[i] == '-' && arg[i])
		i++;

	if (!arg[i])
		return E_INVALID;
	if (i == 0 || i > 2)
		return 0;

	if (i == 1) // e.g: -v
	{
		int j = 1;
		while (arg[j])
		{
			option = load_short_option(options, arg[j]);
			if (option == NULL)
				return E_UNKNOW;
			if (option->need_arg && arg[j + 1])
				return E_INVALID;
					j++;
			set_value(option, argv, idx, argc);
		}
	}
	else // e.g: --verbose
	{
		option = load_large_option(options, &arg[2]);
		if (option == NULL)
			return E_UNKNOW;

		option->data.dec = 1;
		set_value(option, argv, idx, argc);
	}
	return 0;
}

void show_options(t_option* options)
{
	int i = 0;
	printf("+++ options +++\n");
	while (options[i].id != NONE)
	{
		printf("\t--%s (-%c) => %s", options[i].name, options[i].c, \
				options[i].user_set ? RED : WHITE);
		if (options[i].type == INT)
			printf("%ld",		options[i].data.dec);
		if (options[i].type == DOUBLE)
			printf("%.2lf",		options[i].data.fract);
		if (options[i].type == STRING)
			printf("\"%s\"",	options[i].data.str);
		if (options[i].type == VOID || options[i].type == HEX)
			printf("%p",		options[i].data.ptr);
		printf("\n" RESET);
		i++;
	}
	printf("\n");
}

t_option* set_option(t_option* options, int id, union u_types data)
{
	t_option* option = get_option(options, id);
	if (options == NULL)
		return NULL;
	option->data = data;
	return option;
}

t_option* get_option(t_option* options, int id)
{
	int i = 0;
	while (options[i].id != NONE)
	{
		if (options[i].id == id)
			return &options[i];
		i++;
	}
	return NULL;
}

t_option* parse_options(int argc, char* argv[])
{
	t_option		options[] =
	{
		{HELP,				'?',	"help",				0, (t_opttype){ .dec = 0 },		INT,	0},
		{VERBOSE,			'v',	"verbose",			0, (t_opttype){ .dec = 0 },		INT,	0},
		{FLOOD,				'f',	"flood",			0, (t_opttype){ .dec = 0 },		INT,	0},
		{IP_TIMESTAMP,		'\0',	"ip-timestamp",		1, (t_opttype){ .dec = 0 },		STRING,	0},
		{PRELOAD,			'l',	"preload",			1, (t_opttype){ .dec = 0 },		INT,	0},
		{NUMERIC,			'n',	"numeric",			0, (t_opttype){ .dec = 0 },		INT,	0},
		{TIMEOUT,			'w',	"timeout",			1, (t_opttype){ .dec = 0 },		INT,	0},
		{LINGER,			'W',	"linger",			1, (t_opttype){ .dec = 0 },		INT,	0},
		{PATTERN,			'p',	"pattern",			1, (t_opttype){ .dec = 0 },		HEX,	0},
		{IGNORE_ROUTING,	'r',	"ignore-routine",	0, (t_opttype){ .dec = 0 },		INT,	0},
		{SIZE,				's',	"size",				1, (t_opttype){ .dec = 56 },	INT,	0},
		{TOS,				'T',	"tos",				1, (t_opttype){ .dec = 0 },		INT,	0},
		{QUIET,				'q',	"quiet",			0, (t_opttype){ .dec = 0 },		INT,	0},
		{VERSION,			'V',	"version",			0, (t_opttype){ .dec = 0 },		INT,	0},
		{USAGE,				'\0',	"usage",			0, (t_opttype){ .dec = 0 },		INT,	0},
		{DEBUG,				'd',	"debug",			0, (t_opttype){ .dec = 0 },		INT,	0},
		{INTERVAL,			'i',	"interval",			1, (t_opttype){ .dec = 1 },		INT,	0},
		{TTL,				'\0',	"ttl",				1, (t_opttype){ .dec = 64 },	INT,	0},
		{NAME,				'\0',	"",					0, (t_opttype){ .str = 0x0 },	STRING,	0},
		{NONE,				'\0',	"",					0, (t_opttype){ .dec = 0 },		INT,	0},	
	};

	for (int i = 1; i < argc; i++)
	{
		int	error = load_option(options, argv, &i, argc);
		if (error == E_UNKNOW)
		{
			dprintf(2, "ft_ping: unrecognized option '%s'\n", argv[i]);
			dprintf(2, SMALL_HELP_TXT);
			return NULL;
		}
		else if (error == E_INVALID)
		{
			dprintf(2, "ft_ping: invalid value\n");
			return NULL;
		}

	}

	if (argv[argc-1][0] == '-' &&
	 !(	get_option(options, HELP)->data.fract ||
		get_option(options, USAGE)->data.fract))
	{
		dprintf(2, "ft_ping: missing host operand\n");
		dprintf(2, SMALL_HELP_TXT);
		return NULL;
	}

	set_option(options, NAME, (t_opttype) { .str = argv[argc-1] });

	t_option* cpy = malloc(sizeof(options));
	if (cpy == NULL)
		return NULL;
	memcpy(cpy, &options, sizeof(options));
	return cpy;
}


