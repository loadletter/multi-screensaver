/* xscreensaver, Copyright (c) 1992-2014 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

/* This file contains code for running an external program to grab an image
	 onto the given window.  The external program in question must take a
	 window ID as its argument, e.g., the "xscreensaver-getimage" program
	 in the hacks/ directory.

	 That program links against utils/grabimage.c, which happens to export the
	 same API as this program (utils/grabclient.c).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>		/* for waitpid() and associated macros */

const char *progname = "xscreensaver-getimage";

/* Spawn a program, and wait for it to finish.
	 If we just use system() for this, then sometimes the subprocess
	 doesn't die when *this* process is sent a TERM signal.  Perhaps
	 this is due to the intermediate /bin/sh that system() uses to
	 parse arguments?  I'm not sure.  But using fork() and execvp()
	 here seems to close the race.
 */
static void
exec_simple_command (const char *command)
{
	char *av[1024];
	int ac = 0;
	char *token = strtok (strdup(command), " \t");
	while (token)
		{
			av[ac++] = token;
			token = strtok(0, " \t");
		}
	av[ac] = 0;

	execvp (av[0], av);	/* shouldn't return. */
}


static void
fork_exec_wait (const char *command)
{
	char buf [255];
	pid_t forked;
	int status;

	switch ((int) (forked = fork ()))
		{
		case -1:
			sprintf (buf, "%s: couldn't fork", progname);
			perror (buf);
			return;

		case 0:
			exec_simple_command (command);
			exit (1);  /* exits child fork */
			break;

		default:
			waitpid (forked, &status, 0);
			break;
		}
}

void
grab_screen_image(const char *windowid)
{
	char cmd_buf[1024];
	sprintf(cmd_buf, "xscreensaver-getimage %s", windowid);
	fork_exec_wait(cmd_buf);
}
