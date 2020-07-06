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
#include <grp.h>
#include <pwd.h>
#include <syslog.h>
#include <errno.h>
#include <mpv/client.h>
#include "mpvd.h"

#ifndef PROGNAME_MPVD
# define PROGNAME_MPVD	"mpvd"
#endif


/* MPVD */
/* private */
/* prototypes */
static int _mpvd_error(char const * message);
static int _mpvd_error_mpv(int error, char const * message);
static int _mpvd_event(MPVDPrefs * prefs, mpv_handle * mpv, mpv_event * event);
static int _mpvd_prefs(MPVDPrefs * prefs);


/* public */
/* functions */
/* mpvd */
int mpvd(MPVDPrefs * prefs, int filec, char * filev[])
{
	mpv_handle * mpv;
	int error;
	int t = 1;
	int i;
	const char * command_loadfile[] = { "loadfile", NULL, "append", NULL };

	if(prefs != NULL && _mpvd_prefs(prefs) != 0)
		return 2;
	if((mpv = mpv_create()) == NULL)
		return 2;
	if(prefs != NULL && prefs->shuffle != 0)
		if((error = mpv_set_property(mpv, "shuffle", MPV_FORMAT_FLAG,
						&t)) != 0)
			_mpvd_error_mpv(error, "shuffle");
	if((error = mpv_initialize(mpv)) != 0)
	{
		mpv_destroy(mpv);
		return _mpvd_error_mpv(error, NULL);
	}
	if((error = mpv_set_property(mpv, "idle", MPV_FORMAT_FLAG, &t)) != 0)
		_mpvd_error_mpv(error, "idle");
	for(i = 0; i < filec; i++)
	{
		command_loadfile[1] = filev[i];
		if((error = mpv_command_async(mpv, 0, command_loadfile)) != 0)
			_mpvd_error_mpv(error, filev[i]);
	}
	while(_mpvd_event(prefs, mpv, mpv_wait_event(mpv, -1)) == 0);
	mpv_destroy(mpv);
	return 0;
}


/* private */
/* mpvd_error */
static int _mpvd_error(char const * message)
{
	fprintf(stderr, "%s%s%s: %s\n", PROGNAME_MPVD,
			(message != NULL) ? ": " : "",
			(message != NULL) ? message : "",
			strerror(errno));
	return 2;
}


/* mpvd_error_mpv */
static int _mpvd_error_mpv(int error, char const * message)
{
	fprintf(stderr, "%s%s%s: %s\n", PROGNAME_MPVD,
			(message != NULL) ? ": " : "",
			(message != NULL) ? message : "",
			mpv_error_string(error));
	return 2;
}


/* mpvd_event */
static void _event_log_message(MPVDPrefs * prefs,
		mpv_event_log_message * log_message);
static void _event_log_message_stdio(mpv_event_log_message * log_message);
static void _event_log_message_syslog(mpv_event_log_message * log_message);

static int _mpvd_event(MPVDPrefs * prefs, mpv_handle * mpv, mpv_event * event)
{
	mpv_event_log_message * log_message;
	char const * command_play[] = { "set", "playlist-pos", "0", NULL };
	char const * command_shuffle[] = { "playlist-shuffle", NULL };
	int error;

	switch(event->event_id)
	{
		case MPV_EVENT_IDLE:
			if(prefs != NULL && prefs->shuffle)
				if((error = mpv_command(mpv, command_shuffle))
						!= 0)
					_mpvd_error_mpv(error, "shuffle");
			if((error = mpv_command(mpv, command_play)) != 0)
				_mpvd_error_mpv(error, "play");
			break;
		case MPV_EVENT_LOG_MESSAGE:
			log_message = event->data;
			_event_log_message(prefs, log_message);
			break;
		case MPV_EVENT_SHUTDOWN:
			return -1;
		default:
			break;
	}
	return 0;
}

static void _event_log_message(MPVDPrefs * prefs,
		mpv_event_log_message * log_message)
{
	if(prefs != NULL && prefs->daemon != 0)
		_event_log_message_syslog(log_message);
	else
		_event_log_message_stdio(log_message);
}

static void _event_log_message_stdio(mpv_event_log_message * log_message)
{
	FILE * fp;

	switch(log_message->log_level)
	{
		case MPV_LOG_LEVEL_ERROR:
		case MPV_LOG_LEVEL_FATAL:
		case MPV_LOG_LEVEL_WARN:
			fp = stderr;
			break;
		default:
			fp = stdout;
			break;
	}
	fprintf(fp, "%s: %s\n", PROGNAME_MPVD, log_message->text);
}

static void _event_log_message_syslog(mpv_event_log_message * log_message)
{
	int priority;

	switch(log_message->log_level)
	{
		case MPV_LOG_LEVEL_NONE:
			return;
		case MPV_LOG_LEVEL_DEBUG:
		case MPV_LOG_LEVEL_TRACE:
			priority = LOG_DEBUG;
			break;
		case MPV_LOG_LEVEL_ERROR:
			priority = LOG_ERR;
			break;
		case MPV_LOG_LEVEL_FATAL:
			priority = LOG_CRIT;
			break;
		case MPV_LOG_LEVEL_WARN:
			priority = LOG_WARNING;
			break;
		case MPV_LOG_LEVEL_INFO:
		case MPV_LOG_LEVEL_V:
		default:
			priority = LOG_INFO;
			break;
	}
	syslog(priority, "%s", log_message->text);
}


/* mpvd_prefs */
static int _mpvd_prefs(MPVDPrefs * prefs)
{
	struct passwd * pw = NULL;
	struct group * gr = NULL;
	FILE * fp = NULL;

	if(prefs->username != NULL && (pw = getpwnam(prefs->username)) == NULL)
		return _mpvd_error(prefs->username);
	if(prefs->groupname != NULL
			&& (gr = getgrnam(prefs->groupname)) == NULL)
		return _mpvd_error(prefs->groupname);
	if(prefs->pidfile != NULL && (fp = fopen(prefs->pidfile, "w")) == NULL)
		return _mpvd_error(prefs->pidfile);
	if(gr != NULL && setegid(gr->gr_gid) != 0)
		_mpvd_error("setegid");
	if(pw != NULL && seteuid(pw->pw_uid) != 0)
		_mpvd_error("seteuid");
	if(prefs->daemon && daemon(0, 0) != 0)
	{
		if(fp != NULL)
			fclose(fp);
		return _mpvd_error("daemon");
	}
	if(fp != NULL)
	{
		/* XXX check for errors */
		fprintf(fp, "%u\n", getpid());
		if(fclose(fp) != 0)
			/* XXX should log instead */
			_mpvd_error(prefs->pidfile);
	}
	return 0;
}
