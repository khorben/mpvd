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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>
#include <syslog.h>
#include <errno.h>
#include <mpv/client.h>
#include "common.h"
#include "mpvd.h"


/* MPVD */
/* private */
/* prototypes */
static int _mpvd_error(char const * message);
static int _mpvd_error_mpv(int error, char const * message);
static int _mpvd_event(MPVDPrefs * prefs, mpv_handle * mpv, mpv_event * event);
static int _mpvd_prefs(MPVDPrefs const * prefs);


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
static int _prefs_setgroups(char const * username);

static int _mpvd_prefs(MPVDPrefs const * prefs)
{
	struct passwd * pw = NULL;
	struct group * gr = NULL;
	uid_t uid;
	gid_t gid;
	size_t len;
	FILE * fp = NULL;

	/* lookup the target user if set */
	if(prefs->username != NULL)
	{
		if((pw = getpwnam(prefs->username)) == NULL)
			return _mpvd_error(prefs->username);
		uid = pw->pw_uid;
		gid = pw->pw_gid;
	}
	/* lookup the target group */
	if(prefs->groupname != NULL)
	{
		if((gr = getgrnam(prefs->groupname)) == NULL)
			return _mpvd_error(prefs->groupname);
		gid = gr->gr_gid;
	}
	else if(pw != NULL && (gr = getgrgid(gid)) == NULL)
		return _mpvd_error("getgrgid");
	/* prepare the environment */
	if(pw != NULL && pw->pw_dir != NULL && (len = strlen(pw->pw_dir)) > 0)
		/* set $HOME */
		if(setenv("HOME", pw->pw_dir, 1) != 0)
			return _mpvd_error("setenv");
	/* open the PID file before dropping permissions */
	if(prefs->pidfile != NULL && (fp = fopen(prefs->pidfile, "w")) == NULL)
		return _mpvd_error(prefs->pidfile);
	/* set the groups and user if set */
	if(gr != NULL)
	{
		if(setgid(gid) != 0)
			_mpvd_error("setgid");
		if(setegid(gid) != 0)
			_mpvd_error("setegid");
	}
	if(prefs->username != NULL)
	{
		_prefs_setgroups(prefs->username);
		if(setuid(uid) != 0)
			_mpvd_error("setuid");
		if(seteuid(uid) != 0)
			_mpvd_error("seteuid");
	}
	/* actually mpvd */
	if(prefs->daemon && daemon(0, 0) != 0)
	{
		if(fp != NULL)
			fclose(fp);
		return _mpvd_error("daemon");
	}
	/* write the PID file */
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

static int _prefs_setgroups(char const * username)
{
	int ret;
	struct group * gr;
	int i;
	int n = 0;
	gid_t * groups = NULL;
	gid_t * p;

	setgroupent(1);
	while((gr = getgrent()) != NULL)
		for(i = 0; gr->gr_mem[i] != NULL; i++)
		{
			if(strcmp(gr->gr_mem[i], username) != 0)
				continue;
			if((p = realloc(groups, sizeof(*groups) * (n + 1)))
					== NULL)
			{
				free(groups);
				return _mpvd_error("realloc");
			}
			groups = p;
			groups[n++] = gr->gr_gid;
		}
	endgrent();
	ret = setgroups(n, groups);
	free(groups);
	if(ret != 0)
		_mpvd_error("setgroups");
	return ret;
}
