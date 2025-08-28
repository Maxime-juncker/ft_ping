#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "options.h"

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
		option->data = (void*)0x1;
	else
	{
		if (*idx + 1 >= argc)
			return E_INVALID;
		*idx += 1;
		if (option->type == STRING)
			option->data = argv[*idx];
		else
		{
			option->data = (void*)strtol(argv[*idx], &end, 10);
			if (end[0] != '\0')
				return E_INVALID;
		}
	}
	return 0;
}

int	load_option(t_option* options, char* argv[], int* idx, int argc)
{
	int			i = 0;
	t_option*	option = NULL;
	char*		arg = argv[*idx];

	while (arg[i] == '-')
		i++;
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
		}
	}
	else // e.g: --verbose
	{
		option = load_large_option(options, &arg[2]);
		if (option == NULL)
			return E_UNKNOW;

		option->data = (void*)0x1;
	}

	set_value(option, argv, idx, argc);
	return 0;
}

void show_options(t_option* options)
{
	int i = 0;
	printf("+++ options +++\n");
	while (options[i].id != NONE)
	{
		if (options[i].type == INT)
			printf("\t--%s (-%c) => %ld\n", options[i].name, options[i].c, (long)options[i].data);
		if (options[i].type == STRING)
			printf("\t--%s (-%c) => \"%s\"\n", options[i].name, options[i].c, (char*)options[i].data);
		if (options[i].type == VOID)
			printf("\t--%s (-%c) => %p\n", options[i].name, options[i].c, options[i].data);
		i++;
	}
	printf("\n");
}

t_option* set_option(t_option* options, int id, void* data)
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

t_option* parse(int argc, char* argv[])
{
	t_option		options[] =
	{
		{HELP,				'?',	"help",				0, (void*)0x0,	INT },
		{VERBOSE,			'v',	"verbose",			0, (void*)0x0,	INT },
		{FLOOD,				'f',	"flood",			0, (void*)0x0,	INT },
		{IP_TIMESTAMP,		'\0',	"ip-timestamp",		1, (void*)0x0,	INT },
		{PRELOAD,			'p',	"preload",			1, (void*)0x0,	INT },
		{NUMERIC,			'n',	"numeric",			0, (void*)0x0,	INT },
		{TIMEOUT,			't',	"timeout",			1, (void*)0x0,	INT },
		{LINGER,			'l',	"linger",			1, (void*)0x0,	INT },
		{PATTERN,			'p',	"pattern",			1, (void*)0x0,	INT },
		{IGNORE_ROUTING,	'r',	"ignore-routine",	0, (void*)0x0,	INT },
		{SIZE,				's',	"size",				1, (void*)56,	INT },
		{TOS,				'T',	"tos",				1, (void*)0x0,	INT },
		{QUIET,				'q',	"quiet",			0, (void*)0x0,	INT },
		{VERSION,			'V',	"version",			0, (void*)0x0,	INT },
		{USAGE,				'\0',	"usage",			0, (void*)0x0,	INT },
		{DEBUG,				'd',	"debug",			0, (void*)0x0,	INT },
		{INTERVAL,			'i',	"interval",			1, (void*)0x1,	INT },
		{TTL,				'\0',	"ttl",				1, (void*)255,	INT },
		{NAME,				'\0',	"",					0, (void*)0x0,	STRING },
		{NONE,				'\0',	"",					0, (void*)0x0,	INT },
	};

	for (int i = 1; i < argc; i++)
	{
		int	error = load_option(options, argv, &i, argc);
		if (error == E_UNKNOW)
		{
			dprintf(2, "ping: unrecognized option '%s'\n", argv[i]);
			dprintf(2, "Try 'ping --help' or 'ping --usage' for more information.\n");
			return NULL;
		}
		else if (error == E_INVALID)
		{
			dprintf(2, "ping: invalid value\n");
		}

	}

	set_option(options, NAME, argv[argc-1]);

	t_option* cpy = malloc(sizeof(options));
	if (cpy == NULL)
		return NULL;
	memcpy(cpy, &options, sizeof(options));
	return cpy;
}


