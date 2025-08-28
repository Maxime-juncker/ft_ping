#ifndef TEXT_H
#define TEXT_H

#include "options.h"

#define SMALL_HELP_TXT "Try 'ft_ping --help' or 'ft_ping --usage' for more information.\n"
#define MISSING_ARG_TXT "ft_ping: missing host operand\n" \
						"Try 'ft_ping --help' or 'ft_ping --usage' for more information.\n"
#define UNKNOWN_HOST_TXT "ft_ping: unknown host\n"
#define HELP_TXT "Usage: ping [OPTION...] HOST ...\n"								\
"Send ICMP ECHO_REQUEST packets to network hosts.\n\n"								\
"  Options valid for all request types:\n\n"										\
"   -d, --debug                show debug logs\n"									\
"   -i, --interval=NUMBER      wait NUMBER seconds between sending each packet\n"	\
"   -n, --numeric              do not resolve host addresses\n"						\
"   -r, --ignore-routing       send directly to a host on an attached network\n"	\
"       --ttl=N                specify N as time-to-live\n"							\
"   -T, --tos=NUM              set type of service (TOS) to NUM\n"					\
"   -v, --verbose              verbose output\n"									\
"   -w, --timeout=N            stop after N seconds\n"								\
"   -W, --linger=N             number of seconds to wait for response\n\n"			\
"  Options valid for --echo requests:\n\n"											\
"   -f, --flood                flood ping (root only)]\n"							\
"       --ip-timestamp=FLAG    IP timestamp of type FLAG, which is one of\n"		\
"                              \"tsonly\" and \"tsaddr\"\n"							\
"   -l, --preload=NUMBER       send NUMBER packets as fast as possible before\n"	\
"                              falling into normal mode of behavior (root only)\n"	\
"   -p, --pattern=PATTERN      fill ICMP packet with given pattern (hex)\n"			\
"   -q, --quiet                quiet output\n"										\
"   -s, --size=NUMBER          send NUMBER data octets\n\n"							\
"   -?, --help                 give this help list\n"								\
"      --usage                give a short usage message\n"							\
"  -V, --version              print program version\n"

#define USAGE_TXT "Usage: ft_ping [-dnrvfq?V] [-c NUMBER] [-i NUMBER] [-T NUM] [-w N]\n"	\
            "\t[-W N] [-l NUMBER] [-p PATTERN] [-s NUMBER] [--debug]\n"						\
            "\t[--interval=NUMBER] [--numeric] [--ignore-routing] [--ttl=N]\n"				\
            "\t[--tos=NUM] [--verbose] [--timeout=N] [--linger=N] [--flood]\n"				\
            "\t[--ip-timestamp=FLAG] [--preload=NUMBER] [--pattern=PATTERN]\n"				\
            "\t[--quiet] [--size=NUMBER] [--help] [--usage] [--version]\n"					\
			"\tHOST ...\n"
#define VERSION_TXT "ft_ping 1.0\nblablablablalblablablablalbalbalbalba[....]\nWritten by mjunker"
int show_text(t_option* options);

#endif // !TEXT_H
