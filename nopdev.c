/*
 * This file is part of nopdev.
 *
 * Copyright (c) 2015 Dima Krasner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fnmatch.h>
#include <paths.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <sys/wait.h>

#define RDONLY_MODE (S_IRUSR | S_IRGRP | S_IROTH)
#define WRONLY_MODE (S_IWUSR | S_IWGRP | S_IWOTH)
#define RDWR_MODE (RDONLY_MODE | WRONLY_MODE)

#define EVT_TYPE(x) {#x, dev_##x}

struct dev_event {
	const char *dev;
	const char *path;
};

struct dev_rule {
	const char *exp;
	const char *user;
	const char *grp;
	mode_t mode;
};

static const struct dev_rule rules[] = {
#include "config.h"
};

static bool load_module(void)
{
	const char *alias;
	pid_t pid;

	/* if the device has a module alias, load the appropriate module */
	alias = getenv("MODALIAS");
	if (NULL != alias) {
		pid = fork();
		switch (pid) {
			case 0:
				/* no need to call closelog() - FD_CLOEXEC should be
				 * set */
				(void) execlp("modprobe",
				              "modprobe",
				              "-s",
				              alias,
				              (char *) NULL);
				exit(EXIT_FAILURE);

			case (-1):
				return false;

			default:
				if (pid != waitpid(pid, NULL, 0))
					return false;
		}
	}

	return true;
}

static bool apply_rule(const struct dev_event *evt, const struct dev_rule *rule)
{
	struct passwd *user;
	struct group *grp;

	/* set the device node ownership and permissions */
	user = getpwnam(rule->user);
	if (NULL == user) {
		syslog(LOG_ERR, "failed to obtain the UID of %s\n", rule->user);
		return false;
	}

	grp = getgrnam(rule->grp);
	if (NULL == grp) {
		syslog(LOG_ERR, "failed to obtain the GID of %s\n", rule->grp);
		return false;
	}

	if (-1 == chown(evt->dev, user->pw_uid, grp->gr_gid))
		return false;

	if (-1 == chmod(evt->dev, rule->mode))
		return false;

	return true;
}

static bool apply_rules(const struct dev_event *evt)
{
	unsigned int i;

	for (i = 0; sizeof(rules) / sizeof(rules[0]) > i; ++i) {
		switch (fnmatch(rules[i].exp, evt->dev, FNM_PATHNAME | FNM_NOESCAPE)) {
			case FNM_NOMATCH:
				continue;

			case 0:
				if (false == apply_rule(evt, &rules[i]))
					return false;

				if (false == load_module())
					return false;

				break;

			default:
				return false;
		}
	}

	return true;
}

static bool dev_del(const struct dev_event *evt)
{
	if (-1 == unlink(evt->dev))
		return false;

	return true;
}

static bool dev_add(const struct dev_event *evt)
{
	bool ret;

	ret = apply_rules(evt);
	if (false == ret)
		(void) dev_del(evt);

	return ret;
}

static bool dev_change(const struct dev_event *evt)
{
	return dev_add(evt);
}

static const struct {
	const char *name;
	bool (*cb)(const struct dev_event *);
} evt_types[] = { EVT_TYPE(add), EVT_TYPE(del), EVT_TYPE(change) };

int main(int argc, char *argv[])
{
	struct dev_event evt;
	const char *act;
	unsigned int i;
	int ret = EXIT_FAILURE;

	if (2 != argc) {
		(void) fprintf(stderr, "Usage: %s SUBSYSTEM\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (0 != geteuid()) {
		(void) write(STDERR_FILENO,
		             "Error: must run as root.\n",
		             sizeof("Error: must run as root.\n") - sizeof(char));
		return EXIT_FAILURE;
	}

	act = getenv("ACTION");
	if (NULL == act)
		return EXIT_FAILURE;

	for (i = 0; sizeof(evt_types) / sizeof(evt_types[0]) > i; ++i) {
		if (0 != strcmp(evt_types[i].name, act))
			continue;

		evt.dev = getenv("DEVNAME");
		if (NULL == evt.dev)
			break;

		evt.path = getenv("DEVPATH");
		if (NULL == evt.path)
			break;

		if (-1 == chdir(_PATH_DEV))
			break;

		openlog("nopdev", LOG_NDELAY | LOG_PID, LOG_USER);
		syslog(LOG_INFO, "handling %s event for %s\n", act, evt.dev);

		if (true == evt_types[i].cb(&evt))
			ret = EXIT_SUCCESS;

		closelog();

		break;
	}

	return ret;
}


