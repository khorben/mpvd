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
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "mpvd.h"


/* main */
/* private */
/* prototypes */
static int _usage(void);


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	MPVDPrefs prefs;
	int o;

	memset(&prefs, 0, sizeof(prefs));
	prefs.daemon = 1;
	prefs.username = PROGNAME_MPVD;
	prefs.groupname = PROGNAME_MPVD;
	prefs.pidfile = "/var/run/" PROGNAME_MPVD ".pid";
	while((o = getopt(argc, argv, "Fg:p:su:")) != -1)
		switch(o)
		{
			case 'F':
				prefs.daemon = 0;
				break;
			case 'g':
				prefs.groupname = optarg;
				break;
			case 'p':
				prefs.pidfile = optarg;
				break;
			case 's':
				prefs.shuffle = 1;
				break;
			case 'u':
				prefs.username = optarg;
				break;
			default:
				return _usage();
		}
	if(optind == argc)
		return _usage();
	return mpvd(&prefs, argc - optind, &argv[optind]);
}


/* private */
/* functions */
/* usage */
static int _usage(void)
{
	fputs("Usage: " PROGNAME_MPVD " [-Fs][-p filename][-u username][-g group] file...\n"
			"  -F	Run in foreground\n"
			"  -g	Use the privileges of this group\n"
			"  -p	Set the PID file\n"
			"  -s	Shuffle the playlist\n"
			"  -u	Use the privileges of this user\n", stderr);
	return 1;
}
