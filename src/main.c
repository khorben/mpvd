/* $Id$ */
/* Copyright (c) 2020 Pierre Pronchery <khorben@defora.org> */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. */



#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "mpvd.h"

#ifndef PROGNAME_MPVD
# define PROGNAME_MPVD	"mpvd"
#endif


/* main */
/* private */
/* prototypes */
static int _error(char const * message);
static int _usage(void);


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	const uid_t uid = 502;
	const gid_t gid = 502;
	char const * pidfile = "/var/run/" PROGNAME_MPVD ".pid";
	int o;
	int daemonize = 1;
	FILE * fp;

	while((o = getopt(argc, argv, "Fp:")) != -1)
		switch(o)
		{
			case 'F':
				daemonize = 0;
				break;
			case 'p':
				pidfile = optarg;
				break;
			default:
				return _usage();
		}
	if(optind == argc)
		return _usage();
	if(daemonize)
	{
		if(daemon(0, 0) != 0)
			return _error("daemon");
		if((fp = fopen(pidfile, "w")) == NULL)
			_error(pidfile);
		else
		{
			fprintf(fp, "%u\n", getpid());
			if(fclose(fp) != 0)
				_error(pidfile);
		}
		if(setegid(gid) != 0)
			_error("setegid");
		if(seteuid(uid) != 0)
			_error("seteuid");
	}
	return mpvd(argc - optind, &argv[optind]);
}


/* private */
/* functions */
static int _error(char const * message)
{
	fprintf(stderr, "%s%s%s: %s\n", PROGNAME_MPVD,
			(message != NULL) ? ": " : "",
			(message != NULL) ? message : "",
			strerror(errno));
	return 2;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: " PROGNAME_MPVD " [-F][-p filename] file...\n"
			"  -F	Run in foreground\n"
			"  -p	Set the PID file\n", stderr);
	return 1;
}
