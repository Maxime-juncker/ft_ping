#include <stdio.h>

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
	if ((long)get_option(options, DEBUG)->data == 1)
		show_options(options);

	return 0;
}
