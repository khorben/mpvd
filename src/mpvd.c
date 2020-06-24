#include <stdio.h>
#include <mpv/client.h>
#include "mpvd.h"

#ifndef PROGNAME_MPVD
# define PROGNAME_MPVD	"mpvd"
#endif


static int _mpvd_error(int error, char const * message);
static int _mpvd_event(mpv_handle * mpv, mpv_event * event);

int mpvd(int filec, char * filev[])
{
	mpv_handle * mpv;
	int error;
	int t = 1;
	int i;
	const char * command_loadfile[] = { "loadfile", NULL, "append", NULL };

	if((mpv = mpv_create()) == NULL)
		return 2;
	if((error = mpv_set_property(mpv, "shuffle", MPV_FORMAT_FLAG, &t)) != 0)
		_mpvd_error(error, "shuffle");
	if((error = mpv_initialize(mpv)) != 0)
	{
		mpv_destroy(mpv);
		return _mpvd_error(error, NULL);
	}
	if((error = mpv_set_property(mpv, "idle", MPV_FORMAT_FLAG, &t)) != 0)
		_mpvd_error(error, "idle");
	for(i = 0; i < filec; i++)
	{
		command_loadfile[1] = filev[i];
		if((error = mpv_command_async(mpv, 0, command_loadfile)) != 0)
			_mpvd_error(error, filev[i]);
	}
	while(_mpvd_event(mpv, mpv_wait_event(mpv, -1)) == 0);
	mpv_destroy(mpv);
	return 0;
}

static int _mpvd_error(int error, char const * message)
{
	fprintf(stderr, "%s%s%s: %s\n", PROGNAME_MPVD,
			(message != NULL) ? ": " : "",
			(message != NULL) ? message : "",
			mpv_error_string(error));
	return 2;
}

static int _mpvd_event(mpv_handle * mpv, mpv_event * event)
{
	mpv_event_log_message * log_message;
	char const * command_shuffle[] = { "playlist-shuffle", NULL };
	char const * command_play[] = { "set", "playlist-pos", "0", NULL };
	int error;

	switch(event->event_id)
	{
		case MPV_EVENT_IDLE:
			if((error = mpv_command(mpv, command_shuffle)) != 0)
				_mpvd_error(error, "shuffle");
			if((error = mpv_command(mpv, command_play)) != 0)
				_mpvd_error(error, "play");
			break;
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
