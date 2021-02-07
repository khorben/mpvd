/* $Id$ */
/* Copyright (c) 2020-2021 Pierre Pronchery <khorben@defora.org> */
/* This file is part of mpvd */
/* All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */



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
	if(getuid() == 0)
	{
		prefs.daemon = 1;
		prefs.username = PROGNAME_MPVD;
		prefs.groupname = PROGNAME_MPVD;
		prefs.pidfile = "/var/run/" PROGNAME_MPVD ".pid";
	}
	while((o = getopt(argc, argv, "BFg:p:su:")) != -1)
		switch(o)
		{
			case 'B':
				prefs.daemon = 1;
				break;
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
	fputs("Usage: " PROGNAME_MPVD " [-BFs][-p filename][-u username][-g group] file...\n"
			"  -B	Run in background\n"
			"  -F	Run in foreground\n"
			"  -g	Use the privileges of this group\n"
			"  -p	Set the PID file\n"
			"  -s	Shuffle the playlist\n"
			"  -u	Use the privileges of this user\n", stderr);
	return 1;
}
