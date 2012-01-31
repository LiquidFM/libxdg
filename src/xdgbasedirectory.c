/* xdgbasedirectory.c: Private file. Base Directory Specification.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * Copyright (C) 2011,2012  Dmitriy Vilkov <dav.daemon@gmail.com>
 *
 * Licensed under the Academic Free License version 2.0
 * Or under the following terms:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "xdgbasedirectory.h"
#include <stdlib.h>
#include <string.h>


void _xdg_for_each_data_dir(XdgDirectoryFunc func, void *user_data)
{
	int stop_processing;
	const char *xdg_data_dirs;

	xdg_data_dirs = getenv("XDG_DATA_HOME");
	if (xdg_data_dirs)
	{
		if (func(xdg_data_dirs, user_data))
			return;
	}
	else
	{
		const char *home = getenv("HOME");

		if (home != NULL)
		{
			char *guessed_xdg_home;

			guessed_xdg_home = malloc(strlen(home) + strlen("/.local/share/") + 1);
			strcpy(guessed_xdg_home, home); strcat(guessed_xdg_home, "/.local/share/");

			stop_processing = func(guessed_xdg_home, user_data);

			free(guessed_xdg_home);

			if (stop_processing)
				return;
		}
	}

	xdg_data_dirs = getenv("XDG_DATA_DIRS");
	if (xdg_data_dirs == NULL)
		xdg_data_dirs = "/usr/local/share/:/usr/share/";

	while (*xdg_data_dirs != '\000')
	{
		const char *end_ptr;
		char *dir;
		int len;

		end_ptr = xdg_data_dirs;
		while (*end_ptr != ':' && *end_ptr != '\000')
		end_ptr ++;

		if (end_ptr == xdg_data_dirs)
		{
			xdg_data_dirs++;
			continue;
		}

		if (*end_ptr == ':')
			len = end_ptr - xdg_data_dirs;
		else
			len = end_ptr - xdg_data_dirs + 1;

		dir = malloc (len + 1);
		strncpy (dir, xdg_data_dirs, len);
		dir[len] = '\0';

		stop_processing = (func) (dir, user_data);

		free (dir);

		if (stop_processing)
			return;

		xdg_data_dirs = end_ptr;
	}
}
