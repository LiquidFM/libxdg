/** @internal @file xdgbasedirectory.h
 *  @brief Private file.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * Specification:
 *  http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 * @copyright
 * Copyright (C) 2011,2012  Dmitriy Vilkov <dav.daemon@gmail.com>
 * @n@n
 * Licensed under the Academic Free License version 2.0
 * Or under the following terms:
 * @n@n
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * @n@n
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 * @n@n
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef XDGBASEDIRECTORY_H_
#define XDGBASEDIRECTORY_H_


/* Callbacks */
typedef int (*XdgDirectoryFunc)(const char *directory, void *user_data);
typedef char *(*XdgIconSearchFunc)(const char *directory, void *user_data);


/* Calls "func" for each "XDG_DATA_" directory */
void _xdg_for_each_data_dir(XdgDirectoryFunc func, void *user_data);


/* Calls "func" for each directory described in "Icon Theme Specification" */
void _xdg_for_each_theme_dir(XdgDirectoryFunc func, void *user_data);


/* Calls "func" for each directory described in "Icon Theme Specification".
 * Returns immediately if "func" returned not null. */
char *_xdg_search_in_each_theme_dir(XdgIconSearchFunc func, void *user_data);

#endif /* XDGBASEDIRECTORY_H_ */
