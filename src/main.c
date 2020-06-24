#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "mpvd.h"

#ifndef PROGNAME_MPVD
# define PROGNAME_MPVD	"mpvd"
#endif


static int _error(char const * message);

int main(int argc, char * argv[])
{
	char const * pidfile = "/var/run/mpvd.pid";
	const int daemonize = 1;
	const uid_t uid = 502;
	const gid_t gid = 502;
	FILE * fp;

	if(daemonize && daemon(0, 0) != 0)
	{
		return _error("daemon");
	}
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
	return mpvd(argc, argv);
}

static int _error(char const * message)
{
	fprintf(stderr, "%s%s%s: %s\n", PROGNAME_MPVD,
			(message != NULL) ? ": " : "",
			(message != NULL) ? message : "",
			strerror(errno));
	return 2;
}
