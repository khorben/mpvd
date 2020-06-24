#include <stdio.h>
#include <mpv/client.h>
#include "mpvd.h"

#ifndef PROGNAME_MPVD
# define PROGNAME_MPVD	"mpvd"
#endif


static int _mpvd_error(int error);
static int _mpvd_event(mpv_event * event);

int mpvd(int filec, char * filev[])
{
	mpv_handle * mpv;
	int error;
	int t = 1;
	int i;
	const char * command[] = { "loadfile", NULL, NULL };

	if((mpv = mpv_create()) == NULL)
		return 2;
	if((error = mpv_set_property(mpv, "shuffle", MPV_FORMAT_FLAG, &t)) != 0)
		_mpvd_error(error);
	if((error = mpv_initialize(mpv)) != 0)
	{
		mpv_destroy(mpv);
		return _mpvd_error(error);
	}
	for(i = 0; i < filec; i++)
	{
		command[1] = filev[i];
		if((error = mpv_command(mpv, command)) != 0)
			_mpvd_error(error);
	}
	while(_mpvd_event(mpv_wait_event(mpv, -1)) == 0);
	mpv_destroy(mpv);
	return 0;
}

static int _mpvd_error(int error)
{
	fprintf(stderr, "%s: %s\n", PROGNAME_MPVD, mpv_error_string(error));
	return 2;
}

static int _mpvd_event(mpv_event * event)
{
	mpv_event_log_message * log_message;

	switch(event->event_id)
	{
		case MPV_EVENT_LOG_MESSAGE:
			log_message = event->data;
			printf("%s: %s\n", PROGNAME_MPVD, log_message->text);
			break;
		case MPV_EVENT_SHUTDOWN:
			return -1;
		default:
			break;
	}
	return 0;
}
